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
    cl_uint nplatforms = 0;

    err = clGetPlatformIDs(1, &platform, &nplatforms);
    if (err != CL_SUCCESS)
        die_cl("clGetPlatformIDs", err);

    cl_device_id device;
    cl_uint ndevices = 0;

    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, &ndevices);
    if (err != CL_SUCCESS)
        die_cl("clGetDeviceIDs", err);

    char platform_name[256] = {0};
    char device_name[512] = {0};

    clGetPlatformInfo(platform, CL_PLATFORM_NAME,
                      sizeof(platform_name), platform_name, NULL);

    clGetDeviceInfo(device, CL_DEVICE_NAME,
                    sizeof(device_name), device_name, NULL);

    printf("Platform: %s\n", platform_name);
    printf("Device:   %s\n", device_name);

    cl_context context =
        clCreateContext(NULL, 1, &device, NULL, NULL, &err);

    if (err != CL_SUCCESS)
        die_cl("clCreateContext", err);

    cl_command_queue queue =
        clCreateCommandQueueWithProperties(context, device, NULL, &err);

    if (err != CL_SUCCESS)
        die_cl("clCreateCommandQueueWithProperties", err);

    size_t source_size = 0;
    char *source = read_text("probes/add1.cl", &source_size);

    const char *sources[] = {source};

    cl_program program =
        clCreateProgramWithSource(context, 1, sources, &source_size, &err);

    if (err != CL_SUCCESS)
        die_cl("clCreateProgramWithSource", err);

    err = clBuildProgram(program, 1, &device, NULL, NULL, NULL);

    if (err != CL_SUCCESS) {
        size_t log_size = 0;

        clGetProgramBuildInfo(program, device,
                              CL_PROGRAM_BUILD_LOG,
                              0, NULL, &log_size);

        char *log = calloc(1, log_size + 1);

        if (log) {
            clGetProgramBuildInfo(program, device,
                                  CL_PROGRAM_BUILD_LOG,
                                  log_size, log, NULL);

            fprintf(stderr, "Build log:\n%s\n", log);
            free(log);
        }

        die_cl("clBuildProgram", err);
    }

    free(source);

    cl_kernel kernel =
        clCreateKernel(program, "add1", &err);

    if (err != CL_SUCCESS)
        die_cl("clCreateKernel", err);

    enum { N = 8 };

    int input[N]  = {0, 1, 2, 3, 4, 5, 6, 7};
    int output[N] = {0};

    cl_mem in_buf =
        clCreateBuffer(context,
                       CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                       sizeof(input),
                       input,
                       &err);

    if (err != CL_SUCCESS)
        die_cl("clCreateBuffer(input)", err);

    cl_mem out_buf =
        clCreateBuffer(context,
                       CL_MEM_WRITE_ONLY,
                       sizeof(output),
                       NULL,
                       &err);

    if (err != CL_SUCCESS)
        die_cl("clCreateBuffer(output)", err);

    err = clSetKernelArg(kernel, 0, sizeof(in_buf), &in_buf);
    if (err != CL_SUCCESS)
        die_cl("clSetKernelArg(0)", err);

    err = clSetKernelArg(kernel, 1, sizeof(out_buf), &out_buf);
    if (err != CL_SUCCESS)
        die_cl("clSetKernelArg(1)", err);

    size_t global = N;

    err = clEnqueueNDRangeKernel(queue,
                                 kernel,
                                 1,
                                 NULL,
                                 &global,
                                 NULL,
                                 0,
                                 NULL,
                                 NULL);

    if (err != CL_SUCCESS)
        die_cl("clEnqueueNDRangeKernel", err);

    err = clEnqueueReadBuffer(queue,
                              out_buf,
                              CL_TRUE,
                              0,
                              sizeof(output),
                              output,
                              0,
                              NULL,
                              NULL);

    if (err != CL_SUCCESS)
        die_cl("clEnqueueReadBuffer", err);

    int ok = 1;

    for (int i = 0; i < N; i++) {
        printf("%d -> %d\n", input[i], output[i]);

        if (output[i] != input[i] + 1)
            ok = 0;
    }

    printf("\nSOURCE SMOKE RESULT: %s\n", ok ? "PASS" : "FAIL");

    clReleaseMemObject(out_buf);
    clReleaseMemObject(in_buf);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);

    return ok ? 0 : 1;
}
