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
"\n"
"    for (uint i = lid; i < 8192; i += lsize)\n"
"        scratch[i] = 0xA5C30000u ^ i;\n"
"\n"
"    barrier(CLK_LOCAL_MEM_FENCE);\n"
"\n"
"    uint idx = (lid * 131u + 17u) & 8191u;\n"
"    sink[get_global_id(0)] = scratch[idx];\n"
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
    clGetDeviceInfo(dev, CL_DEVICE_LOCAL_MEM_SIZE, sizeof(local_mem), &local_mem, NULL);

    printf("device=%s\n", name);
    printf("compute_units=%u\n", cus);
    printf("local_mem=%llu\n", (unsigned long long)local_mem);
    printf("victim_lds_bytes=%u\n", LDS_WORDS * 4u);

    if (local_mem < LDS_WORDS * sizeof(uint32_t)) {
        printf("SEC2-VICTIM=SKIP local memory too small\n");
        return 5;
    }

    ctx = clCreateContext(NULL, 1, &dev, NULL, NULL, &err);
    printf("clCreateContext=%d\n", err);
    if (!ctx || err != CL_SUCCESS) return 6;

    q = clCreateCommandQueueWithProperties(ctx, dev, NULL, &err);
    printf("clCreateCommandQueueWithProperties=%d\n", err);
    if (!q || err != CL_SUCCESS) return 7;

    prog = clCreateProgramWithSource(ctx, 1, &src, NULL, &err);
    printf("clCreateProgramWithSource=%d\n", err);
    if (!prog || err != CL_SUCCESS) return 8;

    err = clBuildProgram(prog, 1, &dev, NULL, NULL, NULL);
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
        return 9;
    }

    kernel = clCreateKernel(prog, "lds_victim", &err);
    printf("clCreateKernel=%d\n", err);
    if (!kernel || err != CL_SUCCESS) return 10;

    size_t groups = cus ? (size_t)cus * 16u : 64u;
    size_t global = groups * LOCAL_SIZE;
    size_t bytes = global * sizeof(uint32_t);

    sink = clCreateBuffer(ctx, CL_MEM_WRITE_ONLY, bytes, NULL, &err);
    printf("clCreateBuffer=%d bytes=%zu groups=%zu\n", err, bytes, groups);
    if (!sink || err != CL_SUCCESS) return 11;

    err = clSetKernelArg(kernel, 0, sizeof(sink), &sink);
    printf("clSetKernelArg=%d\n", err);
    if (err != CL_SUCCESS) return 12;

    size_t local = LOCAL_SIZE;
    err = clEnqueueNDRangeKernel(q, kernel, 1, NULL,
                                 &global, &local,
                                 0, NULL, NULL);
    printf("clEnqueueNDRangeKernel=%d global=%zu local=%zu\n",
           err, global, local);
    if (err != CL_SUCCESS) return 13;

    err = clFinish(q);
    printf("clFinish=%d\n", err);
    if (err != CL_SUCCESS) return 14;

    printf("pattern=0x%08x XOR index\n", MAGIC);
    printf("SEC2-VICTIM=PASS\n");

    clReleaseMemObject(sink);
    clReleaseKernel(kernel);
    clReleaseProgram(prog);
    clReleaseCommandQueue(q);
    clReleaseContext(ctx);

    return 0;
}
