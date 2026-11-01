#ifndef H2_COMMON_H
#define H2_COMMON_H
#define _POSIX_C_SOURCE 200809L
#define CL_TARGET_OPENCL_VERSION 300
#include <CL/cl.h>
#include <CL/cl_ext.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
static const char *case_id;
static inline double now_sec(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t); return t.tv_sec+t.tv_nsec/1e9; }
static inline void quoted(const char *s) {
 putchar('"');for(;*s;s++){unsigned char c=(unsigned char)*s;if(c=='"'||c=='\\')printf("\\%c",c);else if(c<32)printf("\\u%04x",c);else putchar(c);}putchar('"');
}
static inline void record(const char *api,cl_int got,cl_int expected) {
 printf("{\"case\":");quoted(case_id);printf(",\"api\":");quoted(api);
 printf(",\"status\":%d,\"expected\":%d,\"monotonic_s\":%.9f,\"classification\":",got,expected,now_sec());
 quoted(got==expected?"expected":((got==CL_OUT_OF_RESOURCES||got==CL_OUT_OF_HOST_MEMORY)?"resource_error_inconclusive":"contract_or_setup_mismatch"));puts("}");
 if(got!=expected){fprintf(stderr,"STOP: unexpected API status; no follow-up dispatch\n");fflush(NULL);_Exit((got==CL_OUT_OF_RESOURCES||got==CL_OUT_OF_HOST_MEMORY)?78:20);}
}
#define OK(expr) record(#expr,(expr),CL_SUCCESS)
#define EXPECT(expr,expected) record(#expr,(expr),(expected))
static inline void need(int yes,const char *message) {
 printf("{\"case\":");quoted(case_id);printf(",\"check\":");quoted(message);printf(",\"pass\":%s}\n",yes?"true":"false");
 if(!yes){fflush(NULL);_Exit(21);}
}
static inline int extension_has(const char *list,const char *name) {
 size_t n=strlen(name);const char *at=list;while((at=strstr(at,name))){if((at==list||at[-1]==' ')&&(at[n]==0||at[n]==' '))return 1;at+=n;}return 0;
}
static inline void maps(void) {
 FILE *f=fopen("/proc/self/maps","r"); if(!f)return;
 char line[4096];while(fgets(line,sizeof(line),f))if(strstr(line,"OpenCL")||strstr(line,"nvidia")||strstr(line,"libcuda")) {
  printf("{\"case\":");quoted(case_id);printf(",\"library_map\":");quoted(line);puts("}");
 }fclose(f);
}
static inline cl_device_id select_device(cl_platform_id *platform) {
 cl_uint np=0;OK(clGetPlatformIDs(0,NULL,&np));need(np>0&&np<=16,"bounded platform count");
 cl_platform_id ps[16];OK(clGetPlatformIDs(np,ps,NULL));cl_device_id selected=NULL;
 for(cl_uint p=0;p<np;p++) {
  cl_uint n=0;cl_int e=clGetDeviceIDs(ps[p],CL_DEVICE_TYPE_GPU,0,NULL,&n);
  if(e==CL_DEVICE_NOT_FOUND)continue;
  record("clGetDeviceIDs(count)",e,CL_SUCCESS);need(n>0&&n<=16,"bounded GPU count");
  cl_device_id ds[16];OK(clGetDeviceIDs(ps[p],CL_DEVICE_TYPE_GPU,n,ds,NULL));
  for(cl_uint d=0;d<n;d++) {
   cl_uint vendor=0;char name[256]={0},driver[256]={0};
   OK(clGetDeviceInfo(ds[d],CL_DEVICE_VENDOR_ID,sizeof(vendor),&vendor,NULL));
   OK(clGetDeviceInfo(ds[d],CL_DEVICE_NAME,sizeof(name),name,NULL));
   if(vendor!=0x10de||strcmp(name,"NVIDIA GeForce RTX 3090"))continue;
   OK(clGetDeviceInfo(ds[d],CL_DRIVER_VERSION,sizeof(driver),driver,NULL));
   need(!strcmp(driver,"580.178.04"),"pinned NVIDIA driver 580.178.04");
   cl_device_pci_bus_info_khr pci={0};OK(clGetDeviceInfo(ds[d],CL_DEVICE_PCI_BUS_INFO_KHR,sizeof(pci),&pci,NULL));
   need(pci.pci_domain==0&&pci.pci_bus==1&&pci.pci_device==0&&pci.pci_function==0,"pinned PCI 0000:01:00.0");
   need(selected==NULL,"unique target GPU");selected=ds[d];*platform=ps[p];
   printf("{\"case\":");quoted(case_id);printf(",\"device\":");quoted(name);printf(",\"driver\":");quoted(driver);puts("}");
  }
 }
 need(selected!=NULL,"target present; no index-based fallback");maps();return selected;
}
static inline void completed(cl_event event) {
 double deadline=now_sec()+5.0;cl_int status=CL_QUEUED;
 do {OK(clGetEventInfo(event,CL_EVENT_COMMAND_EXECUTION_STATUS,sizeof(status),&status,NULL));
  if(status<=CL_COMPLETE)break;
  struct timespec delay={0,10000000};nanosleep(&delay,NULL);
 }while(now_sec()<deadline);
 need(status==CL_COMPLETE,"event completed within five seconds");OK(clWaitForEvents(1,&event));
}
static inline void profile_event(cl_event event) {
 cl_ulong times[4]={0};const cl_profiling_info keys[]={CL_PROFILING_COMMAND_QUEUED,CL_PROFILING_COMMAND_SUBMIT,CL_PROFILING_COMMAND_START,CL_PROFILING_COMMAND_END};
 for(int i=0;i<4;i++)OK(clGetEventProfilingInfo(event,keys[i],sizeof(times[i]),&times[i],NULL));
 need(times[0]<=times[1]&&times[1]<=times[2]&&times[2]<=times[3],"nondecreasing profiling timestamps");
 printf("{\"case\":");quoted(case_id);printf(",\"event\":\"%p\",\"profiling_ns\":[%llu,%llu,%llu,%llu]}\n",(void *)event,(unsigned long long)times[0],(unsigned long long)times[1],(unsigned long long)times[2],(unsigned long long)times[3]);
}
#endif
