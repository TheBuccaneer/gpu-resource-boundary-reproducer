/* Prepared only. Queries only; no contexts, semaphore objects, queues or dispatch. */
#include "h2_common.h"
#include "b12_validation.h"
static int mismatches=0;
static void result(const char *check,int valid) {
 printf("{\"case\":\"B12-Q\",\"check\":");quoted(check);printf(",\"pass\":%s}\n",valid?"true":"false");
 if(!valid)mismatches++;
}
static b12_list query(cl_platform_id p,cl_device_id d,int device,cl_uint key) {
 b12_list x={0};size_t bytes=0;
 x.size_status=device?clGetDeviceInfo(d,key,0,NULL,&bytes):clGetPlatformInfo(p,key,0,NULL,&bytes);
 printf("{\"scope\":\"%s\",\"device_handle\":\"%p\",\"selector\":%u,\"phase\":\"size\",\"status\":%d,\"size\":%zu,\"size_valid\":%s}\n",device?"device":"platform",(void *)d,key,x.size_status,bytes,x.size_status==0?"true":"false");
 if(x.size_status)return x; /* Never interpret error output as an empty list. */
 x.size=bytes;
 if(bytes>B12_MAX_BYTES||bytes%sizeof(uint32_t)){x.value_status=CL_INVALID_VALUE;return x;}
 if(!bytes)return x; /* Successful empty list: no second call is necessary. */
 x.value_status=device?clGetDeviceInfo(d,key,bytes,x.values,&x.returned_size):clGetPlatformInfo(p,key,bytes,x.values,&x.returned_size);
 printf("{\"scope\":\"%s\",\"device_handle\":\"%p\",\"selector\":%u,\"phase\":\"value\",\"status\":%d,\"size\":%zu,\"size_valid\":%s,\"raw_hex\":\"",device?"device":"platform",(void *)d,key,x.value_status,x.returned_size,x.value_status==0?"true":"false");
 if(x.value_status==0){const unsigned char *raw=(const unsigned char *)x.values;for(size_t i=0;i<bytes;i++)printf("%02x",raw[i]);}
 puts("\"}");return x;
}
static size_t handles(const char *ext,uint32_t out[5]) {
 size_t n=0;
 if(extension_has(ext,"cl_khr_external_semaphore_opaque_fd"))out[n++]=CL_SEMAPHORE_HANDLE_OPAQUE_FD_KHR;
 if(extension_has(ext,"cl_khr_external_semaphore_sync_fd"))out[n++]=CL_SEMAPHORE_HANDLE_SYNC_FD_KHR;

#ifdef CL_SEMAPHORE_HANDLE_OPAQUE_WIN32_KHR
 if(extension_has(ext,"cl_khr_external_semaphore_win32")){out[n++]=CL_SEMAPHORE_HANDLE_OPAQUE_WIN32_KHR;out[n++]=CL_SEMAPHORE_HANDLE_OPAQUE_WIN32_KMT_KHR;}

#endif
#ifdef CL_SEMAPHORE_HANDLE_D3D12_FENCE_KHR
 if(extension_has(ext,"cl_khr_external_semaphore_dx_fence"))out[n++]=CL_SEMAPHORE_HANDLE_D3D12_FENCE_KHR;

#endif
 return n;
}
int main(int argc,char **argv) {
 if(argc!=2||strcmp(argv[1],"--approved-query-run")){fputs("Prepared only. Explicit external approval required before --approved-query-run\n",stderr);return 64;}
 setbuf(stdout,NULL);case_id="B12-Q";cl_platform_id p=NULL;cl_device_id target=select_device(&p);
 char pe[8192]={0};OK(clGetPlatformInfo(p,CL_PLATFORM_EXTENSIONS,sizeof(pe),pe,NULL));
 need(extension_has(pe,"cl_khr_semaphore")&&extension_has(pe,"cl_khr_external_semaphore"),"platform extension gates");
 /* Include ALL platform devices, not merely the selected GPU, in intersections. */
 cl_uint count=0;OK(clGetDeviceIDs(p,CL_DEVICE_TYPE_ALL,0,NULL,&count));need(count>0&&count<=16,"bounded complete platform device set");
 cl_device_id devices[16];OK(clGetDeviceIDs(p,CL_DEVICE_TYPE_ALL,count,devices,NULL));
 b12_list pt=query(p,NULL,0,CL_PLATFORM_SEMAPHORE_TYPES_KHR),pi=query(p,NULL,0,CL_PLATFORM_SEMAPHORE_IMPORT_HANDLE_TYPES_KHR),px=query(p,NULL,0,CL_PLATFORM_SEMAPHORE_EXPORT_HANDLE_TYPES_KHR);
 b12_list dt[16],di[16],dx[16];uint32_t binary[]={CL_SEMAPHORE_TYPE_BINARY_KHR},allowed[5];
 size_t nallowed=handles(pe,allowed);int platform_valid=b12_valid(&pt,binary,1)&&b12_valid(&pi,allowed,nallowed)&&b12_valid(&px,allowed,nallowed);
 result("all platform list sizes, enums, uniqueness and successful value phases",platform_valid);
 int all_devices_valid=1,any_binary=0,target_found=0;
 for(cl_uint i=0;i<count;i++) {
  if(devices[i]==target)target_found=1;
  char ext[8192]={0};OK(clGetDeviceInfo(devices[i],CL_DEVICE_EXTENSIONS,sizeof(ext),ext,NULL));
  need(extension_has(ext,"cl_khr_semaphore")&&extension_has(ext,"cl_khr_external_semaphore"),"each device has advertised platform extensions");
  dt[i]=query(p,devices[i],1,CL_DEVICE_SEMAPHORE_TYPES_KHR);di[i]=query(p,devices[i],1,CL_DEVICE_SEMAPHORE_IMPORT_HANDLE_TYPES_KHR);dx[i]=query(p,devices[i],1,CL_DEVICE_SEMAPHORE_EXPORT_HANDLE_TYPES_KHR);
  nallowed=handles(ext,allowed);
  int valid=b12_valid(&dt[i],binary,1)&&b12_valid(&di[i],allowed,nallowed)&&b12_valid(&dx[i],allowed,nallowed);
  result("device list sizes, enums, uniqueness and successful value phases",valid);
  if(!valid){all_devices_valid=0;continue;}
  if(b12_has(&dt[i],CL_SEMAPHORE_TYPE_BINARY_KHR))any_binary=1;
  result("device supports import or export (import-only is legal)",b12_device_directions(&di[i],&dx[i]));
 }
 result("target included in complete device set",target_found);
 if(all_devices_valid){result("at least one device supports mandatory binary type",any_binary);}
 if(platform_valid&&all_devices_valid) {
  result("platform type list equals device intersection",b12_intersection(&pt,dt,count));
  result("platform import list equals device intersection",b12_intersection(&pi,di,count));
  result("platform export list equals device intersection",b12_intersection(&px,dx,count));
 }else result("intersection checks admissible only after valid list queries",0);
 char name[256]={0},byte=0;OK(clGetPlatformInfo(p,CL_PLATFORM_NAME,sizeof(name),name,NULL));OK(clGetDeviceInfo(target,CL_DEVICE_NAME,sizeof(name),name,NULL));
 size_t ignored=0;EXPECT(clGetPlatformInfo(p,CL_SEMAPHORE_TYPE_KHR,0,NULL,&ignored),CL_INVALID_VALUE);EXPECT(clGetDeviceInfo(target,CL_SEMAPHORE_TYPE_KHR,0,NULL,&ignored),CL_INVALID_VALUE);
 EXPECT(clGetDeviceInfo(target,CL_DEVICE_NAME,1,&byte,NULL),CL_INVALID_VALUE);EXPECT(clGetPlatformInfo(p,CL_PLATFORM_NAME,1,&byte,NULL),CL_INVALID_VALUE);
 return mismatches?20:0;
}
