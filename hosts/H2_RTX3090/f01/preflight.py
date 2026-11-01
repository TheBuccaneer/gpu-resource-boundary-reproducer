"""F01 material bindings and read-only host ledger. No OpenCL imports/calls."""
from pathlib import Path
import json,os,sqlite3,sys,hashlib
B=Path(__file__).resolve().parent;ROOT=B.parents[2];C1=ROOT/'hosts/H2_RTX3090/c1'
sys.path.insert(0,str(C1/'reproducers'))
import provenance as p
CASE='H2-F01';ID='H2-F01-REJECTED-FOREIGN-STATE-PILOT-20261101-01'
PLAN=B/'PLAN_V1.json';BINARY=B/'build/h2_f01'
MANIFEST=C1/'approvals/f01_rejected_foreign_state_pilot_20261101.json'
BINDING=C1/'approvals/f01_rejected_foreign_state_binding_20261101.json'
WORKSPACE=C1/'approvals/f01_workspace_20261101.json'
D=ROOT/'hosts/H2_RTX3090/results/preparation/H2_F01_20261101'
HISTORY=D/'PRESERVATION_BEFORE.json';BUILD=D/'BUILD_PROVENANCE.json'
OUTPUT=ROOT/'hosts/H2_RTX3090/results/c1/f01_pilot_20261101/H2-F01'
RUNTIME_FILES=tuple('/usr/lib/x86_64-linux-gnu/'+n for n in ['libOpenCL.so.1.0.0','libnvidia-opencl.so.580.178.04','libcuda.so.580.178.04'])
def need(ok,why):
 if not ok:raise ValueError(why)
def check(ambient):
 a=json.loads(MANIFEST.read_text());frozen=json.loads(BINDING.read_text())['frozen_manifest_fields']
 need(set(a)==set(frozen)|{'status','approval_evidence'} and all(a[k]==v for k,v in frozen.items()),'F01 frozen approval fields changed')
 need(a['schema_version']==1 and a['approval_id']==ID and a['allowed_cases']==[CASE] and type(a['max_repeats']) is int and a['max_repeats']==1,'Wrong F01 identity/scope/budget')
 need(a['status'] in ['pending_explicit_approval','explicitly_approved'],'Invalid approval status')
 need(bool(a['approval_evidence']) if a['status']=='explicitly_approved' else a['approval_evidence'] is None,'Approval evidence/state mismatch')
 env=p.stable_child_environment(a,ambient);tools=p.validate_execution_environment(a,env)
 workspace=p.workspace_provenance(ROOT)
 need(workspace['workspace_sha256']==a['workspace_sha256'],'Workspace changed')
 need({k:workspace['git'].get(k) for k in p.GIT_BINDING_KEYS}==a['git_binding'],'Git state changed')
 need(p.sha(WORKSPACE)==a['workspace_snapshot_sha256'] and p.sha(BUILD)==a['build_provenance_sha256'],'Build/workspace evidence changed')
 need(str(BINARY)==a['binary_path'] and BINARY.is_file() and not BINARY.is_symlink() and os.access(BINARY,os.X_OK),'F01 binary path/type changed')
 need(p.sha(BINARY)==a['binary_sha256'] and p.sha(PLAN)==a['plan_sha256'],'F01 binary/plan changed')
 plan=json.loads(PLAN.read_text());need(plan['plan_id']==CASE and plan['plan_version']==1 and len(plan['cases'])==1 and plan['cases'][0]['id']==CASE and plan['cases'][0]['repeat_processes']==1,'Wrong F01 plan')
 references=json.loads((C1.parent/'inventory/2026-10-31_C0_release/library_hashes.json').read_text())
 need(set(a['runtime_file_sha256'])==set(RUNTIME_FILES),'Incomplete runtime binding')
 for file,h in a['runtime_file_sha256'].items():need(p.sha(file)==h and any(x.get('resolved')==file and x['sha256']==h for x in references),'Runtime/C0 file changed')
 need(p.sha(HISTORY)==a['history_sha256'],'Historical baseline changed')
 history=json.loads(HISTORY.read_text())
 for file,h in history['historical_files_sha256'].items():need(p.sha(ROOT/file)==h,'Historical S01/H1/C1 artifact changed: '+file)
 ledger=p.trial_ledger_path();need(str(ledger)==a['ledger_path'],'Ledger path changed');p._check_ledger_file(ledger);before=p.sha(ledger)
 db=sqlite3.connect(ledger.as_uri()+'?mode=ro',uri=True)
 try:
  db.execute('PRAGMA query_only=ON');need(db.execute('PRAGMA integrity_check').fetchall()==[('ok',)],'Ledger damaged')
  need(dict(db.execute('SELECT name,sql FROM sqlite_master WHERE sql IS NOT NULL'))==p.LEDGER_SCHEMA,'Ledger schema changed')
  need(db.execute('SELECT * FROM metadata').fetchall()==[(a['ledger_id'],1)],'Ledger UUID changed')
  rows=db.execute('SELECT * FROM trials').fetchall();need([list(x) for x in rows]==history['ledger_rows'] and len(rows)==8,'Historical receipts changed/trial added')
  for aid,cid,n,raw,h in rows:
   rec=json.loads(raw);need(hashlib.sha256(raw.encode()).hexdigest()==h and (rec['approval_id'],rec['case_id'],rec['repeat_number'])==(aid,cid,n) and rec['ledger_id']==a['ledger_id'],'Invalid receipt')
   need(aid!=ID,'F01 already consumed')
 finally:db.close()
 need(before==p.sha(ledger)==history['ledger_sha256'],'Ledger bytes changed')
 need(not OUTPUT.exists(),'F01 output already exists; never retry')
 if a['status']=='explicitly_approved':p.validate_approval(a,BINARY,CASE,1,workspace,a['plan_sha256'],env)
 return a,env,workspace,{'preflight':'PASS','case':CASE,'status':a['status'],'execution_authorized':a['status']=='explicitly_approved','approval_id':ID,'repeat':1,'ambient_PATH':ambient.get('PATH'),'effective_PATH':env['PATH'],'programs':tools,'workspace_sha256':workspace['workspace_sha256'],'binary_sha256':a['binary_sha256'],'plan_sha256':a['plan_sha256'],'ledger_trials':len(rows),'ledger_sha256':before,'historical_files_unchanged':len(history['historical_files_sha256']),'runtime_processes_started':0,'GPU_dispatches':0}
