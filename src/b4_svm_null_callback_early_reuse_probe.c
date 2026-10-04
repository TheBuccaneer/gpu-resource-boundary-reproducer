#define CL_TARGET_OPENCL_VERSION 300
#include <CL/cl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sched.h>

#define BYTES 4096
#define N (BYTES / sizeof(uint32_t))
#define LOOPS 20000000u

#define A_INIT   0x11111111u
#define OLD_WRITE 0xa5a5a5a5u
#define B0_CANARY 0xbbbbbbbbu
#define B1_CANARY 0xccccccccu

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

    cl_command_queue q1 = NULL;
    cl_command_queue q2 = NULL;

    cl_program prog = NULL;
    cl_kernel kernel = NULL;
    cl_mem sink = NULL;

    cl_event kernel_event = NULL;
    cl_event free_event = NULL;

    uint32_t *a = NULL;
    uint32_t *b = NULL;

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

    q1 = clCreateCommandQueueWithProperties(ctx, dev, NULL, &err);
    printf("clCreateCommandQueue(q1)=%d\n", err);
    if (!q1 || err != CL_SUCCESS) return 6;

    q2 = clCreateCommandQueueWithProperties(ctx, dev, NULL, &err);
    printf("clCreateCommandQueue(q2)=%d\n", err);
    if (!q2 || err != CL_SUCCESS) return 7;

    /*
     * Old lifetime A.
     */
    a = clSVMAlloc(ctx, CL_MEM_READ_WRITE, BYTES, 0);

    printf("alloc_A=%s addr=%p\n",
           a ? "success" : "failed", (void *)a);

    if (!a) return 8;

    err = clEnqueueSVMMap(
        q1, CL_TRUE, CL_MAP_WRITE,
        a, BYTES, 0, NULL, NULL);

    printf("map_A=%d\n", err);
    if (err != CL_SUCCESS) return 9;

    for (size_t i = 0; i < N; i++)
        a[i] = A_INIT;

    err = clEnqueueSVMUnmap(q1, a, 0, NULL, NULL);
    printf("unmap_A=%d\n", err);
    if (err != CL_SUCCESS) return 10;

    err = clFinish(q1);
    printf("finish_A_init=%d\n", err);
    if (err != CL_SUCCESS) return 11;

    uint32_t sink_init = 0x12345678u;

    sink = clCreateBuffer(
        ctx,
        CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
        sizeof(sink_init),
        &sink_init,
        &err);

    printf("clCreateBuffer(sink)=%d\n", err);
    if (!sink || err != CL_SUCCESS) return 12;

    prog = clCreateProgramWithSource(ctx, 1, &src, NULL, &err);
    printf("clCreateProgramWithSource=%d\n", err);
    if (!prog || err != CL_SUCCESS) return 13;

    err = clBuildProgram(prog, 1, &dev, NULL, NULL, NULL);
    printf("clBuildProgram=%d\n", err);
    if (err != CL_SUCCESS) return 14;

    kernel = clCreateKernel(prog, "delayed_write", &err);
    printf("clCreateKernel=%d\n", err);
    if (!kernel || err != CL_SUCCESS) return 15;

    err = clSetKernelArgSVMPointer(kernel, 0, a);
    printf("set_arg_A=%d\n", err);
    if (err != CL_SUCCESS) return 16;

    err = clSetKernelArg(kernel, 1, sizeof(sink), &sink);
    printf("set_arg_sink=%d\n", err);
    if (err != CL_SUCCESS) return 17;

    cl_uint loops = LOOPS;

    err = clSetKernelArg(kernel, 2, sizeof(loops), &loops);
    printf("set_arg_loops=%d loops=%u\n", err, loops);
    if (err != CL_SUCCESS) return 18;

    size_t global = 1;

    double t_kernel_enqueue = now_ms();

    err = clEnqueueNDRangeKernel(
        q1, kernel, 1, NULL,
        &global, NULL,
        0, NULL, &kernel_event);

    printf("enqueue_kernel=%d\n", err);
    if (err != CL_SUCCESS) return 19;

    void *to_free[1] = { a };

    err = clEnqueueSVMFree(
        q1,
        1,
        to_free,
        NULL,
        NULL,
        1,
        &kernel_event,
        &free_event);

    printf("enqueue_free_A=%d\n", err);
    if (err != CL_SUCCESS) return 20;

    /*
     * Important: flush but DO NOT wait.
     * We poll the free event so that clWaitForEvents cannot
     * accidentally introduce stronger synchronization.
     */
    err = clFlush(q1);
    printf("clFlush(q1)=%d\n", err);
    if (err != CL_SUCCESS) return 21;

    /*
     * B4-C3e:
     * Do NOT wait for the SVM-free event to complete.
     *
     * We only wait until Rusticl reports that the free command has reached
     * CL_SUBMITTED (or a later state), then inspect the dependency status.
     *
     * Correct lifetime ordering must still prevent reuse of A while the
     * dependent kernel is active.  The host never accesses A again.
     */
    cl_int free_status = CL_QUEUED;
    cl_int kernel_status_at_free_gate = -999;

    double poll_start = now_ms();

    for (;;) {
        err = clGetEventInfo(
            free_event,
            CL_EVENT_COMMAND_EXECUTION_STATUS,
            sizeof(free_status),
            &free_status,
            NULL);

        if (err != CL_SUCCESS) {
            printf("free_event_query_err=%d\n", err);
            return 22;
        }

        if (free_status <= CL_SUBMITTED)
            break;

        if (now_ms() - poll_start > 2000.0) {
            printf("free_event_submission_poll_timeout=1\n");
            return 23;
        }

        sched_yield();
    }

    double t_free_gate = now_ms();

    err = clGetEventInfo(
        kernel_event,
        CL_EVENT_COMMAND_EXECUTION_STATUS,
        sizeof(kernel_status_at_free_gate),
        &kernel_status_at_free_gate,
        NULL);

    printf("probe_variant=NULL_CALLBACK_SUBMITTED_GATE\n");
    printf("free_event_status_at_gate=%d\n", free_status);
    printf("kernel_status_at_free_gate=%d query_err=%d\n",
           kernel_status_at_free_gate, err);
    printf("free_gate_after_kernel_enqueue_ms=%.3f\n",
           t_free_gate - t_kernel_enqueue);

    /*
     * Never access A again.
     *
     * Request an independent new SVM allocation B.  A correct implementation
     * must not make A reusable while its dependent kernel still requires its
     * lifetime.  Same-address reuse while that kernel is active is therefore
     * the condition of interest.
     *
     * Start a completely new lifetime B.
     */
    b = clSVMAlloc(ctx, CL_MEM_READ_WRITE, BYTES, 0);

    printf("alloc_B=%s addr=%p same_addr=%d\n",
           b ? "success" : "failed",
           (void *)b,
           b && ((void *)b == (void *)a));

    if (!b) return 24;

    err = clEnqueueSVMMap(
        q2, CL_TRUE, CL_MAP_WRITE,
        b, BYTES, 0, NULL, NULL);

    printf("map_B_write=%d\n", err);
    if (err != CL_SUCCESS) return 25;

    for (size_t i = 0; i < N; i++)
        b[i] = 0xddddddddu;

    b[0] = B0_CANARY;
    b[1] = B1_CANARY;

    err = clEnqueueSVMUnmap(q2, b, 0, NULL, NULL);
    printf("unmap_B_write=%d\n", err);
    if (err != CL_SUCCESS) return 26;

    err = clFinish(q2);
    printf("finish_B_init=%d\n", err);
    if (err != CL_SUCCESS) return 27;

    cl_int kernel_status_after_B_init = -999;

    err = clGetEventInfo(
        kernel_event,
        CL_EVENT_COMMAND_EXECUTION_STATUS,
        sizeof(kernel_status_after_B_init),
        &kernel_status_after_B_init,
        NULL);

    printf("kernel_status_after_B_init=%d query_err=%d\n",
           kernel_status_after_B_init, err);

    double t_B_init_done = now_ms();

    printf("B_init_after_kernel_enqueue_ms=%.3f\n",
           t_B_init_done - t_kernel_enqueue);

    /*
     * Now wait for the old kernel for real.
     */
    err = clWaitForEvents(1, &kernel_event);
    printf("wait_old_kernel=%d\n", err);
    if (err != CL_SUCCESS) return 28;

    /*
     * The overlap observation is finished.  Complete the free command now
     * so cleanup and final readback do not race command completion.
     */
    err = clWaitForEvents(1, &free_event);
    printf("wait_free_event=%d\n", err);
    if (err != CL_SUCCESS) return 33;

    double t_kernel_done = now_ms();

    printf("kernel_done_after_enqueue_ms=%.3f\n",
           t_kernel_done - t_kernel_enqueue);

    /*
     * Read only the NEW allocation B.
     */
    err = clEnqueueSVMMap(
        q2, CL_TRUE, CL_MAP_READ,
        b, BYTES, 0, NULL, NULL);

    printf("map_B_read=%d\n", err);
    if (err != CL_SUCCESS) return 29;

    uint32_t b0 = b[0];
    uint32_t b1 = b[1];

    printf("B0_after_old_kernel=0x%08x\n", b0);
    printf("B1_after_old_kernel=0x%08x\n", b1);

    err = clEnqueueSVMUnmap(q2, b, 0, NULL, NULL);
    printf("unmap_B_read=%d\n", err);
    if (err != CL_SUCCESS) return 30;

    clFinish(q2);

    /*
     * Free only the new lifetime B.
     * A was already freed by clEnqueueSVMFree.
     */
    clSVMFree(ctx, b);
    b = NULL;

    clReleaseEvent(free_event);
    clReleaseEvent(kernel_event);
    clReleaseMemObject(sink);
    clReleaseKernel(kernel);
    clReleaseProgram(prog);
    clReleaseCommandQueue(q2);
    clReleaseCommandQueue(q1);
    clReleaseContext(ctx);

    if (b0 == OLD_WRITE && b1 == B1_CANARY) {
        printf("B4-C3e=RUNTIME_FREE_INTEGRITY_INTERESTING\n");
        return 31;
    }

    if (b0 != B0_CANARY || b1 != B1_CANARY) {
        printf("B4-C3e=UNEXPECTED_CORRUPTION\n");
        return 32;
    }

    printf("B4-C3e=NO_CORRUPTION_OBSERVED\n");
    return 0;
}
