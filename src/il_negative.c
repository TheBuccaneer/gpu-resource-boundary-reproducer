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

    unsigned char *buf = malloc((size_t)n);
    if (!buf) {
        perror("malloc");
        exit(1);
    }

    if (fread(buf, 1, (size_t)n, f) != (size_t)n) {
        perror("fread");
        exit(1);
    }

    fclose(f);
    *size_out = (size_t)n;
    return buf;
}

int main(void)
{
    cl_int err;
    cl_platform_id platform;
    cl_device_id device;

    err = clGetPlatformIDs(1, &platform, NULL);
    printf("clGetPlatformIDs: %d\n", err);
    if (err != CL_SUCCESS) return 1;

    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    printf("clGetDeviceIDs: %d\n", err);
    if (err != CL_SUCCESS) return 1;

    cl_context ctx = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    printf("clCreateContext: %d\n", err);
    if (err != CL_SUCCESS) return 1;

    size_t size = 0;
    unsigned char *il =
        read_file("artifacts/add1_bad_memorymodel.spv", &size);

    cl_program p = clCreateProgramWithIL(ctx, il, size, &err);
    printf("clCreateProgramWithIL: %d\n", err);

    if (err == CL_SUCCESS) {
        err = clBuildProgram(p, 1, &device, NULL, NULL, NULL);
        printf("clBuildProgram: %d\n", err);

        size_t log_size = 0;
        clGetProgramBuildInfo(p, device, CL_PROGRAM_BUILD_LOG,
                              0, NULL, &log_size);

        if (log_size > 1) {
            char *log = calloc(1, log_size + 1);
            clGetProgramBuildInfo(p, device, CL_PROGRAM_BUILD_LOG,
                                  log_size, log, NULL);
            printf("Build log:\n%s\n", log);
            free(log);
        }

        clReleaseProgram(p);
    }

    free(il);
    clReleaseContext(ctx);
    return 0;
}
