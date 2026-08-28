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
        fprintf(stderr, "usage: %s A_BYTES B_BYTES\n", argv[0]);
        return 90;
    }

    size_t a_bytes = parse_size(argv[1]);
    size_t b_bytes = parse_size(argv[2]);

    printf("A_bytes=%zu\n", a_bytes);
    printf("B_bytes=%zu\n", b_bytes);
    printf("requested_sum=%zu\n", a_bytes + b_bytes);

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

    cl_ulong device_local = 0;

    err = clGetDeviceInfo(
        dev, CL_DEVICE_LOCAL_MEM_SIZE,
        sizeof(device_local), &device_local, NULL);

    printf("CL_DEVICE_LOCAL_MEM_SIZE=%llu err=%d\n",
           (unsigned long long)device_local, err);

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
        "__kernel void b3_multi("
        "    __local uchar *a,"
        "    __local uchar *b,"
        "    uint a_last,"
        "    uint b_last,"
        "    __global uint *out) {"
        "  if (get_local_id(0) == 0) {"
        "    a[0]      = (uchar)0x11;"
        "    a[a_last] = (uchar)0x22;"
        "    b[0]      = (uchar)0x33;"
        "    b[b_last] = (uchar)0x44;"
        "  }"
        "  barrier(CLK_LOCAL_MEM_FENCE);"
        "  if (get_local_id(0) == 0) {"
        "    out[0] = (uint)a[0];"
        "    out[1] = (uint)a[a_last];"
        "    out[2] = (uint)b[0];"
        "    out[3] = (uint)b[b_last];"
        "  }"
        "}";

    cl_program p =
        clCreateProgramWithSource(ctx, 1, &src, NULL, &err);

    printf("clCreateProgramWithSource=%d\n", err);
    if (err != CL_SUCCESS)
        return 6;

    err = clBuildProgram(p, 1, &dev, NULL, NULL, NULL);
    printf("clBuildProgram=%d\n", err);

    if (err != CL_SUCCESS)
        return 7;

    cl_kernel k =
        clCreateKernel(p, "b3_multi", &err);

    printf("clCreateKernel=%d\n", err);
    if (err != CL_SUCCESS)
        return 8;

    cl_uint a_last = (cl_uint)(a_bytes - 1);
    cl_uint b_last = (cl_uint)(b_bytes - 1);

    err = clSetKernelArg(k, 0, a_bytes, NULL);
    printf("clSetKernelArg(A,%zu)=%d\n", a_bytes, err);
    if (err != CL_SUCCESS)
        return 9;

    err = clSetKernelArg(k, 1, b_bytes, NULL);
    printf("clSetKernelArg(B,%zu)=%d\n", b_bytes, err);
    if (err != CL_SUCCESS)
        return 10;

    err = clSetKernelArg(k, 2, sizeof(a_last), &a_last);
    printf("clSetKernelArg(a_last)=%d\n", err);
    if (err != CL_SUCCESS)
        return 11;

    err = clSetKernelArg(k, 3, sizeof(b_last), &b_last);
    printf("clSetKernelArg(b_last)=%d\n", err);
    if (err != CL_SUCCESS)
        return 12;

    cl_uint init[4] = {0,0,0,0};

    cl_mem out =
        clCreateBuffer(ctx,
                       CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
                       sizeof(init), init, &err);

    printf("clCreateBuffer=%d\n", err);
    if (err != CL_SUCCESS)
        return 13;

    err = clSetKernelArg(k, 4, sizeof(out), &out);
    printf("clSetKernelArg(out)=%d\n", err);
    if (err != CL_SUCCESS)
        return 14;

    cl_ulong reported_local = 0;

    err = clGetKernelWorkGroupInfo(
        k, dev, CL_KERNEL_LOCAL_MEM_SIZE,
        sizeof(reported_local), &reported_local, NULL);

    printf("CL_KERNEL_LOCAL_MEM_SIZE=%llu err=%d\n",
           (unsigned long long)reported_local, err);

    if (err != CL_SUCCESS)
        return 15;

    if (reported_local < a_bytes + b_bytes) {
        printf("LOCAL_MEM_UNDERREPORT\n");
        return 16;
    }

    size_t local = 64;
    size_t global = 64;

    err = clEnqueueNDRangeKernel(
        q, k, 1,
        NULL, &global, &local,
        0, NULL, NULL);

    printf("clEnqueueNDRangeKernel=%d\n", err);

    if (err != CL_SUCCESS)
        return 17;

    err = clFinish(q);
    printf("clFinish=%d\n", err);

    if (err != CL_SUCCESS)
        return 18;

    cl_uint result[4] = {0,0,0,0};

    err = clEnqueueReadBuffer(
        q, out, CL_TRUE,
        0, sizeof(result), result,
        0, NULL, NULL);

    printf("clEnqueueReadBuffer=%d\n", err);

    if (err != CL_SUCCESS)
        return 19;

    printf("A_first=0x%02x\n", result[0]);
    printf("A_last=0x%02x\n", result[1]);
    printf("B_first=0x%02x\n", result[2]);
    printf("B_last=0x%02x\n", result[3]);

    if (result[0] != 0x11 ||
        result[1] != 0x22 ||
        result[2] != 0x33 ||
        result[3] != 0x44) {
        printf("B3-MULTI=FAIL\n");
        return 20;
    }

    printf("B3-MULTI=PASS\n");

    clReleaseMemObject(out);
    clReleaseKernel(k);
    clReleaseProgram(p);
    clReleaseCommandQueue(q);
    clReleaseContext(ctx);

    return 0;
}
