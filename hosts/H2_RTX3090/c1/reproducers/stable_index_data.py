#!/usr/bin/python3
"""New INDEX-DATA fixed-environment contract. Default is read-only preflight.
No historical API approval/helper is used or modified. Native execution needs GO.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import sqlite3
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
import provenance as p

C1 = Path(__file__).resolve().parents[1]
ROOT = C1.parents[2]
CASE = 'S01-INDEX-DATA'
ID = 'H2-S01-INDEX-DATA-STABLE-ENV-PILOT-20261101-01'
MANIFEST = C1 / 'approvals/s01_index_data_stable_env_pilot_20261101.json'
BINDING = C1 / 'approvals/s01_index_data_stable_env_binding_20261101.json'
OUTPUT = C1.parent / 'results/c1/s01_data_pilot_20261101/S01-INDEX-DATA_STABLE_ENV'


def need(ok, message):
    if not ok:
        raise ValueError(message)


def preflight(ambient):
    a = json.loads(MANIFEST.read_bytes())
    binding = json.loads(BINDING.read_bytes())
    frozen = binding['frozen_manifest_fields']
    need(set(a) == set(frozen) | {'status', 'approval_evidence'}, 'Manifest fields changed')
    need(all(a[k] == v for k, v in frozen.items()), 'Frozen manifest binding changed')
    need(a['status'] in ('pending_explicit_approval', 'explicitly_approved'), 'Invalid approval state')
    need(a['approval_id'] == ID and a['allowed_cases'] == [CASE]
         and type(a['max_repeats']) is int and a['max_repeats'] == 1, 'Wrong case/identity/budget')
    need(a['schema_version'] == 1, 'Wrong approval schema')
    need(not OUTPUT.exists(), 'Trial output already exists; stop, never retry')
    effective = p.stable_child_environment(a, ambient)
    environment = p.validate_execution_environment(a, effective)
    # Absolute, hash-verified Git; never ambient-PATH Git and no old API preflight.
    workspace = p.workspace_provenance(ROOT)
    need(workspace['workspace_sha256'] == a['workspace_sha256'], 'Workspace changed')
    need({k: workspace['git'].get(k) for k in p.GIT_BINDING_KEYS} == a['git_binding'], 'Git state changed')
    need((a['git_mode'], a['git_head']) == (workspace['git']['mode'], workspace['git'].get('head')), 'Git identity changed')
    binary = Path(a['binary_path'])
    need(binary == ROOT / 'hosts/H2_RTX3090/s01/pilot/h2_s01_api_pilot'
         and binary.is_file() and not binary.is_symlink() and os.access(binary, os.X_OK), 'Wrong binary path/type')
    need(p.sha(binary) == a['binary_sha256'], 'Binary changed')
    need(p.sha(C1.parent / 's01/TEST_CASES.json') == a['plan_sha256'], 'Plan changed')
    need(p.sha(C1 / 'approvals/s01_index_data_stable_env_workspace_20261101.json')
         == a['workspace_snapshot_sha256'], 'Workspace snapshot changed')
    need(p.sha(C1.parent / 's01/pilot/BUILD_PROVENANCE.json') == a['build_provenance_sha256'], 'Build provenance changed')
    references = json.loads((C1.parent / 'inventory/2026-10-31_C0_release/library_hashes.json').read_text())
    required = ['/usr/lib/x86_64-linux-gnu/' + x for x in
                ('libOpenCL.so.1.0.0', 'libnvidia-opencl.so.580.178.04', 'libcuda.so.580.178.04')]
    need(set(a['runtime_file_sha256']) == set(required), 'Incomplete runtime file binding')
    for lib, digest in a['runtime_file_sha256'].items():
        need(p.sha(lib) == digest and any(x.get('resolved') == lib and x['sha256'] == digest for x in references),
             'Runtime/C0 file changed: ' + lib)
    history_path = ROOT / binding['history_path']
    need(p.sha(history_path) == binding['history_sha256'], 'Historical preservation baseline changed')
    history = json.loads(history_path.read_text())
    for file, digest in history['historical_files_sha256'].items():
        need(p.sha(ROOT / file) == digest, 'Historical artifact changed: ' + file)
    for file in json.loads((ROOT / 'notes/H1_RELOCATION_2026-10-31.json').read_text())['files']:
        need(p.sha(ROOT / file['new_path']) == file['sha256'], 'H1 original changed')
    ledger = p.trial_ledger_path()
    need(str(ledger) == a['ledger_path'], 'Ledger path changed')
    p._check_ledger_file(ledger)
    before = p.sha(ledger)
    db = sqlite3.connect(ledger.as_uri() + '?mode=ro', uri=True)
    try:
        db.execute('PRAGMA query_only=ON')
        need(db.execute('PRAGMA integrity_check').fetchall() == [('ok',)], 'Ledger damaged')
        need(dict(db.execute('SELECT name, sql FROM sqlite_master WHERE sql IS NOT NULL')) == p.LEDGER_SCHEMA,
             'Ledger schema changed')
        need(db.execute('SELECT * FROM metadata').fetchall() == [(a['ledger_id'], 1)], 'Ledger UUID changed')
        rows = db.execute('SELECT * FROM trials').fetchall()
        need([list(x) for x in rows] == history['ledger_rows'], 'Historical ledger receipts changed or trial added')
        for aid, cid, repeat, data, digest in rows:
            record = json.loads(data)
            need(hashlib.sha256(data.encode()).hexdigest() == digest
                 and (record['approval_id'], record['case_id'], record['repeat_number']) == (aid, cid, repeat)
                 and record['ledger_id'] == a['ledger_id'], 'Invalid historical receipt')
            need(aid != ID, 'New approval already consumed')
    finally:
        db.close()
    need(p.sha(ledger) == before == history['ledger_sha256'], 'Ledger changed since preparation')
    if a['status'] == 'explicitly_approved':
        p.validate_approval(a, binary, CASE, 1, workspace, a['plan_sha256'], effective)
    else:
        need(a['approval_evidence'] is None, 'Pending manifest has approval evidence')
    return a, effective, {
        'preflight': 'PASS', 'case': CASE, 'approval_id': ID, 'repeat': 1,
        'status': a['status'], 'execution_authorized': a['status'] == 'explicitly_approved',
        'ambient_PATH': ambient.get('PATH'), 'effective_PATH': effective['PATH'],
        'PATH_policy': 'explicit fixed-path-v1; ambient PATH recorded, replaced only under new contract',
        'execution_environment': environment, 'workspace_sha256': workspace['workspace_sha256'],
        'binary_sha256': p.sha(binary), 'plan_sha256': a['plan_sha256'],
        'ledger_sha256': before, 'ledger_trials': len(rows), 'historical_files_unchanged': len(history['historical_files_sha256']),
        'H1_originals_unchanged': 729, 'runtime_trials_started': 0, 'GPU_dispatches_executed': 0}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--execute-approved-plan', action='store_true')
    args = parser.parse_args()
    try:
        approval, effective, result = preflight(os.environ)
        print(json.dumps(result, indent=2), flush=True)
        if not args.execute_approved_plan:
            return 0
        need(approval['status'] == 'explicitly_approved', 'New explicit GO missing; no supervisor or trial started')
        command = ['/usr/bin/python3', '-I', str(C1 / 'reproducers/run_approved_case.py'),
                   '--execute-approved-plan', '--approval-note', 'New fixed-path-v1 INDEX-DATA only; one trial, no retry',
                   '--approval-manifest', str(MANIFEST), '--binary', approval['binary_path'],
                   '--case', CASE, '--repeat-number', '1', '--out', str(OUTPUT)]
        # Replace launcher with the absolute checked interpreter; no shell/PATH resolution.
        os.execve(command[0], command, effective)
    except (OSError, ValueError, KeyError, sqlite3.Error) as exc:
        print('STOP: ' + str(exc), file=sys.stderr)
        return 2


if __name__ == '__main__':
    raise SystemExit(main())
