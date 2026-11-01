#define CL_TARGET_OPENCL_VERSION 300
#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>

static unsigned char *read_file(const char *path, size_t *n)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        perror(path);
        exit(1);
    }

    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    rewind(f);

    unsigned char *buf = malloc((size_t)len);
    if (!buf)
        exit(1);

    if (fread(buf, 1, (size_t)len, f) != (size_t)len)
        exit(1);

    fclose(f);
    *n = (size_t)len;
    return buf;
}

int main(void)
{
    cl_int err;
    cl_platform_id platform;
    cl_device_id device;

    err = clGetPlatformIDs(1, &platform, NULL);
    if (err != CL_SUCCESS) return 2;

    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    if (err != CL_SUCCESS) return 3;

    cl_context ctx = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    if (err != CL_SUCCESS) return 4;

    size_t il_size = 0;
    unsigned char *il =
        read_file("artifacts/local_limit_65540.spv", &il_size);

    cl_program program =
        clCreateProgramWithIL(ctx, il, il_size, &err);

    printf("clCreateProgramWithIL=%d\n", err);
    fflush(stdout);

    if (err != CL_SUCCESS)
        return 5;

    err = clBuildProgram(program, 1, &device, NULL, NULL, NULL);

    printf("clBuildProgram=%d\n", err);
    fflush(stdout);

    if (err != CL_SUCCESS) {
        size_t log_n = 0;
        clGetProgramBuildInfo(program, device,
                              CL_PROGRAM_BUILD_LOG,
                              0, NULL, &log_n);

        char *log = calloc(1, log_n + 1);
        if (log) {
            clGetProgramBuildInfo(program, device,
                                  CL_PROGRAM_BUILD_LOG,
                                  log_n, log, NULL);
            printf("%s\n", log);
            free(log);
        }

        return 6;
    }

    printf("about_to_clCreateKernel\n");
    fflush(stdout);

    cl_kernel kernel =
        clCreateKernel(program, "local_limit", &err);

    printf("clCreateKernel=%d\n", err);
    fflush(stdout);

    if (err == CL_SUCCESS) {
        cl_ulong local_mem = 0;

        err = clGetKernelWorkGroupInfo(
            kernel, device,
            CL_KERNEL_LOCAL_MEM_SIZE,
            sizeof(local_mem),
            &local_mem,
            NULL);

        printf("CL_KERNEL_LOCAL_MEM_SIZE=%llu query_err=%d\n",
               (unsigned long long)local_mem, err);

        clReleaseKernel(kernel);
    }

    clReleaseProgram(program);
    clReleaseContext(ctx);
    free(il);

    return 0;
}
