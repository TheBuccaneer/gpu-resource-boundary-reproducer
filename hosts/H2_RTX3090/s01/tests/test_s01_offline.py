import copy,importlib.util,json,re,unittest
from pathlib import Path
from protocol_fixture import fixture
B=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('s01_oracle',B/'verify_log.py');o=importlib.util.module_from_spec(spec);spec.loader.exec_module(o)
CASES=json.loads((B/'TEST_CASES.json').read_text())['cases']
class S01Offline(unittest.TestCase):
 def reject(self,rows,name):
  with self.assertRaises(ValueError):o.verify(rows,name)
 def test_case_mapping_and_execution_gate(self):
  text=(B/'h2_s01.c').read_text();decl=text.split('static const char *cases[]={',1)[1].split('};',1)[0]
  self.assertEqual(set(re.findall(r'"([^"]+)"',decl)),{x['id'] for x in CASES});self.assertEqual(len(CASES),11)
  self.assertLess(text.index('if(!known)'),text.index('select_device(&platform)'));self.assertIn('if(data) {',text)
  self.assertTrue(all(not x['runtime_executed'] for x in CASES))
 def test_complete_protocols(self):
  for c in CASES:self.assertTrue(o.verify(fixture(c,o),c['id']))
 def test_every_required_record_removed_duplicated_or_swapped(self):
  for c in CASES:
   rows=fixture(c,o);name=c['id']
   for i in range(len(rows)):
    self.reject(rows[:i]+rows[i+1:],name)
    self.reject(rows[:i]+[rows[i]]+rows[i:],name)
    if i+1<len(rows) and rows[i]!=rows[i+1]:
     bad=copy.deepcopy(rows);bad[i],bad[i+1]=bad[i+1],bad[i];self.reject(bad,name)
 def test_case_object_context_and_status_mutations(self):
  for c in CASES:
   rows=fixture(c,o);name=c['id']
   for i,x in enumerate(rows):
    for field in ['case','object','context','a','b','event','arg_index','arg_size','status','expected']:
     if field not in x:continue
     bad=copy.deepcopy(rows);bad[i][field]=x[field]+1 if type(x[field])==int else '0xdeadbeef';self.reject(bad,name)
    bad=copy.deepcopy(rows);del bad[i]['case'];self.reject(bad,name)
 def test_payload_guards_length_and_false_expected_tags(self):
  for c in CASES:
   rows=fixture(c,o);name=c['id']
   for i,x in enumerate(rows):
    if 'data_u32' not in x:continue
    for word in [0,16,79,159]:
     bad=copy.deepcopy(rows);bad[i]['data_u32'][word]^=1;self.reject(bad,name)
    bad=copy.deepcopy(rows);bad[i]['data_u32'].pop();self.reject(bad,name)
    bad=copy.deepcopy(rows);bad[i]['expected_input_tag']=99;bad[i]['data_u32']=o.expected(x['storage_tag'],99);self.reject(bad,name)
 def test_daybreak_minimal_false_success_logs(self):
  self.reject([{'case':'S01-CLONE-API','api':'unrelated setup','status':0,'expected':0},{'case':'S01-CLONE-API','outcome':'expected_observations_complete','argument_state_observed_by_data':False}],'S01-CLONE-API')
  self.reject([{'outcome':'expected_observations_complete'}],'S01-CLONE-DATA')
 def test_clone_stale_noop_wrong_output_and_double_effect(self):
  for c in CASES:
   if not(c['clone'] and c['kernel_dispatches']):continue
   rows=fixture(c,o);name=c['id']
   outputs=[i for i,x in enumerate(rows) if x.get('storage_tag')==10 and x.get('expected_input_tag')==1];last=outputs[-1]
   # CPU memory model: reset removes stale A result. No-op leaves reset;
   # wrong output leaves original reset and corrupts clone output;
   # double effect writes original correctly but also corrupts clone output.
   reset=o.expected(10,0);wantA=o.expected(10,1);wantC=o.expected(12,3)
   self.assertNotEqual(reset,wantA)
   for wrong_original in [reset]:
    bad=copy.deepcopy(rows);bad[last]['data_u32']=wrong_original;self.reject(bad,name)
   clone_reads=[i for i,x in enumerate(rows) if x.get('storage_tag')==12];bad=copy.deepcopy(rows);bad[clone_reads[-1]]['data_u32']=o.expected(12,1);self.assertNotEqual(wantC,bad[clone_reads[-1]]['data_u32']);self.reject(bad,name)
   reset_action=next(i for i,x in enumerate(rows) if x.get('operation')=='reset');self.reject(rows[:reset_action]+rows[reset_action+1:],name)
if __name__=='__main__':unittest.main(verbosity=2)
