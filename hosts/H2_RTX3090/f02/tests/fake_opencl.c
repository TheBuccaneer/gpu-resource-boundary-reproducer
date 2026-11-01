/* CPU-only fake runtime. Linked statically with F02, NEVER with libOpenCL. */
#define _POSIX_C_SOURCE 200809L
#define CL_TARGET_OPENCL_VERSION 300
#include <CL/cl.h>
#include <CL/cl_ext.h>
#include <stdint.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
struct _cl_platform_id{int unused;};struct _cl_device_id{int unused;};
struct _cl_context{int alive;};struct _cl_program{cl_context ctx;};
struct _cl_mem{cl_context ctx;size_t bytes;cl_uint *words;int alive;};
struct _cl_kernel{cl_context ctx;cl_mem args[2];cl_program program;int clone;};struct _cl_command_queue{cl_context ctx;};struct _cl_event{cl_int state;cl_command_queue queue;cl_command_type type;};
static struct _cl_platform_id platform;static struct _cl_device_id device;
static cl_mem buffers[8];static unsigned nbuf,dispatches;static cl_kernel clone_kernel;
static const char *scenario(void){const char *s=getenv("F02_CPU_SCENARIO");return s?s:"valid";}
static int is(const char *s){return !strcmp(scenario(),s);}
static void mark(const char *s){const char *p=getenv("F02_CPU_MARKER");
 if(p){FILE *f=fopen(p,"a");
 if(f){fprintf(f,"%s\n",s);fclose(f);}}}
static cl_int value(size_t size,void *out,size_t *length,const void *input,size_t bytes){if(length)*length=bytes;
 if(out){if(size<bytes)return CL_INVALID_VALUE;
 memcpy(out,input,bytes);}return CL_SUCCESS;}
cl_int clGetPlatformIDs(cl_uint n,cl_platform_id *ids,cl_uint *count){if(count)*count=1;
 if(ids){if(n<1)return CL_INVALID_VALUE;ids[0]=&platform;}return 0;}
cl_int clGetDeviceIDs(cl_platform_id p,cl_device_type type,cl_uint n,cl_device_id *ids,cl_uint *count){(void)p;
 if(type!=CL_DEVICE_TYPE_GPU)return CL_DEVICE_NOT_FOUND;
 if(count)*count=1;
 if(ids){if(n<1)return CL_INVALID_VALUE;ids[0]=&device;}return 0;}
cl_int clGetDeviceInfo(cl_device_id d,cl_device_info key,size_t size,void *out,size_t *length){
 (void)d;cl_uint vendor=0x10de;cl_device_pci_bus_info_khr pci={0,1,0,0};
 if(key==CL_DEVICE_NUMERIC_VERSION){cl_version v=CL_MAKE_VERSION(3,0,0);return value(size,out,length,&v,sizeof(v));}
 if(key==CL_DEVICE_VENDOR_ID)return value(size,out,length,&vendor,sizeof(vendor));
 if(key==CL_DEVICE_NAME){const char text[]="NVIDIA GeForce RTX 3090";return value(size,out,length,text,sizeof(text));}
 if(key==CL_DRIVER_VERSION){const char text[]="580.178.04";return value(size,out,length,text,sizeof(text));}
 if(key==CL_DEVICE_PCI_BUS_INFO_KHR)return value(size,out,length,&pci,sizeof(pci));
 return CL_INVALID_VALUE;
}
cl_context clCreateContext(const cl_context_properties *props,cl_uint n,const cl_device_id *ds,void (CL_CALLBACK *notify)(const char *,const void *,size_t,void *),void *user,cl_int *error){(void)props;(void)ds;(void)notify;(void)user;
 if(n!=1){*error=CL_INVALID_VALUE;return NULL;}cl_context c=calloc(1,sizeof(*c));
 if(!c){*error=CL_OUT_OF_HOST_MEMORY;return NULL;}c->alive=1;*error=0;return c;}
cl_program clCreateProgramWithSource(cl_context c,cl_uint n,const char **strings,const size_t *length,cl_int *error){(void)length;
 if(!c||!c->alive||n!=1||!strstr(strings[0],"__kernel void f02")){*error=CL_INVALID_VALUE;return NULL;}cl_program p=malloc(sizeof(*p));
 if(!p){*error=CL_OUT_OF_HOST_MEMORY;return NULL;}p->ctx=c;*error=0;return p;}
cl_int clBuildProgram(cl_program p,cl_uint n,const cl_device_id *ds,const char *options,void (CL_CALLBACK *notify)(cl_program,void *),void *user){(void)p;(void)ds;(void)notify;(void)user;return n==1&&!strcmp(options,"-cl-std=CL1.2")?0:CL_INVALID_VALUE;}
cl_int clGetProgramBuildInfo(cl_program p,cl_device_id d,cl_program_build_info key,size_t size,void *out,size_t *length){(void)p;(void)d;
 if(key!=CL_PROGRAM_BUILD_LOG)return CL_INVALID_VALUE;
 const char empty[]="";return value(size,out,length,empty,sizeof(empty));}
cl_kernel clCreateKernel(cl_program p,const char *name,cl_int *error){if(strcmp(name,"f02")){*error=CL_INVALID_KERNEL_NAME;return NULL;}cl_kernel k=calloc(1,sizeof(*k));
 if(!k){*error=CL_OUT_OF_HOST_MEMORY;return NULL;}k->ctx=p->ctx;k->program=p;*error=0;return k;}
cl_int clGetKernelInfo(cl_kernel k,cl_kernel_info key,size_t size,void *out,size_t *length){
 if(key==CL_KERNEL_CONTEXT)return value(size,out,length,&k->ctx,sizeof(k->ctx));
 if(key==CL_KERNEL_PROGRAM)return value(size,out,length,&k->program,sizeof(k->program));
 return CL_INVALID_VALUE;
}
cl_kernel clCloneKernel(cl_kernel k,cl_int *error){
 mark("clone");if(is("clone_error")){*error=CL_OUT_OF_RESOURCES;return NULL;}
 cl_kernel c=malloc(sizeof(*c));if(!c){*error=CL_OUT_OF_HOST_MEMORY;return NULL;}
 *c=*k;c->clone=1;clone_kernel=c;
 if(is("wrong_output"))c->args[1]=buffers[4];
 if(is("wrong_input"))c->args[0]=buffers[1];
 *error=0;return c;
}
cl_mem clCreateBuffer(cl_context c,cl_mem_flags flags,size_t size,void *host,cl_int *error){if(flags!=(CL_MEM_READ_WRITE|CL_MEM_COPY_HOST_PTR)||size!=640||!host||nbuf>=8){*error=CL_INVALID_VALUE;return NULL;}cl_mem m=calloc(1,sizeof(*m));
 if(!m){*error=CL_OUT_OF_HOST_MEMORY;return NULL;}m->words=malloc(size);
 if(!m->words){free(m);*error=CL_OUT_OF_HOST_MEMORY;return NULL;}m->ctx=c;m->bytes=size;m->alive=1;memcpy(m->words,host,size);buffers[nbuf++]=m;*error=0;return m;}
cl_int clGetMemObjectInfo(cl_mem m,cl_mem_info key,size_t size,void *out,size_t *length){
 if(!m||!m->alive)return CL_INVALID_MEM_OBJECT;
 if(key==CL_MEM_CONTEXT)return value(size,out,length,&m->ctx,sizeof(m->ctx));
 if(key==CL_MEM_SIZE)return value(size,out,length,&m->bytes,sizeof(m->bytes));
 return CL_INVALID_VALUE;
}
cl_int clSetKernelArg(cl_kernel k,cl_uint index,size_t size,const void *arg){
 if(index>=2)return CL_INVALID_ARG_INDEX;
 if(size!=sizeof(cl_mem))return CL_INVALID_ARG_SIZE;
 if(!arg)return CL_INVALID_ARG_VALUE;
 cl_mem mem;memcpy(&mem,arg,sizeof(mem));
 if(!mem||!mem->alive||mem->ctx!=k->ctx)return CL_INVALID_MEM_OBJECT;
 k->args[index]=mem;
 if(is("coupled")&&clone_kernel&&!k->clone)clone_kernel->args[index]=mem;
 return 0;
}
cl_command_queue clCreateCommandQueueWithProperties(cl_context c,cl_device_id d,const cl_queue_properties *props,cl_int *error){(void)d;(void)props;mark("queue");cl_command_queue q=malloc(sizeof(*q));
 if(!q){*error=CL_OUT_OF_HOST_MEMORY;return NULL;}q->ctx=c;*error=0;return q;}
cl_int clGetCommandQueueInfo(cl_command_queue q,cl_command_queue_info key,size_t size,void *out,size_t *length){
 if(key==CL_QUEUE_CONTEXT)return value(size,out,length,&q->ctx,sizeof(q->ctx));
 if(key==CL_QUEUE_DEVICE){cl_device_id d=&device;return value(size,out,length,&d,sizeof(d));}
 if(key==CL_QUEUE_PROPERTIES){cl_command_queue_properties flags=0;return value(size,out,length,&flags,sizeof(flags));}
 return CL_INVALID_VALUE;
}
static cl_int event(cl_event *out,cl_int state,cl_command_queue q,cl_command_type type){if(!out)return CL_INVALID_VALUE;*out=malloc(sizeof(**out));
 if(!*out)return CL_OUT_OF_HOST_MEMORY;
 (*out)->state=state;(*out)->queue=q;(*out)->type=type;return 0;}
cl_int clEnqueueNDRangeKernel(cl_command_queue q,cl_kernel k,cl_uint dim,const size_t *offset,const size_t *global,const size_t *local,cl_uint waits,const cl_event *list,cl_event *out){
 (void)list;mark("dispatch");dispatches++;if(is("hang")){signal(SIGTERM,SIG_IGN);for(;;)pause();}
 if(dispatches>3||dim!=1||offset||*global!=64||*local!=1||waits||q->ctx!=k->ctx)return CL_INVALID_VALUE;
 for(unsigned n=0;n<2;n++)if(!k->args[n]||!k->args[n]->alive)return CL_INVALID_KERNEL_ARGS;
 if(is("dispatch_error"))return CL_OUT_OF_RESOURCES;
 if(!(is("stale_after_reset")&&dispatches==2)){
  for(unsigned i=16;i<80;i++)k->args[1]->words[i]=k->args[0]->words[i]^0xceae4507u;
 }
 if(is("guard_corrupt"))k->args[1]->words[159]^=1u;
 if(is("input_corrupt"))buffers[0]->words[159]^=1u;
 if(is("sentinel_corrupt"))buffers[4]->words[159]^=1u;
 return event(out,is("bad_completion")?-1:CL_COMPLETE,q,CL_COMMAND_NDRANGE_KERNEL);
}
cl_int clEnqueueWriteBuffer(cl_command_queue q,cl_mem m,cl_bool blocking,size_t offset,size_t size,const void *host,cl_uint waits,const cl_event *list,cl_event *out){
 (void)list;mark("reset");if(q->ctx!=m->ctx||!m->alive||blocking!=CL_FALSE||offset||size!=640||waits)return CL_INVALID_VALUE;
 if(!is("reset_noop"))memcpy(m->words,host,size);
 return event(out,CL_COMPLETE,q,CL_COMMAND_WRITE_BUFFER);
}
cl_int clEnqueueReadBuffer(cl_command_queue q,cl_mem m,cl_bool blocking,size_t offset,size_t size,void *host,cl_uint waits,const cl_event *list,cl_event *out){(void)list;mark("read");
 if(q->ctx!=m->ctx||!m->alive||blocking!=CL_FALSE||offset||size!=640||waits)return CL_INVALID_VALUE;
 memcpy(host,m->words,size);return event(out,CL_COMPLETE,q,CL_COMMAND_READ_BUFFER);}
cl_int clFlush(cl_command_queue q){(void)q;return 0;}
cl_int clGetEventInfo(cl_event e,cl_event_info key,size_t size,void *out,size_t *length){
 if(key==CL_EVENT_COMMAND_EXECUTION_STATUS)return value(size,out,length,&e->state,sizeof(e->state));
 if(key==CL_EVENT_COMMAND_QUEUE)return value(size,out,length,&e->queue,sizeof(e->queue));
 if(key==CL_EVENT_CONTEXT)return value(size,out,length,&e->queue->ctx,sizeof(e->queue->ctx));
 if(key==CL_EVENT_COMMAND_TYPE)return value(size,out,length,&e->type,sizeof(e->type));
 return CL_INVALID_VALUE;
}
cl_int clWaitForEvents(cl_uint n,const cl_event *es){return n==1&&es[0]->state==CL_COMPLETE?0:CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST;}
cl_int clReleaseEvent(cl_event e){free(e);return 0;}
cl_int clReleaseCommandQueue(cl_command_queue q){free(q);return 0;}
cl_int clReleaseKernel(cl_kernel k){free(k);return 0;}
cl_int clReleaseProgram(cl_program p){free(p);return 0;}
cl_int clReleaseMemObject(cl_mem m){if(!m->alive)return CL_INVALID_MEM_OBJECT;m->alive=0;free(m->words);free(m);return 0;}
cl_int clReleaseContext(cl_context c){c->alive=0;free(c);return 0;}
