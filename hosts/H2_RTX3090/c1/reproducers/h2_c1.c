/* Prepared only: requires separate approval before ANY runtime invocation.
 * One bounded case per fresh process. No local-memory capacity or lifetime triggers.
 */
#include "h2_common.h"
#define WORDS 4096
#define SENTINEL 0xa55aa55au
static const char *src=
"__kernel void base(__global uint *p){size_t i=get_global_id(0);if(i<64)p[i]=((uint)i^0x12345678u)+1u;}\n"
"__attribute__((reqd_work_group_size(32,1,1))) __kernel void req32(__global uint *p){size_t i=get_global_id(0);if(i<64)p[i]=((uint)i^0x12345678u)+1u;}\n";
static cl_program program(cl_context c,cl_device_id d) {
 cl_int e;cl_program p=clCreateProgramWithSource(c,1,&src,NULL,&e);record("clCreateProgramWithSource",e,CL_SUCCESS);need(p!=NULL,"program nonnull");
 e=clBuildProgram(p,1,&d,"-cl-std=CL1.2",NULL,NULL);
 char log[65536]={0};size_t n=0;cl_int log_status=clGetProgramBuildInfo(p,d,CL_PROGRAM_BUILD_LOG,sizeof(log)-1,log,&n);record("clGetProgramBuildInfo(log)",log_status,CL_SUCCESS);fprintf(stderr,"BUILD_LOG(%zu):\n%s\n",n,log);
 record("clBuildProgram(CL1.2)",e,CL_SUCCESS);return p;
}
static cl_mem buffer(cl_context c) {
 cl_uint data[WORDS];for(size_t i=0;i<WORDS;i++)data[i]=SENTINEL;
 cl_int e;cl_mem b=clCreateBuffer(c,CL_MEM_READ_WRITE|CL_MEM_COPY_HOST_PTR,sizeof(data),data,&e);record("clCreateBuffer(16KiB)",e,CL_SUCCESS);need(b!=NULL,"buffer nonnull");return b;
}
static void check_data(cl_command_queue q,cl_mem b,int kernel_result) {
 cl_uint data[WORDS];OK(clEnqueueReadBuffer(q,b,CL_TRUE,0,sizeof(data),data,0,NULL,NULL));
 int equal=1;for(size_t i=0;i<WORDS;i++){cl_uint expected=i<64?(kernel_result?(((cl_uint)i^0x12345678u)+1u):0x01234567u):SENTINEL;if(data[i]!=expected)equal=0;}
 printf("{\"case\":");quoted(case_id);printf(",\"data_u32_hex\":\"");for(size_t i=0;i<WORDS;i++)printf("%08x",data[i]);puts("\"}");
 need(equal,"full 16KiB data and guard comparison");
}
static cl_command_queue queue(cl_context c,cl_device_id d,int ooo) {
 cl_queue_properties props[]={CL_QUEUE_PROPERTIES,CL_QUEUE_PROFILING_ENABLE|(ooo?CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE:0),0};
 cl_int e;cl_command_queue q=clCreateCommandQueueWithProperties(c,d,props,&e);record("clCreateCommandQueueWithProperties",e,CL_SUCCESS);need(q!=NULL,"queue nonnull");
 cl_command_queue_properties flags=0;OK(clGetCommandQueueInfo(q,CL_QUEUE_PROPERTIES,sizeof(flags),&flags,NULL));
 need((flags&(cl_command_queue_properties)props[1])==(cl_command_queue_properties)props[1],"requested queue properties active");
 printf("{\"case\":");quoted(case_id);printf(",\"queue\":\"%p\",\"context\":\"%p\",\"queue_flags\":%llu}\n",(void *)q,(void *)c,(unsigned long long)flags);return q;
}
int main(int argc,char **argv) {
 const char *cases[]={"BL-KERNEL","BL-BUFFER","B2-VALID-L1","B2-VALID-L16","B2-VALID-L32","B2-DIM0","B2-DIM4","B2-REQ-VALID","B2-REQ-MISMATCH","B2-NONDIVISIBLE","B7-OWN","B7-FOREIGN-AB","B7-FOREIGN-BA","B7-INDEX","B7-SIZE","B10-IO","B10-XQUEUE","B10-OOO","B10-USER","B10-MARKER-VALID","B10-FOREIGN-EVENT","B10-WAITLIST-NULL","B10-WAITLIST-ZERO","B10-USER-ERROR","B10-USER-DOUBLE","B10-USER-INVALID"};
 if(argc!=3||strcmp(argv[1],"--approved-run")){fputs("Prepared only: --approved-run CASE (separate approval required).\n",stderr);return 64;}
 case_id=argv[2];int known=0;for(size_t i=0;i<sizeof(cases)/sizeof(cases[0]);i++)if(!strcmp(case_id,cases[i]))known=1;
 if(!known){fputs("Unknown case; no OpenCL call made.\n",stderr);return 64;}
 setbuf(stdout,NULL);cl_platform_id platform=NULL;cl_device_id d=select_device(&platform);cl_int e;
 cl_context_properties cp[]={CL_CONTEXT_PLATFORM,(cl_context_properties)platform,0};
 cl_context a=clCreateContext(cp,1,&d,NULL,NULL,&e);record("clCreateContext(A)",e,CL_SUCCESS);need(a!=NULL,"context A nonnull");
 int is_b7=!strncmp(case_id,"B7-",3);int is_b2=!strncmp(case_id,"B2-",3);
 if(is_b7) {
  cl_context b=clCreateContext(cp,1,&d,NULL,NULL,&e);record("clCreateContext(B)",e,CL_SUCCESS);need(b!=NULL&&a!=b,"distinct same-device contexts");
  cl_context owner=!strcmp(case_id,"B7-FOREIGN-BA")?b:a;cl_context other=owner==a?b:a;
  cl_program p=program(owner,d);cl_kernel k=clCreateKernel(p,"base",&e);record("clCreateKernel",e,CL_SUCCESS);need(k!=NULL,"kernel nonnull");
  cl_mem own=buffer(owner),foreign=buffer(other);cl_context actual=NULL;
  OK(clGetMemObjectInfo(own,CL_MEM_CONTEXT,sizeof(actual),&actual,NULL));need(actual==owner,"own buffer context");
  OK(clGetMemObjectInfo(foreign,CL_MEM_CONTEXT,sizeof(actual),&actual,NULL));need(actual==other,"foreign buffer context");
  printf("{\"case\":");quoted(case_id);printf(",\"owner_context\":\"%p\",\"foreign_context\":\"%p\",\"own_mem\":\"%p\",\"foreign_mem\":\"%p\",\"sizeof_mem\":%zu}\n",(void *)owner,(void *)other,(void *)own,(void *)foreign,sizeof(cl_mem));
  OK(clSetKernelArg(k,0,sizeof(own),&own));
  if(strstr(case_id,"FOREIGN"))EXPECT(clSetKernelArg(k,0,sizeof(foreign),&foreign),CL_INVALID_CONTEXT);
  else if(!strcmp(case_id,"B7-INDEX"))EXPECT(clSetKernelArg(k,1,sizeof(own),&own),CL_INVALID_ARG_INDEX);
  else if(!strcmp(case_id,"B7-SIZE"))EXPECT(clSetKernelArg(k,0,sizeof(own)-1,&own),CL_INVALID_ARG_SIZE);
  /* No queue or enqueue exists on ANY B7 path, including unexpected success. */
  OK(clReleaseKernel(k));OK(clReleaseProgram(p));OK(clReleaseMemObject(foreign));OK(clReleaseMemObject(own));OK(clReleaseContext(b));
 }else {
  int ooo=!strcmp(case_id,"B10-OOO");
  if(ooo){cl_command_queue_properties v=0;OK(clGetDeviceInfo(d,CL_DEVICE_QUEUE_ON_HOST_PROPERTIES,sizeof(v),&v,NULL));need((v&CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE)!=0,"OOO gate");}
  cl_command_queue q=queue(a,d,ooo);cl_mem mem=buffer(a);cl_event event=NULL;
  if(!strcmp(case_id,"BL-KERNEL")||is_b2) {
   cl_program p=program(a,d);const char *name=strstr(case_id,"B2-REQ-")==case_id?"req32":"base";
   cl_kernel k=clCreateKernel(p,name,&e);record("clCreateKernel",e,CL_SUCCESS);need(k!=NULL,"kernel nonnull");OK(clSetKernelArg(k,0,sizeof(mem),&mem));
   size_t kwg=0,cwg[3]={0};OK(clGetKernelWorkGroupInfo(k,d,CL_KERNEL_WORK_GROUP_SIZE,sizeof(kwg),&kwg,NULL));OK(clGetKernelWorkGroupInfo(k,d,CL_KERNEL_COMPILE_WORK_GROUP_SIZE,sizeof(cwg),cwg,NULL));
   need(kwg>=32,"kernel supports bounded local size 32");printf("{\"case\":");quoted(case_id);printf(",\"kernel_wg\":%zu,\"compile_wg\":[%zu,%zu,%zu]}\n",kwg,cwg[0],cwg[1],cwg[2]);
   size_t g[4]={64,1,1,1},l[4]={32,1,1,1};cl_uint dim=1;cl_int want=0;
   if(!strcmp(case_id,"B2-VALID-L1"))l[0]=1;
   if(!strcmp(case_id,"B2-VALID-L16"))l[0]=16;
   if(!strcmp(case_id,"B2-DIM0")){dim=0;want=CL_INVALID_WORK_DIMENSION;}
   if(!strcmp(case_id,"B2-DIM4")){dim=4;want=CL_INVALID_WORK_DIMENSION;}
   if(strstr(case_id,"B2-REQ-")==case_id)need(cwg[0]==32&&cwg[1]==1&&cwg[2]==1,"req32 metadata");
   if(!strcmp(case_id,"B2-REQ-MISMATCH")){l[0]=16;want=CL_INVALID_WORK_GROUP_SIZE;}
   if(!strcmp(case_id,"B2-NONDIVISIBLE")){g[0]=65;want=CL_INVALID_WORK_GROUP_SIZE;}
   printf("{\"case\":");quoted(case_id);printf(",\"work_dim\":%u,\"global\":[%zu,%zu,%zu,%zu],\"local\":[%zu,%zu,%zu,%zu]}\n",dim,g[0],g[1],g[2],g[3],l[0],l[1],l[2],l[3]);
   EXPECT(clEnqueueNDRangeKernel(q,k,dim,NULL,g,l,0,NULL,&event),want);
   if(!want){OK(clFlush(q));completed(event);check_data(q,mem,1);}else need(event==NULL,"rejected enqueue created no event");
   OK(clReleaseKernel(k));OK(clReleaseProgram(p));
  }else if(!strcmp(case_id,"B10-MARKER-VALID")) {
   cl_event u=clCreateUserEvent(a,&e);record("clCreateUserEvent",e,CL_SUCCESS);need(u!=NULL,"user event nonnull");OK(clSetUserEventStatus(u,CL_COMPLETE));
   OK(clEnqueueMarkerWithWaitList(q,1,&u,&event));OK(clFlush(q));completed(event);OK(clReleaseEvent(u));
  }else if(!strcmp(case_id,"B10-FOREIGN-EVENT")) {
   cl_context b=clCreateContext(cp,1,&d,NULL,NULL,&e);record("clCreateContext(B)",e,CL_SUCCESS);
   cl_event u=clCreateUserEvent(b,&e);record("clCreateUserEvent(B)",e,CL_SUCCESS);need(u!=NULL,"foreign user event nonnull");OK(clSetUserEventStatus(u,CL_COMPLETE));
   EXPECT(clEnqueueMarkerWithWaitList(q,1,&u,&event),CL_INVALID_CONTEXT);need(event==NULL,"foreign-event rejection creates no event");OK(clReleaseEvent(u));OK(clReleaseContext(b));
  }else if(!strcmp(case_id,"B10-WAITLIST-NULL")) {
   EXPECT(clEnqueueMarkerWithWaitList(q,1,NULL,&event),CL_INVALID_EVENT_WAIT_LIST);need(event==NULL,"bad waitlist creates no event");
  }else if(!strcmp(case_id,"B10-WAITLIST-ZERO")) {
   cl_event u=clCreateUserEvent(a,&e);record("clCreateUserEvent",e,CL_SUCCESS);need(u!=NULL,"user event nonnull");OK(clSetUserEventStatus(u,CL_COMPLETE));
   EXPECT(clEnqueueMarkerWithWaitList(q,0,&u,&event),CL_INVALID_EVENT_WAIT_LIST);need(event==NULL,"bad waitlist creates no event");OK(clReleaseEvent(u));
  }else if(!strncmp(case_id,"B10-USER-",9)) {
   cl_event u=clCreateUserEvent(a,&e);record("clCreateUserEvent",e,CL_SUCCESS);need(u!=NULL,"user event nonnull");
   if(!strcmp(case_id,"B10-USER-ERROR")){
    cl_event positive=clCreateUserEvent(a,&e);record("clCreateUserEvent(positive)",e,CL_SUCCESS);need(positive!=NULL,"positive user event");OK(clSetUserEventStatus(positive,CL_COMPLETE));OK(clWaitForEvents(1,&positive));OK(clReleaseEvent(positive));
    OK(clSetUserEventStatus(u,-1));EXPECT(clWaitForEvents(1,&u),CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST);}
   else if(!strcmp(case_id,"B10-USER-DOUBLE")){OK(clSetUserEventStatus(u,CL_COMPLETE));EXPECT(clSetUserEventStatus(u,CL_COMPLETE),CL_INVALID_OPERATION);}
   else {EXPECT(clSetUserEventStatus(u,CL_RUNNING),CL_INVALID_VALUE);OK(clSetUserEventStatus(u,CL_COMPLETE));}
   OK(clReleaseEvent(u));
  }else {
   cl_uint pattern=0x01234567u;
   if(!strcmp(case_id,"B10-USER")) {
    cl_event u=clCreateUserEvent(a,&e);record("clCreateUserEvent",e,CL_SUCCESS);need(u!=NULL,"user event nonnull");
    OK(clEnqueueFillBuffer(q,mem,&pattern,sizeof(pattern),0,64*sizeof(pattern),1,&u,&event));OK(clFlush(q));
    cl_int status=0;OK(clGetEventInfo(event,CL_EVENT_COMMAND_EXECUTION_STATUS,sizeof(status),&status,NULL));
    /* Resolve u before any object-release call, even if the observation is wrong. */
    OK(clSetUserEventStatus(u,CL_COMPLETE));need(status>CL_COMPLETE,"dependent event incomplete before user signal");completed(event);check_data(q,mem,0);OK(clReleaseEvent(u));
   }else {
    cl_mem other=buffer(a);cl_event first=NULL,copy=NULL;
    OK(clEnqueueFillBuffer(q,other,&pattern,sizeof(pattern),0,64*sizeof(pattern),0,NULL,&first));
    cl_command_queue q2=!strcmp(case_id,"B10-XQUEUE")?queue(a,d,0):q;
    OK(clFlush(q));
    /* IO deliberately relies on ordering; XQUEUE/OOO use explicit dependencies. */
    cl_uint count=!strcmp(case_id,"B10-IO")?0:1;const cl_event *wait=count?&first:NULL;
    OK(clEnqueueCopyBuffer(q2,other,mem,0,0,64*sizeof(pattern),count,wait,&copy));OK(clFlush(q2));completed(copy);check_data(q,mem,0);
    completed(first);profile_event(first);profile_event(copy);OK(clReleaseEvent(first));OK(clReleaseEvent(copy));OK(clReleaseMemObject(other));if(q2!=q)OK(clReleaseCommandQueue(q2));
   }
  }
  if(event){profile_event(event);OK(clReleaseEvent(event));}
  OK(clReleaseMemObject(mem));OK(clReleaseCommandQueue(q));
 }
 OK(clReleaseContext(a));printf("{\"case\":");quoted(case_id);puts(",\"outcome\":\"expected_observations_complete\"}");return 0;
}
