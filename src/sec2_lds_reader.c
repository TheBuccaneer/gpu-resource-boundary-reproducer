#define CL_TARGET_OPENCL_VERSION 300
#include <CL/cl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define LDS_WORDS 8192u
#define LOCAL_SIZE 256u
#define MAGIC 0xA5C30000u

static const char *src =
"__kernel void lds_reader(__global uint *out)\n"
"{\n"
"    volatile __local uint scratch[8192];\n"
"    uint lid = (uint)get_local_id(0);\n"
"    uint lsize = (uint)get_local_size(0);\n"
"    uint group = (uint)get_group_id(0);\n"
"\n"
"    for (uint i = lid; i < 8192; i += lsize) {\n"
"        uint v = scratch[i];\n"
"        out[group * 8192u + i] = v;\n"
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
    cl_mem outbuf = NULL;
    uint32_t *out = NULL;

    err = clGetPlatformIDs(0, NULL, &np);
    printf("clGetPlatformIDs=%d platforms=%u\n", err, np);
    if (err != CL_SUCCESS || np == 0) return 2;

    err = clGetPlatformIDs(1, &platform, NULL);
    printf("clGetPlatformIDs(select)=%d\n", err);
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

    q = clCreateCommandQueueWithProperties(ctx, dev, NULL, &err);
    printf("clCreateCommandQueueWithProperties=%d\n", err);
    if (!q || err != CL_SUCCESS) return 6;

    prog = clCreateProgramWithSource(ctx, 1, &src, NULL, &err);
    printf("clCreateProgramWithSource=%d\n", err);
    if (!prog || err != CL_SUCCESS) return 7;

    err = clBuildProgram(prog, 1, &dev, "-cl-opt-disable", NULL, NULL);
    printf("clBuildProgram=%d\n", err);

    if (err != CL_SUCCESS) {
        size_t n = 0;
        clGetProgramBuildInfo(prog, dev, CL_PROGRAM_BUILD_LOG,
                              0, NULL, &n);
        char *log = calloc(1, n + 1);
        if (log) {
            clGetProgramBuildInfo(prog, dev, CL_PROGRAM_BUILD_LOG,
                                  n, log, NULL);
            printf("BUILD_LOG:\n%s\n", log);
            free(log);
        }
        return 8;
    }

    kernel = clCreateKernel(prog, "lds_reader", &err);
    printf("clCreateKernel=%d\n", err);
    if (!kernel || err != CL_SUCCESS) return 9;

    cl_ulong kernel_local = 0;
    err = clGetKernelWorkGroupInfo(kernel, dev,
                                   CL_KERNEL_LOCAL_MEM_SIZE,
                                   sizeof(kernel_local),
                                   &kernel_local, NULL);
    printf("CL_KERNEL_LOCAL_MEM_SIZE=%llu err=%d\n",
           (unsigned long long)kernel_local, err);

    size_t groups = cus ? (size_t)cus * 16u : 64u;
    size_t global = groups * LOCAL_SIZE;
    size_t words = groups * LDS_WORDS;
    size_t bytes = words * sizeof(uint32_t);

    out = calloc(words, sizeof(uint32_t));
    if (!out) return 10;

    outbuf = clCreateBuffer(ctx, CL_MEM_WRITE_ONLY, bytes, NULL, &err);
    printf("clCreateBuffer=%d bytes=%zu groups=%zu\n",
           err, bytes, groups);
    if (!outbuf || err != CL_SUCCESS) return 11;

    err = clSetKernelArg(kernel, 0, sizeof(outbuf), &outbuf);
    printf("clSetKernelArg=%d\n", err);
    if (err != CL_SUCCESS) return 12;

    size_t local = LOCAL_SIZE;
    err = clEnqueueNDRangeKernel(q, kernel, 1, NULL,
                                 &global, &local,
                                 0, NULL, NULL);
    printf("clEnqueueNDRangeKernel=%d global=%zu local=%zu\n",
           err, global, local);
    if (err != CL_SUCCESS) return 13;

    err = clEnqueueReadBuffer(q, outbuf, CL_TRUE, 0,
                              bytes, out, 0, NULL, NULL);
    printf("clEnqueueReadBuffer=%d\n", err);
    if (err != CL_SUCCESS) return 14;

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
        printf("SEC2-READER=OBSERVED\n");
    else
        printf("SEC2-READER=NO_EXACT_MATCH\n");

    clReleaseMemObject(outbuf);
    clReleaseKernel(kernel);
    clReleaseProgram(prog);
    clReleaseCommandQueue(q);
    clReleaseContext(ctx);
    free(out);

    return 0;
}
