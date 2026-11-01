#!/usr/bin/env python3
"""Read-only preflight; never reserves trials, loads OpenCL or launches a child."""
import argparse,hashlib,json,os,sqlite3,stat,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[4]
PILOT=Path(__file__).resolve().parent
sys.path.insert(0,str(ROOT/'hosts/H2_RTX3090/c1/reproducers'))
import provenance as p
CASES=['S01-OWN-API','S01-INDEX-API','S01-SIZE-API','S01-FOREIGN-API','S01-CLONE-API']
APPROVAL=ROOT/'hosts/H2_RTX3090/c1/approvals/s01_api_pilot_20261031.json'
def need(ok,message):
 if not ok:raise ValueError(message)
def check(case=None):
 a=json.loads(APPROVAL.read_text());snapshot=json.loads((APPROVAL.parent/'s01_api_pilot_workspace.json').read_text())
 receipt=json.loads((APPROVAL.parent/'s01_api_pilot_binding.json').read_text())
 need(a['status'] in ['pending_explicit_approval','explicitly_approved'],'Unknown approval state')
 need(a['schema_version']==1 and a['allowed_cases']==CASES and type(a['max_repeats'])==int and a['max_repeats']==1,'Scope must be exactly five API-only pilots, repeat1')
 for key,value in receipt['frozen_manifest_fields'].items():need(a.get(key)==value,'Frozen binding changed: '+key)
 if a['status']=='explicitly_approved':need(bool(a.get('approval_evidence')),'Missing actual approval evidence')
 need(a['plan_sha256']==p.sha(ROOT/'hosts/H2_RTX3090/s01/TEST_CASES.json'),'Plan changed')
 plan=json.loads((ROOT/'hosts/H2_RTX3090/s01/TEST_CASES.json').read_text())
 need(all(next(x for x in plan['cases'] if x['id']==name)['kernel_dispatches']==0 for name in CASES),'Non-API case')
 binary=Path(a['binary_path']);build=json.loads((PILOT/'BUILD_PROVENANCE.json').read_text())
 need(str(binary)==str(PILOT/'h2_s01_api_pilot')==build['binary_path'],'Binary path changed')
 need(binary.is_file() and not binary.is_symlink() and stat.S_IMODE(binary.stat().st_mode)&0o111,'Binary is not executable')
 need(p.sha(binary)==a['binary_sha256']==build['binary_sha256'],'Binary hash changed')
 need(a['environment']=={key:os.environ.get(key) for key in p.ENV_KEYS},'Runtime selection environment changed')
 for lib in build['runtime_file_checks']:need(p.sha(lib['path'])==lib['sha256']==lib['c0_sha256'],'C0 runtime file changed: '+lib['path'])
 workspace=p.workspace_provenance(ROOT)
 need(workspace['workspace_sha256']==a['workspace_sha256']==snapshot['workspace_sha256'],'Workspace content changed; re-freeze needs review')
 # Generated result/approval files may change porcelain status between pilots.
 # Full source content stays bound; HEAD/branch/index/worktree diffs stay fixed.
 for key in ['mode','head','branch','dirty','working_diff_sha256','staged_diff_sha256']:
  need(workspace['git'].get(key)==snapshot['git'].get(key),'Git state changed: '+key)
 need(p.sha(APPROVAL.parent/'s01_api_pilot_workspace.json')==a['workspace_snapshot_sha256'],'Frozen snapshot file changed')
 need(p.sha(PILOT/'BUILD_PROVENANCE.json')==a['build_provenance_sha256'],'Build provenance changed')
 need(a['git_mode']==workspace['git']['mode'] and a['git_head']==workspace['git'].get('head'),'Git binding changed')
 ledger=p.trial_ledger_path();p._check_ledger_file(ledger)
 need(str(ledger)==a['ledger_path'],'Ledger path changed')
 db=sqlite3.connect(ledger.as_uri()+'?mode=ro',uri=True)
 try:
  db.execute('PRAGMA query_only=ON')
  need(db.execute('PRAGMA integrity_check').fetchall()==[('ok',)],'Damaged ledger')
  need(dict(db.execute('SELECT name, sql FROM sqlite_master WHERE sql IS NOT NULL'))==p.LEDGER_SCHEMA,'Ledger schema changed')
  need(db.execute('SELECT * FROM metadata').fetchall()==[(a['ledger_id'],1)],'Ledger identity changed')
  consumed=[]
  for aid,cid,number,data,digest in db.execute('SELECT * FROM trials'):
   record=json.loads(data)
   need(hashlib.sha256(data.encode()).hexdigest()==digest and (record['approval_id'],record['case_id'],record['repeat_number'])==(aid,cid,number) and record['ledger_id']==a['ledger_id'],'Damaged trial receipt')
   if aid==a['approval_id']:
    need(record['approval_manifest_sha256']==p.sha(APPROVAL),'Approval content changed after first use')
    consumed.append((cid,number))
 finally:db.close()
 if case is not None:need(case in CASES and (case,1) not in consumed,'Case is not approved or pilot already consumed')
 return {'preflight':'PASS','status':a['status'],'execution_authorized':a['status']=='explicitly_approved','binary_sha256':a['binary_sha256'],'workspace_sha256':workspace['workspace_sha256'],'git_head':workspace['git'].get('head'),'git_dirty':workspace['git'].get('dirty'),'ledger_id':a['ledger_id'],'consumed_for_this_approval':consumed,'ledger_open_mode':'ro; query_only ON','child_processes_started':0,'OpenCL_calls':0}
if __name__=='__main__':
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--case',choices=CASES);args=ap.parse_args()
 try:print(json.dumps(check(args.case),indent=2))
 except Exception as exc:raise SystemExit('STOP: '+str(exc))
