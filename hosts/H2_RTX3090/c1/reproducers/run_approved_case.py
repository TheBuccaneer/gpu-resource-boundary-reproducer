#!/usr/bin/env python3
"""One explicitly approved future case; never automatically repeats or builds.
CPU-only end-to-end tested; never used here to execute an OpenCL process.
"""
import argparse
import datetime
import hashlib
import json
import os
from pathlib import Path
import resource
import signal
import subprocess
import sys
import time
sys.path.insert(0, str(Path(__file__).resolve().parent))  # Explicit reviewed sibling imports under python -I.
from provenance import ENV_KEYS, validate_approval, sealed_executable, workspace_provenance, trial_ledger_path, reserve_trial, validate_execution_environment

C1 = Path(__file__).resolve().parents[1]
WALL_SECONDS = 60
TRIAL_LEDGER = trial_ledger_path()
REQUIRED_LIBRARIES = tuple('/usr/lib/x86_64-linux-gnu/' + n for n in
    ('libOpenCL.so.1.0.0', 'libnvidia-opencl.so.580.178.04', 'libcuda.so.580.178.04'))


def sha(p):
    h = hashlib.sha256()
    with p.open('rb') as f:
        for block in iter(lambda: f.read(1024 * 1024), b''):
            h.update(block)
    return h.hexdigest()


def limits():
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    resource.setrlimit(resource.RLIMIT_CPU, (45, 45))
    resource.setrlimit(resource.RLIMIT_FSIZE, (16 * 1024 * 1024, 16 * 1024 * 1024))


def rss_group(group):
    total = 0
    for p in Path('/proc').iterdir():
        if not p.name.isdigit():
            continue
        try:
            if os.getpgid(int(p.name)) != group:
                continue
            for line in (p / 'status').read_text().splitlines():
                if line.startswith('VmRSS:'):
                    total += int(line.split()[1]) * 1024
        except (OSError, ProcessLookupError, ValueError):
            pass
    return total


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--execute-approved-plan', action='store_true')
    parser.add_argument('--approval-note', required=True)
    parser.add_argument('--approval-manifest', required=True, type=Path)
    parser.add_argument('--case', required=True)
    parser.add_argument('--binary', required=True, type=Path)
    parser.add_argument('--out', required=True, type=Path)
    parser.add_argument('--repeat-number', type=int, choices=range(1, 6), required=True)
    a = parser.parse_args()
    if not a.execute_approved_plan:
        parser.error('Explicit external approval is required before --execute-approved-plan may be used.')
    plan = json.loads((C1 / 'TEST_CASES.json').read_text())
    known = {r['id']: r for r in plan['cases']}
    known['B12-Q'] = plan['b12_query']
    plan_path = C1 / 'TEST_CASES.json'
    s01_path = C1.parent / 's01/TEST_CASES.json'
    if s01_path.exists():
        s01 = json.loads(s01_path.read_text())
        for row in s01['cases']:
            if row['id'] in known:
                parser.error('Duplicate case ID in S01 plan.')
            known[row['id']] = row
        if a.case in {row['id'] for row in s01['cases']}:
            plan, plan_path = s01, s01_path
    if a.case not in known:
        parser.error('Unknown case: no process started.')
    binary = a.binary.resolve(strict=True)
    # C1=/repo/hosts/H2/c1: parents[2] is repository root.
    root = C1.parents[2]
    approval_path = a.approval_manifest.resolve(strict=True)
    approval_dir = (C1 / 'approvals').resolve()
    if not approval_path.is_relative_to(approval_dir):
        parser.error('Store the reviewed approval manifest under c1/approvals/.')
    approval_bytes = approval_path.read_bytes()
    approval = json.loads(approval_bytes)
    # Verify opted-in tool bindings BEFORE invoking Git for workspace provenance.
    execution_environment = validate_execution_environment(approval, os.environ)
    workspace_before = workspace_provenance(root)
    binary = validate_approval(approval, binary, a.case, a.repeat_number,
                               workspace_before, sha(plan_path), os.environ)
    execution_fd, execution_snapshot = sealed_executable(binary, approval.get('binary_sha256'))
    out = a.out.resolve()
    if not out.is_relative_to((C1.parent / 'results/c1').resolve()):
        os.close(execution_fd)
        parser.error('Run outputs must be in the excluded generated H2 results/c1 directory.')
    out.mkdir(parents=True, exist_ok=False)
    # Targeted file-integrity checks against existing C0, no new device inventory.
    raw = C1.parent / 'inventory/2026-10-31_C0_release'
    references = json.loads((raw / 'library_hashes.json').read_text())
    libraries = []
    for name in REQUIRED_LIBRARIES:
        matches = [r for r in references if r.get('resolved') == name]
        if not matches:
            raise SystemExit('STOP: missing C0 library reference: ' + name)
        ref = matches[0]
        value = sha(Path(ref['resolved']))
        libraries.append({'path': ref['resolved'], 'sha256': value, 'c0_sha256': ref['sha256']})
        if value != ref['sha256']:
            raise SystemExit('STOP: source C0 library no longer matches: ' + name)
    if execution_environment is not None and approval.get('runtime_file_sha256') != {
            row['path']: row['sha256'] for row in libraries}:
        raise SystemExit('STOP: stable approval runtime file bindings differ from C0/current files.')
    args = [str(binary), '--approved-query-run'] if a.case == 'B12-Q' else [str(binary), '--approved-run', a.case]
    info = {'case': a.case, 'approval_package': known[a.case]['approval_package'],
            'approval_note': a.approval_note, 'repeat': a.repeat_number, 'argv': args,
            'binary_sha256': execution_snapshot['sha256'], 'executable_snapshot': execution_snapshot,
            'approval_manifest_sha256': hashlib.sha256(approval_bytes).hexdigest(),
            'approval_id': approval['approval_id'], 'libraries_before': libraries,
            'workspace_before_sha256': workspace_before['workspace_sha256'],
            'git_before': workspace_before['git'],
            'sources_sha256': {p.name: sha(p) for p in sorted((C1 / 'reproducers').glob('*')) if p.is_file()},
            'test_plan_sha256': sha(plan_path), 'test_plan_path': str(plan_path),
            'wall_limit_seconds': WALL_SECONDS,
            'spec_commit': plan['spec_commit'], 'cts_commit': plan['cts_commit'],
            'source_c0_opencl_sha256': sha(raw / 'opencl_query.stdout'),
            'original_c1_input_handover_zip_sha256': '3aacc0512a5af7bf972d65a2dfdbb0184d181053b79af8c487f1c3eacf3935c1',
            'environment': {k: os.environ.get(k) for k in ENV_KEYS},
            'execution_environment': execution_environment,
            'start_utc': datetime.datetime.now(datetime.timezone.utc).isoformat()}
    (out / 'workspace_before.json').write_text(json.dumps(workspace_before, indent=2))
    (out / 'approval_used.json').write_bytes(approval_bytes)
    (out / 'run.json').write_text(json.dumps(info, indent=2))
    reason = None
    peak = 0
    child = None
    launch_error = None
    with (out / 'stdout.jsonl').open('wb') as stdout, (out / 'stderr.log').open('wb') as stderr:
        start = time.monotonic()
        try:
            # Durable consumption is independent of --out and precedes every exec.
            try:
                info['trial_ledger'] = reserve_trial(TRIAL_LEDGER, approval, a.case, a.repeat_number,
                    info['approval_manifest_sha256'], {
                        'binary_sha256': info['binary_sha256'], 'binary_path': str(binary),
                        'plan_sha256': info['test_plan_sha256'],
                        'workspace_sha256': info['workspace_before_sha256'],
                        'environment': info['environment'],
                        'execution_environment': execution_environment,
                        'git': info['git_before']})
                (out / 'run.json').write_text(json.dumps(info, indent=2))
            except Exception as exc:
                reason = 'trial_ledger_rejected'
                info['trial_ledger_error'] = str(exc)
                raise OSError('Trial ledger rejected: ' + str(exc)) from exc
            child = subprocess.Popen(args, executable=f'/proc/self/fd/{execution_fd}', pass_fds=(execution_fd,),
                                     stdout=stdout, stderr=stderr, start_new_session=True, preexec_fn=limits)
        except OSError as exc:
            reason = reason or 'process_start_failed'
            launch_error = str(exc)
        finally:
            os.close(execution_fd)
        while child is not None and child.poll() is None:
            current = rss_group(child.pid)
            peak = max(peak, current)
            if time.monotonic() - start > WALL_SECONDS:
                reason = 'wall_timeout'
            elif current > 2 * 1024**3:
                reason = 'rss_limit'
            elif sum((out / name).stat().st_size for name in ['stdout.jsonl', 'stderr.log']) > 16 * 1024**2:
                reason = 'output_limit'
            if reason:
                try:
                    os.killpg(child.pid, signal.SIGTERM)
                except ProcessLookupError:
                    pass
                try:
                    child.wait(timeout=1)
                except subprocess.TimeoutExpired:
                    try:
                        os.killpg(child.pid, signal.SIGKILL)
                    except ProcessLookupError:
                        pass
                    try:
                        child.wait(timeout=2)
                    except subprocess.TimeoutExpired:
                        reason += "_termination_unconfirmed"
                break
            time.sleep(0.05)
    if child is not None and child.returncode is not None and rss_group(child.pid) > 0:
        reason = reason or 'remaining_process_group'
        try:
            os.killpg(child.pid, signal.SIGKILL)
        except ProcessLookupError:
            pass
    code = child.returncode if child is not None else None
    if reason is None and code not in (0, 77, 78):
        reason = 'child_signal' if code is not None and code < 0 else 'child_exit_nonzero'
    workspace_after = workspace_provenance(root)
    (out / 'workspace_after.json').write_text(json.dumps(workspace_after, indent=2))
    if workspace_after['workspace_sha256'] != workspace_before['workspace_sha256']:
        reason = reason or 'workspace_changed_during_run'
    info.update({'git_after': workspace_after['git'], 'workspace_after_sha256': workspace_after['workspace_sha256'],
                 'workspace_before_record_sha256': sha(out / 'workspace_before.json'),
                 'workspace_after_record_sha256': sha(out / 'workspace_after.json'), 'end_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
                 'elapsed_seconds': time.monotonic() - start, 'returncode': code, 'launch_error': launch_error,
                 'termination_signal': -code if code is not None and code < 0 else None,
                 'outcome': 'failed' if reason else ('capability_skip' if code == 77 else ('inconclusive' if code == 78 else 'passed')),
                 'approval_used_sha256': sha(out / 'approval_used.json'),
                 'stop_reason': reason, 'observed_process_group_peak_rss_bytes': peak,
                 'gpu_completion_guaranteed_by_process_exit': False,
                 'stdout_sha256': sha(out / 'stdout.jsonl'), 'stderr_sha256': sha(out / 'stderr.log')})
    (out / 'run.json').write_text(json.dumps(info, indent=2))
    print('One case completed; no retries or next cases were started. Review', out / 'run.json')
    return 20 if reason else (code if code in (77, 78) else 0)


if __name__ == '__main__':
    raise SystemExit(main())
