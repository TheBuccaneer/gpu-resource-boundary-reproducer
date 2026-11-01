/* Minimal offline discovery source. No approval/supervisor/ledger integration. */
#include "../c1/reproducers/h2_common.h"
#include <stdint.h>
#define WORDS 64u
#define TARGET 16u
#define NONCE 0xd2289d0fu
#define MARKER 0x7a3ff789u
static const char *source="__kernel void svm_probe(__global uint *p){p[16]=0x7a3ff789u;}";
static cl_uint initial(unsigned role,unsigned i){return (cl_uint)(0x9e3779b9u*(i+1u))^NONCE^(0x01020304u*role);}
static void done(cl_command_queue q,cl_event e,const char *stage){
 need(e!=NULL,"event exists");OK(clFlush(q));cl_int s=CL_QUEUED;double until=now_sec()+5;
 do{
  OK(clGetEventInfo(e,CL_EVENT_COMMAND_EXECUTION_STATUS,sizeof(s),&s,NULL));
  printf("{\"stage\":");quoted(stage);printf(",\"event\":\"%p\",\"raw_event_status\":%d}\n",(void *)e,s);
  if(s<=CL_COMPLETE)break;
  struct timespec pause={0,10000000};nanosleep(&pause,NULL);
 }while(now_sec()<until);
 need(s==CL_COMPLETE,"normal event completion within5s");OK(clWaitForEvents(1,&e));OK(clReleaseEvent(e));
}
static void map(cl_command_queue q,cl_uint *ptr,cl_map_flags flags,const char *stage){
 cl_event e=NULL;OK(clEnqueueSVMMap(q,CL_FALSE,flags,ptr,WORDS*sizeof(*ptr),0,NULL,&e));done(q,e,stage);
}
static void unmap(cl_command_queue q,cl_uint *ptr,const char *stage){
 cl_event e=NULL;OK(clEnqueueSVMUnmap(q,ptr,0,NULL,&e));done(q,e,stage);
}
static void initialize(cl_command_queue q,cl_uint *ptr,unsigned role){
 map(q,ptr,CL_MAP_WRITE,"initialize-map");for(unsigned i=0;i<WORDS;i++)ptr[i]=initial(role,i);unmap(q,ptr,"initialize-unmap");
}
static void read_words(cl_command_queue q,cl_uint *ptr,unsigned role,cl_uint out[WORDS]){
 map(q,ptr,CL_MAP_READ,"read-map");memcpy(out,ptr,WORDS*sizeof(*ptr));
 printf("{\"read_role\":%u,\"queue\":\"%p\",\"pointer\":\"%p\",\"words\":[",role,(void *)q,(void *)ptr);
 for(unsigned i=0;i<WORDS;i++){printf("%s%u",i?",":"",out[i]);}
 puts("]}");unmap(q,ptr,"read-unmap");
}
static void queue_owner(cl_command_queue q,cl_context ctx,cl_device_id device){
 cl_context actual=NULL;cl_device_id d=NULL;OK(clGetCommandQueueInfo(q,CL_QUEUE_CONTEXT,sizeof(actual),&actual,NULL));OK(clGetCommandQueueInfo(q,CL_QUEUE_DEVICE,sizeof(d),&d,NULL));need(actual==ctx&&d==device,"queue owner/device match");
}
int main(int argc,char **argv){
 if(argc!=3||strcmp(argv[1],"--run-after-separate-go")||(strcmp(argv[2],"positive")&&strcmp(argv[2],"sentinel")&&strcmp(argv[2],"foreign"))){fputs("Offline preparation only; separate future GO required; cases positive/sentinel/foreign\n",stderr);return 64;}
 const char *mode=argv[2];case_id="SVM-POINTER-DISCOVERY";setbuf(stdout,NULL);
#ifdef SVM_CPU_FIXTURE
 const char *origin="CPU_FAKE_ONLY";
#else
 const char *origin="OPENCL_RUNTIME";
#endif
 printf("{\"schema\":1,\"origin\":");quoted(origin);printf(",\"mode\":");quoted(mode);printf(",\"nonce\":%u,\"marker\":%u}\n",NONCE,MARKER);
 cl_platform_id platform=NULL;cl_device_id d=select_device(&platform);cl_device_svm_capabilities caps=0;
 OK(clGetDeviceInfo(d,CL_DEVICE_SVM_CAPABILITIES,sizeof(caps),&caps,NULL));printf("{\"svm_capabilities\":%llu}\n",(unsigned long long)caps);need((caps&CL_DEVICE_SVM_COARSE_GRAIN_BUFFER)!=0,"coarse SVM supported");
 cl_int err;const cl_context_properties props[]={CL_CONTEXT_PLATFORM,(cl_context_properties)platform,0};
 cl_context a=clCreateContext(props,1,&d,NULL,NULL,&err);record("context A",err,0);need(a!=NULL,"context A exists");
 cl_context b=clCreateContext(props,1,&d,NULL,NULL,&err);record("context B",err,0);need(b!=NULL&&a!=b,"two distinct live contexts");
 const cl_queue_properties qp[]={CL_QUEUE_PROPERTIES,0,0};
 cl_command_queue qa=clCreateCommandQueueWithProperties(a,d,qp,&err);record("queue A",err,0);need(qa!=NULL,"queue A exists");queue_owner(qa,a,d);
 cl_command_queue qb=clCreateCommandQueueWithProperties(b,d,qp,&err);record("queue B",err,0);need(qb!=NULL&&qb!=qa,"queue B distinct");queue_owner(qb,b,d);
 cl_uint *pa=clSVMAlloc(a,CL_MEM_READ_WRITE,WORDS*sizeof(cl_uint),0);cl_uint *pb=clSVMAlloc(b,CL_MEM_READ_WRITE,WORDS*sizeof(cl_uint),0);
 need(pa!=NULL&&pb!=NULL&&pa!=pb,"two distinct live SVM allocations");need((uintptr_t)pa%sizeof(cl_uint)==0&&(uintptr_t)pb%sizeof(cl_uint)==0,"aligned uint pointers");
 printf("{\"A\":\"%p\",\"B\":\"%p\",\"QA\":\"%p\",\"QB\":\"%p\",\"A_pointer\":\"%p\",\"B_pointer\":\"%p\",\"allocation_bytes\":%zu}\n",(void *)a,(void *)b,(void *)qa,(void *)qb,(void *)pa,(void *)pb,WORDS*sizeof(cl_uint));
 initialize(qa,pa,1);initialize(qb,pb,2);
 cl_program program=NULL;cl_kernel kernel=NULL;unsigned dispatches=0;int rejected=0;
 if(strcmp(mode,"sentinel")){
  program=clCreateProgramWithSource(a,1,&source,NULL,&err);record("program A",err,0);need(program!=NULL,"program exists");
  err=clBuildProgram(program,1,&d,"-cl-std=CL2.0",NULL,NULL);char log[4096]={0};OK(clGetProgramBuildInfo(program,d,CL_PROGRAM_BUILD_LOG,sizeof(log)-1,log,NULL));fprintf(stderr,"BUILD_LOG: %s\n",log);record("build",err,0);
  kernel=clCreateKernel(program,"svm_probe",&err);record("kernel A",err,0);need(kernel!=NULL,"kernel exists");cl_context owner=NULL;OK(clGetKernelInfo(kernel,CL_KERNEL_CONTEXT,sizeof(owner),&owner,NULL));need(owner==a,"kernel in A");
  cl_uint *selected=!strcmp(mode,"foreign")?pb:pa;
  cl_int setter=clSetKernelArgSVMPointer(kernel,0,selected);
  printf("{\"setter_kernel\":\"%p\",\"kernel_context\":\"%p\",\"pointer\":\"%p\",\"pointer_owner\":\"%p\",\"raw_setter_status\":%d}\n",(void *)kernel,(void *)a,(void *)selected,(void *)(selected==pb?b:a),setter);
  if(setter!=CL_SUCCESS){
   if(strcmp(mode,"foreign"))record("positive setter",setter,0);
   puts("{\"stop\":\"setter_rejected_before_dispatch\"}");rejected=1;goto cleanup;
  }
  const size_t one=1;cl_event e=NULL;
  printf("{\"dispatch\":1,\"queue\":\"%p\",\"kernel\":\"%p\",\"workitems\":1,\"local\":1}\n",(void *)qa,(void *)kernel);
  cl_int enqueue=clEnqueueNDRangeKernel(qa,kernel,1,NULL,&one,&one,0,NULL,&e);record("raw enqueue",enqueue,0);dispatches=1;done(qa,e,"A-kernel");
 }
 /* B is never mapped through QA, nor read until the A event is complete. */
 cl_uint av[WORDS],bv[WORDS];read_words(qa,pa,1,av);read_words(qb,pb,2,bv);int valid=1;
 for(unsigned i=0;i<WORDS;i++){
  cl_uint awant=!strcmp(mode,"positive")&&i==TARGET?MARKER:initial(1,i);
  if(av[i]!=awant)valid=0;
  if(!strcmp(mode,"foreign")&&i==TARGET){if(bv[i]!=initial(2,i)&&bv[i]!=MARKER)valid=0;}
  else if(bv[i]!=initial(2,i))valid=0;
 }
 printf("{\"B_target_changed\":%s,\"interpretation\":\"same_process_context_candidate_only_no_conformance_or_security_claim\"}\n",bv[TARGET]!=initial(2,TARGET)?"true":"false");
 need(valid,"all controls and63 guards per allocation valid");
cleanup:
 if(kernel){OK(clReleaseKernel(kernel));}
 if(program){OK(clReleaseProgram(program));}
 clSVMFree(b,pb);clSVMFree(a,pa);OK(clReleaseCommandQueue(qb));OK(clReleaseCommandQueue(qa));OK(clReleaseContext(b));OK(clReleaseContext(a));
 printf("{\"terminal\":\"observations_complete\",\"dispatches\":%u,\"setter_rejected\":%s}\n",dispatches,rejected?"true":"false");return 0;
}
