#define CL_TARGET_OPENCL_VERSION 300
#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>

static void print_local_info(cl_kernel k, cl_device_id dev,
                             const char *label)
{
    cl_ulong local = 0;
    size_t wg = 0;
    cl_int e1, e2;

    e1 = clGetKernelWorkGroupInfo(
        k, dev, CL_KERNEL_LOCAL_MEM_SIZE,
        sizeof(local), &local, NULL);

    e2 = clGetKernelWorkGroupInfo(
        k, dev, CL_KERNEL_WORK_GROUP_SIZE,
        sizeof(wg), &wg, NULL);

    printf("%s_LOCAL_MEM=%llu err=%d\n",
           label, (unsigned long long)local, e1);

    printf("%s_KERNEL_WG=%zu err=%d\n",
           label, wg, e2);
}

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

    cl_ulong device_local = 0;
    size_t device_wg = 0;

    err = clGetDeviceInfo(
        dev, CL_DEVICE_LOCAL_MEM_SIZE,
        sizeof(device_local), &device_local, NULL);

    printf("CL_DEVICE_LOCAL_MEM_SIZE=%llu err=%d\n",
           (unsigned long long)device_local, err);

    err = clGetDeviceInfo(
        dev, CL_DEVICE_MAX_WORK_GROUP_SIZE,
        sizeof(device_wg), &device_wg, NULL);

    printf("CL_DEVICE_MAX_WORK_GROUP_SIZE=%zu err=%d\n",
           device_wg, err);

    cl_context ctx =
        clCreateContext(NULL, 1, &dev, NULL, NULL, &err);

    printf("clCreateContext=%d\n", err);
    if (err != CL_SUCCESS)
        return 4;

    const char *src =
        "__kernel void b3_cap("
        "    __local uchar *a,"
        "    __local uchar *b,"
        "    __global uint *out) {"
        "  size_t lid = get_local_id(0);"
        "  if (lid == 0) {"
        "    a[0] = (uchar)0x11;"
        "    b[0] = (uchar)0x22;"
        "  }"
        "  barrier(CLK_LOCAL_MEM_FENCE);"
        "  if (lid == 0)"
        "    out[0] = (uint)a[0] + (uint)b[0];"
        "}";

    cl_program p =
        clCreateProgramWithSource(ctx, 1, &src, NULL, &err);

    printf("clCreateProgramWithSource=%d\n", err);
    if (err != CL_SUCCESS)
        return 5;

    err = clBuildProgram(p, 1, &dev, NULL, NULL, NULL);
    printf("clBuildProgram=%d\n", err);

    if (err != CL_SUCCESS) {
        size_t n = 0;

        clGetProgramBuildInfo(
            p, dev, CL_PROGRAM_BUILD_LOG,
            0, NULL, &n);

        char *log = calloc(1, n + 1);

        if (log) {
            clGetProgramBuildInfo(
                p, dev, CL_PROGRAM_BUILD_LOG,
                n, log, NULL);

            printf("BUILD_LOG:\n%s\n", log);
            free(log);
        }

        return 6;
    }

    cl_kernel k =
        clCreateKernel(p, "b3_cap", &err);

    printf("clCreateKernel=%d\n", err);
    if (err != CL_SUCCESS)
        return 7;

    print_local_info(k, dev, "BEFORE");

    err = clSetKernelArg(k, 0, 4096, NULL);
    printf("clSetKernelArg(A,4096)=%d\n", err);

    if (err != CL_SUCCESS)
        return 8;

    print_local_info(k, dev, "AFTER_A");

    err = clSetKernelArg(k, 1, 8192, NULL);
    printf("clSetKernelArg(B,8192)=%d\n", err);

    if (err != CL_SUCCESS)
        return 9;

    print_local_info(k, dev, "AFTER_AB");

    cl_ulong final_local = 0;

    err = clGetKernelWorkGroupInfo(
        k, dev, CL_KERNEL_LOCAL_MEM_SIZE,
        sizeof(final_local), &final_local, NULL);

    if (err != CL_SUCCESS)
        return 10;

    if (final_local != 12288) {
        printf("EXPECTED_LOCAL_MEM=12288\n");
        printf("B3-C0=UNEXPECTED\n");
        return 20;
    }

    printf("B3-C0=PASS\n");

    clReleaseKernel(k);
    clReleaseProgram(p);
    clReleaseContext(ctx);

    return 0;
}
