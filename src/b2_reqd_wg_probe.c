#define CL_TARGET_OPENCL_VERSION 300
#include <CL/cl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

static size_t parse_size(const char *s)
{
    errno = 0;
    char *end = NULL;
    unsigned long long v = strtoull(s, &end, 10);

    if (errno || !end || *end != '\0' || v == 0) {
        fprintf(stderr, "invalid size: %s\n", s);
        exit(90);
    }

    return (size_t)v;
}

int main(int argc, char **argv)
{
    setvbuf(stdout, NULL, _IONBF, 0);

    if (argc != 3) {
        fprintf(stderr, "usage: %s LOCAL GLOBAL\n", argv[0]);
        return 90;
    }

    size_t local  = parse_size(argv[1]);
    size_t global = parse_size(argv[2]);

    printf("requested_local=%zu\n", local);
    printf("requested_global=%zu\n", global);

    cl_int err;
    cl_uint np = 0;

    err = clGetPlatformIDs(0, NULL, &np);
    printf("clGetPlatformIDs=%d platforms=%u\n", err, np);
    if (err != CL_SUCCESS || np == 0)
        return 1;

    cl_platform_id *platforms = calloc(np, sizeof(*platforms));
    if (!platforms)
        return 2;

    clGetPlatformIDs(np, platforms, NULL);

    cl_device_id dev = NULL;

    for (cl_uint i = 0; i < np && !dev; ++i) {
        cl_uint nd = 0;

        if (clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_GPU,
                           0, NULL, &nd) == CL_SUCCESS &&
            nd > 0) {
            cl_device_id *devs = calloc(nd, sizeof(*devs));
            if (!devs)
                return 2;

            clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_GPU,
                           nd, devs, NULL);
            dev = devs[0];
            free(devs);
        }
    }

    free(platforms);

    if (!dev)
        return 3;

    cl_context ctx =
        clCreateContext(NULL, 1, &dev, NULL, NULL, &err);
    printf("clCreateContext=%d\n", err);
    if (err != CL_SUCCESS)
        return 4;

    cl_command_queue q =
        clCreateCommandQueueWithProperties(ctx, dev, NULL, &err);
    printf("clCreateCommandQueueWithProperties=%d\n", err);
    if (err != CL_SUCCESS)
        return 5;

    const char *src =
        "__attribute__((reqd_work_group_size(64,1,1))) "
        "__kernel void b2_reqd(__global uint *out) {"
        "  if (get_global_id(0) == 0) {"
        "    out[0] = (uint)get_local_size(0);"
        "    out[1] = (uint)get_global_size(0);"
        "  }"
        "}";

    cl_program p =
        clCreateProgramWithSource(ctx, 1, &src, NULL, &err);
    printf("clCreateProgramWithSource=%d\n", err);
    if (err != CL_SUCCESS)
        return 6;

    err = clBuildProgram(p, 1, &dev, NULL, NULL, NULL);
    printf("clBuildProgram=%d\n", err);

    if (err != CL_SUCCESS) {
        size_t n = 0;
        clGetProgramBuildInfo(p, dev, CL_PROGRAM_BUILD_LOG,
                              0, NULL, &n);

        char *log = calloc(1, n + 1);
        if (log) {
            clGetProgramBuildInfo(p, dev, CL_PROGRAM_BUILD_LOG,
                                  n, log, NULL);
            printf("BUILD_LOG:\n%s\n", log);
            free(log);
        }
        return 7;
    }

    cl_kernel k = clCreateKernel(p, "b2_reqd", &err);
    printf("clCreateKernel=%d\n", err);
    if (err != CL_SUCCESS)
        return 8;

    size_t compile_wg[3] = {0,0,0};
    err = clGetKernelWorkGroupInfo(
        k, dev, CL_KERNEL_COMPILE_WORK_GROUP_SIZE,
        sizeof(compile_wg), compile_wg, NULL);

    printf("CL_KERNEL_COMPILE_WORK_GROUP_SIZE=%zu,%zu,%zu err=%d\n",
           compile_wg[0], compile_wg[1], compile_wg[2], err);

    size_t kernel_wg = 0;
    err = clGetKernelWorkGroupInfo(
        k, dev, CL_KERNEL_WORK_GROUP_SIZE,
        sizeof(kernel_wg), &kernel_wg, NULL);

    printf("CL_KERNEL_WORK_GROUP_SIZE=%zu err=%d\n",
           kernel_wg, err);

    cl_uint init[2] = {0,0};

    cl_mem out = clCreateBuffer(
        ctx,
        CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
        sizeof(init), init, &err);

    printf("clCreateBuffer=%d\n", err);
    if (err != CL_SUCCESS)
        return 9;

    err = clSetKernelArg(k, 0, sizeof(out), &out);
    printf("clSetKernelArg=%d\n", err);
    if (err != CL_SUCCESS)
        return 10;

    err = clEnqueueNDRangeKernel(
        q, k, 1,
        NULL, &global, &local,
        0, NULL, NULL);

    printf("clEnqueueNDRangeKernel=%d\n", err);

    if (err != CL_SUCCESS)
        return 11;

    err = clFinish(q);
    printf("clFinish=%d\n", err);
    if (err != CL_SUCCESS)
        return 12;

    cl_uint result[2] = {0,0};

    err = clEnqueueReadBuffer(
        q, out, CL_TRUE,
        0, sizeof(result), result,
        0, NULL, NULL);

    printf("clEnqueueReadBuffer=%d\n", err);
    if (err != CL_SUCCESS)
        return 13;

    printf("observed_local=%u\n", result[0]);
    printf("observed_global=%u\n", result[1]);

    if (compile_wg[0] != 64 ||
        compile_wg[1] != 1 ||
        compile_wg[2] != 1 ||
        result[0] != local ||
        result[1] != global) {
        printf("B2-C6c1=FAIL\n");
        return 14;
    }

    printf("B2-C6c1=PASS\n");

    clReleaseMemObject(out);
    clReleaseKernel(k);
    clReleaseProgram(p);
    clReleaseCommandQueue(q);
    clReleaseContext(ctx);

    return 0;
}
