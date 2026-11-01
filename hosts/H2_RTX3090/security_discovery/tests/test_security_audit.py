#!/usr/bin/env python3
"""CPU-only audit regressions: no GPU, OpenCL loader, contexts or device queries."""
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[4]
REPRO = ROOT / 'hosts/H2_RTX3090/c1/reproducers'
spec = importlib.util.spec_from_file_location('audit_provenance', REPRO / 'provenance.py')
p = importlib.util.module_from_spec(spec)
spec.loader.exec_module(p)


class AuditTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='opencl-offline-audit-')
        self.root = Path(self.temp.name)
        self.source = self.root / 'fixture.c'
        self.binary = self.root / 'fixture'
        self.source.write_text('#include <stdio.h>\nint main(void){puts("CPU_FIXTURE_ONLY");return 0;}\n')
        subprocess.run(['gcc', str(self.source), '-o', str(self.binary)], check=True, capture_output=True)
        self.workspace = {'workspace_sha256': 'w' * 64,
                          'git': {'mode': 'repository', 'head': 'h' * 40}}
        self.env = {k: None for k in p.ENV_KEYS}
        self.approval = {'schema_version': 1, 'status': 'explicitly_approved',
                         'approval_id': 'OFFLINE_FIXTURE_NOT_GPU_APPROVAL',
                         'approval_evidence': 'Synthetic CPU test only',
                         'binary_path': str(self.binary.resolve()), 'binary_sha256': p.sha(self.binary),
                         'allowed_cases': ['FIXTURE'], 'max_repeats': 1, 'plan_sha256': 'p' * 64,
                         'workspace_sha256': 'w' * 64, 'git_mode': 'repository', 'git_head': 'h' * 40,
                         'environment': self.env.copy()}

    def tearDown(self):
        self.temp.cleanup()

    def validate(self, **kwargs):
        args = dict(approval=self.approval, binary=self.binary, case='FIXTURE', repeat=1,
                    workspace=self.workspace, plan_sha='p' * 64, env=self.env)
        args.update(kwargs)
        return p.validate_approval(**args)

    def test_valid_binding_and_sealed_cpu_execution(self):
        self.validate()
        fd, meta = p.sealed_executable(self.binary, self.approval['binary_sha256'])
        try:
            # Change disk contents AFTER sealing: only approved snapshot must execute.
            self.binary.write_bytes(b'changed disk path')
            r = subprocess.run([str(self.binary)], executable=f'/proc/self/fd/{fd}', pass_fds=(fd,), capture_output=True, text=True)
            self.assertEqual((r.returncode, r.stdout), (0, 'CPU_FIXTURE_ONLY\n'))
            self.assertEqual(meta['sha256'], self.approval['binary_sha256'])
            with self.assertRaises(OSError):
                os.write(fd, b'x')
        finally:
            os.close(fd)

    def test_tampered_binary_rejected_before_child(self):
        self.binary.write_bytes(self.binary.read_bytes() + b'tamper')
        with self.assertRaises(ValueError):
            p.sealed_executable(self.binary, self.approval['binary_sha256'])

    def test_script_rejected(self):
        self.binary.write_text('#!/bin/sh\necho nope\n')
        with self.assertRaises(ValueError):
            p.sealed_executable(self.binary, p.sha(self.binary))

    def test_missing_or_draft_approval(self):
        self.approval['status'] = 'draft'
        with self.assertRaises(ValueError):
            self.validate()

    def test_wrong_path_case_repeat_plan_workspace_commit_environment(self):
        variants = [dict(binary=self.source), dict(case='OTHER'), dict(repeat=2), dict(plan_sha='different'),
                    dict(workspace={'workspace_sha256': 'changed', 'git': self.workspace['git']}),
                    dict(workspace={'workspace_sha256': 'w' * 64, 'git': {'mode': 'repository', 'head': 'different'}}),
                    dict(env={**self.env, 'LD_PRELOAD': 'unexpected'})]
        for v in variants:
            with self.subTest(v=list(v)), self.assertRaises(ValueError):
                self.validate(**v)

    def test_git_untracked_content_and_generated_output_provenance(self):
        repo = self.root / 'repo'
        repo.mkdir()
        subprocess.run(['git', 'init', '--quiet', str(repo)], check=True)
        subprocess.run(['git', '-C', str(repo), '-c', 'user.name=Offline Fixture', '-c', 'user.email=fixture@invalid.test',
                        'commit', '--quiet', '--allow-empty', '-m', 'offline provenance fixture'], check=True)
        (repo / 'untracked.txt').write_text('version one')
        before = p.workspace_provenance(repo)
        self.assertEqual(before['git']['mode'], 'repository')
        self.assertIn('untracked.txt', before['git']['status_porcelain_v1_z'])
        (repo / 'untracked.txt').write_text('version two')
        after = p.workspace_provenance(repo)
        self.assertNotEqual(before['workspace_sha256'], after['workspace_sha256'])
        # Same porcelain status is insufficient: content hash must change for untracked edits.
        self.assertEqual(before['git']['status_sha256'], after['git']['status_sha256'])
        generated = repo / 'hosts/H2_RTX3090/results/c1/run/stdout.jsonl'
        generated.parent.mkdir(parents=True)
        generated.write_text('generated output')
        self.assertEqual(after['workspace_sha256'], p.workspace_provenance(repo)['workspace_sha256'])

    def test_exported_snapshot_does_not_invent_git_commit(self):
        exported = self.root / 'export'
        exported.mkdir()
        result = p.workspace_provenance(exported)
        self.assertEqual(result['git']['mode'], 'exported_snapshot')
        self.assertIsNone(result['git']['head'])

    def test_b12_pure_c_assertions(self):
        binary = self.root / 'b12-validation'
        src = Path(__file__).with_name('test_b12_validation.c')
        subprocess.run(['gcc', '-std=c11', '-Wall', '-Wextra', '-Werror', '-I', str(REPRO), str(src), '-o', str(binary)], check=True, capture_output=True)
        r = subprocess.run([str(binary)], capture_output=True, text=True, check=True)
        self.assertIn('22 pure-CPU', r.stdout)


if __name__ == '__main__':
    unittest.main(verbosity=2)
