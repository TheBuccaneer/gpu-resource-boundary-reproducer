/* Prepared only. No contexts, semaphore creation, queues, programs or dispatch. */
#include "h2_common.h"
static int mismatches=0;
static void query(cl_platform_id p,cl_device_id d,int device,cl_uint key,cl_int expected) {
 size_t bytes=0;cl_int rc=device?clGetDeviceInfo(d,key,0,NULL,&bytes):clGetPlatformInfo(p,key,0,NULL,&bytes);
 printf("{\"scope\":\"%s\",\"selector\":%u,\"phase\":\"size\",\"status\":%d,\"size\":%zu,\"size_valid\":%s}\n",device?"device":"platform",key,rc,bytes,rc==0?"true":"false");
 /* Continue collecting independent query rows on mismatch; never use size from an error. */
 if(rc!=expected){mismatches++;fprintf(stderr,"Query mismatch scope=%d key=0x%x got=%d expected=%d\n",device,key,rc,expected);}
 if(rc||expected)return;
 if(!bytes){if(device&&key==CL_DEVICE_SEMAPHORE_TYPES_KHR)mismatches++;return;}
 need(bytes<=4096,"bounded info query");unsigned char data[4096]={0};size_t actual=0;
 rc=device?clGetDeviceInfo(d,key,bytes,data,&actual):clGetPlatformInfo(p,key,bytes,data,&actual);
 printf("{\"scope\":\"%s\",\"selector\":%u,\"phase\":\"value\",\"status\":%d,\"size\":%zu,\"raw_hex\":\"",device?"device":"platform",key,rc,actual);
 if(rc==0){for(size_t i=0;i<bytes;i++)printf("%02x",data[i]);}
 puts("\"}");
 if(rc!=CL_SUCCESS||actual!=bytes)mismatches++;
 if(rc==0&&device&&key==CL_DEVICE_SEMAPHORE_TYPES_KHR){need(bytes%sizeof(cl_semaphore_type_khr)==0,"type list element width");int found=0;for(size_t i=0;i<bytes;i+=sizeof(cl_uint)){cl_uint v=0;memcpy(&v,data+i,sizeof(v));if(v==CL_SEMAPHORE_TYPE_BINARY_KHR)found=1;}need(found,"mandatory binary type present");}
}
int main(int argc,char **argv) {
 if(argc!=2||strcmp(argv[1],"--approved-query-run")){fputs("Prepared query reproducer. Run only after explicit approval: --approved-query-run\n",stderr);return 64;}
 setbuf(stdout,NULL);case_id="B12-Q";cl_platform_id p=NULL;cl_device_id d=select_device(&p);
 /* Extension gates are checked before interpreting extension-specific selectors. */
 char pe[8192]={0},de[8192]={0};OK(clGetPlatformInfo(p,CL_PLATFORM_EXTENSIONS,sizeof(pe),pe,NULL));OK(clGetDeviceInfo(d,CL_DEVICE_EXTENSIONS,sizeof(de),de,NULL));
 need(extension_has(pe,"cl_khr_semaphore")&&extension_has(de,"cl_khr_semaphore"),"semaphore advertised in both scopes");
 need(extension_has(pe,"cl_khr_external_semaphore")&&extension_has(de,"cl_khr_external_semaphore"),"external semaphore advertised in both scopes");
 query(p,d,0,CL_PLATFORM_NAME,CL_SUCCESS);query(p,d,1,CL_DEVICE_NAME,CL_SUCCESS);
 query(p,d,0,CL_PLATFORM_SEMAPHORE_TYPES_KHR,CL_SUCCESS);query(p,d,1,CL_DEVICE_SEMAPHORE_TYPES_KHR,CL_SUCCESS);
 query(p,d,0,CL_PLATFORM_SEMAPHORE_IMPORT_HANDLE_TYPES_KHR,CL_SUCCESS);query(p,d,1,CL_DEVICE_SEMAPHORE_IMPORT_HANDLE_TYPES_KHR,CL_SUCCESS);
 query(p,d,0,CL_PLATFORM_SEMAPHORE_EXPORT_HANDLE_TYPES_KHR,CL_SUCCESS);query(p,d,1,CL_DEVICE_SEMAPHORE_EXPORT_HANDLE_TYPES_KHR,CL_SUCCESS);
 /* Object selector is invalid in device/platform query namespaces. */
 query(p,d,0,CL_SEMAPHORE_TYPE_KHR,CL_INVALID_VALUE);query(p,d,1,CL_SEMAPHORE_TYPE_KHR,CL_INVALID_VALUE);
 char byte=0;EXPECT(clGetDeviceInfo(d,CL_DEVICE_NAME,1,&byte,NULL),CL_INVALID_VALUE);
 EXPECT(clGetPlatformInfo(p,CL_PLATFORM_NAME,1,&byte,NULL),CL_INVALID_VALUE);
 return mismatches?20:0; /* Per-row statuses remain the evidence. */
}
