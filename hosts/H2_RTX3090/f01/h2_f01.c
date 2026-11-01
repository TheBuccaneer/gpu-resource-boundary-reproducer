/* H2-F01 v1: new relation, not a replacement for any S01 oracle. */
#include "../c1/reproducers/h2_common.h"
_Static_assert(sizeof(cl_mem)==8,"F01 is pinned to H2's 64-bit ABI");
#define WORDS 160u
#define ACTIVE 64u
#define OFFSET 16u
#define NONCE 0xf010ca5eu
#define SALT 0x37a1b5c9u
#define CONTROL_SALT 0x68b2d7e1u
static const char *kernel_source="__kernel void f01(__global const uint *a,__global uint *out,__global const uint *c,__global uint *control){size_t i=get_global_id(0);if(i<64){out[i+16]=a[i+16]^0x37a1b5c9u;control[i+16]=c[i+16]^0x68b2d7e1u;}}";
static unsigned generation;
static cl_uint initial(unsigned tag,size_t i){return ((cl_uint)(0x9e3779b9u*(cl_uint)(i+1)))^((cl_uint)tag*0x01020304u)^NONCE;}
static void api(const char *name,cl_int value){record(name,value,CL_SUCCESS);}
static unsigned object(const char *role,const char *kind,const void *handle,cl_context context,unsigned tag){
 unsigned gen=++generation;
 printf("{\"case\":\"H2-F01\",\"object\":");quoted(role);printf(",\"kind\":");quoted(kind);
 printf(",\"handle\":\"%p\",\"generation\":%u,\"context\":\"%p\",\"tag\":%u}\n",handle,gen,(void *)context,tag);return gen;
}
static cl_mem buffer(cl_context ctx,const char *role,unsigned tag){
 cl_uint data[WORDS];for(size_t i=0;i<WORDS;i++)data[i]=initial(tag,i);
 cl_int error;cl_mem mem=clCreateBuffer(ctx,CL_MEM_READ_WRITE|CL_MEM_COPY_HOST_PTR,sizeof(data),data,&error);
 char label[128];snprintf(label,sizeof(label),"clCreateBuffer(%s)",role);api(label,error);need(mem!=NULL,"new buffer exists");
 cl_context actual=NULL;api("clGetMemObjectInfo(CL_MEM_CONTEXT)",clGetMemObjectInfo(mem,CL_MEM_CONTEXT,sizeof(actual),&actual,NULL));need(actual==ctx,"buffer context query matches owner");
 (void)object(role,"buffer",mem,actual,tag);return mem;
}
static void set(cl_kernel kernel,unsigned index,cl_mem mem,const char *label){
 printf("{\"case\":\"H2-F01\",\"setter\":");quoted(label);printf(",\"kernel\":\"%p\",\"index\":%u,\"bytes\":%zu,\"buffer\":\"%p\"}\n",(void *)kernel,index,sizeof(mem),(void *)mem);
 api(label,clSetKernelArg(kernel,index,sizeof(mem),&mem));
}
static void event_done(cl_event event,const char *stage){
 need(event!=NULL,"event exists");unsigned gen=++generation;
 printf("{\"case\":\"H2-F01\",\"event_begin\":");quoted(stage);printf(",\"handle\":\"%p\",\"generation\":%u}\n",(void *)event,gen);
 cl_int state=CL_QUEUED;double deadline=now_sec()+5.0;
 do{
  api("clGetEventInfo(status)",clGetEventInfo(event,CL_EVENT_COMMAND_EXECUTION_STATUS,sizeof(state),&state,NULL));
  printf("{\"case\":\"H2-F01\",\"event_status\":%d,\"handle\":\"%p\",\"generation\":%u}\n",state,(void *)event,gen);
  if(state<=CL_COMPLETE)break;
  struct timespec delay={0,10000000};nanosleep(&delay,NULL);
 }while(now_sec()<deadline);
 need(state==CL_COMPLETE,"event complete within five seconds");api("clWaitForEvents",clWaitForEvents(1,&event));api("clReleaseEvent",clReleaseEvent(event));
 printf("{\"case\":\"H2-F01\",\"event_end\":%u}\n",gen);
}
static void readback(cl_command_queue q,cl_mem mem,const char *role,unsigned tag,unsigned source_tag,cl_uint salt){
 cl_uint data[WORDS];cl_event event=NULL;
 printf("{\"case\":\"H2-F01\",\"read\":");quoted(role);printf(",\"queue\":\"%p\",\"buffer\":\"%p\",\"bytes\":%zu}\n",(void *)q,(void *)mem,sizeof(data));
 api("clEnqueueReadBuffer",clEnqueueReadBuffer(q,mem,CL_FALSE,0,sizeof(data),data,0,NULL,&event));api("clFlush",clFlush(q));event_done(event,role);
 int equal=1;printf("{\"case\":\"H2-F01\",\"data\":");quoted(role);printf(",\"buffer\":\"%p\",\"words\":[",(void *)mem);
 for(size_t i=0;i<WORDS;i++){
  cl_uint want=source_tag&&i>=OFFSET&&i<OFFSET+ACTIVE?initial(source_tag,i)^salt:initial(tag,i);
  if(data[i]!=want)equal=0;
  printf("%s%u",i?",":"",data[i]);
 }
 puts("]}");need(equal,"all 160 words match host reference");
}
static void release_mem(cl_mem mem,const char *role){
 printf("{\"case\":\"H2-F01\",\"release\":");quoted(role);puts("}");api("clReleaseMemObject",clReleaseMemObject(mem));
}
int main(int argc,char **argv){
 if(argc!=3||strcmp(argv[1],"--approved-run")||strcmp(argv[2],"H2-F01")){fputs("New separate H2-F01 approval required\n",stderr);return 64;}
 case_id="H2-F01";setbuf(stdout,NULL);
#ifdef F01_CPU_FIXTURE
 const char *origin="CPU_FAKE_RUNTIME";
#else
 const char *origin="OPENCL_RUNTIME";
#endif
 printf("{\"case\":\"H2-F01\",\"schema\":1,\"plan_version\":1,\"origin\":");quoted(origin);puts(",\"global_workitems\":64,\"local_workitems\":1,\"words_per_buffer\":160}");
 cl_platform_id platform=NULL;cl_device_id device=select_device(&platform);cl_int error;
 const cl_context_properties props[]={CL_CONTEXT_PLATFORM,(cl_context_properties)platform,0};
 cl_context a=clCreateContext(props,1,&device,NULL,NULL,&error);api("clCreateContext(A)",error);need(a!=NULL,"context A exists");(void)object("A","context",a,a,0);
 cl_context b=clCreateContext(props,1,&device,NULL,NULL,&error);api("clCreateContext(B)",error);need(b!=NULL&&b!=a,"contexts distinct and live");(void)object("B","context",b,b,0);
 cl_program program=clCreateProgramWithSource(a,1,&kernel_source,NULL,&error);api("clCreateProgramWithSource",error);need(program!=NULL,"program exists");(void)object("P","program",program,a,0);
 error=clBuildProgram(program,1,&device,"-cl-std=CL1.2",NULL,NULL);char log[65536]={0};api("clGetProgramBuildInfo",clGetProgramBuildInfo(program,device,CL_PROGRAM_BUILD_LOG,sizeof(log)-1,log,NULL));fprintf(stderr,"BUILD_LOG: %s\n",log);api("clBuildProgram",error);
 cl_kernel kernel=clCreateKernel(program,"f01",&error);api("clCreateKernel",error);need(kernel!=NULL,"kernel exists");cl_context owner=NULL;
 api("clGetKernelInfo(CL_KERNEL_CONTEXT)",clGetKernelInfo(kernel,CL_KERNEL_CONTEXT,sizeof(owner),&owner,NULL));need(owner==a,"kernel context query matches A");(void)object("K","kernel",kernel,owner,0);
 printf("{\"case\":\"H2-F01\",\"kernel_program\":\"%p\",\"program\":\"%p\"}\n",(void *)kernel,(void *)program);
 cl_mem a1=buffer(a,"A1",1),ao=buffer(a,"AO",10),b1=buffer(b,"B1",2),sentinel=buffer(a,"AS",30),ci=buffer(a,"CI",3),co=buffer(a,"CO",11);
 cl_mem live[]={a1,ao,b1,sentinel,ci,co};int distinct=1;for(unsigned x=0;x<6;x++)for(unsigned y=x+1;y<6;y++)if(live[x]==live[y])distinct=0;need(distinct,"six live buffers distinct");
 set(kernel,0,a1,"set own A1");set(kernel,1,ao,"set own AO");set(kernel,2,ci,"set fresh control input");set(kernel,3,co,"set fresh control output");
 printf("{\"case\":\"H2-F01\",\"setter\":\"foreign transition\",\"kernel\":\"%p\",\"index\":0,\"bytes\":%zu,\"buffer\":\"%p\"}\n",(void *)kernel,sizeof(b1),(void *)b1);
 cl_int rejected=clSetKernelArg(kernel,0,sizeof(b1),&b1);
 const char *status_class=rejected==CL_INVALID_CONTEXT?"reject_invalid_context":(rejected==CL_INVALID_MEM_OBJECT?"reject_invalid_mem_object":(rejected==CL_SUCCESS?"unexpected_success":((rejected==CL_OUT_OF_RESOURCES||rejected==CL_OUT_OF_HOST_MEMORY)?"resource_error":"unexpected_rejection")));
 printf("{\"case\":\"H2-F01\",\"foreign_status\":%d,\"status_class\":",rejected);quoted(status_class);puts("}");
 if(rejected!=CL_INVALID_CONTEXT&&rejected!=CL_INVALID_MEM_OBJECT){
  printf("{\"case\":\"H2-F01\",\"stop\":\"before_queue_and_dispatch\",\"status_class\":");quoted(status_class);puts("}");fflush(NULL);return rejected==CL_OUT_OF_RESOURCES||rejected==CL_OUT_OF_HOST_MEMORY?78:20;
 }
 /* No subsequent clSetKernelArg. Queues exist only after accepted rejection. */
 const cl_queue_properties qp[]={CL_QUEUE_PROPERTIES,0,0};
 cl_command_queue qa=clCreateCommandQueueWithProperties(a,device,qp,&error);api("clCreateCommandQueue(A)",error);need(qa!=NULL,"queue A exists");(void)object("QA","queue",qa,a,0);
 cl_command_queue qb=clCreateCommandQueueWithProperties(b,device,qp,&error);api("clCreateCommandQueue(B)",error);need(qb!=NULL&&qb!=qa,"queue B exists and differs from A");(void)object("QB","queue",qb,b,0);
 const size_t global=ACTIVE,local=1;cl_event event=NULL;
 printf("{\"case\":\"H2-F01\",\"dispatch\":1,\"kernel\":\"%p\",\"queue\":\"%p\",\"global\":64,\"local\":1}\n",(void *)kernel,(void *)qa);
 api("clEnqueueNDRangeKernel",clEnqueueNDRangeKernel(qa,kernel,1,NULL,&global,&local,0,NULL,&event));api("clFlush",clFlush(qa));event_done(event,"kernel");
 /* Fresh positive arm first; four required boundary/sentinel readbacks next. */
 readback(qa,co,"CO",11,3,CONTROL_SALT);readback(qa,ci,"CI",3,0,0);
 readback(qa,ao,"AO",10,1,SALT);readback(qa,a1,"A1",1,0,0);readback(qb,b1,"B1",2,0,0);readback(qa,sentinel,"AS",30,0,0);
 api("clReleaseCommandQueue(B)",clReleaseCommandQueue(qb));api("clReleaseCommandQueue(A)",clReleaseCommandQueue(qa));api("clReleaseKernel",clReleaseKernel(kernel));api("clReleaseProgram",clReleaseProgram(program));
 release_mem(co,"CO");release_mem(ci,"CI");release_mem(sentinel,"AS");release_mem(b1,"B1");release_mem(ao,"AO");release_mem(a1,"A1");api("clReleaseContext(B)",clReleaseContext(b));api("clReleaseContext(A)",clReleaseContext(a));
 puts("{\"case\":\"H2-F01\",\"terminal\":\"observations_complete\",\"dispatches\":1,\"readbacks\":6,\"completions\":7}");return 0;
}
