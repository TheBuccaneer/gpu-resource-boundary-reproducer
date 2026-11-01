#!/usr/bin/python3
"""Strict independent F02 v1 stream oracle. Real runtime mode is the default.
No plan, C, fixture or old oracle imports. Every successful protocol row required.
"""
import argparse,json,math,re,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from oracle_values import expected,NONCE,SALT
class ProtocolError(ValueError):pass
class DataMismatch(ProtocolError):
 def __init__(self,stage,index,got,want):
  self.stage,self.index,self.got,self.want=stage,index,got,want
  super().__init__(f'data mismatch {stage}[{index}]: {got} != {want}')
def require(ok,message):
 if not ok:raise ProtocolError(message)
def pointer(value):return isinstance(value,str) and bool(re.fullmatch(r'0x[0-9a-fA-F]+',value)) and int(value,16)>0
class Reader:
 def __init__(self,rows):
  self.rows=rows;self.at=0;self.objects={};self.kinds={};self.generation=0;self.time=0
  require(isinstance(rows,list) and bool(rows),'empty/non-list log')
  for row in rows:require(type(row) is dict and row.get('case')=='F02-CLONE-SNAPSHOT-REVERSE','missing/wrong case')
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
 def completion(self,stage):
  self.check('event exists');row=self.peek();handle=row.get('handle');require(pointer(handle),'invalid event');self.generation+=1;gen=self.generation
  self.take({'event_begin':stage,'handle':handle,'generation':gen})
  for label in ['clGetEventInfo(queue)','clGetEventInfo(context)','clGetEventInfo(type)']:self.api(label)
  self.check('event queue context and type match');prior=3
  for _ in range(1024):
   self.api('clGetEventInfo(status)');state=self.peek().get('event_status');require(type(state) is int and 0<=state<=prior,'abnormal event progression');prior=state
   self.take({'event_status':state,'handle':handle,'generation':gen})
   if state==0:break
  else:raise ProtocolError('event poll bound')
  self.check('event complete within five seconds');self.api('clWaitForEvents');self.api('clReleaseEvent');self.take({'event_end':gen})
 def setter(self,label,index,role):
  self.take({'setter':label,'kernel':self.objects['K'],'index':index,'bytes':8,'buffer':self.objects[role]});self.api(label)
 def kernel(self,role):
  self.api('clGetKernelInfo(CL_KERNEL_CONTEXT)');self.api('clGetKernelInfo(CL_KERNEL_PROGRAM)');self.check('kernel context and program match');self.obj(role,'kernel','A');self.take({'kernel_program':self.objects[role],'program':self.objects['P']})
 def dispatch(self,number,kernel,stage):
  self.take({'dispatch':number,'kernel':self.objects[kernel],'queue':self.objects['Q'],'global':64,'local':1});self.api('clEnqueueNDRangeKernel');self.api('clFlush');self.completion(stage)
 def readback(self,stage,role):
  self.take({'read':stage,'queue':self.objects['Q'],'buffer':self.objects[role],'bytes':640});self.api('clEnqueueReadBuffer');self.api('clFlush');self.completion(stage)
  values=self.peek().get('words');require(type(values) is list and len(values)==160 and all(type(x) is int and 0<=x<=4294967295 for x in values),'invalid data vector')
  for i,(value,want) in enumerate(zip(values,expected(stage))):
   if value!=want:raise DataMismatch(stage,i,value,want)
  self.take({'data':stage,'role':role,'buffer':self.objects[role],'words':values});self.check('all 160 words match host reference')
 def end(self):require(self.at==len(self.rows),'trailing/duplicate rows')
def verify(rows,origin='OPENCL_RUNTIME'):
 require(origin in ('OPENCL_RUNTIME','CPU_FAKE_RUNTIME'),'unknown origin')
 rd=Reader(rows);rd.take({'schema':1,'plan_version':1,'origin':origin,'nonce':NONCE,'salt':SALT,'global_workitems':64,'local_workitems':1,'words_per_buffer':160});rd.enumeration(origin)
 rd.api('clGetDeviceInfo(CL_DEVICE_NUMERIC_VERSION)');rd.check('core clone API version available')
 rd.api('clCreateContext(A)');rd.check('context A exists');rd.obj('A','context',None)
 rd.api('clCreateProgramWithSource');rd.check('program exists');rd.obj('P','program','A');rd.api('clGetProgramBuildInfo');rd.api('clBuildProgram');rd.api('clCreateKernel');rd.check('original exists');rd.kernel('K')
 for role,tag in [('A1',1),('A3',3),('O0',10),('O1',11),('O2',12)]:
  rd.api('clCreateBuffer('+role+')');rd.check('new buffer exists');rd.api('clGetMemObjectInfo(CL_MEM_CONTEXT)');rd.check('buffer context query matches owner');rd.api('clGetMemObjectInfo(CL_MEM_SIZE)');rd.check('buffer size is 640 bytes');rd.obj(role,'buffer','A',tag)
 rd.check('five live buffers distinct');rd.api('clCreateCommandQueue(A)');rd.check('queue exists')
 for label in ['clGetCommandQueueInfo(CL_QUEUE_CONTEXT)','clGetCommandQueueInfo(CL_QUEUE_DEVICE)','clGetCommandQueueInfo(CL_QUEUE_PROPERTIES)']:rd.api(label)
 rd.check('queue owner device and in-order properties match');rd.obj('Q','queue','A')
 rd.setter('original A1',0,'A1');rd.setter('original O0',1,'O0');rd.api('clCloneKernel');rd.check('clone distinct and live');rd.kernel('C')
 rd.dispatch(1,'C','clone-first');rd.readback('O0:first','O0')
 rd.take({'reset':'O0','queue':rd.objects['Q'],'buffer':rd.objects['O0'],'bytes':640});rd.api('clEnqueueWriteBuffer(reset)');rd.api('clFlush');rd.completion('reset-write');rd.readback('O0:reset','O0')
 rd.setter('original A3',0,'A3');rd.setter('original O1',1,'O1')
 rd.dispatch(2,'C','clone-second');rd.readback('O0:second','O0')
 rd.dispatch(3,'K','original-final');rd.readback('O1:final','O1')
 for role in ['A1','A3','O2']:rd.readback(role,role)
 for label in ['clReleaseCommandQueue','clReleaseKernel(clone)','clReleaseKernel(original)','clReleaseProgram']:rd.api(label)
 for role in ['O2','O1','O0','A3','A1']:rd.take({'release':role});rd.api('clReleaseMemObject')
 rd.api('clReleaseContext');rd.take({'terminal':'observations_complete','dispatches':3,'resets':1,'readbacks':7,'completions':11});rd.end()
 return {'outcome':'PASS_OBSERVED_STATE' if origin=='OPENCL_RUNTIME' else 'PASS_CPU_FIXTURE','dispatches':3,'resets':1,'readbacks':7,'words':1120,'completions':11,'scope':'inherited clone bindings and original-to-clone independence in observed in-context path; no OS/tenant boundary tested'}
def assess(rows,origin='OPENCL_RUNTIME'):
 try:return verify(rows,origin)
 except DataMismatch as exc:return {'outcome':'UNVERIFIED_DATA_CANDIDATE','reason':str(exc),'stage':exc.stage,'word':exc.index,'claim':'requires independent provenance/controls review; no confirmed vulnerability'}
 except (ProtocolError,ValueError,KeyError,TypeError,StopIteration) as exc:return {'outcome':'INCONCLUSIVE_PROTOCOL_OR_API','reason':str(exc)}
if __name__=='__main__':
 ap=argparse.ArgumentParser();ap.add_argument('--log',type=Path,required=True);ap.add_argument('--cpu-fixture',action='store_true');a=ap.parse_args()
 try:rows=[json.loads(x) for x in a.log.read_text().splitlines() if x.strip()];result=assess(rows,'CPU_FAKE_RUNTIME' if a.cpu_fixture else 'OPENCL_RUNTIME')
 except (OSError,ValueError) as exc:result={'outcome':'INCONCLUSIVE_INVALID_LOG','reason':str(exc)}
 print(json.dumps(result,indent=2));raise SystemExit(0 if result['outcome'] in ('PASS_OBSERVED_STATE','PASS_CPU_FIXTURE') else 1)
