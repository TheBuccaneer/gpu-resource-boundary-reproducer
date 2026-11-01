"""F02 material bindings and read-only host ledger. No OpenCL imports/calls."""
from pathlib import Path
import receipt_audit as audit
import json,os,sqlite3,sys,hashlib
B=Path(__file__).resolve().parent;ROOT=B.parents[2];C1=ROOT/'hosts/H2_RTX3090/c1'
sys.path.insert(0,str(C1/'reproducers'))
import provenance as p
CASE='F02-CLONE-SNAPSHOT-REVERSE';ID='H2-F02-CLONE-SNAPSHOT-REVERSE-PILOT-20261101-01'
PLAN=B/'PLAN_V1.json';BINARY=B/'build/h2_f02'
MANIFEST=C1/'approvals/f02_clone_snapshot_reverse_pilot_20261101.json'
BINDING=C1/'approvals/f02_clone_snapshot_reverse_binding_20261101.json'
WORKSPACE=C1/'approvals/f02_workspace_20261101.json'
D=ROOT/'hosts/H2_RTX3090/results/preparation/H2_F02_20261101'
HISTORY=D/'PRESERVATION_BEFORE.json';BUILD=D/'BUILD_PROVENANCE.json'
OUTPUT=ROOT/'hosts/H2_RTX3090/results/c1/f02_pilot_20261101/F02-CLONE-SNAPSHOT-REVERSE'
RUNTIME_FILES=tuple('/usr/lib/x86_64-linux-gnu/'+n for n in ['libOpenCL.so.1.0.0','libnvidia-opencl.so.580.178.04','libcuda.so.580.178.04'])
def need(ok,why):
 if not ok:raise ValueError(why)
def validate_scope(a):
 need(a['schema_version']==1 and a['approval_id']==ID and a['allowed_cases']==[CASE] and type(a['max_repeats']) is int and a['max_repeats']==1,'Wrong F02 identity/scope/budget')
 need(a['status'] in ['pending_explicit_approval','explicitly_approved'],'Invalid approval status')
 need(bool(a['approval_evidence']) if a['status']=='explicitly_approved' else a['approval_evidence'] is None,'Approval evidence/state mismatch')
def match_frozen_ledger(proof,history):
 need(proof['rows']==history['ledger_rows'] and proof['ledger_sha256_before']==proof['ledger_sha256_after']==history['ledger_sha256'],'Ledger differs from frozen ten receipts')
 need(not any(row[0]==ID or row[1]==CASE for row in proof['rows']),'F02 already consumed')
def check(ambient):
 a=json.loads(MANIFEST.read_text());frozen=json.loads(BINDING.read_text())['frozen_manifest_fields']
 need(set(a)==set(frozen)|{'status','approval_evidence'} and all(a[k]==v for k,v in frozen.items()),'F02 frozen approval fields changed')
 validate_scope(a)
 need(a['status'] in ['pending_explicit_approval','explicitly_approved'],'Invalid approval status')
 need(bool(a['approval_evidence']) if a['status']=='explicitly_approved' else a['approval_evidence'] is None,'Approval evidence/state mismatch')
 for file,digest in a['prepared_artifacts_sha256'].items():need(p.sha(ROOT/file)==digest,'Prepared artifact changed: '+file)
 env=p.stable_child_environment(a,ambient);tools=p.validate_execution_environment(a,env)
 workspace=p.workspace_provenance(ROOT)
 need(workspace['workspace_sha256']==a['workspace_sha256'],'Workspace changed')
 need((a['git_mode'],a['git_head'])==(workspace['git']['mode'],workspace['git'].get('head')),'Git identity changed')
 need({k:workspace['git'].get(k) for k in p.GIT_BINDING_KEYS}==a['git_binding'],'Git state changed')
 need(p.sha(WORKSPACE)==a['workspace_snapshot_sha256'] and p.sha(BUILD)==a['build_provenance_sha256'],'Build/workspace evidence changed')
 need(str(BINARY)==a['binary_path'] and BINARY.is_file() and not BINARY.is_symlink() and os.access(BINARY,os.X_OK),'F02 binary path/type changed')
 need(p.sha(BINARY)==a['binary_sha256'] and p.sha(PLAN)==a['plan_sha256'],'F02 binary/plan changed')
 plan=json.loads(PLAN.read_text());need(plan['plan_id']==CASE and plan['plan_version']==1 and len(plan['cases'])==1 and plan['cases'][0]['id']==CASE and plan['cases'][0]['repeat_processes']==1,'Wrong F02 plan')
 references=json.loads((C1.parent/'inventory/2026-10-31_C0_release/library_hashes.json').read_text())
 need(set(a['runtime_file_sha256'])==set(RUNTIME_FILES),'Incomplete runtime binding')
 for file,h in a['runtime_file_sha256'].items():need(p.sha(file)==h and any(x.get('resolved')==file and x['sha256']==h for x in references),'Runtime/C0 file changed')
 need(p.sha(HISTORY)==a['history_sha256'],'Historical baseline changed')
 history=json.loads(HISTORY.read_text())
 for file,h in history['historical_files_sha256'].items():need(p.sha(ROOT/file)==h,'Historical S01/H1/C1 artifact changed: '+file)
 ledger=p.trial_ledger_path();need(str(ledger)==a['ledger_path'],'Ledger path changed')
 proof=audit.read_and_verify(ROOT,ledger);match_frozen_ledger(proof,history);rows=proof['rows'];before=proof['ledger_sha256_after']
 need(not OUTPUT.exists(),'F02 output already exists; never retry')
 if a['status']=='explicitly_approved':p.validate_approval(a,BINARY,CASE,1,workspace,a['plan_sha256'],env)
 return a,env,workspace,{'preflight':'PASS','case':CASE,'status':a['status'],'execution_authorized':a['status']=='explicitly_approved','approval_id':ID,'repeat':1,'ambient_PATH':ambient.get('PATH'),'effective_PATH':env['PATH'],'programs':tools,'workspace_sha256':workspace['workspace_sha256'],'binary_sha256':a['binary_sha256'],'plan_sha256':a['plan_sha256'],'ledger_trials':len(rows),'ledger_sha256':before,'historical_files_unchanged':len(history['historical_files_sha256']),'runtime_processes_started':0,'GPU_dispatches':0}
