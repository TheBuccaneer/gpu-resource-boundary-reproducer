/* S01: one explicitly approved case; no execution during preparation. */
#include "../c1/reproducers/h2_common.h"
#include <dlfcn.h>
_Static_assert(sizeof(cl_mem)==8,"S01 protocol targets H2 64-bit ABI");
#define WORDS 160u
#define ACTIVE 64u
#define OFFSET 16u
#define SALT 0x37a1b5c9u
static const char *source="__kernel void s01(__global const uint *in,__global uint *out){size_t i=get_global_id(0);if(i<64)out[i+16]=in[i+16]^0x37a1b5c9u;}";
static const char *cases[]={"S01-OWN-API","S01-FOREIGN-API","S01-INDEX-API","S01-SIZE-API","S01-OWN-DATA","S01-ROLLBACK-DATA","S01-INDEX-DATA","S01-SIZE-DATA","S01-CLONE-DATA","S01-CLONE-API","S01-CLONE-CONTROL-DATA"};
static void status(const char *api,cl_int got,cl_int want) {
 const char *kind=got==want?"expected":((got==CL_OUT_OF_RESOURCES||got==CL_OUT_OF_HOST_MEMORY)?"resource_error_inconclusive":"contract_or_setup_mismatch");
 printf("{\"case\":");quoted(case_id);printf(",\"api\":");quoted(api);printf(",\"status\":%d,\"expected\":%d,\"classification\":",got,want);quoted(kind);puts("}");
 if(got!=want){fflush(NULL);_Exit((got==CL_OUT_OF_RESOURCES||got==CL_OUT_OF_HOST_MEMORY)?78:20);}
}
#define SUCCESS(x) status(#x,(x),CL_SUCCESS)
#define RELEASE(expr,ptr) do {action("release",ptr,NULL);SUCCESS(expr);}while(0)
static void action(const char *op,const void *a,const void *b) {
 printf("{\"case\":");quoted(case_id);printf(",\"operation\":");quoted(op);printf(",\"a\":\"%p\",\"b\":\"%p\"}\n",a,b);
}
static void finish_event(cl_event event) {
 action("event",event,NULL);double deadline=now_sec()+5.0;cl_int state=CL_QUEUED;
 do {
  SUCCESS(clGetEventInfo(event,CL_EVENT_COMMAND_EXECUTION_STATUS,sizeof(state),&state,NULL));
  printf("{\"case\":");quoted(case_id);printf(",\"event\":\"%p\",\"execution_status\":%d}\n",(void *)event,state);
  if(state<=CL_COMPLETE)break;
  struct timespec delay={0,10000000};nanosleep(&delay,NULL);
 }while(now_sec()<deadline);
 need(state==CL_COMPLETE,"event completed within five seconds");SUCCESS(clWaitForEvents(1,&event));
}
static void setarg(cl_kernel k,cl_uint index,cl_mem m,size_t bytes,cl_int want,const char *label) {
 action("setarg",k,m);
 printf("{\"case\":");quoted(case_id);printf(",\"arg_index\":%u,\"arg_size\":%zu}\n",index,bytes);
 status(label,clSetKernelArg(k,index,bytes,&m),want);
}
static cl_uint value(unsigned tag,size_t i) {return (cl_uint)(0x9e3779b9u*(cl_uint)(i+1))^((cl_uint)tag*0x1020304u);}
static cl_mem make_buffer(cl_context ctx,unsigned tag) {
 cl_uint data[WORDS];for(size_t i=0;i<WORDS;i++)data[i]=value(tag,i);
 cl_int e;cl_mem m=clCreateBuffer(ctx,CL_MEM_READ_WRITE|CL_MEM_COPY_HOST_PTR,sizeof(data),data,&e);status("clCreateBuffer",e,0);need(m!=NULL,"buffer nonnull");
 cl_context actual=NULL;SUCCESS(clGetMemObjectInfo(m,CL_MEM_CONTEXT,sizeof(actual),&actual,NULL));need(actual==ctx,"buffer context matches ledger");
 printf("{\"case\":");quoted(case_id);printf(",\"object\":\"%p\",\"context\":\"%p\",\"tag\":%u,\"bytes\":%zu,\"host_refs\":1}\n",(void *)m,(void *)ctx,tag,sizeof(data));return m;
}
static cl_command_queue make_queue(cl_context ctx,cl_device_id d) {
 const cl_queue_properties props[]={CL_QUEUE_PROPERTIES,0,0};cl_int e;
 cl_command_queue q=clCreateCommandQueueWithProperties(ctx,d,props,&e);status("clCreateCommandQueueWithProperties",e,0);need(q!=NULL,"queue nonnull");action("queue",q,ctx);return q;
}
static void read_check(cl_command_queue q,cl_mem m,unsigned storage_tag,unsigned input_tag) {
 cl_uint data[WORDS];cl_event ev=NULL;action("read",q,m);
 SUCCESS(clEnqueueReadBuffer(q,m,CL_FALSE,0,sizeof(data),data,0,NULL,&ev));SUCCESS(clFlush(q));finish_event(ev);
 int equal=1;printf("{\"case\":");quoted(case_id);printf(",\"object\":\"%p\",\"storage_tag\":%u,\"expected_input_tag\":%u,\"data_u32\":[",(void *)m,storage_tag,input_tag);
 for(size_t i=0;i<WORDS;i++) {
  cl_uint want=value(storage_tag,i);
  if(input_tag&&i>=OFFSET&&i<OFFSET+ACTIVE)want=value(input_tag,i)^SALT;
  if(data[i]!=want)equal=0;
  printf("%s%u",i?",":"",data[i]);
 }
 puts("]}");need(equal,"entire output payload and guards match host reference");SUCCESS(clReleaseEvent(ev));
}
static void dispatch(cl_command_queue q,cl_kernel k) {
 const size_t g=ACTIVE,l=1;cl_event e=NULL;action("dispatch",q,k);
 SUCCESS(clEnqueueNDRangeKernel(q,k,1,NULL,&g,&l,0,NULL,&e));SUCCESS(clFlush(q));finish_event(e);SUCCESS(clReleaseEvent(e));
}
static void reset_output(cl_command_queue q,cl_mem m) {
 cl_uint data[WORDS];for(size_t i=0;i<WORDS;i++)data[i]=value(10,i);cl_event e=NULL;action("reset",q,m);
 SUCCESS(clEnqueueWriteBuffer(q,m,CL_FALSE,0,sizeof(data),data,0,NULL,&e));SUCCESS(clFlush(q));finish_event(e);SUCCESS(clReleaseEvent(e));
 read_check(q,m,10,0);
}
int main(int argc,char **argv) {
 if(argc!=3||strcmp(argv[1],"--approved-run")){fputs("Separate approval required: --approved-run S01-CASE\n",stderr);return 64;}
 case_id=argv[2];int known=0;for(size_t i=0;i<sizeof(cases)/sizeof(cases[0]);i++)if(!strcmp(case_id,cases[i]))known=1;
 if(!known){fputs("Unknown case; no OpenCL calls\n",stderr);return 64;}
 setbuf(stdout,NULL);int data=strstr(case_id,"-DATA")!=NULL,clone=strstr(case_id,"CLONE")!=NULL;
 cl_platform_id platform=NULL;cl_device_id d=select_device(&platform);cl_int e;
 printf("{\"case\":");quoted(case_id);puts(",\"phase\":\"target_selected\",\"schema_version\":2,\"sizeof_mem\":8}");
 char version[256]={0};SUCCESS(clGetPlatformInfo(platform,CL_PLATFORM_VERSION,sizeof(version),version,NULL));
 unsigned major=0,minor=0;need(sscanf(version,"OpenCL %u.%u",&major,&minor)==2,"parse platform API version");
 printf("{\"case\":");quoted(case_id);printf(",\"platform_version\":");quoted(version);printf(",\"data_arm\":%s,\"clone_arm\":%s}\n",data?"true":"false",clone?"true":"false");
 /* Target is OpenCL 3.0. Clone introduced in 2.1; absent loader entry point
  * is setup/capability skip, not a claim of normative optionality on 3.0. */
 if(clone&&((major<2||(major==2&&minor<1))||dlsym(RTLD_DEFAULT,"clCloneKernel")==NULL)) {
  puts("{\"outcome\":\"capability_or_loader_skip\",\"reason\":\"clone version or symbol unavailable\"}");return 77;
 }
 cl_bool compiler=CL_FALSE;SUCCESS(clGetDeviceInfo(d,CL_DEVICE_COMPILER_AVAILABLE,sizeof(compiler),&compiler,NULL));
 if(!compiler){puts("{\"outcome\":\"capability_skip\",\"reason\":\"source compiler unavailable\"}");return 77;}
 const cl_context_properties props[]={CL_CONTEXT_PLATFORM,(cl_context_properties)platform,0};
 cl_context a=clCreateContext(props,1,&d,NULL,NULL,&e);status("clCreateContext(A)",e,0);need(a!=NULL,"A exists");
 cl_context b=clCreateContext(props,1,&d,NULL,NULL,&e);status("clCreateContext(B)",e,0);need(b!=NULL&&a!=b,"distinct live same-device contexts");action("contexts",a,b);
 cl_program p=clCreateProgramWithSource(a,1,&source,NULL,&e);status("clCreateProgramWithSource",e,0);need(p!=NULL,"program exists");action("program",p,a);
 e=clBuildProgram(p,1,&d,"-cl-std=CL1.2",NULL,NULL);
 char log[65536]={0};SUCCESS(clGetProgramBuildInfo(p,d,CL_PROGRAM_BUILD_LOG,sizeof(log)-1,log,NULL));fprintf(stderr,"BUILD_LOG: %s\n",log);status("clBuildProgram(-cl-std=CL1.2)",e,0);
 cl_kernel k=clCreateKernel(p,"s01",&e);status("clCreateKernel",e,0);need(k!=NULL,"kernel exists");
 cl_context owner=NULL;SUCCESS(clGetKernelInfo(k,CL_KERNEL_CONTEXT,sizeof(owner),&owner,NULL));need(owner==a,"kernel owned by A");action("kernel",k,a);
 cl_mem input=make_buffer(a,1),foreign=make_buffer(b,2),other=make_buffer(a,3),out[3];
 for(unsigned i=0;i<3;i++)out[i]=make_buffer(a,10+i);
 setarg(k,0,input,sizeof(input),0,"own input setter");setarg(k,1,out[0],sizeof(out[0]),0,"own output setter");
 if(strstr(case_id,"FOREIGN")||strstr(case_id,"ROLLBACK")||!strcmp(case_id,"S01-CLONE-DATA"))setarg(k,0,foreign,sizeof(foreign),CL_INVALID_CONTEXT,"transition foreign context setter");
 else if(strstr(case_id,"INDEX"))setarg(k,2,input,sizeof(input),CL_INVALID_ARG_INDEX,"transition invalid index");
 else if(strstr(case_id,"SIZE"))setarg(k,0,input,sizeof(input)-1,CL_INVALID_ARG_SIZE,"transition invalid size");
 else setarg(k,0,input,sizeof(input),0,"own repeated setter");
 /* Every mismatch exits above, before clone, queue creation or enqueue. */
 cl_kernel copy=NULL;
 if(clone){copy=clCloneKernel(k,&e);status("clCloneKernel",e,0);need(copy!=NULL&&copy!=k,"distinct clone");action("clone",copy,k);}
 if(data) {
  cl_command_queue qa=make_queue(a,d),qb=make_queue(b,d);
  if(copy)setarg(copy,1,out[1],sizeof(out[1]),0,"clone output setter");
  dispatch(qa,k);read_check(qa,out[0],10,1);
  if(copy) {
   dispatch(qa,copy);read_check(qa,out[1],11,1);
   setarg(copy,0,other,sizeof(other),0,"clone rebind input");setarg(copy,1,out[2],sizeof(out[2]),0,"clone rebind output");
   dispatch(qa,copy);read_check(qa,out[2],12,3);
   /* Reset and confirm completion: stale original output cannot pass. */
   reset_output(qa,out[0]);
   dispatch(qa,k);read_check(qa,out[0],10,1);read_check(qa,out[2],12,3);
  }
  read_check(qa,input,1,0);read_check(qa,other,3,0);read_check(qb,foreign,2,0);
  if(!copy){read_check(qa,out[1],11,0);read_check(qa,out[2],12,0);}
  RELEASE(clReleaseCommandQueue(qb),qb);RELEASE(clReleaseCommandQueue(qa),qa);
 }
 if(copy)RELEASE(clReleaseKernel(copy),copy);
 RELEASE(clReleaseKernel(k),k);RELEASE(clReleaseProgram(p),p);
 for(unsigned i=0;i<3;i++)RELEASE(clReleaseMemObject(out[i]),out[i]);
 RELEASE(clReleaseMemObject(other),other);RELEASE(clReleaseMemObject(foreign),foreign);RELEASE(clReleaseMemObject(input),input);RELEASE(clReleaseContext(b),b);RELEASE(clReleaseContext(a),a);
 printf("{\"case\":");quoted(case_id);printf(",\"outcome\":\"expected_observations_complete\",\"argument_state_observed_by_data\":%s}\n",data?"true":"false");return 0;
}
