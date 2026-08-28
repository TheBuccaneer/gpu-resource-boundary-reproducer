#define CL_TARGET_OPENCL_VERSION 300
#include <CL/cl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define LOOPS 20000000u

static double now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1000000.0;
}

static const char *src =
"__kernel void delayed_write(__global uint *p,\n"
"                            __global uint *sink,\n"
"                            uint loops)\n"
"{\n"
"    if (get_global_id(0) == 0) {\n"
"        uint x = sink[0] ^ 0x9e3779b9u;\n"
"        for (uint i = 0; i < loops; i++) {\n"
"            x = x * 1664525u + 1013904223u;\n"
"            x ^= x >> 13;\n"
"        }\n"
"        sink[0] = x;\n"
"        p[0] = 0xa5a5a5a5u;\n"
"    }\n"
"}\n";

int main(void)
{
    setbuf(stdout, NULL);

    cl_int err;
    cl_uint np = 0;
    cl_platform_id platform = NULL;
    cl_device_id dev = NULL;
    cl_context ctx = NULL;
    cl_command_queue q = NULL;
    cl_program prog = NULL;
    cl_kernel kernel = NULL;
    cl_mem sink = NULL;
    cl_event ev = NULL;
    uint32_t *svm = NULL;

    err = clGetPlatformIDs(0, NULL, &np);
    printf("clGetPlatformIDs=%d platforms=%u\n", err, np);
    if (err != CL_SUCCESS || !np) return 2;

    err = clGetPlatformIDs(1, &platform, NULL);
    printf("clGetPlatformIDs(select)=%d\n", err);
    if (err != CL_SUCCESS) return 3;

    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 1, &dev, NULL);
    printf("clGetDeviceIDs=%d\n", err);
    if (err != CL_SUCCESS) return 4;

    ctx = clCreateContext(NULL, 1, &dev, NULL, NULL, &err);
    printf("clCreateContext=%d\n", err);
    if (!ctx || err != CL_SUCCESS) return 5;

    const cl_queue_properties props[] = {
        CL_QUEUE_PROPERTIES,
        CL_QUEUE_PROFILING_ENABLE,
        0
    };

    q = clCreateCommandQueueWithProperties(ctx, dev, props, &err);
    printf("clCreateCommandQueueWithProperties=%d\n", err);
    if (!q || err != CL_SUCCESS) return 6;

    svm = clSVMAlloc(ctx, CL_MEM_READ_WRITE, 4096, 0);
    printf("clSVMAlloc=%s addr=%p\n",
           svm ? "success" : "failed", (void *)svm);
    if (!svm) return 7;

    err = clEnqueueSVMMap(q, CL_TRUE, CL_MAP_WRITE,
                          svm, 4096, 0, NULL, NULL);
    printf("clEnqueueSVMMap=%d\n", err);
    if (err != CL_SUCCESS) return 8;

    svm[0] = 0x11111111u;

    err = clEnqueueSVMUnmap(q, svm, 0, NULL, NULL);
    printf("clEnqueueSVMUnmap=%d\n", err);
    if (err != CL_SUCCESS) return 9;

    uint32_t sink_init = 0x12345678u;

    sink = clCreateBuffer(ctx,
                          CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
                          sizeof(sink_init),
                          &sink_init, &err);
    printf("clCreateBuffer(sink)=%d\n", err);
    if (!sink || err != CL_SUCCESS) return 10;

    prog = clCreateProgramWithSource(ctx, 1, &src, NULL, &err);
    printf("clCreateProgramWithSource=%d\n", err);
    if (!prog || err != CL_SUCCESS) return 11;

    err = clBuildProgram(prog, 1, &dev, NULL, NULL, NULL);
    printf("clBuildProgram=%d\n", err);
    if (err != CL_SUCCESS) return 12;

    kernel = clCreateKernel(prog, "delayed_write", &err);
    printf("clCreateKernel=%d\n", err);
    if (!kernel || err != CL_SUCCESS) return 13;

    err = clSetKernelArgSVMPointer(kernel, 0, svm);
    printf("clSetKernelArgSVMPointer=%d\n", err);
    if (err != CL_SUCCESS) return 14;

    err = clSetKernelArg(kernel, 1, sizeof(sink), &sink);
    printf("clSetKernelArg(sink)=%d\n", err);
    if (err != CL_SUCCESS) return 15;

    cl_uint loops = LOOPS;

    err = clSetKernelArg(kernel, 2, sizeof(loops), &loops);
    printf("clSetKernelArg(loops)=%d loops=%u\n", err, loops);
    if (err != CL_SUCCESS) return 16;

    size_t global = 1;

    double t0 = now_ms();

    err = clEnqueueNDRangeKernel(q, kernel, 1, NULL,
                                 &global, NULL,
                                 0, NULL, &ev);
    printf("clEnqueueNDRangeKernel=%d\n", err);
    if (err != CL_SUCCESS) return 17;

    err = clWaitForEvents(1, &ev);

    double t1 = now_ms();

    printf("clWaitForEvents=%d\n", err);
    printf("host_elapsed_ms=%.3f\n", t1 - t0);

    if (err != CL_SUCCESS) return 18;

    cl_ulong ts_start = 0, ts_end = 0;

    cl_int e1 = clGetEventProfilingInfo(
        ev, CL_PROFILING_COMMAND_START,
        sizeof(ts_start), &ts_start, NULL);

    cl_int e2 = clGetEventProfilingInfo(
        ev, CL_PROFILING_COMMAND_END,
        sizeof(ts_end), &ts_end, NULL);

    printf("profiling_start_err=%d\n", e1);
    printf("profiling_end_err=%d\n", e2);

    if (e1 == CL_SUCCESS && e2 == CL_SUCCESS && ts_end >= ts_start) {
        double gpu_ms = (ts_end - ts_start) / 1000000.0;
        printf("gpu_elapsed_ms=%.3f\n", gpu_ms);
    }

    err = clEnqueueSVMMap(q, CL_TRUE, CL_MAP_READ,
                          svm, 4096, 0, NULL, NULL);
    printf("clEnqueueSVMMap(read)=%d\n", err);
    if (err != CL_SUCCESS) return 19;

    printf("svm_final=0x%08x\n", svm[0]);

    err = clEnqueueSVMUnmap(q, svm, 0, NULL, NULL);
    printf("clEnqueueSVMUnmap(read)=%d\n", err);

    clFinish(q);

    clSVMFree(ctx, svm);
    clReleaseEvent(ev);
    clReleaseMemObject(sink);
    clReleaseKernel(kernel);
    clReleaseProgram(prog);
    clReleaseCommandQueue(q);
    clReleaseContext(ctx);

    printf("B4-EH3-PREP=PASS\n");
    return 0;
}
