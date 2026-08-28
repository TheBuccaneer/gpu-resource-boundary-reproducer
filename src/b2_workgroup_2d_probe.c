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
        fprintf(stderr, "usage: %s LOCAL_X LOCAL_Y\n", argv[0]);
        return 90;
    }

    size_t local[2] = {
        parse_size(argv[1]),
        parse_size(argv[2])
    };

    size_t global[2] = {
        local[0],
        local[1]
    };

    printf("requested_local=%zux%zu\n", local[0], local[1]);
    printf("requested_product=%zu\n", local[0] * local[1]);

    cl_int err;
    cl_uint np = 0;

    err = clGetPlatformIDs(0, NULL, &np);
    printf("clGetPlatformIDs=%d platforms=%u\n", err, np);
    if (err != CL_SUCCESS || np == 0)
        return 1;

    cl_platform_id *platforms = calloc(np, sizeof(*platforms));
    clGetPlatformIDs(np, platforms, NULL);

    cl_device_id dev = NULL;

    for (cl_uint i = 0; i < np && !dev; ++i) {
        cl_uint nd = 0;

        if (clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_GPU,
                           0, NULL, &nd) == CL_SUCCESS && nd > 0) {
            cl_device_id *devs = calloc(nd, sizeof(*devs));
            clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_GPU,
                           nd, devs, NULL);
            dev = devs[0];
            free(devs);
        }
    }

    free(platforms);

    if (!dev)
        return 2;

    size_t item_sizes[3] = {0,0,0};
    clGetDeviceInfo(dev, CL_DEVICE_MAX_WORK_ITEM_SIZES,
                    sizeof(item_sizes), item_sizes, NULL);

    printf("device_max_item_sizes=%zu,%zu,%zu\n",
           item_sizes[0], item_sizes[1], item_sizes[2]);

    cl_context ctx =
        clCreateContext(NULL, 1, &dev, NULL, NULL, &err);
    printf("clCreateContext=%d\n", err);
    if (err != CL_SUCCESS)
        return 3;

    cl_command_queue q =
        clCreateCommandQueueWithProperties(ctx, dev, NULL, &err);
    printf("clCreateCommandQueueWithProperties=%d\n", err);
    if (err != CL_SUCCESS)
        return 4;

    const char *src =
        "__kernel void b2_2d(__global uint *out) {"
        "  if (get_global_id(0) == 0 && get_global_id(1) == 0) {"
        "    out[0] = (uint)get_local_size(0);"
        "    out[1] = (uint)get_local_size(1);"
        "    out[2] = (uint)(get_local_size(0) * get_local_size(1));"
        "  }"
        "}";

    cl_program p =
        clCreateProgramWithSource(ctx, 1, &src, NULL, &err);
    printf("clCreateProgramWithSource=%d\n", err);
    if (err != CL_SUCCESS)
        return 5;

    err = clBuildProgram(p, 1, &dev, NULL, NULL, NULL);
    printf("clBuildProgram=%d\n", err);
    if (err != CL_SUCCESS)
        return 6;

    cl_kernel k = clCreateKernel(p, "b2_2d", &err);
    printf("clCreateKernel=%d\n", err);
    if (err != CL_SUCCESS)
        return 7;

    size_t kernel_wg = 0;
    err = clGetKernelWorkGroupInfo(
        k, dev, CL_KERNEL_WORK_GROUP_SIZE,
        sizeof(kernel_wg), &kernel_wg, NULL);

    printf("CL_KERNEL_WORK_GROUP_SIZE=%zu err=%d\n",
           kernel_wg, err);

    cl_uint init[3] = {0,0,0};

    cl_mem out =
        clCreateBuffer(ctx,
                       CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
                       sizeof(init), init, &err);
    printf("clCreateBuffer=%d\n", err);
    if (err != CL_SUCCESS)
        return 8;

    err = clSetKernelArg(k, 0, sizeof(out), &out);
    printf("clSetKernelArg=%d\n", err);
    if (err != CL_SUCCESS)
        return 9;

    err = clEnqueueNDRangeKernel(
        q, k, 2,
        NULL, global, local,
        0, NULL, NULL);

    printf("clEnqueueNDRangeKernel=%d\n", err);

    if (err != CL_SUCCESS)
        return 10;

    err = clFinish(q);
    printf("clFinish=%d\n", err);
    if (err != CL_SUCCESS)
        return 11;

    cl_uint result[3] = {0,0,0};

    err = clEnqueueReadBuffer(
        q, out, CL_TRUE,
        0, sizeof(result), result,
        0, NULL, NULL);

    printf("clEnqueueReadBuffer=%d\n", err);
    if (err != CL_SUCCESS)
        return 12;

    printf("observed_local=%ux%u\n", result[0], result[1]);
    printf("observed_product=%u\n", result[2]);

    if (result[0] != local[0] ||
        result[1] != local[1] ||
        result[2] != local[0] * local[1]) {
        printf("B2-C7=FAIL\n");
        return 13;
    }

    printf("B2-C7=PASS\n");

    clReleaseMemObject(out);
    clReleaseKernel(k);
    clReleaseProgram(p);
    clReleaseCommandQueue(q);
    clReleaseContext(ctx);

    return 0;
}
