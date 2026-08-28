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

    size_t local = parse_size(argv[1]);
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

    if (!dev) {
        printf("NO_GPU_DEVICE\n");
        return 3;
    }

    size_t device_wg = 0;
    clGetDeviceInfo(dev, CL_DEVICE_MAX_WORK_GROUP_SIZE,
                    sizeof(device_wg), &device_wg, NULL);

    printf("device_max_work_group_size=%zu\n", device_wg);

    cl_context ctx =
        clCreateContext(NULL, 1, &dev, NULL, NULL, &err);

    printf("clCreateContext=%d\n", err);
    if (err != CL_SUCCESS)
        return 4;

    const cl_queue_properties props[] = {0};

    cl_command_queue queue =
        clCreateCommandQueueWithProperties(ctx, dev, props, &err);

    printf("clCreateCommandQueueWithProperties=%d\n", err);
    if (err != CL_SUCCESS)
        return 5;

    const char *src =
        "__kernel void b2_wg(__global uint *out) {"
        "  if (get_global_id(0) == 0) {"
        "    out[0] = (uint)get_local_size(0);"
        "    out[1] = (uint)get_global_size(0);"
        "    out[2] = (uint)get_num_groups(0);"
        "    out[3] = (uint)get_group_id(0);"
        "  }"
        "}";

    cl_program prog =
        clCreateProgramWithSource(ctx, 1, &src, NULL, &err);

    printf("clCreateProgramWithSource=%d\n", err);
    if (err != CL_SUCCESS)
        return 6;

    err = clBuildProgram(prog, 1, &dev, NULL, NULL, NULL);
    printf("clBuildProgram=%d\n", err);

    if (err != CL_SUCCESS)
        return 7;

    cl_kernel kernel = clCreateKernel(prog, "b2_wg", &err);
    printf("clCreateKernel=%d\n", err);

    if (err != CL_SUCCESS)
        return 8;

    size_t kernel_wg = 0;
    err = clGetKernelWorkGroupInfo(
        kernel, dev, CL_KERNEL_WORK_GROUP_SIZE,
        sizeof(kernel_wg), &kernel_wg, NULL);

    printf("kernel_max_work_group_size=%zu err=%d\n",
           kernel_wg, err);

    size_t preferred = 0;
    err = clGetKernelWorkGroupInfo(
        kernel, dev, CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE,
        sizeof(preferred), &preferred, NULL);

    printf("kernel_preferred_multiple=%zu err=%d\n",
           preferred, err);

    cl_uint init[4] = {0, 0, 0, 0};

    cl_mem out =
        clCreateBuffer(ctx,
                       CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
                       sizeof(init), init, &err);

    printf("clCreateBuffer=%d\n", err);
    if (err != CL_SUCCESS)
        return 9;

    err = clSetKernelArg(kernel, 0, sizeof(out), &out);
    printf("clSetKernelArg=%d\n", err);

    if (err != CL_SUCCESS)
        return 10;

    err = clEnqueueNDRangeKernel(
        queue, kernel, 1,
        NULL, &global, &local,
        0, NULL, NULL);

    printf("clEnqueueNDRangeKernel=%d\n", err);

    if (err != CL_SUCCESS)
        return 11;

    err = clFinish(queue);
    printf("clFinish=%d\n", err);

    if (err != CL_SUCCESS)
        return 12;

    cl_uint result[4] = {0, 0, 0, 0};

    err = clEnqueueReadBuffer(
        queue, out, CL_TRUE,
        0, sizeof(result), result,
        0, NULL, NULL);

    printf("clEnqueueReadBuffer=%d\n", err);

    if (err != CL_SUCCESS)
        return 13;

    printf("observed_local=%u\n", result[0]);
    printf("observed_global=%u\n", result[1]);
    printf("observed_num_groups=%u\n", result[2]);
    printf("observed_group_id=%u\n", result[3]);

    unsigned expected_groups =
        (unsigned)(global / local);

    if (result[0] != local ||
        result[1] != global ||
        result[2] != expected_groups ||
        result[3] != 0) {
        printf("B2-WG=FAIL\n");
        return 14;
    }

    printf("B2-WG=PASS\n");

    clReleaseMemObject(out);
    clReleaseKernel(kernel);
    clReleaseProgram(prog);
    clReleaseCommandQueue(queue);
    clReleaseContext(ctx);

    return 0;
}
