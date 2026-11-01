"""Fixed-path-v1 regressions. Only harmless CPU fixture and temporary ledgers."""
import copy
import importlib.util
import json
import os
from pathlib import Path
import sqlite3
import subprocess
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[4]
spec = importlib.util.spec_from_file_location('existing_cpu_e2e', ROOT / 'hosts/H2_RTX3090/s01/tests/test_supervisor_e2e.py')
e2e = importlib.util.module_from_spec(spec)
spec.loader.exec_module(e2e)


class StableEnvironmentTests(unittest.TestCase):
    def setUp(self):
        self.f = e2e.SupervisorE2E('test_pass')
        self.f.setUp()
        self.addCleanup(self.f.tearDown)
        self.p = self.f.p
        self.legacy = copy.deepcopy(self.f.approval)
        self.a = self.f.approval
        self.a['execution_environment'] = {'version': 1, 'path': self.p.STABLE_PATH, 'tools': self.p.stable_tool_snapshot()}
        self.a['environment']['PATH'] = self.p.STABLE_PATH
        ws = self.p.workspace_provenance(self.f.root)
        self.a['git_binding'] = {k: ws['git'].get(k) for k in self.p.GIT_BINDING_KEYS}
        refs = json.loads((self.f.raw / 'library_hashes.json').read_text())
        self.a['runtime_file_sha256'] = {x['resolved']: x['sha256'] for x in refs}
        self.write()
        self.ledger_before = self.p.sha(self.f.ledger)

    def write(self):
        self.f.manifest.write_text(json.dumps(self.a))

    def invoke(self, case='CPU-PASS', env=None, out_name=None):
        cmd, out = self.f.command(case, out_name=out_name)
        cmd = ['/usr/bin/python3', '-I', *cmd[1:]]
        return subprocess.run(cmd, env=env, capture_output=True, text=True, timeout=15), out

    def effective(self, **changes):
        return self.p.stable_child_environment(self.a, {**os.environ, **changes})

    def assert_no_start(self):
        self.assertEqual(self.f.starts(), 0)
        self.assertEqual(self.p.sha(self.f.ledger), self.ledger_before)

    def test_changing_codex_sessions_same_fixed_environment(self):
        for case, code, path in [('CPU-PASS', 0, '/tmp/codex-session-A:/usr/bin:/bin'),
                                 ('CPU-SKIP', 77, '/tmp/codex-session-B:/bin:/usr/bin:/usr/bin')]:
            env = self.effective(PATH=path)
            self.assertEqual(env['PATH'], '/usr/bin:/bin')
            result, out = self.invoke(case, env)
            self.assertEqual(result.returncode, code, result.stderr)
            info = self.f.verify_log(out)
            self.assertEqual(info['environment']['PATH'], '/usr/bin:/bin')
            self.assertEqual(info['execution_environment'], self.a['execution_environment'])
            self.assertEqual(info['trial_ledger']['reservation']['binding']['execution_environment'], self.a['execution_environment'])
            self.assertEqual(info['trial_ledger']['reservation']['binding']['environment'], self.a['environment'])

    def test_ambient_fake_programs_are_not_resolved_or_started(self):
        fake = self.f.ledger.parent / 'session-bin'
        fake.mkdir()
        marker = self.f.ledger.parent / 'FAKE_EXECUTED'
        for name in ['python3', 'git', 'bash']:
            path = fake / name
            path.write_text('#!/bin/sh\necho BAD > ' + str(marker) + '\nexit 91\n')
            path.chmod(0o700)
        env = self.effective(PATH=str(fake) + ':/usr/bin:/bin')
        result, out = self.invoke(env=env)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.f.verify_log(out)
        self.assertFalse(marker.exists())

    def test_direct_supervisor_requires_exact_fixed_path(self):
        result, out = self.invoke(env={**os.environ, 'PATH': '/tmp/session:/usr/bin:/bin'})
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('Invalid stable execution environment/PATH', result.stderr)
        self.assertFalse(out.exists())
        self.assert_no_start()

    def test_every_runtime_selection_variable_remains_exact(self):
        for key in self.p.ENV_KEYS:
            if key == 'PATH':
                continue
            with self.subTest(key=key), self.assertRaisesRegex(ValueError, 'Runtime environment differs: ' + key):
                self.effective(**{key: 'CPU_ONLY_TAMPERED'})
        self.assert_no_start()

    def test_supervisor_rejects_runtime_selection_tamper(self):
        env = self.effective()
        env['OCL_ICD_VENDORS'] = '/CPU_ONLY/WRONG'
        result, out = self.invoke(env=env)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('Runtime environment differs: OCL_ICD_VENDORS', result.stderr)
        self.assertFalse(out.exists())
        self.assert_no_start()

    def test_program_hash_tamper_rejected_before_git_and_child(self):
        self.a['execution_environment']['tools']['git']['sha256'] = '0' * 64
        self.write()
        result, out = self.invoke(env={**os.environ, 'PATH': self.p.STABLE_PATH})
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('Stable executable binding changed', result.stderr)
        self.assertFalse(out.exists())
        self.assert_no_start()

    def test_program_target_tamper_rejected(self):
        self.a['execution_environment']['tools']['python3']['resolved_path'] = '/tmp/fake-python'
        with self.assertRaisesRegex(ValueError, 'Stable executable binding changed'):
            self.effective()
        self.assert_no_start()

    def test_resolution_tamper_fails_closed(self):
        with patch.object(self.p.shutil, 'which', return_value='/tmp/malicious/git'):
            with self.assertRaisesRegex(ValueError, 'Stable executable resolution changed'):
                self.effective()
        self.assert_no_start()

    def test_unbound_interpreter_rejected(self):
        with patch.object(self.p.sys, 'executable', '/tmp/unbound-python'):
            with self.assertRaisesRegex(ValueError, 'unbound Python interpreter'):
                self.effective()
        self.assert_no_start()

    def test_missing_or_weakened_environment_contract_rejected(self):
        for change in ['missing', 'path', 'runtime_key', 'tool']:
            a = copy.deepcopy(self.a)
            if change == 'missing':
                del a['execution_environment']
            elif change == 'path':
                a['execution_environment']['path'] = '/tmp:/usr/bin:/bin'
            elif change == 'runtime_key':
                del a['environment']['LD_PRELOAD']
            else:
                del a['execution_environment']['tools']['bash']
            with self.subTest(change=change), self.assertRaises(ValueError):
                self.p.stable_child_environment(a, os.environ)
        self.assert_no_start()

    def test_pending_manifest_is_not_authorized(self):
        self.a.update(status='pending_explicit_approval', approval_evidence=None)
        self.write()
        result, out = self.invoke(env=self.effective())
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('No explicitly approved manifest', result.stderr)
        self.assertFalse(out.exists())
        self.assert_no_start()

    def test_runtime_library_hash_tamper_no_trial(self):
        key = next(iter(self.a['runtime_file_sha256']))
        self.a['runtime_file_sha256'][key] = '0' * 64
        self.write()
        result, out = self.invoke(env=self.effective())
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('stable approval runtime file bindings differ', result.stderr)
        self.assert_no_start()

    def test_git_diff_binding_still_checked(self):
        self.a['git_binding']['working_diff_sha256'] = '0' * 64
        self.write()
        result, out = self.invoke(env=self.effective())
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('Stable approval Git state differs', result.stderr)
        self.assertFalse(out.exists())
        self.assert_no_start()

    def test_legacy_path_binding_not_relaxed(self):
        self.f.manifest.write_text(json.dumps(self.legacy))
        changed = {**os.environ, 'PATH': '/new/session:' + os.environ['PATH']}
        result, out = self.invoke(env=changed)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('Runtime environment differs: PATH', result.stderr)
        self.assertFalse(out.exists())
        self.assert_no_start()

    def test_historical_manifest_and_receipt_unchanged_after_new_contract_trial(self):
        self.f.manifest.write_text(json.dumps(self.legacy))
        old_manifest = self.f.manifest
        old_bytes = old_manifest.read_bytes()
        result, _ = self.invoke(env=dict(os.environ))
        self.assertEqual(result.returncode, 0, result.stderr)
        with sqlite3.connect(self.f.ledger) as db:
            old_rows = db.execute('SELECT * FROM trials').fetchall()
        self.a['approval_id'] = 'NEW_STABLE_CPU_APPROVAL'
        self.f.manifest = self.f.ap / 'stable.json'
        self.write()
        result, out = self.invoke('CPU-SKIP', self.effective())
        self.assertEqual(result.returncode, 77, result.stderr)
        self.f.verify_log(out)
        self.assertEqual(old_manifest.read_bytes(), old_bytes)
        with sqlite3.connect(self.f.ledger) as db:
            rows = db.execute('SELECT * FROM trials WHERE approval_id=?', (self.legacy['approval_id'],)).fetchall()
        self.assertEqual(rows, old_rows)


if __name__ == '__main__':
    unittest.main(verbosity=2)
