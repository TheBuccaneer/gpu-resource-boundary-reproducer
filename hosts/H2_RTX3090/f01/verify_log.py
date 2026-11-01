#!/usr/bin/python3
"""Independent strict F01 v1 protocol oracle. Does not read the new plan or C.
Default accepts only real-origin evidence; CPU fixtures require explicit selection.
"""
import argparse,json,math,re,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from oracle_values import expected,foreign_output_word
class ProtocolError(ValueError):pass
class DataMismatch(ProtocolError):
 def __init__(self,role,index,got,want):
  self.role,self.index,self.got,self.want=role,index,got,want
  self.foreign_signature=role=='AO' and 16<=index<80 and got==foreign_output_word(index)
  super().__init__(f'data mismatch {role}[{index}]: {got} != {want}')
class GateStop(ProtocolError):
 def __init__(self,status,classification):self.status,self.classification=status,classification;super().__init__('pre-dispatch stop: '+classification)
def require(ok,message):
 if not ok:raise ProtocolError(message)
def pointer(value):return isinstance(value,str) and bool(re.fullmatch(r'0x[0-9a-fA-F]+',value)) and int(value,16)>0
class Reader:
 def __init__(self,rows):
  self.rows=rows;self.at=0;self.objects={};self.kinds={};self.generation=0;self.time=0
  require(isinstance(rows,list) and bool(rows),'empty/non-list log')
  for row in rows:require(type(row) is dict and row.get('case')=='H2-F01','missing/wrong case')
 def peek(self):return self.rows[self.at] if self.at<len(self.rows) else {}
 def take(self,fields,optional=()):
  require(self.at<len(self.rows),'missing required row: '+str(fields));row=self.rows[self.at];self.at+=1
  require(set(row)==set(fields)|{'case'}|set(optional).intersection(row),'missing/extra fields at '+str(self.at))
  for k,v in fields.items():require(type(row.get(k)) is type(v) and row[k]==v,'wrong '+k+' at '+str(self.at))
  return row
 def api(self,label):
  row=self.take({'api':label,'status':0,'expected':0,'classification':'expected'},('monotonic_s',))
  t=row.get('monotonic_s');require(type(t) in (float,int) and math.isfinite(t) and t>=self.time,'bad API time');self.time=t
 def check(self,label):self.take({'check':label,'pass':True})
 def obj(self,role,kind,ctx,tag=0):
  row=self.peek();handle=row.get('handle');require(pointer(handle),'invalid object handle')
  require(all(handle!=self.objects[other] for other in self.objects if self.kinds[other]==kind),'aliased simultaneously live '+kind)
  self.generation+=1
  context=handle if kind=='context' else self.objects[ctx]
  self.take({'object':role,'kind':kind,'handle':handle,'generation':self.generation,'context':context,'tag':tag});self.objects[role]=handle;self.kinds[role]=kind
 def enumeration(self,origin):
  self.api('clGetPlatformIDs(0,NULL,&np)');self.check('bounded platform count');self.api('clGetPlatformIDs(np,ps,NULL)')
  target=0;platforms=0
  while self.peek().get('api')=='clGetDeviceIDs(count)':
   platforms+=1;require(platforms<=16,'too many platforms');self.api('clGetDeviceIDs(count)');self.check('bounded GPU count');self.api('clGetDeviceIDs(ps[p],CL_DEVICE_TYPE_GPU,n,ds,NULL)')
   devices=0
   while self.peek().get('api')=='clGetDeviceInfo(ds[d],CL_DEVICE_VENDOR_ID,sizeof(vendor),&vendor,NULL)':
    devices+=1;require(devices<=16,'too many devices');self.api('clGetDeviceInfo(ds[d],CL_DEVICE_VENDOR_ID,sizeof(vendor),&vendor,NULL)');self.api('clGetDeviceInfo(ds[d],CL_DEVICE_NAME,sizeof(name),name,NULL)')
    if self.peek().get('api')=='clGetDeviceInfo(ds[d],CL_DRIVER_VERSION,sizeof(driver),driver,NULL)':
     target+=1;self.api('clGetDeviceInfo(ds[d],CL_DRIVER_VERSION,sizeof(driver),driver,NULL)');self.check('pinned NVIDIA driver 580.178.04');self.api('clGetDeviceInfo(ds[d],CL_DEVICE_PCI_BUS_INFO_KHR,sizeof(pci),&pci,NULL)');self.check('pinned PCI 0000:01:00.0');self.check('unique target GPU');self.take({'device':'NVIDIA GeForce RTX 3090','driver':'580.178.04'})
   require(devices>0,'missing devices')
  require(target==1 and platforms>0,'missing/duplicate target');self.check('target present; no index-based fallback')
  maps=[]
  while 'library_map' in self.peek():
   text=self.peek()['library_map'];require(isinstance(text,str) and len(text)<4096,'invalid map');self.take({'library_map':text});maps.append(text)
  if origin=='OPENCL_RUNTIME':
   for name in ['libOpenCL.so.1.0.0','libnvidia-opencl.so.580.178.04','libcuda.so.580.178.04']:require(any('/usr/lib/x86_64-linux-gnu/'+name in x for x in maps),'required runtime mapping absent')
  else:require(not maps,'CPU fixture unexpectedly mapped a runtime/device library')
 def setter(self,label,index,role):
  self.take({'setter':label,'kernel':self.objects['K'],'index':index,'bytes':8,'buffer':self.objects[role]})
  self.api(label)
 def completion(self,stage):
  self.check('event exists');row=self.peek();handle=row.get('handle');require(pointer(handle),'invalid event');self.generation+=1;gen=self.generation
  self.take({'event_begin':stage,'handle':handle,'generation':gen});prior=3
  for _ in range(1024):
   self.api('clGetEventInfo(status)');state=self.peek().get('event_status');require(type(state) is int and 0<=state<=prior,'abnormal event progression');prior=state
   self.take({'event_status':state,'handle':handle,'generation':gen})
   if state==0:break
  else:raise ProtocolError('event poll bound')
  self.check('event complete within five seconds');self.api('clWaitForEvents');self.api('clReleaseEvent');self.take({'event_end':gen})
 def readback(self,role,queue):
  self.take({'read':role,'queue':self.objects[queue],'buffer':self.objects[role],'bytes':640})
  self.api('clEnqueueReadBuffer');self.api('clFlush');self.completion(role)
  values=self.peek().get('words');require(type(values) is list and len(values)==160 and all(type(x) is int and 0<=x<=4294967295 for x in values),'invalid data vector')
  for i,(value,want) in enumerate(zip(values,expected(role))):
   if value!=want:raise DataMismatch(role,i,value,want)
  self.take({'data':role,'buffer':self.objects[role],'words':values});self.check('all 160 words match host reference')
 def end(self):require(self.at==len(self.rows),'trailing/duplicate rows')
def verify(rows,origin='OPENCL_RUNTIME'):
 require(origin in ('OPENCL_RUNTIME','CPU_FAKE_RUNTIME'),'unknown origin')
 rd=Reader(rows);rd.take({'schema':1,'plan_version':1,'origin':origin,'global_workitems':64,'local_workitems':1,'words_per_buffer':160});rd.enumeration(origin)
 rd.api('clCreateContext(A)');rd.check('context A exists');rd.obj('A','context',None)
 rd.api('clCreateContext(B)');rd.check('contexts distinct and live');rd.obj('B','context',None)
 rd.api('clCreateProgramWithSource');rd.check('program exists');rd.obj('P','program','A');rd.api('clGetProgramBuildInfo');rd.api('clBuildProgram');rd.api('clCreateKernel');rd.check('kernel exists');rd.api('clGetKernelInfo(CL_KERNEL_CONTEXT)');rd.check('kernel context query matches A');rd.obj('K','kernel','A');rd.take({'kernel_program':rd.objects['K'],'program':rd.objects['P']})
 for role,ctx,tag in [('A1','A',1),('AO','A',10),('B1','B',2),('AS','A',30),('CI','A',3),('CO','A',11)]:
  rd.api('clCreateBuffer('+role+')');rd.check('new buffer exists');rd.api('clGetMemObjectInfo(CL_MEM_CONTEXT)');rd.check('buffer context query matches owner');rd.obj(role,'buffer',ctx,tag)
 rd.check('six live buffers distinct')
 for label,index,role in [('set own A1',0,'A1'),('set own AO',1,'AO'),('set fresh control input',2,'CI'),('set fresh control output',3,'CO')]:rd.setter(label,index,role)
 rd.take({'setter':'foreign transition','kernel':rd.objects['K'],'index':0,'bytes':8,'buffer':rd.objects['B1']})
 status=rd.peek().get('foreign_status');require(type(status) is int,'missing actual foreign status')
 classes={-34:'reject_invalid_context',-38:'reject_invalid_mem_object',0:'unexpected_success',-5:'resource_error',-6:'resource_error'}
 classification=classes.get(status,'unexpected_rejection');rd.take({'foreign_status':status,'status_class':classification})
 if status not in (-34,-38):
  rd.take({'stop':'before_queue_and_dispatch','status_class':classification});rd.end();raise GateStop(status,classification)
 rd.api('clCreateCommandQueue(A)');rd.check('queue A exists');rd.obj('QA','queue','A')
 rd.api('clCreateCommandQueue(B)');rd.check('queue B exists and differs from A');rd.obj('QB','queue','B')
 rd.take({'dispatch':1,'kernel':rd.objects['K'],'queue':rd.objects['QA'],'global':64,'local':1});rd.api('clEnqueueNDRangeKernel');rd.api('clFlush');rd.completion('kernel')
 for role,queue in [('CO','QA'),('CI','QA'),('AO','QA'),('A1','QA'),('B1','QB'),('AS','QA')]:rd.readback(role,queue)
 for label in ['clReleaseCommandQueue(B)','clReleaseCommandQueue(A)','clReleaseKernel','clReleaseProgram']:rd.api(label)
 for role in ['CO','CI','AS','B1','AO','A1']:rd.take({'release':role});rd.api('clReleaseMemObject')
 rd.api('clReleaseContext(B)');rd.api('clReleaseContext(A)');rd.take({'terminal':'observations_complete','dispatches':1,'readbacks':6,'completions':7});rd.end()
 return {'outcome':'PASS_OBSERVED_STATE' if origin=='OPENCL_RUNTIME' else 'PASS_CPU_FIXTURE','actual_foreign_status':status,'API_code_class':classification,'API_code_priority_question':'not resolved or re-labelled','state_integrity':'preserved in observed data path','possible_context_data_effect':False,'OS_process_or_tenant_boundary':'not tested','fresh_positive_control':'separate own input/output arm in same dispatch, not a separate execution','dispatches':1,'readbacks':6,'words':960,'boundary_suffix_words':640,'control_words':320,'completions':7}
def assess(rows,origin='OPENCL_RUNTIME'):
 try:return verify(rows,origin)
 except GateStop as exc:return {'outcome':'INCONCLUSIVE_RESOURCE' if exc.status in (-5,-6) else 'STOP_API_STATUS','actual_foreign_status':exc.status,'API_code_class':exc.classification,'state_integrity':'not observed','possible_context_data_effect':False,'OS_process_or_tenant_boundary':'not tested'}
 except DataMismatch as exc:return {'outcome':'UNVERIFIED_DATA_CANDIDATE','reason':str(exc),'role':exc.role,'word':exc.index,'foreign_pattern_signature':exc.foreign_signature,'state_integrity':'not confirmed; controls/provenance require review','possible_context_data_effect':exc.foreign_signature,'OS_process_or_tenant_boundary':'not tested'}
 except ProtocolError as exc:return {'outcome':'INCONCLUSIVE_PROTOCOL_OR_API','reason':str(exc),'state_integrity':'not confirmed','OS_process_or_tenant_boundary':'not tested'}
if __name__=='__main__':
 ap=argparse.ArgumentParser();ap.add_argument('--log',type=Path,required=True);ap.add_argument('--cpu-fixture',action='store_true');a=ap.parse_args()
 result=assess([json.loads(x) for x in a.log.read_text().splitlines() if x.strip()],'CPU_FAKE_RUNTIME' if a.cpu_fixture else 'OPENCL_RUNTIME');print(json.dumps(result,indent=2));raise SystemExit(0 if result['outcome'] in ('PASS_OBSERVED_STATE','PASS_CPU_FIXTURE') else 1)
