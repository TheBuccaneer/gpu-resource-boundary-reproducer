"""CPU-only artifact fixtures. All approval files/ledgers are private temp data."""
from pathlib import Path
import contextlib,copy,hashlib,importlib.util,io,json,sqlite3,sys,tempfile,unittest
from unittest.mock import patch
HERE=Path(__file__).resolve().parents[1];sys.path.insert(0,str(HERE));import receipt_audit as r;import preflight as m
p=r.p
class LedgerTests(unittest.TestCase):
 def setUp(self):
  self.temp=tempfile.TemporaryDirectory(prefix='clone-rebind-cpu-');self.addCleanup(self.temp.cleanup);self.root=Path(self.temp.name);self.ledger=self.root/'private-state/ledger.sqlite3';identity=p.initialize_trial_ledger(self.ledger);self.uuid=identity['ledger_id'];mock=patch.object(r,'LEDGER_ID',self.uuid);mock.start();self.addCleanup(mock.stop)
  for n,(aid,case,name,path) in enumerate(r.EXPECTED):
   ap=self.root/'hosts/H2_RTX3090/c1/approvals'/name;ap.parent.mkdir(parents=True,exist_ok=True);run=self.root/path;run.mkdir(parents=True)
   ws={'workspace_sha256':hashlib.sha256(b'[]').hexdigest(),'files':[]};git={'mode':'repository','head':'c'*40,'branch':'CPU_ONLY','dirty':False,'working_diff_sha256':'0'*64,'staged_diff_sha256':'0'*64}
   env={key:('/usr/bin:/bin' if n>=6 else '/CPU_ONLY_OLD_PATH') if key=='PATH' else None for key in p.ENV_KEYS}
   a={'schema_version':1,'status':'explicitly_approved','approval_evidence':'Private CPU fixture only, no actual GPU authorization','approval_id':aid,'allowed_cases':[case],'max_repeats':1,'binary_path':'/CPU_ONLY_NOT_EXECUTED/'+case,'binary_sha256':'b'*64,'plan_sha256':'p'*64,'workspace_sha256':ws['workspace_sha256'],'git_mode':git['mode'],'git_head':git['head'],'environment':env,'ledger_path':str(self.ledger),'ledger_id':self.uuid}
   if n>=6:a['execution_environment']={'version':1,'path':'/usr/bin:/bin','tools':p.stable_tool_snapshot()}
   ap.write_text(json.dumps(a));(run/'approval_used.json').write_bytes(ap.read_bytes());(run/'stdout.jsonl').write_text('CPU artifact fixture only, no execution\n');(run/'stderr.log').write_text('')
   for moment in ['before','after']:(run/f'workspace_{moment}.json').write_text(json.dumps(ws))
   bind={'binary_path':a['binary_path'],'binary_sha256':a['binary_sha256'],'plan_sha256':a['plan_sha256'],'workspace_sha256':a['workspace_sha256'],'git':git}
   if n>=6:bind.update(environment=env,execution_environment=a['execution_environment'])
   receipt=p.reserve_trial(self.ledger,a,case,1,p.sha(ap),bind)
   i={'case':case,'repeat':1,'approval_id':aid,'argv':[a['binary_path'],'--approved-run',case],'approval_manifest_sha256':p.sha(ap),'binary_sha256':a['binary_sha256'],'test_plan_sha256':a['plan_sha256'],'workspace_before_sha256':ws['workspace_sha256'],'workspace_after_sha256':ws['workspace_sha256'],'git_before':git,'environment':env,'trial_ledger':receipt,'outcome':'failed' if case=='S01-FOREIGN-API' else 'passed','returncode':20 if case=='S01-FOREIGN-API' else 0}
   if n>=6:i['execution_environment']=a['execution_environment']
   for field,file in [('stdout_sha256','stdout.jsonl'),('stderr_sha256','stderr.log'),('workspace_before_record_sha256','workspace_before.json'),('workspace_after_record_sha256','workspace_after.json')]:i[field]=p.sha(run/file)
   (run/'run.json').write_text(json.dumps(i))
  self.proof=r.read_and_verify(self.root,self.ledger);self.baseline={'ledger_rows':copy.deepcopy(self.proof['rows']),'ledger_sha256':self.proof['ledger_sha256_after']}
 def audit(self):return r.read_and_verify(self.root,self.ledger)
 def test_exact_ten_and_read_only_bytes(self):
  before=p.sha(self.ledger);proof=self.audit();self.assertEqual(proof['trials'],10);self.assertEqual(p.sha(self.ledger),before);m.match_frozen_ledger(proof,self.baseline)
 def test_foreign_failure_is_a_valid_consumed_receipt(self):
  row=next(x for x in self.proof['verified_receipts'] if x['case']=='S01-FOREIGN-API');self.assertEqual(row['receipt_integrity'],'PASS');self.assertEqual((row['run_outcome'],row['child_returncode']),('failed',20))
 def test_extra_trial_is_never_accepted(self):
  a={'approval_id':'CPU_UNPLANNED','allowed_cases':['CPU_UNKNOWN'],'max_repeats':1,'ledger_path':str(self.ledger),'ledger_id':self.uuid};p.reserve_trial(self.ledger,a,'CPU_UNKNOWN',1,'CPU_ONLY',{})
  with self.assertRaisesRegex(ValueError,'Exactly the ten'):self.audit()
 def test_missing_trial_rejected(self):
  with sqlite3.connect(self.ledger) as db:db.execute('DELETE FROM trials WHERE approval_id=?',(r.EXPECTED[0][0],));db.commit()
  with self.assertRaises(ValueError):self.audit()
 def test_case_or_repeat_mismatch_rejected(self):
  with sqlite3.connect(self.ledger) as db:db.execute('UPDATE trials SET repeat_number=2 WHERE approval_id=?',(r.EXPECTED[-1][0],));db.commit()
  with self.assertRaises(ValueError):self.audit()
 def test_corrupt_receipt_hash_rejected(self):
  with sqlite3.connect(self.ledger) as db:db.execute('UPDATE trials SET record_sha256=? WHERE approval_id=?',('0'*64,r.EXPECTED[-1][0]));db.commit()
  with self.assertRaisesRegex(ValueError,'Receipt digest'):self.audit()
 def test_rehashed_modified_receipt_not_laundered(self):
  row=self.proof['rows'][-1];rec=json.loads(row[3]);rec['binding']['binary_sha256']='0'*64;raw=json.dumps(rec,sort_keys=True,separators=(',',':'));digest=hashlib.sha256(raw.encode()).hexdigest()
  with sqlite3.connect(self.ledger) as db:db.execute('UPDATE trials SET record_json=?,record_sha256=? WHERE approval_id=?',(raw,digest,row[0]));db.commit()
  with self.assertRaises(ValueError):self.audit()
 def test_approval_change_after_reservation_rejected(self):
  name=r.EXPECTED[-1][2];file=self.root/'hosts/H2_RTX3090/c1/approvals'/name;a=json.loads(file.read_text());a['approval_evidence']='changed';file.write_text(json.dumps(a))
  with self.assertRaisesRegex(ValueError,'manifest hash'):self.audit()
 def test_run_artifact_change_rejected(self):
  file=self.root/r.EXPECTED[-2][3]/'stdout.jsonl';file.write_text('tampered')
  with self.assertRaisesRegex(ValueError,'Raw artifact'):self.audit()
 def test_wrong_UUID_schema_or_unavailable_ledger_rejected(self):
  with sqlite3.connect(self.ledger) as db:db.execute('UPDATE metadata SET ledger_id=?',('0'*36,));db.commit()
  with self.assertRaises(ValueError):self.audit()
  with sqlite3.connect(self.ledger) as db:
   db.execute('UPDATE metadata SET ledger_id=?',(self.uuid,));db.execute('CREATE TRIGGER unexpected AFTER INSERT ON trials BEGIN SELECT 1; END');db.commit()
  with self.assertRaisesRegex(ValueError,'schema'):self.audit()
  self.ledger.chmod(0o400)
  with self.assertRaises(ValueError):self.audit()
 def test_exact_snapshot_rejects_even_internally_consistent_replacement(self):
  altered=copy.deepcopy(self.proof);altered['rows'][0][4]='f'*64
  with self.assertRaises(ValueError):m.match_frozen_ledger(altered,self.baseline)
  altered=copy.deepcopy(self.proof);altered['ledger_sha256_after']='0'*64
  with self.assertRaises(ValueError):m.match_frozen_ledger(altered,self.baseline)
 def test_scope_cannot_expand_or_reuse_old_ID(self):
  valid={'schema_version':1,'approval_id':m.ID,'allowed_cases':[m.CASE],'max_repeats':1,'status':'pending_explicit_approval','approval_evidence':None};m.validate_scope(valid)
  for key,value in [('approval_id','H2-S01-CLONE-CONTROL-DATA-STABLE-ENV-PILOT-20261101-01'),('allowed_cases',[m.CASE,'S01-SIZE-DATA']),('max_repeats',2),('max_repeats',True),('approval_evidence','not actually granted')]:
   with self.subTest(key=key),self.assertRaises(ValueError):m.validate_scope({**valid,key:value})
if __name__=='__main__':unittest.main(verbosity=2)
