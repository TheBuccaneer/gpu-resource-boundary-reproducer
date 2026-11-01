#define CL_TARGET_OPENCL_VERSION 300
#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);

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

    cl_uint max_dims = 0;
    err = clGetDeviceInfo(dev,
                          CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS,
                          sizeof(max_dims), &max_dims, NULL);

    printf("CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS=%u err=%d\n",
           max_dims, err);

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
        "__kernel void b2_dim(void) {"
        "  size_t x = get_global_id(0);"
        "  (void)x;"
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

    cl_kernel k = clCreateKernel(p, "b2_dim", &err);
    printf("clCreateKernel=%d\n", err);
    if (err != CL_SUCCESS)
        return 7;

    size_t global[4] = {1, 1, 1, 1};
    size_t local[4]  = {1, 1, 1, 1};

    printf("requested_work_dim=4\n");

    err = clEnqueueNDRangeKernel(
        q, k, 4,
        NULL,
        global,
        local,
        0, NULL, NULL);

    printf("clEnqueueNDRangeKernel=%d\n", err);

    clReleaseKernel(k);
    clReleaseProgram(p);
    clReleaseCommandQueue(q);
    clReleaseContext(ctx);

    if (err == CL_INVALID_WORK_DIMENSION) {
        printf("B2-C6b=PASS\n");
        return 0;
    }

    printf("B2-C6b=UNEXPECTED\n");
    return 20;
}
