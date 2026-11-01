#define CL_TARGET_OPENCL_VERSION 300
#include <CL/cl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdatomic.h>
#include <time.h>
#include <sched.h>

#define BYTES       4096
#define N           (BYTES / sizeof(uint32_t))
#define LOOPS       20000000u

#define A_INIT      0x11111111u
#define OLD_WRITE   0xa5a5a5a5u
#define B0_CANARY   0xbbbbbbbbu
#define B1_CANARY   0xccccccccu
#define B1_GPUWRITE 0xeeeeeeeeu

struct cb_state {
    cl_context ctx;
    cl_event old_event;
    atomic_int called;
    atomic_int old_status;
    atomic_int query_err;
    atomic_int free_done;
};

static double now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1000000.0;
}

static void CL_CALLBACK free_callback(
    cl_command_queue queue,
    cl_uint num,
    void *ptrs[],
    void *user_data)
{
    (void)queue;

    struct cb_state *s = user_data;

    cl_int status = -999;
    cl_int err = clGetEventInfo(
        s->old_event,
        CL_EVENT_COMMAND_EXECUTION_STATUS,
        sizeof(status), &status, NULL);

    atomic_store(&s->query_err, err);
    atomic_store(&s->old_status, status);
    atomic_store(&s->called, 1);

    for (cl_uint i = 0; i < num; i++)
        clSVMFree(s->ctx, ptrs[i]);

    atomic_store(&s->free_done, 1);
}

static const char *src =
"__kernel void old_delayed(__global uint *p,\n"
"                          __global uint *sink,\n"
"                          uint loops)\n"
"{\n"
"  if (get_global_id(0) == 0) {\n"
"    uint x = sink[0] ^ 0x9e3779b9u;\n"
"    for (uint i = 0; i < loops; i++) {\n"
"      x = x * 1664525u + 1013904223u;\n"
"      x ^= x >> 13;\n"
"    }\n"
"    sink[0] = x;\n"
"    p[0] = 0xa5a5a5a5u;\n"
"  }\n"
"}\n"
"\n"
"__kernel void probe_new(__global uint *p,\n"
"                        __global uint *out)\n"
"{\n"
"  if (get_global_id(0) == 0) {\n"
"    out[0] = p[0];\n"
"    p[1] = 0xeeeeeeeeu;\n"
"  }\n"
"}\n";

int main(void)
{
    setbuf(stdout, NULL);

    cl_int err;
    cl_uint np = 0;
    cl_platform_id platform = NULL;
    cl_device_id dev = NULL;
    cl_context ctx = NULL;
    cl_command_queue q1 = NULL, q2 = NULL;
    cl_program prog = NULL;
    cl_kernel old_kernel = NULL, new_kernel = NULL;
    cl_mem sink = NULL, out = NULL;
    cl_event old_ev = NULL, free_ev = NULL, new_ev = NULL;

    uint32_t *a = NULL, *b = NULL;
    uintptr_t a_addr = 0;

    struct cb_state state;
    atomic_init(&state.called, 0);
    atomic_init(&state.old_status, -999);
    atomic_init(&state.query_err, -999);
    atomic_init(&state.free_done, 0);

    err = clGetPlatformIDs(0, NULL, &np);
    printf("clGetPlatformIDs=%d platforms=%u\n", err, np);
    if (err || !np) return 2;

    err = clGetPlatformIDs(1, &platform, NULL);
    printf("clGetPlatformIDs(select)=%d\n", err);
    if (err) return 3;

    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 1, &dev, NULL);
    printf("clGetDeviceIDs=%d\n", err);
    if (err) return 4;

    ctx = clCreateContext(NULL, 1, &dev, NULL, NULL, &err);
    printf("clCreateContext=%d\n", err);
    if (!ctx || err) return 5;

    const cl_queue_properties props[] = {
        CL_QUEUE_PROPERTIES, CL_QUEUE_PROFILING_ENABLE, 0
    };

    q1 = clCreateCommandQueueWithProperties(ctx, dev, props, &err);
    printf("create_q1=%d\n", err);
    if (!q1 || err) return 6;

    q2 = clCreateCommandQueueWithProperties(ctx, dev, props, &err);
    printf("create_q2=%d\n", err);
    if (!q2 || err) return 7;

    prog = clCreateProgramWithSource(ctx, 1, &src, NULL, &err);
    printf("create_program=%d\n", err);
    if (!prog || err) return 8;

    err = clBuildProgram(prog, 1, &dev, NULL, NULL, NULL);
    printf("build_program=%d\n", err);
    if (err) return 9;

    old_kernel = clCreateKernel(prog, "old_delayed", &err);
    printf("create_old_kernel=%d\n", err);
    if (!old_kernel || err) return 10;

    new_kernel = clCreateKernel(prog, "probe_new", &err);
    printf("create_new_kernel=%d\n", err);
    if (!new_kernel || err) return 11;

    uint32_t sink_init = 0x12345678u;
    sink = clCreateBuffer(ctx,
                         CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
                         sizeof(sink_init), &sink_init, &err);
    printf("create_sink=%d\n", err);
    if (!sink || err) return 12;

    uint32_t out_init = 0;
    out = clCreateBuffer(ctx,
                        CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
                        sizeof(out_init), &out_init, &err);
    printf("create_out=%d\n", err);
    if (!out || err) return 13;

    /* ---------- Lifetime A ---------- */

    a = clSVMAlloc(ctx, CL_MEM_READ_WRITE, BYTES, 0);
    a_addr = (uintptr_t)a;

    printf("alloc_A=%s addr=0x%llx\n",
           a ? "success" : "failed",
           (unsigned long long)a_addr);
    if (!a) return 14;

    err = clEnqueueSVMMap(q1, CL_TRUE, CL_MAP_WRITE,
                          a, BYTES, 0, NULL, NULL);
    printf("map_A=%d\n", err);
    if (err) return 15;

    for (size_t i = 0; i < N; i++)
        a[i] = A_INIT;

    err = clEnqueueSVMUnmap(q1, a, 0, NULL, NULL);
    printf("unmap_A=%d\n", err);
    if (err) return 16;

    err = clFinish(q1);
    printf("finish_A_init=%d\n", err);
    if (err) return 17;

    err = clSetKernelArgSVMPointer(old_kernel, 0, a);
    printf("set_old_A=%d\n", err);
    if (err) return 18;

    err = clSetKernelArg(old_kernel, 1, sizeof(sink), &sink);
    printf("set_old_sink=%d\n", err);
    if (err) return 19;

    cl_uint loops = LOOPS;
    err = clSetKernelArg(old_kernel, 2, sizeof(loops), &loops);
    printf("set_old_loops=%d loops=%u\n", err, loops);
    if (err) return 20;

    size_t global = 1;
    double t0 = now_ms();

    err = clEnqueueNDRangeKernel(q1, old_kernel, 1, NULL,
                                 &global, NULL, 0, NULL, &old_ev);
    printf("enqueue_old=%d\n", err);
    if (err) return 21;

    state.ctx = ctx;
    state.old_event = old_ev;

    void *ptrs[1] = { a };

    err = clEnqueueSVMFree(q1, 1, ptrs,
                           free_callback, &state,
                           1, &old_ev, &free_ev);
    printf("enqueue_free_callback=%d\n", err);
    if (err) return 22;

    err = clFlush(q1);
    printf("flush_q1=%d\n", err);
    if (err) return 23;

    double poll_start = now_ms();

    while (!atomic_load(&state.free_done)) {
        if (now_ms() - poll_start > 2000.0) {
            printf("callback_timeout=1\n");
            return 24;
        }
        sched_yield();
    }

    double t_free = now_ms();

    printf("callback_called=%d\n", atomic_load(&state.called));
    printf("callback_query_err=%d\n", atomic_load(&state.query_err));
    printf("callback_old_status=%d\n", atomic_load(&state.old_status));
    printf("callback_free_done=%d\n", atomic_load(&state.free_done));
    printf("callback_after_enqueue_ms=%.3f\n", t_free - t0);

    /*
     * A is gone. Never dereference it again.
     */

    /* ---------- New lifetime B ---------- */

    b = clSVMAlloc(ctx, CL_MEM_READ_WRITE, BYTES, 0);

    printf("alloc_B=%s addr=0x%llx old_addr=0x%llx same_addr=%d\n",
           b ? "success" : "failed",
           (unsigned long long)(uintptr_t)b,
           (unsigned long long)a_addr,
           b && ((uintptr_t)b == a_addr));

    if (!b) return 25;

    err = clEnqueueSVMMap(q2, CL_TRUE, CL_MAP_WRITE,
                          b, BYTES, 0, NULL, NULL);
    printf("map_B_write=%d\n", err);
    if (err) return 26;

    for (size_t i = 0; i < N; i++)
        b[i] = 0xddddddddu;

    b[0] = B0_CANARY;
    b[1] = B1_CANARY;

    err = clEnqueueSVMUnmap(q2, b, 0, NULL, NULL);
    printf("unmap_B_write=%d\n", err);
    if (err) return 27;

    err = clFinish(q2);
    printf("finish_B_init=%d\n", err);
    if (err) return 28;

    cl_int old_status_before_B_kernel = -999;

    err = clGetEventInfo(old_ev,
                         CL_EVENT_COMMAND_EXECUTION_STATUS,
                         sizeof(old_status_before_B_kernel),
                         &old_status_before_B_kernel, NULL);

    printf("old_status_before_B_kernel=%d query_err=%d\n",
           old_status_before_B_kernel, err);

    /* ---------- Device access to NEW lifetime ---------- */

    err = clSetKernelArgSVMPointer(new_kernel, 0, b);
    printf("set_new_B=%d\n", err);
    if (err) return 29;

    err = clSetKernelArg(new_kernel, 1, sizeof(out), &out);
    printf("set_new_out=%d\n", err);
    if (err) return 30;

    double t_b_enqueue = now_ms();

    err = clEnqueueNDRangeKernel(q2, new_kernel, 1, NULL,
                                 &global, NULL, 0, NULL, &new_ev);

    printf("enqueue_B_kernel=%d\n", err);
    if (err) return 31;

    err = clWaitForEvents(1, &new_ev);
    double t_b_done = now_ms();

    printf("wait_B_kernel=%d\n", err);
    printf("B_kernel_host_elapsed_ms=%.3f\n",
           t_b_done - t_b_enqueue);
    printf("B_kernel_done_after_old_enqueue_ms=%.3f\n",
           t_b_done - t0);

    if (err) return 32;

    cl_int old_status_after_B_kernel = -999;

    err = clGetEventInfo(old_ev,
                         CL_EVENT_COMMAND_EXECUTION_STATUS,
                         sizeof(old_status_after_B_kernel),
                         &old_status_after_B_kernel, NULL);

    printf("old_status_after_B_kernel=%d query_err=%d\n",
           old_status_after_B_kernel, err);

    uint32_t device_read = 0;

    err = clEnqueueReadBuffer(q2, out, CL_TRUE,
                              0, sizeof(device_read),
                              &device_read,
                              0, NULL, NULL);

    printf("read_B_kernel_output=%d\n", err);
    printf("B_device_read=0x%08x\n", device_read);

    if (err) return 33;

    /* Wait for old A kernel only now. */

    err = clWaitForEvents(1, &old_ev);
    double t_old_done = now_ms();

    printf("wait_old_kernel=%d\n", err);
    printf("old_kernel_done_after_enqueue_ms=%.3f\n",
           t_old_done - t0);

    if (err) return 34;

    err = clWaitForEvents(1, &free_ev);
    printf("wait_free_event=%d\n", err);
    if (err) return 35;

    /* Observe only B. */

    err = clEnqueueSVMMap(q2, CL_TRUE, CL_MAP_READ,
                          b, BYTES, 0, NULL, NULL);
    printf("map_B_read=%d\n", err);
    if (err) return 36;

    uint32_t b0 = b[0];
    uint32_t b1 = b[1];

    printf("B0_final=0x%08x\n", b0);
    printf("B1_final=0x%08x\n", b1);

    err = clEnqueueSVMUnmap(q2, b, 0, NULL, NULL);
    printf("unmap_B_read=%d\n", err);
    if (err) return 37;

    clFinish(q2);

    clSVMFree(ctx, b);

    clReleaseEvent(new_ev);
    clReleaseEvent(free_ev);
    clReleaseEvent(old_ev);
    clReleaseMemObject(out);
    clReleaseMemObject(sink);
    clReleaseKernel(new_kernel);
    clReleaseKernel(old_kernel);
    clReleaseProgram(prog);
    clReleaseCommandQueue(q2);
    clReleaseCommandQueue(q1);
    clReleaseContext(ctx);

    if (device_read != B0_CANARY ||
        b0 != B0_CANARY ||
        b1 != B1_GPUWRITE) {
        printf("B4-C3d=DEVICE_ISOLATION_INTERESTING\n");
        return 38;
    }

    if (old_status_after_B_kernel != CL_COMPLETE) {
        printf("B4-C3d=PARALLEL_DEVICE_ISOLATION_CONFIRMED\n");
        return 0;
    }

    printf("B4-C3d=SERIALIZED_DEVICE_CONTAINMENT_CONFIRMED\n");
    return 0;
}
