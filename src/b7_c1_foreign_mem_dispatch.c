#define CL_TARGET_OPENCL_VERSION 300
#include <CL/cl.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static const char *src =
"__kernel void foreign_write(__global uint *out)\n"
"{\n"
"    if (get_global_id(0) == 0)\n"
"        out[0] = 0xB7C10001u;\n"
"}\n";

static void build_log(cl_program p, cl_device_id d)
{
    size_t n = 0;
    clGetProgramBuildInfo(p, d, CL_PROGRAM_BUILD_LOG, 0, NULL, &n);
    char *s = calloc(1, n + 1);
    if (!s) return;
    clGetProgramBuildInfo(p, d, CL_PROGRAM_BUILD_LOG, n, s, NULL);
    fprintf(stderr, "%s\n", s);
    free(s);
}

int main(void)
{
    setbuf(stdout, NULL);

    cl_int err;
    cl_uint np = 0;
    cl_platform_id platform = NULL;
    cl_device_id dev = NULL;

    err = clGetPlatformIDs(0, NULL, &np);
    printf("platform_count err=%d count=%u\n", err, np);
    if (err != CL_SUCCESS || np == 0) return 2;

    err = clGetPlatformIDs(1, &platform, NULL);
    if (err != CL_SUCCESS) return 3;

    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 1, &dev, NULL);
    if (err != CL_SUCCESS) return 4;

    char name[256] = {0};
    clGetDeviceInfo(dev, CL_DEVICE_NAME, sizeof(name), name, NULL);
    printf("device=%s\n", name);

    cl_context ctxA = clCreateContext(NULL, 1, &dev, NULL, NULL, &err);
    printf("ctxA create=%d\n", err);
    if (!ctxA || err != CL_SUCCESS) return 5;

    cl_context ctxB = clCreateContext(NULL, 1, &dev, NULL, NULL, &err);
    printf("ctxB create=%d\n", err);
    if (!ctxB || err != CL_SUCCESS) return 6;

    printf("contexts_distinct=%d\n", ctxA != ctxB);

    cl_command_queue qA =
        clCreateCommandQueueWithProperties(ctxA, dev, NULL, &err);
    printf("qA create=%d\n", err);
    if (!qA || err != CL_SUCCESS) return 7;

    cl_command_queue qB =
        clCreateCommandQueueWithProperties(ctxB, dev, NULL, &err);
    printf("qB create=%d\n", err);
    if (!qB || err != CL_SUCCESS) return 8;

    uint32_t initA = 0xA1A2A3A4u;
    uint32_t initB = 0xB1B2B3B4u;

    cl_mem bufA =
        clCreateBuffer(ctxA, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
                       sizeof(initA), &initA, &err);
    printf("bufA create=%d\n", err);
    if (!bufA || err != CL_SUCCESS) return 9;

    cl_mem bufB =
        clCreateBuffer(ctxB, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
                       sizeof(initB), &initB, &err);
    printf("bufB create=%d\n", err);
    if (!bufB || err != CL_SUCCESS) return 10;

    cl_context memCtxA = NULL, memCtxB = NULL;
    clGetMemObjectInfo(bufA, CL_MEM_CONTEXT,
                       sizeof(memCtxA), &memCtxA, NULL);
    clGetMemObjectInfo(bufB, CL_MEM_CONTEXT,
                       sizeof(memCtxB), &memCtxB, NULL);

    printf("bufA_matches_ctxA=%d\n", memCtxA == ctxA);
    printf("bufB_matches_ctxB=%d\n", memCtxB == ctxB);

    cl_program progB =
        clCreateProgramWithSource(ctxB, 1, &src, NULL, &err);
    printf("progB create=%d\n", err);
    if (!progB || err != CL_SUCCESS) return 11;

    err = clBuildProgram(progB, 1, &dev, "-cl-opt-disable", NULL, NULL);
    printf("progB build=%d\n", err);
    if (err != CL_SUCCESS) {
        build_log(progB, dev);
        return 12;
    }

    cl_kernel kernelB = clCreateKernel(progB, "foreign_write", &err);
    printf("kernelB create=%d\n", err);
    if (!kernelB || err != CL_SUCCESS) return 13;

    /*
     * Positive control: kernel B -> buffer B.
     */
    err = clSetKernelArg(kernelB, 0, sizeof(bufB), &bufB);
    printf("POS set_bufB=%d\n", err);
    if (err != CL_SUCCESS) return 14;

    size_t one = 1;

    err = clEnqueueNDRangeKernel(qB, kernelB, 1, NULL,
                                 &one, &one, 0, NULL, NULL);
    printf("POS enqueue=%d\n", err);
    if (err != CL_SUCCESS) return 15;

    err = clFinish(qB);
    printf("POS finish=%d\n", err);
    if (err != CL_SUCCESS) return 16;

    uint32_t readB = 0;
    err = clEnqueueReadBuffer(qB, bufB, CL_TRUE, 0,
                              sizeof(readB), &readB,
                              0, NULL, NULL);
    printf("POS read=%d value=0x%08x expected=0xB7C10001\n",
           err, readB);

    if (err != CL_SUCCESS || readB != 0xB7C10001u)
        return 17;

    /*
     * Verify foreign target before experiment.
     */
    uint32_t preA = 0;
    err = clEnqueueReadBuffer(qA, bufA, CL_TRUE, 0,
                              sizeof(preA), &preA,
                              0, NULL, NULL);
    printf("FOREIGN pre_read=%d value=0x%08x expected=0xA1A2A3A4\n",
           err, preA);

    if (err != CL_SUCCESS || preA != 0xA1A2A3A4u)
        return 18;

    /*
     * Critical B7-C1 transition:
     * kernel belongs to ctxB, memory object belongs to ctxA.
     */
    cl_int set_foreign =
        clSetKernelArg(kernelB, 0, sizeof(bufA), &bufA);

    printf("B7-C1 set_foreign=%d spec_expected=%d\n",
           set_foreign, CL_INVALID_CONTEXT);

    if (set_foreign != CL_SUCCESS) {
        printf("B7-C1-CLASS=SET_TIME_REJECT\n");
        return 0;
    }

    cl_int enqueue_foreign =
        clEnqueueNDRangeKernel(qB, kernelB, 1, NULL,
                               &one, &one, 0, NULL, NULL);

    printf("B7-C1 enqueue_foreign=%d\n", enqueue_foreign);

    if (enqueue_foreign != CL_SUCCESS) {
        printf("B7-C1-CLASS=DELAYED_ENQUEUE_REJECT\n");
        return 0;
    }

    cl_int finish_foreign = clFinish(qB);

    printf("B7-C1 finish_foreign=%d\n", finish_foreign);

    if (finish_foreign != CL_SUCCESS) {
        printf("B7-C1-CLASS=DELAYED_EXECUTION_FAILURE\n");
        return 0;
    }

    /*
     * qB has completed according to clFinish.
     * Read bufA only through its legitimate ctxA queue.
     */
    uint32_t postA = 0;

    cl_int read_foreign =
        clEnqueueReadBuffer(qA, bufA, CL_TRUE, 0,
                            sizeof(postA), &postA,
                            0, NULL, NULL);

    printf("B7-C1 post_read=%d value=0x%08x marker=0xB7C10001\n",
           read_foreign, postA);

    if (read_foreign == CL_SUCCESS && postA == 0xB7C10001u)
        printf("B7-C1-CLASS=FOREIGN_ACCESS_OBSERVED\n");
    else if (read_foreign == CL_SUCCESS && postA == 0xA1A2A3A4u)
        printf("B7-C1-CLASS=NO_EFFECT_AFTER_SUCCESS\n");
    else
        printf("B7-C1-CLASS=POST_READ_FAILURE_OR_OTHER\n");

    clReleaseKernel(kernelB);
    clReleaseProgram(progB);
    clReleaseMemObject(bufB);
    clReleaseMemObject(bufA);
    clReleaseCommandQueue(qB);
    clReleaseCommandQueue(qA);
    clReleaseContext(ctxB);
    clReleaseContext(ctxA);

    return 0;
}
