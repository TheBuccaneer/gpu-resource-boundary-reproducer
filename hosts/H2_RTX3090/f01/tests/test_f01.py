"""Actual new C program against a statically linked CPU fake, independent oracle."""
from pathlib import Path
import copy,importlib.util,json,os,subprocess,tempfile,unittest
B=Path(__file__).resolve().parents[1];R=B.parents[2]
spec=importlib.util.spec_from_file_location('f01_oracle',B/'verify_log.py');o=importlib.util.module_from_spec(spec);spec.loader.exec_module(o)
class F01Tests(unittest.TestCase):
 @classmethod
 def setUpClass(cls):
  cls.temp=tempfile.TemporaryDirectory(prefix='f01-fake-cpu-');cls.addClassCleanup(cls.temp.cleanup);cls.work=Path(cls.temp.name);cls.binary=cls.work/'f01_cpu'
  subprocess.run(['/usr/bin/gcc','-std=c11','-Wall','-Wextra','-Werror','-Wpedantic','-O2','-DF01_CPU_FIXTURE',str(B/'h2_f01.c'),str(B/'tests/fake_opencl.c'),'-o',str(cls.binary)],check=True,capture_output=True)
  cls.good={}
  for mode in ['reject38','reject34']:
   x,rows,marks=cls.run_fake(mode);assert x.returncode==0,(x.returncode,x.stderr);assert marks.count('foreign_setter')==1 and marks.count('dispatch')==1 and marks.count('read')==6;cls.good[mode]=rows
 @classmethod
 def run_fake(cls,mode):
  marker=cls.work/(mode+'.calls');marker.unlink(missing_ok=True)
  x=subprocess.run([str(cls.binary),'--approved-run','H2-F01'],env={**os.environ,'F01_CPU_SCENARIO':mode,'F01_CPU_MARKER':str(marker)},capture_output=True,text=True,timeout=10)
  return x,[json.loads(line) for line in x.stdout.splitlines()],marker.read_text().splitlines() if marker.exists() else []
 def test_separate_raw_rejection_classes_and_full_state_oracle(self):
  for mode,status in [('reject38',-38),('reject34',-34)]:
   result=o.verify(self.good[mode],'CPU_FAKE_RUNTIME');self.assertEqual(result['actual_foreign_status'],status);self.assertEqual((result['dispatches'],result['readbacks'],result['words'],result['completions']),(1,6,960,7));self.assertEqual(result['outcome'],'PASS_CPU_FIXTURE')
 def test_fixture_cannot_pass_as_real_runtime_evidence(self):
  with self.assertRaises(o.ProtocolError):o.verify(self.good['reject38'])
 def test_wrong_success_resources_and_other_status_stop_before_queue(self):
  for mode,code in [('unexpected_success',20),('resource5',78),('resource6',78),('other49',20)]:
   with self.subTest(mode=mode):
    x,rows,marks=self.run_fake(mode);self.assertEqual(x.returncode,code);self.assertEqual(marks,['foreign_setter']);self.assertFalse(any('dispatch' in row or 'object' in row and row['kind']=='queue' for row in rows));self.assertFalse(o.assess(rows,'CPU_FAKE_RUNTIME')['outcome'].startswith('PASS'))
 def test_foreign_binding_fault_is_candidate_not_confirmed_boundary_break(self):
  x,rows,_=self.run_fake('foreign_bind');self.assertEqual(x.returncode,21);result=o.assess(rows,'CPU_FAKE_RUNTIME');self.assertEqual(result['outcome'],'UNVERIFIED_DATA_CANDIDATE');self.assertTrue(result['foreign_pattern_signature']);self.assertEqual(result['OS_process_or_tenant_boundary'],'not tested')
 def test_deleted_argument_and_abnormal_event_never_data_pass(self):
  for mode in ['clear_arg0','bad_completion']:
   x,rows,_=self.run_fake(mode);self.assertIn(x.returncode,[20,21]);self.assertFalse(o.assess(rows,'CPU_FAKE_RUNTIME')['outcome'].startswith('PASS'))
 def test_fake_guard_B1_and_sentinel_corruption_is_detected(self):
  for mode in ['guard_corrupt','b1_corrupt','sentinel_corrupt']:
   with self.subTest(mode=mode):
    x,rows,_=self.run_fake(mode);self.assertEqual(x.returncode,21);self.assertEqual(o.assess(rows,'CPU_FAKE_RUNTIME')['outcome'],'UNVERIFIED_DATA_CANDIDATE')
 def test_all_960_word_mutations_rejected(self):
  rows=self.good['reject38'];count=0
  for i,row in enumerate(rows):
   if 'words' not in row:continue
   for word in range(160):
    bad=copy.deepcopy(rows);bad[i]['words'][word]^=1
    with self.assertRaises(o.ProtocolError):o.verify(bad,'CPU_FAKE_RUNTIME')
    count+=1
  self.assertEqual(count,960)
 def test_every_required_row_removed_duplicated_or_swapped(self):
  rows=self.good['reject38']
  for i in range(len(rows)):
   with self.subTest(row=i):
    with self.assertRaises(o.ProtocolError):o.verify(rows[:i]+rows[i+1:],'CPU_FAKE_RUNTIME')
    with self.assertRaises(o.ProtocolError):o.verify(rows[:i]+[rows[i]]+rows[i:],'CPU_FAKE_RUNTIME')
    if i+1<len(rows) and rows[i]!=rows[i+1]:
     bad=list(rows);bad[i],bad[i+1]=bad[i+1],bad[i]
     with self.assertRaises(o.ProtocolError):o.verify(bad,'CPU_FAKE_RUNTIME')
 def test_context_generation_alias_event_and_length_mutations(self):
  rows=self.good['reject38']
  for i,row in enumerate(rows):
   for field in ['context','generation','handle','kernel','kernel_program','program','queue','buffer','event_status','bytes','foreign_status']:
    if field not in row:continue
    bad=copy.deepcopy(rows);bad[i][field]=row[field]+1 if type(row[field]) is int else '0xdeadbeef'
    with self.subTest(row=i,field=field),self.assertRaises(o.ProtocolError):o.verify(bad,'CPU_FAKE_RUNTIME')
   if 'words' in row:
    bad=copy.deepcopy(rows);bad[i]['words'].pop()
    with self.assertRaises(o.ProtocolError):o.verify(bad,'CPU_FAKE_RUNTIME')
 def test_no_dispatch_missing_readback_cleanup_or_terminal_can_pass(self):
  rows=self.good['reject38']
  for remove in [lambda x:'dispatch' in x,lambda x:x.get('data')=='B1',lambda x:x.get('data')=='CO',lambda x:x.get('api')=='clReleaseContext(A)',lambda x:'terminal' in x]:
   with self.assertRaises(o.ProtocolError):o.verify([x for x in rows if not remove(x)],'CPU_FAKE_RUNTIME')
 def test_value_oracle_has_no_plan_logger_source_dependencies(self):
  text=(B/'oracle_values.py').read_text();self.assertNotIn('import ',text);self.assertNotIn('read_text',text);self.assertNotIn('PLAN_V1.json',(B/'verify_log.py').read_text())
  self.assertTrue(all(o.expected('AO')[i]!=o.foreign_output_word(i) for i in range(16,80)))
 def test_CPU_binary_has_no_OpenCL_or_CUDA_needed_library(self):
  deps=subprocess.check_output(['/usr/bin/readelf','-d',str(self.binary)],text=True)
  self.assertNotIn('libOpenCL',deps);self.assertNotIn('libcuda',deps);self.assertNotIn('nvidia',deps)
 def test_wrong_case_cli_has_no_fake_runtime_calls(self):
  marker=self.work/'wrong.calls';x=subprocess.run([str(self.binary),'--approved-run','S01-ROLLBACK-DATA'],env={**os.environ,'F01_CPU_MARKER':str(marker)},capture_output=True,text=True);self.assertEqual(x.returncode,64);self.assertFalse(marker.exists())
if __name__=='__main__':unittest.main(verbosity=2)
