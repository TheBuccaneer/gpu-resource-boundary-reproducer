/* F02 v1. Separate, bounded clone snapshot/reverse test. No foreign setters. */
#include "../c1/reproducers/h2_common.h"
_Static_assert(sizeof(cl_mem)==8,"Pinned H2 64-bit ABI");
#define WORDS 160u
#define ACTIVE 64u
#define OFFSET 16u
#define NONCE 0x53f8695au
#define SALT 0xceae4507u
static const char *kernel_source="__kernel void f02(__global const uint *a,__global uint *out){size_t i=get_global_id(0);if(i<64)out[i+16]=a[i+16]^0xceae4507u;}";
static unsigned generation;
static cl_context owner_context;
static cl_uint initial(unsigned tag,size_t i){return ((cl_uint)(0x9e3779b9u*(cl_uint)(i+1)))^((cl_uint)tag*0x01020304u)^NONCE;}
static void api(const char *name,cl_int value){record(name,value,CL_SUCCESS);}
static unsigned object(const char *role,const char *kind,const void *handle,cl_context context,unsigned tag){
 unsigned gen=++generation;
 printf("{\"case\":\"F02-CLONE-SNAPSHOT-REVERSE\",\"object\":");quoted(role);printf(",\"kind\":");quoted(kind);
 printf(",\"handle\":\"%p\",\"generation\":%u,\"context\":\"%p\",\"tag\":%u}\n",handle,gen,(void *)context,tag);return gen;
}
static cl_mem buffer(cl_context ctx,const char *role,unsigned tag){
 cl_uint data[WORDS];for(size_t i=0;i<WORDS;i++)data[i]=initial(tag,i);
 cl_int error;cl_mem mem=clCreateBuffer(ctx,CL_MEM_READ_WRITE|CL_MEM_COPY_HOST_PTR,sizeof(data),data,&error);
 char label[128];snprintf(label,sizeof(label),"clCreateBuffer(%s)",role);api(label,error);need(mem!=NULL,"new buffer exists");
 cl_context actual=NULL;api("clGetMemObjectInfo(CL_MEM_CONTEXT)",clGetMemObjectInfo(mem,CL_MEM_CONTEXT,sizeof(actual),&actual,NULL));need(actual==ctx,"buffer context query matches owner");
 size_t bytes=0;api("clGetMemObjectInfo(CL_MEM_SIZE)",clGetMemObjectInfo(mem,CL_MEM_SIZE,sizeof(bytes),&bytes,NULL));need(bytes==sizeof(data),"buffer size is 640 bytes");
 (void)object(role,"buffer",mem,actual,tag);return mem;
}
static void set(cl_kernel kernel,unsigned index,cl_mem mem,const char *label){
 printf("{\"case\":\"F02-CLONE-SNAPSHOT-REVERSE\",\"setter\":");quoted(label);printf(",\"kernel\":\"%p\",\"index\":%u,\"bytes\":%zu,\"buffer\":\"%p\"}\n",(void *)kernel,index,sizeof(mem),(void *)mem);
 api(label,clSetKernelArg(kernel,index,sizeof(mem),&mem));
}
static void event_done(cl_command_queue queue,cl_event event,const char *stage,cl_command_type type){
 need(event!=NULL,"event exists");unsigned gen=++generation;
 printf("{\"case\":\"F02-CLONE-SNAPSHOT-REVERSE\",\"event_begin\":");quoted(stage);printf(",\"handle\":\"%p\",\"generation\":%u}\n",(void *)event,gen);
 cl_command_queue actual_queue=NULL;cl_context actual_context=NULL;cl_command_type actual_type=0;
 api("clGetEventInfo(queue)",clGetEventInfo(event,CL_EVENT_COMMAND_QUEUE,sizeof(actual_queue),&actual_queue,NULL));
 api("clGetEventInfo(context)",clGetEventInfo(event,CL_EVENT_CONTEXT,sizeof(actual_context),&actual_context,NULL));
 api("clGetEventInfo(type)",clGetEventInfo(event,CL_EVENT_COMMAND_TYPE,sizeof(actual_type),&actual_type,NULL));
 need(actual_queue==queue&&actual_context==owner_context&&actual_type==type,"event queue context and type match");
 cl_int state=CL_QUEUED;double deadline=now_sec()+5.0;
 do{
  api("clGetEventInfo(status)",clGetEventInfo(event,CL_EVENT_COMMAND_EXECUTION_STATUS,sizeof(state),&state,NULL));
  printf("{\"case\":\"F02-CLONE-SNAPSHOT-REVERSE\",\"event_status\":%d,\"handle\":\"%p\",\"generation\":%u}\n",state,(void *)event,gen);
  if(state<=CL_COMPLETE)break;
  struct timespec delay={0,10000000};nanosleep(&delay,NULL);
 }while(now_sec()<deadline);
 need(state==CL_COMPLETE,"event complete within five seconds");api("clWaitForEvents",clWaitForEvents(1,&event));api("clReleaseEvent",clReleaseEvent(event));
 printf("{\"case\":\"F02-CLONE-SNAPSHOT-REVERSE\",\"event_end\":%u}\n",gen);
}
static void readback(cl_command_queue q,cl_mem mem,const char *role,const char *stage,unsigned tag,unsigned source_tag,cl_uint salt){
 cl_uint data[WORDS];cl_event event=NULL;
 printf("{\"case\":\"F02-CLONE-SNAPSHOT-REVERSE\",\"read\":");quoted(stage);printf(",\"queue\":\"%p\",\"buffer\":\"%p\",\"bytes\":%zu}\n",(void *)q,(void *)mem,sizeof(data));
 api("clEnqueueReadBuffer",clEnqueueReadBuffer(q,mem,CL_FALSE,0,sizeof(data),data,0,NULL,&event));api("clFlush",clFlush(q));event_done(q,event,stage,CL_COMMAND_READ_BUFFER);
 int equal=1;printf("{\"case\":\"F02-CLONE-SNAPSHOT-REVERSE\",\"data\":");quoted(stage);printf(",\"role\":");quoted(role);printf(",\"buffer\":\"%p\",\"words\":[",(void *)mem);
 for(size_t i=0;i<WORDS;i++){
  cl_uint want=source_tag&&i>=OFFSET&&i<OFFSET+ACTIVE?initial(source_tag,i)^salt:initial(tag,i);
  if(data[i]!=want)equal=0;
  printf("%s%u",i?",":"",data[i]);
 }
 puts("]}");need(equal,"all 160 words match host reference");
}
static void release_mem(cl_mem mem,const char *role){
 printf("{\"case\":\"F02-CLONE-SNAPSHOT-REVERSE\",\"release\":");quoted(role);puts("}");api("clReleaseMemObject",clReleaseMemObject(mem));
}

static void kernel_identity(cl_kernel k,const char *role,cl_context ctx,cl_program program){
 cl_context actual=NULL;cl_program actual_program=NULL;
 api("clGetKernelInfo(CL_KERNEL_CONTEXT)",clGetKernelInfo(k,CL_KERNEL_CONTEXT,sizeof(actual),&actual,NULL));
 api("clGetKernelInfo(CL_KERNEL_PROGRAM)",clGetKernelInfo(k,CL_KERNEL_PROGRAM,sizeof(actual_program),&actual_program,NULL));
 need(actual==ctx&&actual_program==program,"kernel context and program match");(void)object(role,"kernel",k,actual,0);
 printf("{\"case\":\"F02-CLONE-SNAPSHOT-REVERSE\",\"kernel_program\":\"%p\",\"program\":\"%p\"}\n",(void *)k,(void *)program);
}
static void dispatch(cl_command_queue q,cl_kernel k,unsigned number,const char *stage){
 const size_t global=ACTIVE,local=1;cl_event event=NULL;
 printf("{\"case\":\"F02-CLONE-SNAPSHOT-REVERSE\",\"dispatch\":%u,\"kernel\":\"%p\",\"queue\":\"%p\",\"global\":64,\"local\":1}\n",number,(void *)k,(void *)q);
 api("clEnqueueNDRangeKernel",clEnqueueNDRangeKernel(q,k,1,NULL,&global,&local,0,NULL,&event));api("clFlush",clFlush(q));event_done(q,event,stage,CL_COMMAND_NDRANGE_KERNEL);
}
static void reset(cl_command_queue q,cl_mem mem){
 cl_uint data[WORDS];for(size_t i=0;i<WORDS;i++)data[i]=initial(10,i);cl_event event=NULL;
 printf("{\"case\":\"F02-CLONE-SNAPSHOT-REVERSE\",\"reset\":\"O0\",\"queue\":\"%p\",\"buffer\":\"%p\",\"bytes\":%zu}\n",(void *)q,(void *)mem,sizeof(data));
 api("clEnqueueWriteBuffer(reset)",clEnqueueWriteBuffer(q,mem,CL_FALSE,0,sizeof(data),data,0,NULL,&event));api("clFlush",clFlush(q));event_done(q,event,"reset-write",CL_COMMAND_WRITE_BUFFER);
}
int main(int argc,char **argv){
 if(argc!=3||strcmp(argv[1],"--approved-run")||strcmp(argv[2],"F02-CLONE-SNAPSHOT-REVERSE")){fputs("Separate explicit F02 approval required\n",stderr);return 64;}
 case_id="F02-CLONE-SNAPSHOT-REVERSE";setbuf(stdout,NULL);
#ifdef F02_CPU_FIXTURE
 const char *origin="CPU_FAKE_RUNTIME";
#else
 const char *origin="OPENCL_RUNTIME";
#endif
 printf("{\"case\":\"F02-CLONE-SNAPSHOT-REVERSE\",\"schema\":1,\"plan_version\":1,\"origin\":");quoted(origin);
 printf(",\"nonce\":%u,\"salt\":%u,\"global_workitems\":64,\"local_workitems\":1,\"words_per_buffer\":160}\n",NONCE,SALT);
 cl_platform_id platform=NULL;cl_device_id device=select_device(&platform);cl_int error;
 /* Clone is a core 2.1 API, with OpenCL 3.0 availability checked explicitly. */
 cl_version numeric=0;api("clGetDeviceInfo(CL_DEVICE_NUMERIC_VERSION)",clGetDeviceInfo(device,CL_DEVICE_NUMERIC_VERSION,sizeof(numeric),&numeric,NULL));need(numeric>=CL_MAKE_VERSION(2,1,0),"core clone API version available");
 const cl_context_properties props[]={CL_CONTEXT_PLATFORM,(cl_context_properties)platform,0};
 cl_context a=clCreateContext(props,1,&device,NULL,NULL,&error);api("clCreateContext(A)",error);need(a!=NULL,"context A exists");(void)object("A","context",a,a,0);owner_context=a;
 cl_program program=clCreateProgramWithSource(a,1,&kernel_source,NULL,&error);api("clCreateProgramWithSource",error);need(program!=NULL,"program exists");(void)object("P","program",program,a,0);
 error=clBuildProgram(program,1,&device,"-cl-std=CL1.2",NULL,NULL);char log[65536]={0};api("clGetProgramBuildInfo",clGetProgramBuildInfo(program,device,CL_PROGRAM_BUILD_LOG,sizeof(log)-1,log,NULL));fprintf(stderr,"BUILD_LOG: %s\n",log);api("clBuildProgram",error);
 cl_kernel original=clCreateKernel(program,"f02",&error);api("clCreateKernel",error);need(original!=NULL,"original exists");kernel_identity(original,"K",a,program);
 cl_mem a1=buffer(a,"A1",1),a3=buffer(a,"A3",3),o0=buffer(a,"O0",10),o1=buffer(a,"O1",11),o2=buffer(a,"O2",12);
 cl_mem live[]={a1,a3,o0,o1,o2};int distinct=1;for(unsigned x=0;x<5;x++)for(unsigned y=x+1;y<5;y++)if(live[x]==live[y])distinct=0;need(distinct,"five live buffers distinct");
 const cl_queue_properties qp[]={CL_QUEUE_PROPERTIES,0,0};cl_command_queue q=clCreateCommandQueueWithProperties(a,device,qp,&error);api("clCreateCommandQueue(A)",error);need(q!=NULL,"queue exists");
 cl_context qc=NULL;cl_device_id qd=NULL;cl_command_queue_properties flags=~(cl_command_queue_properties)0;
 api("clGetCommandQueueInfo(CL_QUEUE_CONTEXT)",clGetCommandQueueInfo(q,CL_QUEUE_CONTEXT,sizeof(qc),&qc,NULL));
 api("clGetCommandQueueInfo(CL_QUEUE_DEVICE)",clGetCommandQueueInfo(q,CL_QUEUE_DEVICE,sizeof(qd),&qd,NULL));
 api("clGetCommandQueueInfo(CL_QUEUE_PROPERTIES)",clGetCommandQueueInfo(q,CL_QUEUE_PROPERTIES,sizeof(flags),&flags,NULL));
 need(qc==a&&qd==device&&flags==0,"queue owner device and in-order properties match");(void)object("Q","queue",q,qc,0);
 set(original,0,a1,"original A1");set(original,1,o0,"original O0");
 cl_kernel clone=clCloneKernel(original,&error);api("clCloneKernel",error);need(clone!=NULL&&clone!=original,"clone distinct and live");kernel_identity(clone,"C",a,program);
 /* No setter is ever applied to clone. Inputs remain unchanged and alive. */
 dispatch(q,clone,1,"clone-first");readback(q,o0,"O0","O0:first",10,1,SALT);
 reset(q,o0);readback(q,o0,"O0","O0:reset",10,0,0);
 set(original,0,a3,"original A3");set(original,1,o1,"original O1");
 dispatch(q,clone,2,"clone-second");readback(q,o0,"O0","O0:second",10,1,SALT);
 dispatch(q,original,3,"original-final");readback(q,o1,"O1","O1:final",11,3,SALT);
 readback(q,a1,"A1","A1",1,0,0);readback(q,a3,"A3","A3",3,0,0);readback(q,o2,"O2","O2",12,0,0);
 api("clReleaseCommandQueue",clReleaseCommandQueue(q));api("clReleaseKernel(clone)",clReleaseKernel(clone));api("clReleaseKernel(original)",clReleaseKernel(original));api("clReleaseProgram",clReleaseProgram(program));
 release_mem(o2,"O2");release_mem(o1,"O1");release_mem(o0,"O0");release_mem(a3,"A3");release_mem(a1,"A1");api("clReleaseContext",clReleaseContext(a));
 puts("{\"case\":\"F02-CLONE-SNAPSHOT-REVERSE\",\"terminal\":\"observations_complete\",\"dispatches\":3,\"resets\":1,\"readbacks\":7,\"completions\":11}");return 0;
}
