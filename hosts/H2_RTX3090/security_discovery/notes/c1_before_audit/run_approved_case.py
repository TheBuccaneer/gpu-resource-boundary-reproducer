#!/usr/bin/env python3
"""One explicitly approved future case; never automatically repeats or builds.
This file has only been statically checked, not used to execute an OpenCL process.
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
import time

C1 = Path(__file__).resolve().parents[1]


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
    if a.case not in known:
        parser.error('Unknown case: no process started.')
    binary = a.binary.resolve(strict=True)
    out = a.out.resolve()
    out.mkdir(parents=True, exist_ok=False)
    # Targeted file-integrity checks against existing C0, no new device inventory.
    raw = C1.parent / 'inventory/2026-10-31_C0_release'
    references = json.loads((raw / 'library_hashes.json').read_text())
    libraries = []
    for name in ['libOpenCL.so.1.0.0', 'libnvidia-opencl.so.580.178.04', 'libcuda.so.580.178.04']:
        matches = [r for r in references if r.get('resolved') == '/usr/lib/x86_64-linux-gnu/' + name]
        if not matches:
            raise SystemExit('STOP: missing C0 library reference: ' + name)
        ref = matches[0]
        value = sha(Path(ref['resolved']))
        libraries.append({'path': ref['resolved'], 'sha256': value, 'c0_sha256': ref['sha256']})
        if value != ref['sha256']:
            raise SystemExit('STOP: source C0 library no longer matches: ' + name)
    args = [str(binary), '--approved-query-run'] if a.case == 'B12-Q' else [str(binary), '--approved-run', a.case]
    info = {'case': a.case, 'approval_package': known[a.case]['approval_package'],
            'approval_note': a.approval_note, 'repeat': a.repeat_number, 'argv': args,
            'binary_sha256': sha(binary), 'libraries_before': libraries,
            'sources_sha256': {p.name: sha(p) for p in sorted((C1 / 'reproducers').glob('*')) if p.is_file()},
            'test_plan_sha256': sha(C1 / 'TEST_CASES.json'),
            'spec_commit': plan['spec_commit'], 'cts_commit': plan['cts_commit'],
            'source_c0_opencl_sha256': sha(raw / 'opencl_query.stdout'),
            'input_handover_zip_sha256': '3aacc0512a5af7bf972d65a2dfdbb0184d181053b79af8c487f1c3eacf3935c1',
            'environment': {k: os.environ.get(k) for k in ['OCL_ICD_VENDORS', 'OCL_ICD_FILENAMES', 'LD_LIBRARY_PATH', 'LD_PRELOAD', 'CUDA_VISIBLE_DEVICES']},
            'start_utc': datetime.datetime.now(datetime.timezone.utc).isoformat()}
    (out / 'run.json').write_text(json.dumps(info, indent=2))
    reason = None
    peak = 0
    with (out / 'stdout.jsonl').open('wb') as stdout, (out / 'stderr.log').open('wb') as stderr:
        start = time.monotonic()
        child = subprocess.Popen(args, stdout=stdout, stderr=stderr, start_new_session=True, preexec_fn=limits)
        while child.poll() is None:
            current = rss_group(child.pid)
            peak = max(peak, current)
            if time.monotonic() - start > 60:
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
    if child.returncode is not None and rss_group(child.pid) > 0:
        reason = reason or 'remaining_process_group'
        try:
            os.killpg(child.pid, signal.SIGKILL)
        except ProcessLookupError:
            pass
    info.update({'end_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
                 'elapsed_seconds': time.monotonic() - start, 'returncode': child.returncode,
                 'stop_reason': reason, 'observed_process_group_peak_rss_bytes': peak,
                 'gpu_completion_guaranteed_by_process_exit': False,
                 'stdout_sha256': sha(out / 'stdout.jsonl'), 'stderr_sha256': sha(out / 'stderr.log')})
    (out / 'run.json').write_text(json.dumps(info, indent=2))
    print('One case completed; no retries or next cases were started. Review', out / 'run.json')
    return 20 if reason else (0 if child.returncode == 0 else 20)


if __name__ == '__main__':
    raise SystemExit(main())
