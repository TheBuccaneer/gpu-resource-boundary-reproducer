"""Real supervisor CLI in an isolated temp Git repository; no OpenCL loading."""
from contextlib import closing
import importlib.util,json,os,shutil,subprocess,tempfile,unittest,sqlite3,time,signal
from pathlib import Path
ROOT=Path(__file__).resolve().parents[4]
REPRO=ROOT/'hosts/H2_RTX3090/c1/reproducers'
class SupervisorE2E(unittest.TestCase):
 def setUp(self):
  self.tmp=tempfile.TemporaryDirectory(prefix='s01-cpu-e2e-');self.root=Path(self.tmp.name)
  self.c1=self.root/'hosts/H2_RTX3090/c1';self.repro=self.c1/'reproducers';self.repro.mkdir(parents=True)
  for n in ['run_approved_case.py','provenance.py']:shutil.copy2(REPRO/n,self.repro/n)
  self.binary=self.root/'cpu_fixture'
  subprocess.run(['gcc','-std=c11','-Wall','-Wextra','-Werror',str(Path(__file__).with_name('cpu_fixture.c')),'-o',str(self.binary)],check=True)
  self.cases=['CPU-PASS','CPU-FAIL','CPU-SIGNAL','CPU-TIMEOUT','CPU-SKIP','CPU-INCONCLUSIVE']
  (self.c1/'TEST_CASES.json').write_text(json.dumps({'cases':[{'id':n,'approval_package':'CPU_ONLY'} for n in self.cases],'b12_query':{'id':'B12-Q','approval_package':'CPU_ONLY'},'spec_commit':'fixture','cts_commit':'fixture'}))
  self.raw=self.c1.parent/'inventory/2026-10-31_C0_release';self.raw.mkdir(parents=True)
  lib=self.root/'cpu_library_reference';lib.write_bytes(b'not a library; never loaded')
  spec=importlib.util.spec_from_file_location('fixture_provenance',self.repro/'provenance.py');self.p=importlib.util.module_from_spec(spec);spec.loader.exec_module(self.p)
  (self.raw/'library_hashes.json').write_text(json.dumps([{'resolved':str(lib),'sha256':self.p.sha(lib)}]))
  (self.raw/'opencl_query.stdout').write_text('SYNTHETIC CPU BASELINE ONLY')
  # No production bypass CLI is added. Only this isolated wrapper substitutes
  # CPU reference paths and a short timeout; the actual main() path is unchanged.
  self.ledger_temp=tempfile.TemporaryDirectory(prefix="s01-test-state-")
  self.ledger=Path(self.ledger_temp.name)/"state/ledger.sqlite3"
  self.ledger_identity=self.p.initialize_trial_ledger(self.ledger)
  self.spawn_marker=self.ledger.parent/"spawn-attempts"
  self.wrapper=self.root/'invoke.py'
  self.wrapper.write_text('import sys\nsys.path.insert(0,'+repr(str(self.repro))+')\nimport run_approved_case as s\ns.WALL_SECONDS=0.25\ns.TRIAL_LEDGER=s.Path('+repr(str(self.ledger))+')\ns.REQUIRED_LIBRARIES=('+repr(str(lib))+',)\noriginal=s.subprocess.Popen\ndef watched(*args,**kw):\n if str(kw.get("executable","")).startswith("/proc/self/fd/"):\n  with open('+repr(str(self.spawn_marker))+',"a") as marker: marker.write("spawn\\n")\n result=original(*args,**kw)\n if str(kw.get("executable","")).startswith("/proc/self/fd/"):\n  with open('+repr(str(self.ledger.parent/'child-pid'))+',"w") as marker: marker.write(str(result.pid))\n return result\ns.subprocess.Popen=watched\nraise SystemExit(s.main())\n')
  (self.root/'.gitignore').write_text('hosts/H2_RTX3090/results/\nhosts/H2_RTX3090/c1/approvals/\n**/__pycache__/\n')
  subprocess.run(['git','init','-q',str(self.root)],check=True)
  subprocess.run(['git','-C',str(self.root),'-c','user.name=CPU Fixture','-c','user.email=fixture@invalid.test','commit','-q','--allow-empty','-m','CPU fixture only'],check=True)
  self.ap=self.c1/'approvals';self.ap.mkdir()
  ws=self.p.workspace_provenance(self.root)
  self.approval={'schema_version':1,'status':'explicitly_approved','approval_id':'CPU_FIXTURE_ONLY','approval_evidence':'Temporary CPU integration test, no GPU authorization','binary_path':str(self.binary),'binary_sha256':self.p.sha(self.binary),'allowed_cases':self.cases,'max_repeats':1,'ledger_path':str(self.ledger),'ledger_id':self.ledger_identity['ledger_id'],'plan_sha256':self.p.sha(self.c1/'TEST_CASES.json'),'workspace_sha256':ws['workspace_sha256'],'git_mode':ws['git']['mode'],'git_head':ws['git']['head'],'environment':{k:os.environ.get(k) for k in self.p.ENV_KEYS}}
  self.manifest=self.ap/'cpu.json';self.manifest.write_text(json.dumps(self.approval))
 def tearDown(self):self.tmp.cleanup();self.ledger_temp.cleanup()
 def command(self,case,binary=None,repeat=1,out_name=None):
  out=self.c1.parent/'results/c1'/(out_name or case)
  return ['python3',str(self.wrapper),'--execute-approved-plan','--approval-note','CPU fixture only','--approval-manifest',str(self.manifest),'--case',case,'--binary',str(binary or self.binary),'--out',str(out),'--repeat-number',str(repeat)],out
 def invoke(self,case,binary=None,repeat=1,out_name=None):
  command,out=self.command(case,binary,repeat,out_name)
  return subprocess.run(command,capture_output=True,text=True,timeout=15),out
 def verify_log(self,out):
  info=json.loads((out/'run.json').read_text())
  for field,name in [('stdout_sha256','stdout.jsonl'),('stderr_sha256','stderr.log'),('approval_used_sha256','approval_used.json'),('workspace_before_record_sha256','workspace_before.json'),('workspace_after_record_sha256','workspace_after.json')]:self.assertEqual(info[field],self.p.sha(out/name))
  self.assertEqual(info['binary_sha256'],self.approval['binary_sha256'])
  self.assertEqual(info['test_plan_sha256'],self.approval['plan_sha256'])
  self.assertEqual(info['git_before']['head'],self.approval['git_head']);self.assertTrue(info['git_before']['dirty'])
  self.assertIn('invoke.py',info['git_before']['status_porcelain_v1_z'])
  self.assertEqual(info['workspace_before_sha256'],info['workspace_after_sha256'])
  self.assertEqual(info['executable_snapshot']['execution'],'sealed_memfd')
  ledger=info['trial_ledger'];self.assertEqual(ledger['ledger_id'],self.approval['ledger_id'])
  import hashlib
  canonical=json.dumps(ledger['reservation'],sort_keys=True,separators=(',',':')).encode()
  self.assertEqual(ledger['reservation_sha256'],hashlib.sha256(canonical).hexdigest())
  self.assertEqual(ledger['reservation']['approval_manifest_sha256'],self.p.sha(self.manifest))
  self.assertEqual(ledger['reservation']['binding']['workspace_sha256'],info['workspace_before_sha256'])
  self.assertEqual(ledger['reservation']['approval_id'],self.approval['approval_id'])
  self.assertEqual(ledger['reservation']['case_id'],info['case'])
  self.assertEqual(ledger['reservation']['binding']['binary_sha256'],info['binary_sha256'])
  self.assertEqual(ledger['reservation']['binding']['plan_sha256'],info['test_plan_sha256'])
  self.assertEqual(ledger['reservation']['binding']['git'],info['git_before'])
  with closing(sqlite3.connect(self.ledger)) as db:
   stored=db.execute('SELECT record_sha256 FROM trials WHERE approval_id=? AND case_id=? AND repeat_number=?',(self.approval['approval_id'],info['case'],info['repeat'])).fetchone()
  self.assertEqual(stored,(ledger['reservation_sha256'],))
  return info
 def test_pass(self):
  x,out=self.invoke('CPU-PASS');self.assertEqual(x.returncode,0,x.stderr);i=self.verify_log(out);self.assertEqual(i['outcome'],'passed');self.assertIn('CPU_ONLY',(out/'stdout.jsonl').read_text())
 def test_failure_exit(self):
  x,out=self.invoke('CPU-FAIL');self.assertEqual(x.returncode,20,x.stderr);i=self.verify_log(out);self.assertEqual((i['returncode'],i['stop_reason']),(7,'child_exit_nonzero'))
 def test_signal(self):
  x,out=self.invoke('CPU-SIGNAL');self.assertEqual(x.returncode,20,x.stderr);i=self.verify_log(out);self.assertEqual((i['returncode'],i['termination_signal'],i['stop_reason']),(-15,15,'child_signal'))
 def test_timeout_kill_escalation(self):
  x,out=self.invoke('CPU-TIMEOUT');self.assertEqual(x.returncode,20,x.stderr);i=self.verify_log(out);self.assertEqual(i['stop_reason'],'wall_timeout');self.assertEqual(i['returncode'],-9)
 def test_skip(self):
  x,out=self.invoke('CPU-SKIP');self.assertEqual(x.returncode,77,x.stderr);self.assertEqual(self.verify_log(out)['outcome'],'capability_skip')
 def test_inconclusive(self):
  x,out=self.invoke('CPU-INCONCLUSIVE');self.assertEqual(x.returncode,78,x.stderr);i=self.verify_log(out);self.assertEqual(i['outcome'],'inconclusive');self.assertIsNone(i['stop_reason'])
 def test_unapproved_path(self):
  x,out=self.invoke('CPU-PASS',self.wrapper);self.assertNotEqual(x.returncode,0);self.assertFalse(out.exists());self.assertIn('Binary path is not approved',x.stderr)
 def test_unapproved_hash(self):
  self.approval['binary_sha256']='0'*64;self.manifest.write_text(json.dumps(self.approval))
  x,out=self.invoke('CPU-PASS');self.assertNotEqual(x.returncode,0);self.assertFalse(out.exists());self.assertIn('Binary SHA-256 differs',x.stderr)
 def test_s01_plan_binding(self):
  s01=self.c1.parent/'s01';s01.mkdir()
  catalog=s01/'TEST_CASES.json';catalog.write_text(json.dumps({'cases':[{'id':'S01-FIXTURE','approval_package':'CPU_ONLY'}],'spec_commit':'s01-fixture','cts_commit':'fixture'}))
  self.approval['allowed_cases'].append('S01-FIXTURE')
  self.approval['workspace_sha256']=self.p.workspace_provenance(self.root)['workspace_sha256']
  self.manifest.write_text(json.dumps(self.approval))
  x,out=self.invoke('S01-FIXTURE');self.assertNotEqual(x.returncode,0);self.assertFalse(out.exists());self.assertIn('Plan or workspace',x.stderr)
  self.approval['plan_sha256']=self.p.sha(catalog);self.manifest.write_text(json.dumps(self.approval))
  x,out=self.invoke('S01-FIXTURE');self.assertEqual(x.returncode,0,x.stderr)
  info=self.verify_log(out);self.assertEqual(info['test_plan_path'],str(catalog));self.assertEqual(info['spec_commit'],'s01-fixture')
 def test_start_failure_logged(self):
  # ELF magic passes initial byte checks, malformed ELF fails in execve.
  self.binary.write_bytes(b'\x7fELFmalformed');ws=self.p.workspace_provenance(self.root)
  self.approval.update(binary_sha256=self.p.sha(self.binary),workspace_sha256=ws['workspace_sha256']);self.manifest.write_text(json.dumps(self.approval))
  x,out=self.invoke('CPU-PASS');self.assertEqual(x.returncode,20,x.stderr);i=self.verify_log(out);self.assertEqual(i['stop_reason'],'process_start_failed');self.assertIsNone(i['returncode']);self.assertTrue(i['launch_error'])
 def starts(self):
  return len(self.spawn_marker.read_text().splitlines()) if self.spawn_marker.exists() else 0
 def assert_rejected_without_start(self,case='CPU-PASS',out_name='replay',repeat=1):
  before=self.starts();x,out=self.invoke(case,repeat=repeat,out_name=out_name)
  self.assertEqual(x.returncode,20,x.stderr);self.assertEqual(self.starts(),before)
  info=json.loads((out/'run.json').read_text());self.assertEqual(info['stop_reason'],'trial_ledger_rejected')
  self.assertIsNone(info['returncode']);self.assertEqual((out/'stdout.jsonl').read_bytes(),b'')
  return info
 def test_replay_other_out_and_result_cleanup(self):
  x,out=self.invoke('CPU-PASS');self.assertEqual(x.returncode,0,x.stderr);self.assertEqual(self.starts(),1)
  shutil.rmtree(out)
  info=self.assert_rejected_without_start();self.assertIn('already consumed',info['trial_ledger_error'])
 def test_parallel_same_trial_only_one_child(self):
  commands=[self.command('CPU-PASS',out_name='parallel-'+str(i)) for i in range(4)]
  children=[subprocess.Popen(cmd,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True) for cmd,_ in commands]
  codes=[]
  for child in children:
   stdout,stderr=child.communicate(timeout=15);codes.append(child.returncode)
  self.assertEqual(sorted(codes),[0,20,20,20]);self.assertEqual(self.starts(),1)
  with closing(sqlite3.connect(self.ledger)) as db:self.assertEqual(db.execute('SELECT COUNT(*) FROM trials').fetchone()[0],1)
 def test_timeout_remains_consumed(self):
  x,out=self.invoke('CPU-TIMEOUT');self.assertEqual(x.returncode,20,x.stderr)
  self.assertEqual(self.verify_log(out)['stop_reason'],'wall_timeout')
  self.assert_rejected_without_start('CPU-TIMEOUT','after-timeout')
 def test_supervisor_crash_remains_consumed(self):
  cmd,out=self.command('CPU-TIMEOUT');supervisor=subprocess.Popen(cmd,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
  pidfile=self.ledger.parent/'child-pid';child_pid=None
  try:
   deadline=time.monotonic()+5
   while not pidfile.exists() and time.monotonic()<deadline:time.sleep(0.005)
   self.assertTrue(pidfile.exists());child_pid=int(pidfile.read_text())
   supervisor.kill();supervisor.communicate(timeout=5)
  finally:
   if supervisor.poll() is None:supervisor.kill();supervisor.communicate(timeout=5)
   if child_pid:
    try:os.killpg(child_pid,signal.SIGKILL)
    except ProcessLookupError:pass
  self.assert_rejected_without_start('CPU-TIMEOUT','after-crash')
 def test_distinct_approved_repetition_numbers(self):
  self.approval['max_repeats']=2;self.manifest.write_text(json.dumps(self.approval))
  for number in [1,2]:
   x,out=self.invoke('CPU-PASS',repeat=number,out_name='repeat-'+str(number));self.assertEqual(x.returncode,0,x.stderr)
   self.assertEqual(self.verify_log(out)['trial_ledger']['reservation']['repeat_number'],number)
  self.assertEqual(self.starts(),2)
  self.assert_rejected_without_start(repeat=2)
  x,out=self.invoke('CPU-PASS',repeat=3,out_name='outside-scope');self.assertNotEqual(x.returncode,0);self.assertEqual(self.starts(),2)
 def test_missing_ledger_fails_closed(self):
  self.ledger.unlink();self.assert_rejected_without_start();self.assertFalse(self.ledger.exists())
 def test_corrupt_ledger_fails_closed(self):
  self.ledger.write_bytes(b'corrupt sqlite database');self.assert_rejected_without_start()
 def test_invalid_schema_fails_closed(self):
  with closing(sqlite3.connect(self.ledger)) as db:db.execute('DROP TABLE trials')
  self.assert_rejected_without_start()
 def test_unexpected_trigger_fails_closed(self):
  with closing(sqlite3.connect(self.ledger)) as db:
   db.execute("CREATE TRIGGER erase_trial AFTER INSERT ON trials BEGIN DELETE FROM trials; END")
  self.assert_rejected_without_start()
 def test_unavailable_ledger_fails_closed(self):
  self.ledger.chmod(0o400);self.assert_rejected_without_start()
 def test_wrong_ledger_identity_fails_closed(self):
  self.approval['ledger_id']='00000000-0000-0000-0000-000000000000';self.manifest.write_text(json.dumps(self.approval));self.assert_rejected_without_start()
 def test_approval_id_cannot_change_manifest_to_reuse_budget(self):
  x,_=self.invoke('CPU-PASS');self.assertEqual(x.returncode,0,x.stderr)
  self.approval['max_repeats']=2;self.manifest.write_text(json.dumps(self.approval))
  info=self.assert_rejected_without_start(repeat=2);self.assertIn('another manifest',info['trial_ledger_error'])
 def test_failed_exec_remains_consumed(self):
  self.binary.write_bytes(b'\x7fELFmalformed');ws=self.p.workspace_provenance(self.root)
  self.approval.update(binary_sha256=self.p.sha(self.binary),workspace_sha256=ws['workspace_sha256']);self.manifest.write_text(json.dumps(self.approval))
  x,_=self.invoke('CPU-PASS');self.assertEqual(x.returncode,20,x.stderr);self.assertEqual(self.starts(),1)
  self.assert_rejected_without_start()
 def test_reservation_persists_after_post_commit_sync_error(self):
  original=self.p._sync_ledger
  def fail(path):raise OSError('CPU injected sync failure after commit')
  self.p._sync_ledger=fail
  try:
   with self.assertRaises(OSError):self.p.reserve_trial(self.ledger,self.approval,'CPU-PASS',1,self.p.sha(self.manifest),{'cpu_only':True})
  finally:self.p._sync_ledger=original
  self.assert_rejected_without_start()
if __name__=='__main__':unittest.main(verbosity=2)
