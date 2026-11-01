#define CL_TARGET_OPENCL_VERSION 300
#include <CL/cl.h>

#include <stdio.h>
#include <stdlib.h>

static void die_cl(const char *what, cl_int err)
{
    fprintf(stderr, "%s failed: %d\n", what, err);
    exit(1);
}

static char *read_text(const char *path, size_t *size_out)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        perror(path);
        exit(1);
    }

    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    rewind(f);

    char *buf = malloc((size_t)n + 1);
    if (!buf) {
        perror("malloc");
        exit(1);
    }

    if (fread(buf, 1, (size_t)n, f) != (size_t)n) {
        perror("fread");
        exit(1);
    }

    fclose(f);
    buf[n] = '\0';

    *size_out = (size_t)n;
    return buf;
}

int main(void)
{
    cl_int err;

    cl_platform_id platform;
    cl_device_id device;

    err = clGetPlatformIDs(1, &platform, NULL);
    if (err != CL_SUCCESS)
        die_cl("clGetPlatformIDs", err);

    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    if (err != CL_SUCCESS)
        die_cl("clGetDeviceIDs", err);

    cl_context context =
        clCreateContext(NULL, 1, &device, NULL, NULL, &err);

    if (err != CL_SUCCESS)
        die_cl("clCreateContext", err);

    size_t source_size;
    char *source = read_text("probes/add1.cl", &source_size);

    const char *src = source;

    cl_program program =
        clCreateProgramWithSource(context, 1, &src, &source_size, &err);

    if (err != CL_SUCCESS)
        die_cl("clCreateProgramWithSource", err);

    err = clBuildProgram(program, 1, &device, NULL, NULL, NULL);

    if (err != CL_SUCCESS)
        die_cl("clBuildProgram", err);

    size_t binary_size = 0;

    err = clGetProgramInfo(program,
                           CL_PROGRAM_BINARY_SIZES,
                           sizeof(binary_size),
                           &binary_size,
                           NULL);

    if (err != CL_SUCCESS)
        die_cl("CL_PROGRAM_BINARY_SIZES", err);

    printf("Program binary size: %zu bytes\n", binary_size);

    unsigned char *binary = malloc(binary_size);
    if (!binary) {
        perror("malloc");
        exit(1);
    }

    unsigned char *binaries[] = {binary};

    err = clGetProgramInfo(program,
                           CL_PROGRAM_BINARIES,
                           sizeof(binaries),
                           binaries,
                           NULL);

    if (err != CL_SUCCESS)
        die_cl("CL_PROGRAM_BINARIES", err);

    FILE *f = fopen("artifacts/add1_vendor.bin", "wb");
    if (!f) {
        perror("artifacts/add1_vendor.bin");
        exit(1);
    }

    fwrite(binary, 1, binary_size, f);
    fclose(f);

    printf("Saved: artifacts/add1_vendor.bin\n");

    free(binary);
    free(source);

    clReleaseProgram(program);
    clReleaseContext(context);

    return 0;
}
