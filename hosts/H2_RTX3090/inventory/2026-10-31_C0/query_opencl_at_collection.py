#!/usr/bin/env python3
"""C0: enumeration/info only; never creates a context, program, queue or allocation."""
import ctypes as C
import ctypes.util
import json
import re
import sys
from pathlib import Path

def query():
    lib = C.CDLL(ctypes.util.find_library('OpenCL') or 'libOpenCL.so.1')
    lib.clGetPlatformIDs.argtypes = [C.c_uint, C.POINTER(C.c_void_p), C.POINTER(C.c_uint)]
    lib.clGetDeviceIDs.argtypes = [C.c_void_p,C.c_ulonglong,C.c_uint,C.POINTER(C.c_void_p),C.POINTER(C.c_uint)]
    for name in ['clGetPlatformInfo','clGetDeviceInfo']:
        getattr(lib,name).argtypes=[C.c_void_p,C.c_uint,C.c_size_t,C.c_void_p,C.POINTER(C.c_size_t)]
    lib.clGetExtensionFunctionAddressForPlatform.argtypes=[C.c_void_p,C.c_char_p]
    lib.clGetExtensionFunctionAddressForPlatform.restype=C.c_void_p
    def info(fn,obj,key,kind):
        size=C.c_size_t(); rc=fn(obj,key,0,None,C.byref(size))
        result={'selector':hex(key),'status':rc,'size':size.value}
        if rc: return result
        if size.value>1024*1024: return dict(result,error='size guard')
        b=C.create_string_buffer(size.value); rc=fn(obj,key,size.value,b,None)
        result['status']=rc; result['raw_hex']=b.raw.hex()
        if rc: return result
        raw=b.raw
        if kind=='str': value=raw.rstrip(b'\0').decode('utf-8','replace')
        elif kind=='versions':
            value=[]
            for i in range(0,len(raw),68):
                v=int.from_bytes(raw[i:i+4],sys.byteorder)
                value.append({'version':f'{v>>22}.{(v>>12)&1023}.{v&4095}','name':raw[i+4:i+68].split(b'\0')[0].decode('utf-8','replace')})
        elif kind.startswith('array'):
            step=4 if kind=='array4' else C.sizeof(C.c_size_t)
            value=[int.from_bytes(raw[i:i+step],sys.byteorder) for i in range(0,len(raw),step)]
        else: value=int.from_bytes(raw,sys.byteorder)
        result['value']=value
        return result
    props={
    'TYPE':(0x1000,'int'),'VENDOR_ID':(0x1001,'int'),'MAX_COMPUTE_UNITS':(0x1002,'int'),
    'MAX_WORK_ITEM_DIMENSIONS':(0x1003,'int'),'MAX_WORK_GROUP_SIZE':(0x1004,'int'),'MAX_WORK_ITEM_SIZES':(0x1005,'array'),
    'MAX_MEM_ALLOC_SIZE':(0x1010,'int'),'IMAGE_SUPPORT':(0x1016,'int'),'MEM_BASE_ADDR_ALIGN':(0x1019,'int'),
    'GLOBAL_MEM_SIZE':(0x101f,'int'),'MAX_CONSTANT_BUFFER_SIZE':(0x1020,'int'),'LOCAL_MEM_TYPE':(0x1022,'int'),
    'LOCAL_MEM_SIZE':(0x1023,'int'),'PROFILING_TIMER_RESOLUTION':(0x1025,'int'),'AVAILABLE':(0x1027,'int'),
    'COMPILER_AVAILABLE':(0x1028,'int'),'EXECUTION_CAPABILITIES':(0x1029,'int'),'QUEUE_ON_HOST_PROPERTIES':(0x102a,'int'),
    'NAME':(0x102b,'str'),'VENDOR':(0x102c,'str'),'DRIVER_VERSION':(0x102d,'str'),'PROFILE':(0x102e,'str'),
    'VERSION':(0x102f,'str'),'EXTENSIONS':(0x1030,'str'),'HOST_UNIFIED_MEMORY':(0x1035,'int'),
    'OPENCL_C_VERSION':(0x103d,'str'),'LINKER_AVAILABLE':(0x103e,'int'),'PARTITION_MAX_SUB_DEVICES':(0x1043,'int'),
    'PARTITION_PROPERTIES':(0x1044,'array'),'PARTITION_AFFINITY_DOMAIN':(0x1045,'int'),
    'QUEUE_ON_DEVICE_PROPERTIES':(0x104e,'int'),'MAX_ON_DEVICE_QUEUES':(0x1051,'int'),'MAX_ON_DEVICE_EVENTS':(0x1052,'int'),
    'SVM_CAPABILITIES':(0x1053,'int'),'IL_VERSION':(0x105b,'str'),'NUMERIC_VERSION':(0x105e,'int'),
    'EXTENSIONS_WITH_VERSION':(0x1060,'versions'),'ILS_WITH_VERSION':(0x1061,'versions'),
    'ATOMIC_MEMORY_CAPABILITIES':(0x1063,'int'),'ATOMIC_FENCE_CAPABILITIES':(0x1064,'int'),
    'NON_UNIFORM_WORK_GROUP_SUPPORT':(0x1065,'int'),'OPENCL_C_ALL_VERSIONS':(0x1066,'versions'),
    'PREFERRED_WORK_GROUP_SIZE_MULTIPLE':(0x1067,'int'),'GENERIC_ADDRESS_SPACE_SUPPORT':(0x1069,'int'),
    'OPENCL_C_FEATURES':(0x106f,'versions'),'DEVICE_ENQUEUE_CAPABILITIES':(0x1070,'int'),
    'LATEST_CONFORMANCE_VERSION_PASSED':(0x1072,'str'),
    'PCI_BUS_INFO_KHR':(0x410f,'array4'),
    'EXTERNAL_MEMORY_IMPORT_HANDLE_TYPES_KHR':(0x204f,'array4'),
    'SEMAPHORE_TYPES_KHR':(0x204c,'array4'),'SEMAPHORE_IMPORT_HANDLE_TYPES_KHR':(0x204d,'array4'),
    'SEMAPHORE_EXPORT_HANDLE_TYPES_KHR':(0x204e,'array4'),
    'COMMAND_BUFFER_CAPABILITIES_KHR':(0x12a9,'int'),'COMMAND_BUFFER_REQUIRED_QUEUE_PROPERTIES_KHR':(0x12aa,'int'),
    'MUTABLE_DISPATCH_CAPABILITIES_KHR':(0x12b0,'int')}
    n=C.c_uint(); rc=lib.clGetPlatformIDs(0,None,C.byref(n))
    result={'loader':lib._name,'platform_enumeration_status':rc,'platform_count':n.value,'platforms':[]}
    if rc: return result
    platforms=(C.c_void_p*n.value)(); lib.clGetPlatformIDs(n.value,platforms,None)
    for platform in platforms:
        p={'info':{k:info(lib.clGetPlatformInfo,platform,v,'str') for k,v in {'PROFILE':0x900,'VERSION':0x901,'NAME':0x902,'VENDOR':0x903,'EXTENSIONS':0x904}.items()},'devices':[]}
        p['info']['EXTENSIONS_WITH_VERSION']=info(lib.clGetPlatformInfo,platform,0x907,'versions')
        count=C.c_uint(); rc=lib.clGetDeviceIDs(platform,0xffffffff,0,None,C.byref(count))
        p['device_enumeration_status']=rc
        if rc==0:
            devs=(C.c_void_p*count.value)(); lib.clGetDeviceIDs(platform,0xffffffff,count.value,devs,None)
            for device in devs: p['devices'].append({k:info(lib.clGetDeviceInfo,device,v,t) for k,(v,t) in props.items()})
        names=['clCreateProgramWithILKHR','clGetMemObjectDeviceAddressEXT','clCreateCommandBufferKHR','clUpdateMutableCommandsKHR','clCreateSemaphoreWithPropertiesKHR','clGetSemaphoreInfoKHR','clGetSemaphoreHandleForTypeKHR','clCreateBufferWithProperties','clEnqueueAcquireExternalMemObjectsKHR']
        p['extension_entrypoints']={name:bool(lib.clGetExtensionFunctionAddressForPlatform(platform,name.encode())) for name in names}
        result['platforms'].append(p)
    result['loaded_library_maps']=[line.strip() for line in Path('/proc/self/maps').read_text().splitlines() if re.search(r'OpenCL|nvidia|cuda|pocl|Rusticl|rusticl',line)]
    return result
if __name__=='__main__':
    try: print(json.dumps(query(),indent=2))
    except Exception as e: print(json.dumps({'error':str(e)},indent=2)); sys.exit(1)
