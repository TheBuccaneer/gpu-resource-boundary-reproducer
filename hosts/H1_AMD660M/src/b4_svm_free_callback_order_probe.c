#define CL_TARGET_OPENCL_VERSION 300
#include <CL/cl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>

#define N 1024

struct callback_state {
    cl_event kernel_event;
    atomic_int called;
    atomic_int query_err;
    atomic_int observed_status;
};

static void CL_CALLBACK timing_callback(
    cl_command_queue queue,
    cl_uint num_svm_pointers,
    void *svm_pointers[],
    void *user_data)
{
    (void)queue;
    (void)num_svm_pointers;
    (void)svm_pointers;

    struct callback_state *s = user_data;

    cl_int status = -999;
    cl_int err = clGetEventInfo(
        s->kernel_event,
        CL_EVENT_COMMAND_EXECUTION_STATUS,
        sizeof(status),
        &status,
        NULL);

    atomic_store(&s->query_err, err);
    atomic_store(&s->observed_status, status);
    atomic_store(&s->called, 1);

    /*
     * Deliberately DO NOT free here.
     *
     * This callback is only an ordering oracle.
     * The allocation will be freed by the host after
     * the kernel has definitely completed.
     */
}

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

    cl_event kernel_event = NULL;
    cl_event free_event = NULL;

    uint32_t *svm = NULL;

    struct callback_state state;
    atomic_init(&state.called, 0);
    atomic_init(&state.query_err, -999);
    atomic_init(&state.observed_status, -999);
    state.kernel_event = NULL;

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

    q = clCreateCommandQueueWithProperties(ctx, dev, NULL, &err);
    printf("clCreateCommandQueueWithProperties=%d\n", err);
    if (!q || err != CL_SUCCESS) return 6;

    svm = clSVMAlloc(ctx, CL_MEM_READ_WRITE,
                     N * sizeof(uint32_t), 0);

    printf("clSVMAlloc=%s addr=%p\n",
           svm ? "success" : "failed", (void *)svm);

    if (!svm) return 7;

    err = clEnqueueSVMMap(
        q, CL_TRUE, CL_MAP_WRITE,
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
    printf("clEnqueueSVMUnmap=%d\n", err);
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

    err = clSetKernelArg(kernel, 2,
                         sizeof(n_arg), &n_arg);

    printf("clSetKernelArg(n)=%d\n", err);
    if (err != CL_SUCCESS) return 16;

    size_t global = 1;

    err = clEnqueueNDRangeKernel(
        q, kernel, 1, NULL,
        &global, NULL,
        0, NULL, &kernel_event);

    printf("clEnqueueNDRangeKernel=%d event=%s\n",
           err, kernel_event ? "yes" : "no");

    if (err != CL_SUCCESS) return 17;

    state.kernel_event = kernel_event;

    void *ptrs[1] = { svm };

    err = clEnqueueSVMFree(
        q,
        1,
        ptrs,
        timing_callback,
        &state,
        1,
        &kernel_event,
        &free_event);

    printf("clEnqueueSVMFree(callback)=%d event=%s\n",
           err, free_event ? "yes" : "no");

    if (err != CL_SUCCESS) return 18;

    err = clWaitForEvents(1, &free_event);
    printf("clWaitForEvents(free_event)=%d\n", err);
    if (err != CL_SUCCESS) return 19;

    printf("callback_called=%d\n",
           atomic_load(&state.called));

    printf("callback_query_err=%d\n",
           atomic_load(&state.query_err));

    printf("callback_kernel_status=%d\n",
           atomic_load(&state.observed_status));

    cl_int kernel_status_after_free = -999;

    err = clGetEventInfo(
        kernel_event,
        CL_EVENT_COMMAND_EXECUTION_STATUS,
        sizeof(kernel_status_after_free),
        &kernel_status_after_free,
        NULL);

    printf("kernel_status_after_free_event=%d query_err=%d\n",
           kernel_status_after_free, err);

    /*
     * Now establish real completion before touching/freeing SVM.
     */
    err = clWaitForEvents(1, &kernel_event);

    printf("clWaitForEvents(kernel_event)=%d\n", err);
    if (err != CL_SUCCESS) return 20;

    cl_int kernel_status_final = -999;

    err = clGetEventInfo(
        kernel_event,
        CL_EVENT_COMMAND_EXECUTION_STATUS,
        sizeof(kernel_status_final),
        &kernel_status_final,
        NULL);

    printf("kernel_status_final=%d query_err=%d\n",
           kernel_status_final, err);

    cl_ulong actual = 0;

    err = clEnqueueReadBuffer(
        q, out, CL_TRUE,
        0, sizeof(actual), &actual,
        0, NULL, NULL);

    printf("clEnqueueReadBuffer=%d\n", err);
    printf("actual_sum=%llu\n",
           (unsigned long long)actual);

    /*
     * Callback did not free anything.
     * Free only now, after confirmed kernel completion.
     */
    clSVMFree(ctx, svm);
    printf("clSVMFree(final)=done\n");

    clReleaseEvent(free_event);
    clReleaseEvent(kernel_event);
    clReleaseMemObject(out);
    clReleaseKernel(kernel);
    clReleaseProgram(prog);
    clReleaseCommandQueue(q);
    clReleaseContext(ctx);

    if ((uint64_t)actual != expected) {
        printf("B4-C3b=DATA_FAIL\n");
        return 21;
    }

    if (!atomic_load(&state.called)) {
        printf("B4-C3b=CALLBACK_FAIL\n");
        return 22;
    }

    if (atomic_load(&state.observed_status) != CL_COMPLETE) {
        printf("B4-C3b=ORDERING_INTERESTING\n");
        return 23;
    }

    printf("B4-C3b=PASS\n");
    return 0;
}
