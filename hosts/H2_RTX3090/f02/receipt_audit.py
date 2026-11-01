"""Exact ten-trial audit. Read-only live access, portable artifact validation.
A consumed failed trial (historical FOREIGN) is a valid receipt, not a data PASS.
"""
from pathlib import Path
import hashlib,json,sqlite3,sys
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
sys.path.insert(0,str(ROOT/'hosts/H2_RTX3090/c1/reproducers'));import provenance as p
API_BASE='hosts/H2_RTX3090/results/c1/s01_api_pilot_20261031/'
EXPECTED=(
 ('H2-S01-OWN-API-PILOT-20261031-01','S01-OWN-API','s01_own_api_pilot_GO_20261031.json',API_BASE+'S01-OWN-API'),
 ('H2-S01-INDEX-API-PILOT-20261031-01','S01-INDEX-API','s01_index_api_pilot_20261031.json',API_BASE+'S01-INDEX-API'),
 ('H2-S01-SIZE-API-PILOT-20261031-01','S01-SIZE-API','s01_size_api_pilot_20261031.json',API_BASE+'S01-SIZE-API'),
 ('H2-S01-FOREIGN-API-PILOT-20261031-01','S01-FOREIGN-API','s01_foreign_api_pilot_20261031.json',API_BASE+'S01-FOREIGN-API'),
 ('H2-S01-CLONE-API-PILOT-20261031-01','S01-CLONE-API','s01_clone_api_pilot_20261031.json',API_BASE+'S01-CLONE-API'),
 ('H2-S01-OWN-DATA-PILOT-20261031-01','S01-OWN-DATA','s01_own_data_pilot_20261031.json','hosts/H2_RTX3090/results/c1/s01_data_pilot_20261031/S01-OWN-DATA'),
 ('H2-S01-INDEX-DATA-STABLE-ENV-PILOT-20261101-01','S01-INDEX-DATA','s01_index_data_stable_env_pilot_20261101.json','hosts/H2_RTX3090/results/c1/s01_data_pilot_20261101/S01-INDEX-DATA_STABLE_ENV'),
 ('H2-S01-SIZE-DATA-STABLE-ENV-PILOT-20261101-01','S01-SIZE-DATA','s01_size_data_stable_env_pilot_20261101.json','hosts/H2_RTX3090/results/c1/s01_data_pilot_20261101/S01-SIZE-DATA_STABLE_ENV'),
 ('H2-F01-REJECTED-FOREIGN-STATE-PILOT-20261101-01','H2-F01','f01_rejected_foreign_state_pilot_20261101.json','hosts/H2_RTX3090/results/c1/f01_pilot_20261101/H2-F01'),
 ('H2-S01-CLONE-CONTROL-DATA-REBIND-PILOT-20261101-01','S01-CLONE-CONTROL-DATA','s01_clone_control_data_rebind_pilot_20261101.json','hosts/H2_RTX3090/results/c1/s01_data_pilot_20261101/S01-CLONE-CONTROL-DATA_REBIND'))
LEDGER_ID='10859f6a-779a-4dbe-99ca-73a355ade12b'
def need(ok,why):
 if not ok:raise ValueError(why)
def load(file):return json.loads(file.read_text())
def verify_artifacts(root,rows,metadata,schema):
 root=Path(root);need(metadata==[[LEDGER_ID,1]] or metadata==[(LEDGER_ID,1)],'Wrong ledger UUID/version');need(schema==p.LEDGER_SCHEMA,'Wrong ledger schema')
 expected={(aid,case,1):(manifest,run) for aid,case,manifest,run in EXPECTED};now={tuple(x[:3]):x for x in rows}
 need(len(rows)==len(now)==10 and set(now)==set(expected),'Exactly the ten enumerated historical trials are required')
 verified=[]
 for key,(manifest_name,relative_run) in expected.items():
  aid,case,number=key;_,_,_,raw,digest=now[key];need(hashlib.sha256(raw.encode()).hexdigest()==digest,'Receipt digest differs: '+aid)
  rec=json.loads(raw);need((rec['approval_id'],rec['case_id'],rec['repeat_number'])==key and rec['ledger_id']==LEDGER_ID and rec['reservation_state']=='permanently_consumed_before_spawn','Receipt identity/state differs: '+aid)
  manifest=root/'hosts/H2_RTX3090/c1/approvals'/manifest_name;run=root/relative_run;a=load(manifest);i=load(run/'run.json')
  need(a['approval_id']==aid and a['allowed_cases']==[case] and type(a['max_repeats']) is int and a['max_repeats']==1 and a['status']=='explicitly_approved' and bool(a['approval_evidence']),'Historical approval scope/evidence differs: '+aid)
  need(p.sha(manifest)==rec['approval_manifest_sha256']==i['approval_manifest_sha256']==p.sha(run/'approval_used.json'),'Historical approval manifest hash differs: '+aid)
  need(i['case']==case and i['repeat']==number and i['approval_id']==aid and i['argv']==[a['binary_path'],'--approved-run',case],'Run case/repeat/argv differs: '+aid)
  need(i['trial_ledger']['reservation']==rec and i['trial_ledger']['reservation_sha256']==digest,'Run/ledger receipt differs: '+aid)
  bind=rec['binding']
  for field in ['binary_path','binary_sha256','workspace_sha256']:need(bind.get(field)==a[field],'Receipt '+field+' differs: '+aid)
  need(bind['binary_sha256']==i['binary_sha256'] and bind['plan_sha256']==a['plan_sha256']==i['test_plan_sha256'] and bind['workspace_sha256']==i['workspace_before_sha256'],'Historical material bindings differ: '+aid)
  need(bind['git']==i['git_before'] and a['git_mode']==i['git_before']['mode'] and a['git_head']==i['git_before'].get('head'),'Historical Git provenance differs: '+aid)
  need(set(a['environment'])==set(p.ENV_KEYS) and a['environment']==i['environment'],'Historical run environment differs: '+aid)
  if 'environment' in bind:need(bind['environment']==a['environment'],'Receipt environment differs: '+aid)
  if 'execution_environment' in bind:need(bind['execution_environment']==a.get('execution_environment')==i.get('execution_environment'),'Receipt stable-program policy differs: '+aid)
  for field,file in [('stdout_sha256','stdout.jsonl'),('stderr_sha256','stderr.log'),('workspace_before_record_sha256','workspace_before.json'),('workspace_after_record_sha256','workspace_after.json')]:need(i[field]==p.sha(run/file),'Raw artifact hash differs: '+relative_run+'/'+file)
  for moment in ['before','after']:
   w=load(run/f'workspace_{moment}.json');canonical=json.dumps(w['files'],sort_keys=True,separators=(',',':')).encode();need(hashlib.sha256(canonical).hexdigest()==w['workspace_sha256']==i[f'workspace_{moment}_sha256'],'Historical workspace snapshot differs: '+aid)
  need(i['workspace_before_sha256']==i['workspace_after_sha256'],'Historical run changed its source workspace: '+aid)
  # Legacy receipts predate the stable-policy fields: validate their recorded
  # values in their own snapshot, never substitute today's environment.
  verified.append({'approval_id':aid,'case':case,'repeat':number,'receipt_sha256':digest,'approval_manifest_path':str(manifest.relative_to(root)),'approval_manifest_sha256':p.sha(manifest),'run_path':relative_run,'run_json_sha256':p.sha(run/'run.json'),'binary_sha256':a['binary_sha256'],'plan_sha256':a['plan_sha256'],'historical_workspace_sha256':a['workspace_sha256'],'run_outcome':i['outcome'],'child_returncode':i['returncode'],'receipt_integrity':'PASS'})
 return verified

def read_and_verify(root=ROOT,ledger=None):
 ledger=p.trial_ledger_path() if ledger is None else Path(ledger);p._check_ledger_file(ledger);before=p.sha(ledger)
 db=sqlite3.connect(ledger.as_uri()+'?mode=ro',uri=True)
 try:
  db.execute('PRAGMA query_only=ON');need(db.execute('PRAGMA integrity_check').fetchall()==[('ok',)],'Ledger integrity_check failed')
  metadata=db.execute('SELECT * FROM metadata').fetchall();schema=dict(db.execute('SELECT name,sql FROM sqlite_master WHERE sql IS NOT NULL'));rows=db.execute('SELECT * FROM trials').fetchall()
 finally:db.close()
 verified=verify_artifacts(root,rows,metadata,schema);after=p.sha(ledger);need(before==after,'Ledger changed during audit')
 return {'outcome':'PASS','open_mode':'mode=ro; query_only=ON','ledger_path':str(ledger),'ledger_sha256_before':before,'ledger_sha256_after':after,'metadata':[list(x) for x in metadata],'schema':schema,'rows':[list(x) for x in rows],'trials':len(rows),'verified_receipts':verified,'note':'Read-only receipt evidence, never import/restore as a live ledger. No result status is rewritten.'}
if __name__=='__main__':print(json.dumps(read_and_verify(),indent=2))
