"""F02 entrypoint against new C+CPU fake in isolated Git/workspace/ledger only."""
from pathlib import Path
import importlib.util,json,os,shutil,sqlite3,subprocess,unittest
B=Path(__file__).resolve().parents[1];ROOT=B.parents[2]
spec=importlib.util.spec_from_file_location('r3_cpu_setup',ROOT/'hosts/H2_RTX3090/s01/tests/test_supervisor_e2e.py');base=importlib.util.module_from_spec(spec);spec.loader.exec_module(base)
class F02SupervisorTests(unittest.TestCase):
 def setUp(self):
  self.f=base.SupervisorE2E('test_pass');self.f.setUp();self.addCleanup(self.f.tearDown);self.p=self.f.p;self.b=self.f.c1.parent/'f02';(self.b/'build').mkdir(parents=True)
  for n in ['run_case.py','preflight.py','PLAN_V1.json','receipt_audit.py','verify_log.py','oracle_values.py']:shutil.copy2(B/n,self.b/n)
  self.binary=self.b/'build/h2_f02'
  subprocess.run(['/usr/bin/gcc','-std=c11','-Wall','-Wextra','-Werror','-Wpedantic','-DF02_CPU_FIXTURE',str(B/'h2_f02.c'),str(B/'tests/fake_opencl.c'),'-o',str(self.binary)],check=True,capture_output=True)
  for n in range(10):
   old={'approval_id':'CPU_OLD_'+str(n),'allowed_cases':['CPU-OLD'],'max_repeats':1,'ledger_path':str(self.f.ledger),'ledger_id':self.f.ledger_identity['ledger_id']};self.p.reserve_trial(self.f.ledger,old,'CPU-OLD',1,'CPU_ONLY',{})
  with sqlite3.connect(self.f.ledger) as db:self.oldrows=db.execute('SELECT * FROM trials').fetchall()
  self.d=self.f.c1.parent/'results/preparation/H2_F02_20261101';self.d.mkdir(parents=True);self.out=self.f.c1.parent/'results/c1/f02_pilot_20261101/F02-CLONE-SNAPSHOT-REVERSE';self.marker=self.f.ledger.parent/'fake.calls'
  history={'historical_files_sha256':{},'ledger_rows':[list(x) for x in self.oldrows],'ledger_sha256':self.p.sha(self.f.ledger)};(self.d/'PRESERVATION_BEFORE.json').write_text(json.dumps(history));(self.d/'BUILD_PROVENANCE.json').write_text('CPU ONLY, not a production build')
  lib=self.f.root/'cpu_library_reference'
  self.f.wrapper.write_text('import sys\nsys.path.insert(0,'+repr(str(self.b))+')\nimport run_case as s\ns.WALL_SECONDS=0.2\ns.TRIAL_LEDGER=s.Path('+repr(str(self.f.ledger))+')\ns.f.p.trial_ledger_path=lambda:s.TRIAL_LEDGER\ns.f.RUNTIME_FILES=('+repr(str(lib))+',)\nimport sqlite3\ndef private_audit(root,ledger):\n with sqlite3.connect(ledger) as db: rows=db.execute("SELECT * FROM trials").fetchall()\n return {"rows":[list(x) for x in rows],"ledger_sha256_before":s.p.sha(ledger),"ledger_sha256_after":s.p.sha(ledger)}\ns.f.audit.read_and_verify=private_audit\nimport verify_log\nreal_assess=verify_log.assess\nverify_log.assess=lambda rows: {**real_assess(rows,"CPU_FAKE_RUNTIME"),"outcome":"PASS_OBSERVED_STATE"} if real_assess(rows,"CPU_FAKE_RUNTIME")["outcome"]=="PASS_CPU_FIXTURE" else real_assess(rows,"CPU_FAKE_RUNTIME")\nraise SystemExit(s.main())\n')
  ws=self.p.workspace_provenance(self.f.root);self.manifest=self.f.ap/'f02_clone_snapshot_reverse_pilot_20261101.json';self.snapshot=self.f.ap/'f02_workspace_20261101.json';self.snapshot.write_text(json.dumps(ws));self.binding=self.f.ap/'f02_clone_snapshot_reverse_binding_20261101.json'
  self.a={'schema_version':1,'status':'explicitly_approved','approval_id':'H2-F02-CLONE-SNAPSHOT-REVERSE-PILOT-20261101-01','approval_evidence':'CPU fake-only test; no production GPU authorization','allowed_cases':['F02-CLONE-SNAPSHOT-REVERSE'],'max_repeats':1,'binary_path':str(self.binary),'binary_sha256':self.p.sha(self.binary),'plan_sha256':self.p.sha(self.b/'PLAN_V1.json'),'workspace_sha256':ws['workspace_sha256'],'workspace_snapshot_sha256':self.p.sha(self.snapshot),'build_provenance_sha256':self.p.sha(self.d/'BUILD_PROVENANCE.json'),'git_mode':ws['git']['mode'],'git_head':ws['git']['head'],'git_binding':{k:ws['git'].get(k) for k in self.p.GIT_BINDING_KEYS},'environment':{k:('/usr/bin:/bin' if k=='PATH' else os.environ.get(k)) for k in self.p.ENV_KEYS},'execution_environment':{'version':1,'path':'/usr/bin:/bin','tools':self.p.stable_tool_snapshot()},'runtime_file_sha256':{str(lib):self.p.sha(lib)},'ledger_path':str(self.f.ledger),'ledger_id':self.f.ledger_identity['ledger_id'],'history_sha256':self.p.sha(self.d/'PRESERVATION_BEFORE.json'),'prepared_artifacts_sha256':{}}
  self.write();self.binding.write_text(json.dumps({'frozen_manifest_fields':{k:v for k,v in self.a.items() if k not in ['status','approval_evidence']}}))
 def write(self):self.manifest.write_text(json.dumps(self.a))
 def invoke(self,mode='valid',execute=True,env=None):
  args=['/usr/bin/python3','-I',str(self.f.wrapper)]+(['--execute-approved-plan'] if execute else [])
  return subprocess.run(args,env={**os.environ,'F02_CPU_SCENARIO':mode,'F02_CPU_MARKER':str(self.marker),**(env or {})},capture_output=True,text=True,timeout=15)
 def count(self):return self.marker.read_text().splitlines().count('dispatch') if self.marker.exists() else 0
 def verify(self):
  i=json.loads((self.out/'run.json').read_text());self.assertEqual(i['case'],'F02-CLONE-SNAPSHOT-REVERSE');self.assertEqual(i['repeat'],1)
  for field,file in [('stdout_sha256','stdout.jsonl'),('stderr_sha256','stderr.log'),('approval_used_sha256','approval_used.json'),('workspace_before_record_sha256','workspace_before.json'),('workspace_after_record_sha256','workspace_after.json')]:self.assertEqual(i[field],self.p.sha(self.out/file))
  self.assertEqual(i['binary_sha256'],self.a['binary_sha256']);self.assertEqual(i['test_plan_sha256'],self.a['plan_sha256']);self.assertEqual(i['environment']['PATH'],'/usr/bin:/bin');self.assertEqual(i['executable_snapshot']['execution'],'sealed_memfd');self.assertEqual(i['workspace_before_sha256'],i['workspace_after_sha256'])
  with sqlite3.connect(self.f.ledger) as db:rows=db.execute('SELECT * FROM trials').fetchall()
  self.assertEqual(rows[:10],self.oldrows);self.assertEqual(len(rows),11);self.assertEqual(rows[-1][4],i['trial_ledger']['reservation_sha256']);self.assertEqual(json.loads(rows[-1][3]),i['trial_ledger']['reservation']);return i
 def test_new_case_CPU_end_to_end(self):
  x=self.invoke();self.assertEqual(x.returncode,0,x.stderr);self.assertEqual(self.verify()['outcome'],'passed');self.assertEqual(self.count(),3)
 def test_actual_integrated_oracle_rejects_CPU_origin_even_after_child_zero(self):
  text=self.f.wrapper.read_text();text='\n'.join(line for line in text.split('\n') if not line.startswith('verify_log.assess=lambda'))+'\n';self.f.wrapper.write_text(text)
  ws=self.p.workspace_provenance(self.f.root);self.snapshot.write_text(json.dumps(ws));self.a.update(workspace_sha256=ws['workspace_sha256'],workspace_snapshot_sha256=self.p.sha(self.snapshot),git_binding={k:ws['git'].get(k) for k in self.p.GIT_BINDING_KEYS});self.write();self.binding.write_text(json.dumps({'frozen_manifest_fields':{k:v for k,v in self.a.items() if k not in ['status','approval_evidence']}}))
  x=self.invoke();self.assertEqual(x.returncode,20,x.stderr);i=self.verify();self.assertEqual(i['returncode'],0);self.assertEqual(i['stop_reason'],'independent_data_oracle_rejected');self.assertEqual(i['independent_data_oracle']['outcome'],'INCONCLUSIVE_PROTOCOL_OR_API');self.assertEqual(self.count(),3)
 def test_pending_never_spawns_or_reserves(self):
  self.a.update(status='pending_explicit_approval',approval_evidence=None);self.write();before=self.p.sha(self.f.ledger);x=self.invoke();self.assertEqual(x.returncode,2,x.stderr);self.assertIn('GO missing',x.stderr);self.assertFalse(self.marker.exists());self.assertFalse(self.out.exists());self.assertEqual(self.p.sha(self.f.ledger),before)
 def test_read_only_preflight_never_spawns_or_reserves(self):
  before=self.p.sha(self.f.ledger);x=self.invoke(execute=False);self.assertEqual(x.returncode,0,x.stderr);self.assertFalse(self.marker.exists());self.assertFalse(self.out.exists());self.assertEqual(self.p.sha(self.f.ledger),before)
 def test_runtime_environment_tamper_rejected(self):
  before=self.p.sha(self.f.ledger);x=self.invoke(env={'OCL_ICD_VENDORS':'CPU_TAMPER'});self.assertNotEqual(x.returncode,0);self.assertFalse(self.marker.exists());self.assertEqual(self.p.sha(self.f.ledger),before)
 def test_resource_error_consumed(self):
  x=self.invoke('clone_error');self.assertEqual(x.returncode,78,x.stderr);self.assertEqual(self.verify()['outcome'],'inconclusive');self.assertEqual(self.count(),0)
 def test_coupled_data_failure_is_not_pass(self):
  x=self.invoke('coupled');self.assertEqual(x.returncode,20,x.stderr);self.assertEqual(self.verify()['returncode'],21);self.assertEqual(self.count(),2)
 def test_timeout_no_retry_and_receipt_retained(self):
  x=self.invoke('hang');self.assertEqual(x.returncode,20,x.stderr);self.assertEqual(self.verify()['stop_reason'],'wall_timeout');count=self.count();shutil.rmtree(self.out);again=self.invoke();self.assertNotEqual(again.returncode,0);self.assertEqual(self.count(),count)
 def test_changed_manifest_between_check_and_start_is_rejected(self):
  old=self.f.wrapper.read_text();self.f.wrapper.write_text(old.replace('raise SystemExit(s.main())',"a,e,w,r=s.f.check(s.os.environ)\nafter=dict(a);after['approval_evidence']='Changed after check'\ns.f.MANIFEST.write_text(s.json.dumps(after))\ntry:s.execute(a,e,w)\nexcept ValueError as exc:print(str(exc));raise SystemExit(2)"))
  ws=self.p.workspace_provenance(self.f.root);self.snapshot.write_text(json.dumps(ws));self.a.update(workspace_sha256=ws['workspace_sha256'],workspace_snapshot_sha256=self.p.sha(self.snapshot),git_binding={k:ws['git'].get(k) for k in self.p.GIT_BINDING_KEYS});self.write();self.binding.write_text(json.dumps({'frozen_manifest_fields':{k:v for k,v in self.a.items() if k not in ['status','approval_evidence']}}))
  before=self.p.sha(self.f.ledger);x=self.invoke();self.assertEqual(x.returncode,2,x.stderr);self.assertIn('Approval changed after preflight',x.stdout);self.assertFalse(self.marker.exists());self.assertFalse(self.out.exists());self.assertEqual(self.p.sha(self.f.ledger),before)
 def test_old_case_is_not_a_supported_cli_option(self):
  x=subprocess.run(['/usr/bin/python3','-I',str(self.f.wrapper),'--case','S01-ROLLBACK-DATA'],capture_output=True,text=True);self.assertEqual(x.returncode,2);self.assertFalse(self.out.exists());self.assertFalse(self.marker.exists())
 def test_parallel_calls_can_start_at_most_once(self):
  args=['/usr/bin/python3','-I',str(self.f.wrapper),'--execute-approved-plan'];env={**os.environ,'F02_CPU_MARKER':str(self.marker)}
  children=[subprocess.Popen(args,env=env,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True) for _ in range(3)]
  codes=[]
  for child in children:child.communicate(timeout=15);codes.append(child.returncode)
  self.assertEqual(codes.count(0),1);self.assertEqual(self.count(),3);self.verify()
if __name__=='__main__':unittest.main(verbosity=2)
