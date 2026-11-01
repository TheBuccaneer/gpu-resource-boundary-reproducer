"""Actual new C program against a statically linked CPU fake, independent oracle."""
from pathlib import Path
import copy,importlib.util,json,os,subprocess,tempfile,unittest
B=Path(__file__).resolve().parents[1];R=B.parents[2]
spec=importlib.util.spec_from_file_location('f02_oracle',B/'verify_log.py');o=importlib.util.module_from_spec(spec);spec.loader.exec_module(o)
class F02Tests(unittest.TestCase):
 @classmethod
 def setUpClass(cls):
  cls.temp=tempfile.TemporaryDirectory(prefix='f02-fake-cpu-');cls.addClassCleanup(cls.temp.cleanup);cls.work=Path(cls.temp.name);cls.binary=cls.work/'f02_cpu'
  subprocess.run(['/usr/bin/gcc','-std=c11','-Wall','-Wextra','-Werror','-Wpedantic','-O2','-DF02_CPU_FIXTURE',str(B/'h2_f02.c'),str(B/'tests/fake_opencl.c'),'-o',str(cls.binary)],check=True,capture_output=True)
  x,rows,marks=cls.run_fake('valid');assert x.returncode==0,(x.returncode,x.stderr);assert marks.count('clone')==1 and marks.count('dispatch')==3 and marks.count('reset')==1 and marks.count('read')==7;cls.good=rows
 @classmethod
 def run_fake(cls,mode):
  marker=cls.work/(mode+'.calls');marker.unlink(missing_ok=True)
  x=subprocess.run([str(cls.binary),'--approved-run','F02-CLONE-SNAPSHOT-REVERSE'],env={**os.environ,'F02_CPU_SCENARIO':mode,'F02_CPU_MARKER':str(marker)},capture_output=True,text=True,timeout=10)
  return x,[json.loads(line) for line in x.stdout.splitlines()],marker.read_text().splitlines() if marker.exists() else []
 def test_valid_complete_sequence(self):
  result=o.verify(self.good,'CPU_FAKE_RUNTIME');self.assertEqual(result['outcome'],'PASS_CPU_FIXTURE');self.assertEqual((result['dispatches'],result['readbacks'],result['words'],result['completions']),(3,7,1120,11))
 def test_fixture_cannot_pass_as_runtime(self):
  with self.assertRaises(o.ProtocolError):o.verify(self.good)
 def test_wrong_inheritance_coupled_bindings_and_stale_output(self):
  for mode in ['wrong_output','wrong_input','coupled','stale_after_reset','reset_noop']:
   with self.subTest(mode=mode):
    x,rows,marks=self.run_fake(mode);self.assertEqual(x.returncode,21);self.assertEqual(o.assess(rows,'CPU_FAKE_RUNTIME')['outcome'],'UNVERIFIED_DATA_CANDIDATE')
    if mode in ['wrong_output','wrong_input']:self.assertEqual(marks.count('dispatch'),1)
    if mode=='reset_noop':self.assertEqual(marks.count('dispatch'),1)
    if mode in ['coupled','stale_after_reset']:self.assertEqual(marks.count('dispatch'),2)
 def test_resource_error_and_abnormal_event_stop(self):
  for mode in ['clone_error','dispatch_error','bad_completion']:
   x,rows,marks=self.run_fake(mode);self.assertIn(x.returncode,[21,78]);self.assertFalse(o.assess(rows,'CPU_FAKE_RUNTIME')['outcome'].startswith('PASS'));self.assertLess(marks.count('dispatch'),2)
 def test_guard_inputs_and_unused_sentinel_faults(self):
  for mode in ['guard_corrupt','input_corrupt','sentinel_corrupt']:
   x,rows,_=self.run_fake(mode);self.assertEqual(x.returncode,21);self.assertEqual(o.assess(rows,'CPU_FAKE_RUNTIME')['outcome'],'UNVERIFIED_DATA_CANDIDATE')
 def test_all_1120_word_mutations_rejected(self):
  rows=self.good;count=0
  for i,row in enumerate(rows):
   if 'words' not in row:continue
   for word in range(160):
    bad=copy.deepcopy(rows);bad[i]['words'][word]^=1
    with self.assertRaises(o.ProtocolError):o.verify(bad,'CPU_FAKE_RUNTIME')
    count+=1
  self.assertEqual(count,1120)
 def test_every_required_row_removed_duplicated_or_swapped(self):
  rows=self.good
  for i in range(len(rows)):
   with self.subTest(row=i):
    with self.assertRaises(o.ProtocolError):o.verify(rows[:i]+rows[i+1:],'CPU_FAKE_RUNTIME')
    with self.assertRaises(o.ProtocolError):o.verify(rows[:i]+[rows[i]]+rows[i:],'CPU_FAKE_RUNTIME')
    if i+1<len(rows) and rows[i]!=rows[i+1]:
     bad=list(rows);bad[i],bad[i+1]=bad[i+1],bad[i]
     with self.assertRaises(o.ProtocolError):o.verify(bad,'CPU_FAKE_RUNTIME')
 def test_context_generation_alias_event_and_length_mutations(self):
  rows=self.good
  for i,row in enumerate(rows):
   for field in ['context','generation','handle','kernel','kernel_program','program','queue','buffer','event_status','bytes','foreign_status']:
    if field not in row:continue
    bad=copy.deepcopy(rows);bad[i][field]=row[field]+1 if type(row[field]) is int else '0xdeadbeef'
    with self.subTest(row=i,field=field),self.assertRaises(o.ProtocolError):o.verify(bad,'CPU_FAKE_RUNTIME')
   if 'words' in row:
    bad=copy.deepcopy(rows);bad[i]['words'].pop()
    with self.assertRaises(o.ProtocolError):o.verify(bad,'CPU_FAKE_RUNTIME')
 def test_no_missing_dispatch_readback_reset_cleanup_terminal(self):
  for remove in [lambda x:'dispatch' in x,lambda x:'reset' in x,lambda x:x.get('data')=='O0:reset',lambda x:x.get('data')=='O2',lambda x:x.get('api')=='clReleaseKernel(clone)',lambda x:'terminal' in x]:
   with self.assertRaises(o.ProtocolError):o.verify([x for x in self.good if not remove(x)],'CPU_FAKE_RUNTIME')
 def test_independent_oracle_and_fresh_identity_patterns(self):
  text=(B/'oracle_values.py').read_text();self.assertNotIn('import ',text);self.assertNotIn('read_text',text);self.assertNotIn('PLAN_V1.json',(B/'verify_log.py').read_text())
  self.assertTrue(all(o.expected('O0:first')[i]!=o.expected('O0:reset')[i] for i in range(16,80)))
  self.assertTrue(all(o.expected('O0:second')[i]!=o.expected('O1:final')[i] for i in range(16,80)))
 def test_CPU_binary_has_no_OpenCL_or_CUDA_needed_library(self):
  deps=subprocess.check_output(['/usr/bin/readelf','-d',str(self.binary)],text=True)
  self.assertNotIn('libOpenCL',deps);self.assertNotIn('libcuda',deps);self.assertNotIn('nvidia',deps)
 def test_wrong_case_cli_has_no_fake_runtime_calls(self):
  marker=self.work/'wrong.calls';x=subprocess.run([str(self.binary),'--approved-run','S01-ROLLBACK-DATA'],env={**os.environ,'F02_CPU_MARKER':str(marker)},capture_output=True,text=True);self.assertEqual(x.returncode,64);self.assertFalse(marker.exists())
if __name__=='__main__':unittest.main(verbosity=2)
