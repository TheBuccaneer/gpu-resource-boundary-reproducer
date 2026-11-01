#!/usr/bin/env python3
"""Strict ordered S01 schema-v2 oracle; offline, no OpenCL imports/calls.
The transcript is evidence, not authentication: also verify supervisor hashes.
"""
import argparse,json,re
from pathlib import Path
PLAN=Path(__file__).with_name('TEST_CASES.json')
def expected(storage,source):
 def value(tag,i):return ((0x9e3779b9*(i+1))^(tag*0x1020304))&0xffffffff
 return [(value(source,i)^0x37a1b5c9) if source and 16<=i<80 else value(storage,i) for i in range(160)]
def require(ok,message):
 if not ok:raise ValueError(message)
class Reader:
 def __init__(self,rows,name):
  require(bool(rows),'empty log');self.rows=rows;self.name=name;self.at=0;self.refs={}
  for x in rows:require(isinstance(x,dict) and x.get('case')==name,'missing/wrong case')
 def peek(self):return self.rows[self.at] if self.at<len(self.rows) else {}
 def take(self,pattern,extras=()):
  require(self.at<len(self.rows),'missing required step: '+str(pattern));x=self.rows[self.at];self.at+=1
  require(set(x)==set(pattern)|{'case'}|set(extras).intersection(x),'unexpected record fields at '+str(self.at))
  for k,v in pattern.items():
   actual=x.get(k)
   if isinstance(v,str) and v.startswith('@'):
    key=v[1:];require(key not in self.refs and isinstance(actual,str) and re.fullmatch(r'0x[0-9a-fA-F]*[1-9a-fA-F][0-9a-fA-F]*',actual),'invalid/duplicate object declaration')
    # All these declared objects are simultaneously live; within each type distinct.
    group=lambda n:'m' if n.startswith('m') else ('ctx' if n.startswith('ctx') else ('q' if n.startswith('q') else 'k'))
    require(all(actual!=old for n,old in self.refs.items() if group(n)==group(key)),'aliased live object identities');self.refs[key]=actual
   elif isinstance(v,str) and v.startswith('$'):require(actual==self.refs.get(v[1:]),'object/context binding mismatch')
   else:require(type(actual)==type(v) and actual==v,'unexpected '+k+' at '+str(self.at)+': '+repr(actual))
  return x
 def api(self,name):return self.take({'api':name,'status':0,'expected':0,'classification':'expected'},('monotonic_s',))
 def check(self,name):self.take({'check':name,'pass':True})
 def enumeration(self):
  self.api('clGetPlatformIDs(0,NULL,&np)');self.check('bounded platform count');self.api('clGetPlatformIDs(np,ps,NULL)')
  targets=0;devices=0;platforms=0
  while self.peek().get('api')=='clGetDeviceIDs(count)':
   platforms+=1;require(platforms<=16,'platform limit');self.api('clGetDeviceIDs(count)');self.check('bounded GPU count');self.api('clGetDeviceIDs(ps[p],CL_DEVICE_TYPE_GPU,n,ds,NULL)')
   n=0
   while self.peek().get('api')=='clGetDeviceInfo(ds[d],CL_DEVICE_VENDOR_ID,sizeof(vendor),&vendor,NULL)':
    n+=1;devices+=1;require(n<=16,'device limit')
    self.api('clGetDeviceInfo(ds[d],CL_DEVICE_VENDOR_ID,sizeof(vendor),&vendor,NULL)');self.api('clGetDeviceInfo(ds[d],CL_DEVICE_NAME,sizeof(name),name,NULL)')
    if self.peek().get('api')=='clGetDeviceInfo(ds[d],CL_DRIVER_VERSION,sizeof(driver),driver,NULL)':
     targets+=1;self.api('clGetDeviceInfo(ds[d],CL_DRIVER_VERSION,sizeof(driver),driver,NULL)');self.check('pinned NVIDIA driver 580.178.04');self.api('clGetDeviceInfo(ds[d],CL_DEVICE_PCI_BUS_INFO_KHR,sizeof(pci),&pci,NULL)');self.check('pinned PCI 0000:01:00.0');self.check('unique target GPU');self.take({'device':'NVIDIA GeForce RTX 3090','driver':'580.178.04'})
   require(n>0,'empty device enumeration')
  require(targets==1 and devices>0,'target enumeration missing');self.check('target present; no index-based fallback')
  while 'library_map' in self.peek():
   x=self.peek();require(isinstance(x['library_map'],str) and any(s in x['library_map'] for s in ['OpenCL','nvidia','libcuda']),'invalid library map');self.take({'library_map':x['library_map']})
  self.take({'phase':'target_selected','schema_version':2,'sizeof_mem':8})
 def completion(self):
  event=self.peek().get("a");require(isinstance(event,str) and re.fullmatch(r"0x[0-9a-fA-F]+",event) and int(event,16)>0,"invalid event declaration")
  self.take({"operation":"event","a":event,"b":"(nil)"});prior=3
  for _ in range(1024):
   self.take({'api':'clGetEventInfo(event,CL_EVENT_COMMAND_EXECUTION_STATUS,sizeof(state),&state,NULL)','status':0,'expected':0,'classification':'expected'})
   x=self.peek();e=x.get('event');s=x.get('execution_status')
   require(isinstance(e,str) and re.fullmatch(r'0x[0-9a-fA-F]+',e) and int(e,16)>0,'missing event identity')
   require(type(s)==int and 0<=s<=prior,'invalid/abnormal event progression')
   require(event is None or e==event,'event changed during poll');event=e;prior=s;self.take({'event':e,'execution_status':s})
   if s==0:break
  else:raise ValueError('poll limit')
  self.check('event completed within five seconds');self.take({'api':'clWaitForEvents(1,&event)','status':0,'expected':0,'classification':'expected'})
def verify(rows,name):
 case=next((x for x in json.loads(PLAN.read_text())['cases'] if x['id']==name),None);require(case is not None,'unknown case')
 rd=Reader(rows,name);rd.enumeration()
 for spec in case['ordered_protocol']:
  p=dict(spec);kind=p.pop('protocol',None)
  if kind=='completion':rd.completion()
  elif kind=='data':
   x=rd.peek();a=x.get('data_u32');require(isinstance(a,list) and len(a)==160 and all(type(n)==int and 0<=n<=0xffffffff for n in a),'invalid data vector')
   p['data_u32']=expected(p['storage_tag'],p['expected_input_tag']);rd.take(p)
  elif kind=='version':
   version=rd.peek().get('platform_version');require(isinstance(version,str) and re.fullmatch(r'OpenCL 3\.0(?: .*)?',version),'unexpected target APIversion');p['platform_version']=version;rd.take(p)
  else:rd.take(p)
 require(rd.at==len(rows),'trailing/duplicate records');return True
if __name__=='__main__':
 ap=argparse.ArgumentParser();ap.add_argument('--log',required=True,type=Path);ap.add_argument('--case',required=True);a=ap.parse_args()
 verify([json.loads(s) for s in a.log.read_text().splitlines() if s.strip()],a.case);print('PASS: ordered S01 v2 protocol, bound identities, completion, full data and cleanup. Check supervisor provenance separately.')
