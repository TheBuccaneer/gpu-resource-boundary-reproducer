#define CL_TARGET_OPENCL_VERSION 300
#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>

static char *read_file(const char *path, size_t *n)
{
    FILE *f = fopen(path, "rb");
    if (!f) { perror(path); exit(1); }

    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    rewind(f);

    char *buf = malloc((size_t)len + 1);
    if (!buf) exit(1);

    if (fread(buf, 1, (size_t)len, f) != (size_t)len)
        exit(1);

    fclose(f);
    buf[len] = '\0';
    *n = (size_t)len;
    return buf;
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: %s LOCAL_INTS\n", argv[0]);
        return 2;
    }

    cl_int err;
    cl_platform_id platform;
    cl_device_id device;

    err = clGetPlatformIDs(1, &platform, NULL);
    if (err != CL_SUCCESS) return 3;

    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    if (err != CL_SUCCESS) return 4;

    cl_context ctx = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    if (err != CL_SUCCESS) return 5;

    size_t n = 0;
    char *src = read_file("probes/local_limit.cl", &n);
    const char *p = src;

    cl_program program =
        clCreateProgramWithSource(ctx, 1, &p, &n, &err);

    printf("clCreateProgramWithSource=%d\n", err);
    fflush(stdout);

    if (err != CL_SUCCESS) return 6;

    char opts[128];
    snprintf(opts, sizeof(opts), "-DLOCAL_INTS=%s", argv[1]);

    err = clBuildProgram(program, 1, &device, opts, NULL, NULL);

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

        return 7;
    }

    printf("about_to_clCreateKernel\n");
    fflush(stdout);

    cl_kernel k = clCreateKernel(program, "local_limit", &err);

    printf("clCreateKernel=%d\n", err);
    fflush(stdout);

    if (err == CL_SUCCESS) {
        cl_ulong local_mem = 0;

        err = clGetKernelWorkGroupInfo(
            k, device,
            CL_KERNEL_LOCAL_MEM_SIZE,
            sizeof(local_mem),
            &local_mem,
            NULL);

        printf("CL_KERNEL_LOCAL_MEM_SIZE=%llu query_err=%d\n",
               (unsigned long long)local_mem, err);

        clReleaseKernel(k);
    }

    clReleaseProgram(program);
    clReleaseContext(ctx);
    free(src);

    return 0;
}
