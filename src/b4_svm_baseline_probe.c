#define CL_TARGET_OPENCL_VERSION 300
#include <CL/cl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define N 256

static const char *src =
"__kernel void svm_baseline(__global uint *p, uint n)\n"
"{\n"
"    size_t gid = get_global_id(0);\n"
"    if (gid < n)\n"
"        p[gid] += 1;\n"
"}\n";

int main(void)
{
    setbuf(stdout, NULL);

    cl_int err;
    cl_uint np = 0;
    cl_platform_id platform = NULL;
    cl_device_id dev = NULL;
    cl_context ctx = NULL;
    cl_command_queue q = NULL;
    cl_program prog = NULL;
    cl_kernel kernel = NULL;
    uint32_t *svm = NULL;

    err = clGetPlatformIDs(0, NULL, &np);
    printf("clGetPlatformIDs=%d platforms=%u\n", err, np);
    if (err != CL_SUCCESS || np == 0) return 2;

    err = clGetPlatformIDs(1, &platform, NULL);
    printf("clGetPlatformIDs(select)=%d\n", err);
    if (err != CL_SUCCESS) return 3;

    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 1, &dev, NULL);
    printf("clGetDeviceIDs=%d\n", err);
    if (err != CL_SUCCESS) return 4;

    cl_device_svm_capabilities caps = 0;
    err = clGetDeviceInfo(dev, CL_DEVICE_SVM_CAPABILITIES,
                          sizeof(caps), &caps, NULL);
    printf("CL_DEVICE_SVM_CAPABILITIES=0x%llx err=%d\n",
           (unsigned long long)caps, err);
    if (err != CL_SUCCESS ||
        !(caps & CL_DEVICE_SVM_COARSE_GRAIN_BUFFER))
        return 5;

    ctx = clCreateContext(NULL, 1, &dev, NULL, NULL, &err);
    printf("clCreateContext=%d\n", err);
    if (!ctx || err != CL_SUCCESS) return 6;

    q = clCreateCommandQueueWithProperties(ctx, dev, NULL, &err);
    printf("clCreateCommandQueueWithProperties=%d\n", err);
    if (!q || err != CL_SUCCESS) return 7;

    svm = clSVMAlloc(ctx, CL_MEM_READ_WRITE,
                     N * sizeof(uint32_t), 0);
    printf("clSVMAlloc=%s size=%zu\n",
           svm ? "success" : "failed",
           (size_t)N * sizeof(uint32_t));
    if (!svm) return 8;

    err = clEnqueueSVMMap(q, CL_TRUE, CL_MAP_WRITE,
                          svm, N * sizeof(uint32_t),
                          0, NULL, NULL);
    printf("clEnqueueSVMMap(write)=%d\n", err);
    if (err != CL_SUCCESS) return 9;

    for (uint32_t i = 0; i < N; i++)
        svm[i] = 0x1000u + i;

    printf("host_before_first=0x%08x\n", svm[0]);
    printf("host_before_last=0x%08x\n", svm[N - 1]);

    err = clEnqueueSVMUnmap(q, svm, 0, NULL, NULL);
    printf("clEnqueueSVMUnmap(write)=%d\n", err);
    if (err != CL_SUCCESS) return 10;

    err = clFinish(q);
    printf("clFinish(after_write_unmap)=%d\n", err);
    if (err != CL_SUCCESS) return 11;

    prog = clCreateProgramWithSource(ctx, 1, &src, NULL, &err);
    printf("clCreateProgramWithSource=%d\n", err);
    if (!prog || err != CL_SUCCESS) return 12;

    err = clBuildProgram(prog, 1, &dev, NULL, NULL, NULL);
    printf("clBuildProgram=%d\n", err);

    if (err != CL_SUCCESS) {
        size_t n = 0;
        clGetProgramBuildInfo(prog, dev, CL_PROGRAM_BUILD_LOG,
                              0, NULL, &n);
        char *log = calloc(1, n + 1);
        if (log) {
            clGetProgramBuildInfo(prog, dev, CL_PROGRAM_BUILD_LOG,
                                  n, log, NULL);
            printf("BUILD_LOG:\n%s\n", log);
            free(log);
        }
        return 13;
    }

    kernel = clCreateKernel(prog, "svm_baseline", &err);
    printf("clCreateKernel=%d\n", err);
    if (!kernel || err != CL_SUCCESS) return 14;

    err = clSetKernelArgSVMPointer(kernel, 0, svm);
    printf("clSetKernelArgSVMPointer=%d\n", err);
    if (err != CL_SUCCESS) return 15;

    cl_uint n_arg = N;
    err = clSetKernelArg(kernel, 1, sizeof(n_arg), &n_arg);
    printf("clSetKernelArg(n)=%d\n", err);
    if (err != CL_SUCCESS) return 16;

    size_t global = N;

    err = clEnqueueNDRangeKernel(q, kernel, 1, NULL,
                                 &global, NULL,
                                 0, NULL, NULL);
    printf("clEnqueueNDRangeKernel=%d\n", err);
    if (err != CL_SUCCESS) return 17;

    err = clFinish(q);
    printf("clFinish(kernel)=%d\n", err);
    if (err != CL_SUCCESS) return 18;

    err = clEnqueueSVMMap(q, CL_TRUE, CL_MAP_READ,
                          svm, N * sizeof(uint32_t),
                          0, NULL, NULL);
    printf("clEnqueueSVMMap(read)=%d\n", err);
    if (err != CL_SUCCESS) return 19;

    printf("host_after_first=0x%08x\n", svm[0]);
    printf("host_after_last=0x%08x\n", svm[N - 1]);

    unsigned mismatches = 0;
    for (uint32_t i = 0; i < N; i++) {
        uint32_t expected = 0x1001u + i;
        if (svm[i] != expected) {
            if (mismatches < 8)
                printf("mismatch[%u]=0x%08x expected=0x%08x\n",
                       i, svm[i], expected);
            mismatches++;
        }
    }

    printf("mismatches=%u\n", mismatches);

    err = clEnqueueSVMUnmap(q, svm, 0, NULL, NULL);
    printf("clEnqueueSVMUnmap(read)=%d\n", err);
    if (err != CL_SUCCESS) return 20;

    err = clFinish(q);
    printf("clFinish(final_unmap)=%d\n", err);
    if (err != CL_SUCCESS) return 21;

    clSVMFree(ctx, svm);
    svm = NULL;
    printf("clSVMFree=done\n");

    clReleaseKernel(kernel);
    clReleaseProgram(prog);
    clReleaseCommandQueue(q);
    clReleaseContext(ctx);

    if (mismatches) {
        printf("B4-C1=FAIL\n");
        return 22;
    }

    printf("B4-C1=PASS\n");
    return 0;
}
