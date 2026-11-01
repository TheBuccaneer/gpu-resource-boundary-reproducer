"""Synthetic complete protocol fixtures, never a driver/runtime execution."""
import copy,json
from pathlib import Path
B=Path(__file__).resolve().parents[1]
def fixture(case,oracle):
 name=case['id'];rows=[]
 def add(**x):rows.append({'case':name,**x})
 def api(s):add(api=s,status=0,expected=0,monotonic_s=1.0,classification='expected')
 def check(s):add(check=s,**{'pass':True})
 api('clGetPlatformIDs(0,NULL,&np)');check('bounded platform count');api('clGetPlatformIDs(np,ps,NULL)')
 api('clGetDeviceIDs(count)');check('bounded GPU count');api('clGetDeviceIDs(ps[p],CL_DEVICE_TYPE_GPU,n,ds,NULL)')
 api('clGetDeviceInfo(ds[d],CL_DEVICE_VENDOR_ID,sizeof(vendor),&vendor,NULL)');api('clGetDeviceInfo(ds[d],CL_DEVICE_NAME,sizeof(name),name,NULL)');api('clGetDeviceInfo(ds[d],CL_DRIVER_VERSION,sizeof(driver),driver,NULL)');check('pinned NVIDIA driver 580.178.04')
 api('clGetDeviceInfo(ds[d],CL_DEVICE_PCI_BUS_INFO_KHR,sizeof(pci),&pci,NULL)');check('pinned PCI 0000:01:00.0');check('unique target GPU');add(device='NVIDIA GeForce RTX 3090',driver='580.178.04');check('target present; no index-based fallback');add(phase='target_selected',schema_version=2,sizeof_mem=8)
 refs={};event=0xe000
 for pattern in case['ordered_protocol']:
  x=copy.deepcopy(pattern);kind=x.pop('protocol',None)
  if kind=='completion':
   event+=1;add(operation='event',a=hex(event),b='(nil)');add(api='clGetEventInfo(event,CL_EVENT_COMMAND_EXECUTION_STATUS,sizeof(state),&state,NULL)',status=0,expected=0,classification='expected');add(event=hex(event),execution_status=0);check('event completed within five seconds');add(api='clWaitForEvents(1,&event)',status=0,expected=0,classification='expected');continue
  for k,v in list(x.items()):
   if isinstance(v,str) and v.startswith('@'):refs[v[1:]]=hex(0x1000+len(refs)*0x10);x[k]=refs[v[1:]]
   elif isinstance(v,str) and v.startswith('$'):x[k]=refs[v[1:]]
  if kind=='data':x['data_u32']=oracle.expected(x['storage_tag'],x['expected_input_tag'])
  add(**x)
 return rows
