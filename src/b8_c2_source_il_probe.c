#define CL_TARGET_OPENCL_VERSION 300
#include <CL/cl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_file(const char *path, size_t *size_out)
{
    FILE *f = fopen(path, "rb");
    if (!f)
        return NULL;

    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    rewind(f);

    char *buf = malloc((size_t)n + 1);
    if (!buf) {
        fclose(f);
        return NULL;
    }

    if (fread(buf, 1, (size_t)n, f) != (size_t)n) {
        free(buf);
        fclose(f);
        return NULL;
    }

    buf[n] = '\0';
    fclose(f);
    *size_out = (size_t)n;
    return buf;
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "usage: %s source|il LOCAL_BYTES\n", argv[0]);
        return 2;
    }

    const char *mode = argv[1];
    if (strcmp(mode, "source") != 0 && strcmp(mode, "il") != 0)
        return 2;

    size_t local_bytes = (size_t)strtoull(argv[2], NULL, 0);
    cl_int err;

    cl_platform_id platform;
    cl_device_id device;

    err = clGetPlatformIDs(1, &platform, NULL);
    printf("clGetPlatformIDs=%d\n", err);
    if (err != CL_SUCCESS) return 1;

    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    printf("clGetDeviceIDs=%d\n", err);
    if (err != CL_SUCCESS) return 1;

    cl_context context =
        clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    printf("clCreateContext=%d\n", err);
    if (err != CL_SUCCESS) return 1;

    cl_command_queue queue =
        clCreateCommandQueueWithProperties(context, device, NULL, &err);
    printf("clCreateCommandQueueWithProperties=%d\n", err);
    if (err != CL_SUCCESS) return 1;

    cl_int initial = 0;

    cl_mem out =
        clCreateBuffer(context,
                       CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
                       sizeof(initial),
                       &initial,
                       &err);
    printf("clCreateBuffer=%d\n", err);
    if (err != CL_SUCCESS) return 1;

    const char *path = strcmp(mode, "source") == 0
        ? "probes/dynamic_local.cl"
        : "artifacts/b8_c2_dynamic_local.spv";

    size_t src_size;
    char *src = read_file(path, &src_size);
    if (!src) {
        fprintf(stderr, "read_file failed: %s\n", path);
        return 1;
    }

    cl_program program;
    if (strcmp(mode, "source") == 0) {
        const char *sources[] = { src };
        program = clCreateProgramWithSource(
            context, 1, sources, &src_size, &err);
        printf("clCreateProgramWithSource=%d\n", err);
    } else {
        program = clCreateProgramWithIL(
            context, src, src_size, &err);
        printf("clCreateProgramWithIL=%d\n", err);
    }

    if (err != CL_SUCCESS) {
        free(src);
        return 1;
    }

    err = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    printf("clBuildProgram=%d\n", err);

    if (err != CL_SUCCESS) {
        size_t n = 0;
        clGetProgramBuildInfo(program, device,
                              CL_PROGRAM_BUILD_LOG, 0, NULL, &n);
        char *log = calloc(1, n + 1);
        clGetProgramBuildInfo(program, device,
                              CL_PROGRAM_BUILD_LOG, n, log, NULL);
        fprintf(stderr, "%s\n", log);
        free(log);
        return 1;
    }

    cl_kernel kernel =
        clCreateKernel(program, "dynamic_local", &err);
    printf("clCreateKernel=%d\n", err);
    if (err != CL_SUCCESS) return 1;

    err = clSetKernelArg(kernel, 0, local_bytes, NULL);
    printf("clSetKernelArg(local,%zu)=%d\n", local_bytes, err);
    if (err != CL_SUCCESS) return 3;

    err = clSetKernelArg(kernel, 1, sizeof(out), &out);
    printf("clSetKernelArg(out)=%d\n", err);
    if (err != CL_SUCCESS) return 1;

    cl_ulong reported_local = 0;
    err = clGetKernelWorkGroupInfo(kernel, device,
                                   CL_KERNEL_LOCAL_MEM_SIZE,
                                   sizeof(reported_local),
                                   &reported_local, NULL);
    printf("CL_KERNEL_LOCAL_MEM_SIZE=%llu query_err=%d\n",
           (unsigned long long)reported_local, err);
    if (err != CL_SUCCESS) return 1;


    cl_ulong device_limit = 0;
    err = clGetDeviceInfo(device, CL_DEVICE_LOCAL_MEM_SIZE,
                          sizeof(device_limit), &device_limit, NULL);
    if (err != CL_SUCCESS) return 1;

    if ((cl_ulong)local_bytes > device_limit ||
        reported_local > device_limit) {
        printf("B8-C5 PRE_DISPATCH_ONLY mode=%s requested=%zu "
               "reported=%llu limit=%llu\n",
               mode, local_bytes,
               (unsigned long long)reported_local,
               (unsigned long long)device_limit);

        clReleaseKernel(kernel);
        clReleaseProgram(program);
        clReleaseMemObject(out);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        free(src);
        return 0;
    }

    size_t global = 64;
    size_t local = 64;

    err = clEnqueueNDRangeKernel(queue, kernel, 1,
                                 NULL, &global, &local,
                                 0, NULL, NULL);
    printf("clEnqueueNDRangeKernel=%d\n", err);
    if (err != CL_SUCCESS) return 4;

    err = clFinish(queue);
    printf("clFinish=%d\n", err);
    if (err != CL_SUCCESS) return 5;

    cl_int result = 0;
    err = clEnqueueReadBuffer(queue, out, CL_TRUE,
                              0, sizeof(result), &result,
                              0, NULL, NULL);
    printf("clEnqueueReadBuffer=%d\n", err);
    printf("result=0x%08x\n", (unsigned)result);

    if (err != CL_SUCCESS || (unsigned)result != 0x12345678u)
        return 6;

    printf("B8-C2 mode=%s local_bytes=%zu RESULT=PASS\n", mode, local_bytes);

    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseMemObject(out);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    free(src);

    return 0;
}
