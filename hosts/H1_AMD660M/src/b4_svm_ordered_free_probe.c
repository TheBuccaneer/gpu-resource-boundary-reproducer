#define CL_TARGET_OPENCL_VERSION 300
#include <CL/cl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define N 1024

static const char *src =
"__kernel void svm_sum(__global uint *p, __global ulong *out, uint n)\n"
"{\n"
"    if (get_global_id(0) == 0) {\n"
"        ulong s = 0;\n"
"        for (uint i = 0; i < n; i++)\n"
"            s += p[i];\n"
"        out[0] = s;\n"
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
    cl_mem out = NULL;
    uint32_t *svm = NULL;
    cl_event kernel_event = NULL;
    cl_event free_event = NULL;

    err = clGetPlatformIDs(0, NULL, &np);
    printf("clGetPlatformIDs=%d platforms=%u\n", err, np);
    if (err != CL_SUCCESS || np == 0) return 2;

    err = clGetPlatformIDs(1, &platform, NULL);
    printf("clGetPlatformIDs(select)=%d\n", err);
    if (err != CL_SUCCESS) return 3;

    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 1, &dev, NULL);
    printf("clGetDeviceIDs=%d\n", err);
    if (err != CL_SUCCESS) return 4;

    ctx = clCreateContext(NULL, 1, &dev, NULL, NULL, &err);
    printf("clCreateContext=%d\n", err);
    if (!ctx || err != CL_SUCCESS) return 5;

    q = clCreateCommandQueueWithProperties(ctx, dev, NULL, &err);
    printf("clCreateCommandQueueWithProperties=%d\n", err);
    if (!q || err != CL_SUCCESS) return 6;

    svm = clSVMAlloc(ctx, CL_MEM_READ_WRITE,
                     N * sizeof(uint32_t), 0);
    printf("clSVMAlloc=%s addr=%p\n",
           svm ? "success" : "failed", (void *)svm);
    if (!svm) return 7;

    err = clEnqueueSVMMap(q, CL_TRUE, CL_MAP_WRITE,
                          svm, N * sizeof(uint32_t),
                          0, NULL, NULL);
    printf("clEnqueueSVMMap(write)=%d\n", err);
    if (err != CL_SUCCESS) return 8;

    uint64_t expected = 0;
    for (uint32_t i = 0; i < N; i++) {
        svm[i] = i + 1;
        expected += i + 1;
    }

    printf("expected_sum=%llu\n",
           (unsigned long long)expected);

    err = clEnqueueSVMUnmap(q, svm, 0, NULL, NULL);
    printf("clEnqueueSVMUnmap(write)=%d\n", err);
    if (err != CL_SUCCESS) return 9;

    prog = clCreateProgramWithSource(ctx, 1, &src, NULL, &err);
    printf("clCreateProgramWithSource=%d\n", err);
    if (!prog || err != CL_SUCCESS) return 10;

    err = clBuildProgram(prog, 1, &dev, NULL, NULL, NULL);
    printf("clBuildProgram=%d\n", err);
    if (err != CL_SUCCESS) return 11;

    kernel = clCreateKernel(prog, "svm_sum", &err);
    printf("clCreateKernel=%d\n", err);
    if (!kernel || err != CL_SUCCESS) return 12;

    out = clCreateBuffer(ctx, CL_MEM_READ_WRITE,
                         sizeof(cl_ulong), NULL, &err);
    printf("clCreateBuffer(out)=%d\n", err);
    if (!out || err != CL_SUCCESS) return 13;

    err = clSetKernelArgSVMPointer(kernel, 0, svm);
    printf("clSetKernelArgSVMPointer=%d\n", err);
    if (err != CL_SUCCESS) return 14;

    err = clSetKernelArg(kernel, 1, sizeof(out), &out);
    printf("clSetKernelArg(out)=%d\n", err);
    if (err != CL_SUCCESS) return 15;

    cl_uint n_arg = N;
    err = clSetKernelArg(kernel, 2, sizeof(n_arg), &n_arg);
    printf("clSetKernelArg(n)=%d\n", err);
    if (err != CL_SUCCESS) return 16;

    size_t global = 1;

    err = clEnqueueNDRangeKernel(q, kernel, 1, NULL,
                                 &global, NULL,
                                 0, NULL, &kernel_event);
    printf("clEnqueueNDRangeKernel=%d event=%s\n",
           err, kernel_event ? "yes" : "no");
    if (err != CL_SUCCESS) return 17;

    void *to_free[1] = { svm };

    err = clEnqueueSVMFree(q, 1, to_free,
                           NULL, NULL,
                           1, &kernel_event,
                           &free_event);
    printf("clEnqueueSVMFree=%d event=%s\n",
           err, free_event ? "yes" : "no");
    if (err != CL_SUCCESS) return 18;

    /*
     * From this point on, the host never accesses svm again.
     * The free command is ordered after kernel_event.
     */

    err = clWaitForEvents(1, &free_event);
    printf("clWaitForEvents(free)=%d\n", err);
    if (err != CL_SUCCESS) return 19;

    cl_int free_status = 0;
    err = clGetEventInfo(free_event,
                         CL_EVENT_COMMAND_EXECUTION_STATUS,
                         sizeof(free_status),
                         &free_status, NULL);
    printf("free_event_status=%d query_err=%d\n",
           free_status, err);
    if (err != CL_SUCCESS) return 20;

    cl_ulong actual = 0;

    err = clEnqueueReadBuffer(q, out, CL_TRUE,
                              0, sizeof(actual), &actual,
                              0, NULL, NULL);
    printf("clEnqueueReadBuffer(out)=%d\n", err);
    if (err != CL_SUCCESS) return 21;

    printf("actual_sum=%llu\n",
           (unsigned long long)actual);

    /*
     * Test whether a fresh valid allocation may reuse the now-freed VA.
     * Reuse or non-reuse are both valid.
     */
    void *fresh = clSVMAlloc(ctx, CL_MEM_READ_WRITE,
                             N * sizeof(uint32_t), 0);

    printf("fresh_alloc=%s addr=%p old_addr=%p same_addr=%d\n",
           fresh ? "success" : "failed",
           fresh, (void *)svm,
           fresh && fresh == (void *)svm);

    if (!fresh) return 22;

    clSVMFree(ctx, fresh);

    clReleaseEvent(free_event);
    clReleaseEvent(kernel_event);
    clReleaseMemObject(out);
    clReleaseKernel(kernel);
    clReleaseProgram(prog);
    clReleaseCommandQueue(q);
    clReleaseContext(ctx);

    if ((uint64_t)actual != expected) {
        printf("B4-C3=FAIL\n");
        return 23;
    }

    printf("B4-C3=PASS\n");
    return 0;
}
