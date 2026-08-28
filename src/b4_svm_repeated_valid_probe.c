#define CL_TARGET_OPENCL_VERSION 300
#include <CL/cl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define N       1024
#define CYCLES  16

static const char *src =
"__kernel void svm_valid(__global uint *p, uint n)\n"
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

    uintptr_t previous_addr = 0;
    unsigned reuse_previous = 0;
    unsigned total_mismatches = 0;

    err = clGetPlatformIDs(0, NULL, &np);
    printf("clGetPlatformIDs=%d platforms=%u\n", err, np);
    if (err != CL_SUCCESS || np == 0)
        return 2;

    err = clGetPlatformIDs(1, &platform, NULL);
    printf("clGetPlatformIDs(select)=%d\n", err);
    if (err != CL_SUCCESS)
        return 3;

    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 1, &dev, NULL);
    printf("clGetDeviceIDs=%d\n", err);
    if (err != CL_SUCCESS)
        return 4;

    ctx = clCreateContext(NULL, 1, &dev, NULL, NULL, &err);
    printf("clCreateContext=%d\n", err);
    if (!ctx || err != CL_SUCCESS)
        return 5;

    q = clCreateCommandQueueWithProperties(ctx, dev, NULL, &err);
    printf("clCreateCommandQueueWithProperties=%d\n", err);
    if (!q || err != CL_SUCCESS)
        return 6;

    prog = clCreateProgramWithSource(ctx, 1, &src, NULL, &err);
    printf("clCreateProgramWithSource=%d\n", err);
    if (!prog || err != CL_SUCCESS)
        return 7;

    err = clBuildProgram(prog, 1, &dev, NULL, NULL, NULL);
    printf("clBuildProgram=%d\n", err);
    if (err != CL_SUCCESS)
        return 8;

    for (unsigned cycle = 0; cycle < CYCLES; cycle++) {

        uint32_t *svm = clSVMAlloc(
            ctx, CL_MEM_READ_WRITE,
            N * sizeof(uint32_t), 0);

        printf("cycle=%u clSVMAlloc=%s",
               cycle, svm ? "success" : "failed");

        if (!svm) {
            printf("\n");
            return 9;
        }

        uintptr_t addr = (uintptr_t)svm;

        printf(" addr=0x%llx",
               (unsigned long long)addr);

        if (previous_addr && addr == previous_addr) {
            printf(" reuse_previous=1");
            reuse_previous++;
        } else {
            printf(" reuse_previous=0");
        }

        printf("\n");

        err = clEnqueueSVMMap(
            q, CL_TRUE, CL_MAP_WRITE,
            svm, N * sizeof(uint32_t),
            0, NULL, NULL);

        printf("cycle=%u map_write=%d\n", cycle, err);
        if (err != CL_SUCCESS)
            return 10;

        for (uint32_t i = 0; i < N; i++)
            svm[i] = 0x10000000u +
                     cycle * 0x10000u + i;

        err = clEnqueueSVMUnmap(q, svm, 0, NULL, NULL);
        printf("cycle=%u unmap_write=%d\n", cycle, err);
        if (err != CL_SUCCESS)
            return 11;

        err = clFinish(q);
        printf("cycle=%u finish_write=%d\n", cycle, err);
        if (err != CL_SUCCESS)
            return 12;

        cl_kernel kernel = clCreateKernel(prog, "svm_valid", &err);
        printf("cycle=%u clCreateKernel=%d\n", cycle, err);
        if (!kernel || err != CL_SUCCESS)
            return 13;

        err = clSetKernelArgSVMPointer(kernel, 0, svm);
        printf("cycle=%u set_svm_ptr=%d\n", cycle, err);
        if (err != CL_SUCCESS)
            return 14;

        cl_uint n_arg = N;
        err = clSetKernelArg(kernel, 1,
                             sizeof(n_arg), &n_arg);
        printf("cycle=%u set_n=%d\n", cycle, err);
        if (err != CL_SUCCESS)
            return 15;

        size_t global = N;

        err = clEnqueueNDRangeKernel(
            q, kernel, 1, NULL,
            &global, NULL,
            0, NULL, NULL);

        printf("cycle=%u enqueue=%d\n", cycle, err);
        if (err != CL_SUCCESS)
            return 16;

        err = clFinish(q);
        printf("cycle=%u finish_kernel=%d\n", cycle, err);
        if (err != CL_SUCCESS)
            return 17;

        /*
         * Release the kernel before freeing SVM so that the
         * valid-lifetime baseline leaves no kernel object holding
         * the pointer after clSVMFree.
         */
        clReleaseKernel(kernel);

        err = clEnqueueSVMMap(
            q, CL_TRUE, CL_MAP_READ,
            svm, N * sizeof(uint32_t),
            0, NULL, NULL);

        printf("cycle=%u map_read=%d\n", cycle, err);
        if (err != CL_SUCCESS)
            return 18;

        unsigned mismatches = 0;

        for (uint32_t i = 0; i < N; i++) {
            uint32_t expected =
                0x10000001u +
                cycle * 0x10000u + i;

            if (svm[i] != expected) {
                if (mismatches < 4)
                    printf(
                        "cycle=%u mismatch[%u]=0x%08x "
                        "expected=0x%08x\n",
                        cycle, i, svm[i], expected);

                mismatches++;
            }
        }

        printf("cycle=%u first=0x%08x last=0x%08x "
               "mismatches=%u\n",
               cycle, svm[0], svm[N - 1], mismatches);

        total_mismatches += mismatches;

        err = clEnqueueSVMUnmap(q, svm, 0, NULL, NULL);
        printf("cycle=%u unmap_read=%d\n", cycle, err);
        if (err != CL_SUCCESS)
            return 19;

        err = clFinish(q);
        printf("cycle=%u finish_final=%d\n", cycle, err);
        if (err != CL_SUCCESS)
            return 20;

        previous_addr = addr;

        clSVMFree(ctx, svm);
        printf("cycle=%u clSVMFree=done\n", cycle);
    }

    printf("cycles=%d\n", CYCLES);
    printf("reuse_previous_count=%u\n", reuse_previous);
    printf("total_mismatches=%u\n", total_mismatches);

    clReleaseProgram(prog);
    clReleaseCommandQueue(q);
    clReleaseContext(ctx);

    if (total_mismatches) {
        printf("B4-C2=FAIL\n");
        return 21;
    }

    printf("B4-C2=PASS\n");
    return 0;
}
