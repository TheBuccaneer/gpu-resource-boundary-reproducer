#define CL_TARGET_OPENCL_VERSION 300
#include <CL/cl.h>

#include <stdio.h>
#include <stdlib.h>

static unsigned char *read_file(const char *path, size_t *size_out)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        perror(path);
        exit(1);
    }

    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    rewind(f);

    unsigned char *buf = malloc((size_t)n + 1);
    if (!buf) exit(1);

    if (fread(buf, 1, (size_t)n, f) != (size_t)n) exit(1);
    fclose(f);

    buf[n] = 0;
    *size_out = (size_t)n;
    return buf;
}

static void print_build_log(cl_program p, cl_device_id dev)
{
    size_t n = 0;
    clGetProgramBuildInfo(p, dev, CL_PROGRAM_BUILD_LOG, 0, NULL, &n);

    if (n > 1) {
        char *log = calloc(1, n + 1);
        clGetProgramBuildInfo(p, dev, CL_PROGRAM_BUILD_LOG, n, log, NULL);
        printf("Build log:\n%s\n", log);
        free(log);
    }
}

static void test_program(const char *label,
                         cl_program p,
                         cl_int create_err,
                         cl_device_id dev)
{
    printf("\n=== %s ===\n", label);
    printf("program creation: %d\n", create_err);

    if (create_err != CL_SUCCESS)
        return;

    cl_int err = clBuildProgram(p, 1, &dev, NULL, NULL, NULL);
    printf("clBuildProgram: %d\n", err);

    if (err != CL_SUCCESS) {
        print_build_log(p, dev);
        clReleaseProgram(p);
        return;
    }

    cl_kernel k = clCreateKernel(p, "add1", &err);
    printf("clCreateKernel: %d\n", err);

    if (err == CL_SUCCESS) {
        cl_ulong local_mem = 0;

        err = clGetKernelWorkGroupInfo(
            k,
            dev,
            CL_KERNEL_LOCAL_MEM_SIZE,
            sizeof(local_mem),
            &local_mem,
            NULL);

        printf("CL_KERNEL_LOCAL_MEM_SIZE query: %d\n", err);

        if (err == CL_SUCCESS)
            printf("kernel local memory: %llu bytes\n",
                   (unsigned long long)local_mem);


        cl_ulong private_mem = 0;
        err = clGetKernelWorkGroupInfo(k, dev, CL_KERNEL_PRIVATE_MEM_SIZE,
                                      sizeof(private_mem), &private_mem, NULL);
        printf("CL_KERNEL_PRIVATE_MEM_SIZE=%llu err=%d\n",
               (unsigned long long)private_mem, err);

        size_t wg = 0, preferred = 0, compile_wg[3] = {0};

        err = clGetKernelWorkGroupInfo(k, dev, CL_KERNEL_WORK_GROUP_SIZE,
                                      sizeof(wg), &wg, NULL);
        printf("CL_KERNEL_WORK_GROUP_SIZE=%zu err=%d\n", wg, err);

        err = clGetKernelWorkGroupInfo(k, dev,
                                      CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE,
                                      sizeof(preferred), &preferred, NULL);
        printf("CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE=%zu err=%d\n",
               preferred, err);

        err = clGetKernelWorkGroupInfo(k, dev,
                                      CL_KERNEL_COMPILE_WORK_GROUP_SIZE,
                                      sizeof(compile_wg), compile_wg, NULL);
        printf("CL_KERNEL_COMPILE_WORK_GROUP_SIZE=%zu,%zu,%zu err=%d\n",
               compile_wg[0], compile_wg[1], compile_wg[2], err);

        clReleaseKernel(k);
    }

    clReleaseProgram(p);
}

int main(void)
{
    cl_int err;
    cl_platform_id platform;
    cl_device_id device;

    err = clGetPlatformIDs(1, &platform, NULL);
    if (err != CL_SUCCESS) return 1;

    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    if (err != CL_SUCCESS) return 1;

    cl_ulong device_local = 0;
    clGetDeviceInfo(device,
                    CL_DEVICE_LOCAL_MEM_SIZE,
                    sizeof(device_local),
                    &device_local,
                    NULL);

    printf("Device local memory: %llu bytes\n",
           (unsigned long long)device_local);

    cl_context ctx =
        clCreateContext(NULL, 1, &device, NULL, NULL, &err);

    if (err != CL_SUCCESS) return 1;

    /* Source path */
    size_t src_size = 0;
    unsigned char *src_buf =
        read_file("probes/add1.cl", &src_size);

    const char *src = (const char *)src_buf;

    cl_program source_program =
        clCreateProgramWithSource(ctx, 1, &src, &src_size, &err);

    test_program("SOURCE", source_program, err, device);
    free(src_buf);

    /* IL path */
    size_t il_size = 0;
    unsigned char *il =
        read_file("artifacts/add1.spv", &il_size);

    cl_program il_program =
        clCreateProgramWithIL(ctx, il, il_size, &err);

    test_program("SPIR-V / IL", il_program, err, device);
    free(il);

    clReleaseContext(ctx);
    return 0;
}
