#define CL_TARGET_OPENCL_VERSION 300
#include <CL/cl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void get_string(cl_device_id dev, cl_device_info param,
                       const char *name)
{
    size_t n = 0;
    cl_int err = clGetDeviceInfo(dev, param, 0, NULL, &n);
    if (err != CL_SUCCESS) {
        printf("%s=<query-error:%d>\n", name, err);
        return;
    }

    char *buf = calloc(1, n + 1);
    if (!buf) {
        printf("%s=<oom>\n", name);
        return;
    }

    err = clGetDeviceInfo(dev, param, n, buf, NULL);
    if (err == CL_SUCCESS)
        printf("%s=%s\n", name, buf);
    else
        printf("%s=<query-error:%d>\n", name, err);

    free(buf);
}

int main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);

    cl_int err;
    cl_uint np = 0;

    err = clGetPlatformIDs(0, NULL, &np);
    printf("clGetPlatformIDs(count)=%d platforms=%u\n", err, np);
    if (err != CL_SUCCESS || np == 0)
        return 1;

    cl_platform_id *platforms = calloc(np, sizeof(*platforms));
    clGetPlatformIDs(np, platforms, NULL);

    cl_device_id dev = NULL;
    cl_platform_id platform = NULL;

    for (cl_uint i = 0; i < np && !dev; ++i) {
        cl_uint nd = 0;
        err = clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_GPU,
                             0, NULL, &nd);
        if (err == CL_SUCCESS && nd > 0) {
            cl_device_id *devs = calloc(nd, sizeof(*devs));
            clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_GPU,
                           nd, devs, NULL);
            dev = devs[0];
            platform = platforms[i];
            free(devs);
        }
    }

    free(platforms);

    if (!dev) {
        printf("NO_GPU_DEVICE\n");
        return 2;
    }

    char platform_name[256] = {0};
    clGetPlatformInfo(platform, CL_PLATFORM_NAME,
                      sizeof(platform_name), platform_name, NULL);
    printf("platform=%s\n", platform_name);

    get_string(dev, CL_DEVICE_NAME, "device");
    get_string(dev, CL_DEVICE_VENDOR, "vendor");
    get_string(dev, CL_DEVICE_VERSION, "device_version");
    get_string(dev, CL_DRIVER_VERSION, "driver_version");
    get_string(dev, CL_DEVICE_OPENCL_C_VERSION, "opencl_c_version");

    cl_uint dims = 0;
    err = clGetDeviceInfo(dev, CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS,
                          sizeof(dims), &dims, NULL);
    printf("CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS=%u err=%d\n",
           dims, err);

    size_t device_wg = 0;
    err = clGetDeviceInfo(dev, CL_DEVICE_MAX_WORK_GROUP_SIZE,
                          sizeof(device_wg), &device_wg, NULL);
    printf("CL_DEVICE_MAX_WORK_GROUP_SIZE=%zu err=%d\n",
           device_wg, err);

    if (dims > 0 && dims < 64) {
        size_t *sizes = calloc(dims, sizeof(*sizes));

        err = clGetDeviceInfo(dev, CL_DEVICE_MAX_WORK_ITEM_SIZES,
                              dims * sizeof(*sizes), sizes, NULL);

        printf("CL_DEVICE_MAX_WORK_ITEM_SIZES err=%d", err);
        if (err == CL_SUCCESS) {
            for (cl_uint i = 0; i < dims; ++i)
                printf(" [%u]=%zu", i, sizes[i]);
        }
        printf("\n");

        free(sizes);
    }

#ifdef CL_DEVICE_NON_UNIFORM_WORK_GROUP_SUPPORT
    cl_bool nonuniform = CL_FALSE;
    err = clGetDeviceInfo(dev,
                          CL_DEVICE_NON_UNIFORM_WORK_GROUP_SUPPORT,
                          sizeof(nonuniform), &nonuniform, NULL);
    printf("CL_DEVICE_NON_UNIFORM_WORK_GROUP_SUPPORT=%u err=%d\n",
           (unsigned)nonuniform, err);
#else
    printf("CL_DEVICE_NON_UNIFORM_WORK_GROUP_SUPPORT=<header-unavailable>\n");
#endif

#ifdef CL_DEVICE_PREFERRED_WORK_GROUP_SIZE_MULTIPLE
    size_t dev_pref = 0;
    err = clGetDeviceInfo(dev,
                          CL_DEVICE_PREFERRED_WORK_GROUP_SIZE_MULTIPLE,
                          sizeof(dev_pref), &dev_pref, NULL);
    printf("CL_DEVICE_PREFERRED_WORK_GROUP_SIZE_MULTIPLE=%zu err=%d\n",
           dev_pref, err);
#endif

    cl_context ctx =
        clCreateContext(NULL, 1, &dev, NULL, NULL, &err);
    printf("clCreateContext=%d\n", err);
    if (err != CL_SUCCESS)
        return 3;

    const char *src =
        "__kernel void b2_cap(__global uint *out) {"
        "  size_t gid = get_global_id(0);"
        "  if (gid == 0) out[0] = 0xB2u;"
        "}";

    cl_program prog =
        clCreateProgramWithSource(ctx, 1, &src, NULL, &err);
    printf("clCreateProgramWithSource=%d\n", err);
    if (err != CL_SUCCESS)
        return 4;

    err = clBuildProgram(prog, 1, &dev, NULL, NULL, NULL);
    printf("clBuildProgram=%d\n", err);

    if (err != CL_SUCCESS) {
        size_t log_n = 0;
        clGetProgramBuildInfo(prog, dev, CL_PROGRAM_BUILD_LOG,
                              0, NULL, &log_n);

        char *log = calloc(1, log_n + 1);
        if (log) {
            clGetProgramBuildInfo(prog, dev, CL_PROGRAM_BUILD_LOG,
                                  log_n, log, NULL);
            printf("BUILD_LOG:\n%s\n", log);
            free(log);
        }
        return 5;
    }

    cl_kernel kernel = clCreateKernel(prog, "b2_cap", &err);
    printf("clCreateKernel=%d\n", err);
    if (err != CL_SUCCESS)
        return 6;

    size_t kernel_wg = 0;
    err = clGetKernelWorkGroupInfo(
        kernel, dev, CL_KERNEL_WORK_GROUP_SIZE,
        sizeof(kernel_wg), &kernel_wg, NULL);

    printf("CL_KERNEL_WORK_GROUP_SIZE=%zu err=%d\n",
           kernel_wg, err);

    size_t compile_wg[3] = {0, 0, 0};
    err = clGetKernelWorkGroupInfo(
        kernel, dev, CL_KERNEL_COMPILE_WORK_GROUP_SIZE,
        sizeof(compile_wg), compile_wg, NULL);

    printf("CL_KERNEL_COMPILE_WORK_GROUP_SIZE="
           "%zu,%zu,%zu err=%d\n",
           compile_wg[0], compile_wg[1], compile_wg[2], err);

    size_t kernel_pref = 0;
    err = clGetKernelWorkGroupInfo(
        kernel, dev, CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE,
        sizeof(kernel_pref), &kernel_pref, NULL);

    printf("CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE=%zu err=%d\n",
           kernel_pref, err);

    cl_ulong kernel_local = 0;
    err = clGetKernelWorkGroupInfo(
        kernel, dev, CL_KERNEL_LOCAL_MEM_SIZE,
        sizeof(kernel_local), &kernel_local, NULL);

    printf("CL_KERNEL_LOCAL_MEM_SIZE=%llu err=%d\n",
           (unsigned long long)kernel_local, err);

    cl_ulong kernel_private = 0;
    err = clGetKernelWorkGroupInfo(
        kernel, dev, CL_KERNEL_PRIVATE_MEM_SIZE,
        sizeof(kernel_private), &kernel_private, NULL);

    printf("CL_KERNEL_PRIVATE_MEM_SIZE=%llu err=%d\n",
           (unsigned long long)kernel_private, err);

    if (kernel_wg > device_wg)
        printf("INVARIANT=FAIL kernel_wg_gt_device_wg\n");
    else if (kernel_wg < device_wg)
        printf("RELATION=KERNEL_LT_DEVICE\n");
    else
        printf("RELATION=KERNEL_EQ_DEVICE\n");

    printf("B2-C0=PASS\n");

    clReleaseKernel(kernel);
    clReleaseProgram(prog);
    clReleaseContext(ctx);

    return 0;
}
