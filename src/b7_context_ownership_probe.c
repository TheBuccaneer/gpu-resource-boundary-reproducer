#define CL_TARGET_OPENCL_VERSION 300
#include <CL/cl.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK_OBJ(x, label) \
    do { \
        if (!(x)) { \
            fprintf(stderr, "%s=NULL\n", label); \
            return 90; \
        } \
    } while (0)

static const char *kernel_src =
"__kernel void b7_write(__global uint *out)\n"
"{\n"
"    if (get_global_id(0) == 0)\n"
"        out[0] = 0xB7001234u;\n"
"}\n";

static void print_build_log(cl_program prog, cl_device_id dev)
{
    size_t n = 0;
    clGetProgramBuildInfo(prog, dev, CL_PROGRAM_BUILD_LOG, 0, NULL, &n);

    char *log = calloc(1, n + 1);
    if (!log)
        return;

    clGetProgramBuildInfo(prog, dev, CL_PROGRAM_BUILD_LOG,
                          n, log, NULL);
    fprintf(stderr, "%s\n", log);
    free(log);
}

static int same_ptr(const void *a, const void *b)
{
    return a == b;
}

int main(void)
{
    setbuf(stdout, NULL);

    cl_int err;
    cl_uint np = 0;

    cl_platform_id platform = NULL;
    cl_device_id dev = NULL;

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

    char name[256] = {0};
    clGetDeviceInfo(dev, CL_DEVICE_NAME, sizeof(name), name, NULL);
    printf("device=%s\n", name);

    /*
     * Two independent OpenCL contexts on the same device.
     */
    cl_context ctxA =
        clCreateContext(NULL, 1, &dev, NULL, NULL, &err);
    printf("ctxA create=%d handle=%p\n", err, (void *)ctxA);
    if (!ctxA || err != CL_SUCCESS)
        return 5;

    cl_context ctxB =
        clCreateContext(NULL, 1, &dev, NULL, NULL, &err);
    printf("ctxB create=%d handle=%p\n", err, (void *)ctxB);
    if (!ctxB || err != CL_SUCCESS)
        return 6;

    printf("contexts_distinct=%d\n", ctxA != ctxB);

    cl_command_queue qA =
        clCreateCommandQueueWithProperties(ctxA, dev, NULL, &err);
    printf("qA create=%d\n", err);
    if (!qA || err != CL_SUCCESS)
        return 7;

    cl_command_queue qB =
        clCreateCommandQueueWithProperties(ctxB, dev, NULL, &err);
    printf("qB create=%d\n", err);
    if (!qB || err != CL_SUCCESS)
        return 8;

    uint32_t initA = 0xAAAAAAAAu;
    uint32_t initB = 0xBBBBBBBBu;

    cl_mem bufA =
        clCreateBuffer(ctxA,
                       CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
                       sizeof(initA), &initA, &err);
    printf("bufA create=%d\n", err);
    if (!bufA || err != CL_SUCCESS)
        return 9;

    cl_mem bufB =
        clCreateBuffer(ctxB,
                       CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
                       sizeof(initB), &initB, &err);
    printf("bufB create=%d\n", err);
    if (!bufB || err != CL_SUCCESS)
        return 10;

    /*
     * C0 ownership metadata.
     */
    cl_context memCtxA = NULL, memCtxB = NULL;
    cl_context queueCtxA = NULL, queueCtxB = NULL;

    err = clGetMemObjectInfo(bufA, CL_MEM_CONTEXT,
                             sizeof(memCtxA), &memCtxA, NULL);
    printf("bufA CL_MEM_CONTEXT err=%d matches_ctxA=%d\n",
           err, same_ptr(memCtxA, ctxA));

    err = clGetMemObjectInfo(bufB, CL_MEM_CONTEXT,
                             sizeof(memCtxB), &memCtxB, NULL);
    printf("bufB CL_MEM_CONTEXT err=%d matches_ctxB=%d\n",
           err, same_ptr(memCtxB, ctxB));

    err = clGetCommandQueueInfo(qA, CL_QUEUE_CONTEXT,
                                sizeof(queueCtxA), &queueCtxA, NULL);
    printf("qA CL_QUEUE_CONTEXT err=%d matches_ctxA=%d\n",
           err, same_ptr(queueCtxA, ctxA));

    err = clGetCommandQueueInfo(qB, CL_QUEUE_CONTEXT,
                                sizeof(queueCtxB), &queueCtxB, NULL);
    printf("qB CL_QUEUE_CONTEXT err=%d matches_ctxB=%d\n",
           err, same_ptr(queueCtxB, ctxB));

    /*
     * Build equivalent kernels independently in A and B.
     */
    cl_program progA =
        clCreateProgramWithSource(ctxA, 1, &kernel_src, NULL, &err);
    printf("progA create=%d\n", err);
    CHECK_OBJ(progA, "progA");

    err = clBuildProgram(progA, 1, &dev, "-cl-opt-disable",
                         NULL, NULL);
    printf("progA build=%d\n", err);
    if (err != CL_SUCCESS) {
        print_build_log(progA, dev);
        return 11;
    }

    cl_program progB =
        clCreateProgramWithSource(ctxB, 1, &kernel_src, NULL, &err);
    printf("progB create=%d\n", err);
    CHECK_OBJ(progB, "progB");

    err = clBuildProgram(progB, 1, &dev, "-cl-opt-disable",
                         NULL, NULL);
    printf("progB build=%d\n", err);
    if (err != CL_SUCCESS) {
        print_build_log(progB, dev);
        return 12;
    }

    cl_kernel kernelA = clCreateKernel(progA, "b7_write", &err);
    printf("kernelA create=%d\n", err);
    CHECK_OBJ(kernelA, "kernelA");

    cl_kernel kernelB = clCreateKernel(progB, "b7_write", &err);
    printf("kernelB create=%d\n", err);
    CHECK_OBJ(kernelB, "kernelB");

    cl_context kernelCtxA = NULL, kernelCtxB = NULL;

    err = clGetKernelInfo(kernelA, CL_KERNEL_CONTEXT,
                          sizeof(kernelCtxA), &kernelCtxA, NULL);
    printf("kernelA CL_KERNEL_CONTEXT err=%d matches_ctxA=%d\n",
           err, same_ptr(kernelCtxA, ctxA));

    err = clGetKernelInfo(kernelB, CL_KERNEL_CONTEXT,
                          sizeof(kernelCtxB), &kernelCtxB, NULL);
    printf("kernelB CL_KERNEL_CONTEXT err=%d matches_ctxB=%d\n",
           err, same_ptr(kernelCtxB, ctxB));

    /*
     * Positive same-context baselines.
     */
    uint32_t hostA = 0, hostB = 0;

    err = clEnqueueReadBuffer(qA, bufA, CL_TRUE, 0,
                              sizeof(hostA), &hostA,
                              0, NULL, NULL);
    printf("POS read qA/bufA=%d value=0x%08x\n", err, hostA);

    err = clEnqueueReadBuffer(qB, bufB, CL_TRUE, 0,
                              sizeof(hostB), &hostB,
                              0, NULL, NULL);
    printf("POS read qB/bufB=%d value=0x%08x\n", err, hostB);

    /*
     * B7 negative ownership probes.
     *
     * Expected result for each:
     *
     *     CL_INVALID_CONTEXT (-34)
     */

    uint32_t foreign_read = 0;

    cl_int n1 = clEnqueueReadBuffer(qB, bufA, CL_TRUE, 0,
                                     sizeof(foreign_read), &foreign_read,
                                     0, NULL, NULL);
    printf("B7-N1 foreign_buffer_read err=%d expected=%d\n",
           n1, CL_INVALID_CONTEXT);

    cl_int n2 = clEnqueueWriteBuffer(qB, bufA, CL_TRUE, 0,
                                      sizeof(initB), &initB,
                                      0, NULL, NULL);
    printf("B7-N2 foreign_buffer_write err=%d expected=%d\n",
           n2, CL_INVALID_CONTEXT);

    cl_int n3 = clEnqueueCopyBuffer(qB, bufA, bufB,
                                     0, 0, sizeof(uint32_t),
                                     0, NULL, NULL);
    printf("B7-N3 mixed_context_copy err=%d expected=%d\n",
           n3, CL_INVALID_CONTEXT);

    cl_int n4 = clSetKernelArg(kernelB, 0, sizeof(bufA), &bufA);
    printf("B7-N4 foreign_mem_kernel_arg err=%d expected=%d\n",
           n4, CL_INVALID_CONTEXT);

    /*
     * Restore a valid argument after N4 before testing kernel ownership.
     */
    err = clSetKernelArg(kernelB, 0, sizeof(bufB), &bufB);
    printf("POS kernelB/set_bufB=%d\n", err);
    if (err != CL_SUCCESS)
        return 20;

    size_t one = 1;

    cl_int n5 = clEnqueueNDRangeKernel(qA, kernelB, 1, NULL,
                                        &one, &one, 0, NULL, NULL);
    printf("B7-N5 foreign_kernel_enqueue err=%d expected=%d\n",
           n5, CL_INVALID_CONTEXT);

    /*
     * Final positive kernel path verifies that the failed foreign-context
     * operations did not poison the valid B context.
     */
    err = clEnqueueNDRangeKernel(qB, kernelB, 1, NULL,
                                 &one, &one, 0, NULL, NULL);
    printf("POS kernelB/enqueue_qB=%d\n", err);
    if (err != CL_SUCCESS)
        return 21;

    err = clFinish(qB);
    printf("POS kernelB/finish_qB=%d\n", err);
    if (err != CL_SUCCESS)
        return 22;

    hostB = 0;
    err = clEnqueueReadBuffer(qB, bufB, CL_TRUE, 0,
                              sizeof(hostB), &hostB,
                              0, NULL, NULL);
    printf("POS final_read=%d value=0x%08x expected=0xB7001234\n",
           err, hostB);

    int metadata_pass =
        memCtxA == ctxA &&
        memCtxB == ctxB &&
        queueCtxA == ctxA &&
        queueCtxB == ctxB &&
        kernelCtxA == ctxA &&
        kernelCtxB == ctxB;

    int ownership_pass =
        n1 == CL_INVALID_CONTEXT &&
        n2 == CL_INVALID_CONTEXT &&
        n3 == CL_INVALID_CONTEXT &&
        n4 == CL_INVALID_CONTEXT &&
        n5 == CL_INVALID_CONTEXT;

    int functional_pass =
        hostB == 0xB7001234u;

    int pass =
        metadata_pass &&
        ownership_pass &&
        functional_pass;

    printf("B7-C0-METADATA=%s\n",
           metadata_pass ? "PASS" : "FAIL");
    printf("B7-C0-OWNERSHIP=%s\n",
           ownership_pass ? "PASS" : "FAIL");
    printf("B7-C0-FUNCTIONAL=%s\n",
           functional_pass ? "PASS" : "FAIL");
    printf("B7-C0=%s\n",
           pass ? "PASS" : "FAIL");

    clReleaseKernel(kernelB);
    clReleaseKernel(kernelA);
    clReleaseProgram(progB);
    clReleaseProgram(progA);
    clReleaseMemObject(bufB);
    clReleaseMemObject(bufA);
    clReleaseCommandQueue(qB);
    clReleaseCommandQueue(qA);
    clReleaseContext(ctxB);
    clReleaseContext(ctxA);

    return pass ? 0 : 30;
}
