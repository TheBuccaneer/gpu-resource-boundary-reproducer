#define CL_TARGET_OPENCL_VERSION 300

#include <CL/cl.h>
#include <CL/cl_ext.h>

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define BYTES 4096

typedef cl_int (CL_API_CALL *pfn_set_device_ptr_t)(
    cl_kernel,
    cl_uint,
    cl_mem_device_address_ext);

static const char *src =
"__kernel void foreign_read(__global uint *p, __global uint *out)\n"
"{\n"
"    if (get_global_id(0) == 0) {\n"
"        out[0] = p[0];\n"
"        out[1] = p[1];\n"
"        out[2] = p[2];\n"
"        out[3] = p[3];\n"
"    }\n"
"}\n";

int main(void)
{
    setbuf(stdout, NULL);

    cl_int err;
    cl_uint np = 0;

    cl_platform_id platform = NULL;
    cl_device_id dev = NULL;

    cl_context ctxA = NULL;
    cl_context ctxB = NULL;

    cl_command_queue qA = NULL;
    cl_command_queue qB = NULL;

    cl_mem targetB = NULL;
    cl_mem outA = NULL;

    cl_program progA = NULL;
    cl_kernel kernelA = NULL;
    cl_event ev = NULL;

    err = clGetPlatformIDs(0, NULL, &np);
    printf("clGetPlatformIDs=%d platforms=%u\n", err, np);
    if (err != CL_SUCCESS || !np) return 2;

    err = clGetPlatformIDs(1, &platform, NULL);
    printf("clGetPlatformIDs(select)=%d\n", err);
    if (err != CL_SUCCESS) return 3;

    err = clGetDeviceIDs(
        platform,
        CL_DEVICE_TYPE_ALL,
        1,
        &dev,
        NULL);

    printf("clGetDeviceIDs=%d\n", err);
    if (err != CL_SUCCESS) return 4;

    pfn_set_device_ptr_t set_device_ptr =
        (pfn_set_device_ptr_t)
        clGetExtensionFunctionAddressForPlatform(
            platform,
            "clSetKernelArgDevicePointerEXT");

    printf("clSetKernelArgDevicePointerEXT_present=%d\n",
           set_device_ptr != NULL);

    if (!set_device_ptr) return 5;

    /*
     * Two genuinely distinct OpenCL contexts on the same device.
     */
    ctxA = clCreateContext(
        NULL, 1, &dev,
        NULL, NULL, &err);

    printf("create_ctxA=%d\n", err);
    if (!ctxA || err != CL_SUCCESS) return 6;

    ctxB = clCreateContext(
        NULL, 1, &dev,
        NULL, NULL, &err);

    printf("create_ctxB=%d\n", err);
    if (!ctxB || err != CL_SUCCESS) return 7;

    printf("contexts_distinct=%d\n", ctxA != ctxB);

    qA = clCreateCommandQueueWithProperties(
        ctxA, dev, NULL, &err);

    printf("create_qA=%d\n", err);
    if (!qA || err != CL_SUCCESS) return 8;

    qB = clCreateCommandQueueWithProperties(
        ctxB, dev, NULL, &err);

    printf("create_qB=%d\n", err);
    if (!qB || err != CL_SUCCESS) return 9;

    /*
     * ONLY ctxB owns a device-private-address buffer.
     * ctxA deliberately owns no BDA buffer.
     */
    const cl_mem_properties props[] = {
        CL_MEM_DEVICE_PRIVATE_ADDRESS_EXT,
        CL_TRUE,
        0
    };

    targetB = clCreateBufferWithProperties(
        ctxB,
        props,
        CL_MEM_READ_WRITE,
        BYTES,
        NULL,
        &err);

    printf("create_targetB=%d\n", err);
    if (!targetB || err != CL_SUCCESS) return 10;

    const uint32_t marker[4] = {
        0xc0dec0deu,
        0x13579bdfu,
        0x2468ace0u,
        0xfeedfaceu
    };

    err = clEnqueueWriteBuffer(
        qB,
        targetB,
        CL_TRUE,
        0,
        sizeof(marker),
        marker,
        0,
        NULL,
        NULL);

    printf("write_targetB=%d\n", err);
    if (err != CL_SUCCESS) return 11;

    err = clFinish(qB);

    printf("finish_targetB_init=%d\n", err);
    if (err != CL_SUCCESS) return 12;

    cl_mem_device_address_ext foreign_addr = 0;

    err = clGetMemObjectInfo(
        targetB,
        CL_MEM_DEVICE_ADDRESS_EXT,
        sizeof(foreign_addr),
        &foreign_addr,
        NULL);

    printf("query_targetB_address=%d\n", err);
    printf("ctxB_target_address=0x%" PRIx64 "\n",
           (uint64_t)foreign_addr);

    if (err != CL_SUCCESS || foreign_addr == 0)
        return 13;

    /*
     * Normal output buffer belonging ONLY to ctxA.
     */
    uint32_t out_init[4] = {
        0xdeadbeefu,
        0xdeadbeefu,
        0xdeadbeefu,
        0xdeadbeefu
    };

    outA = clCreateBuffer(
        ctxA,
        CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
        sizeof(out_init),
        out_init,
        &err);

    printf("create_outA=%d\n", err);
    if (!outA || err != CL_SUCCESS) return 14;

    progA = clCreateProgramWithSource(
        ctxA,
        1,
        &src,
        NULL,
        &err);

    printf("create_programA=%d\n", err);
    if (!progA || err != CL_SUCCESS) return 15;

    err = clBuildProgram(
        progA,
        1,
        &dev,
        NULL,
        NULL,
        NULL);

    printf("build_programA=%d\n", err);

    if (err != CL_SUCCESS) {
        size_t n = 0;

        clGetProgramBuildInfo(
            progA,
            dev,
            CL_PROGRAM_BUILD_LOG,
            0,
            NULL,
            &n);

        if (n) {
            char *log = malloc(n + 1);

            if (log) {
                clGetProgramBuildInfo(
                    progA,
                    dev,
                    CL_PROGRAM_BUILD_LOG,
                    n,
                    log,
                    NULL);

                log[n] = '\0';
                printf("build_log:\n%s\n", log);
                free(log);
            }
        }

        return 16;
    }

    kernelA = clCreateKernel(
        progA,
        "foreign_read",
        &err);

    printf("create_kernelA=%d\n", err);
    if (!kernelA || err != CL_SUCCESS) return 17;

    /*
     * Critical step:
     *
     * foreign_addr belongs to ctxB.
     * kernelA belongs to ctxA.
     */
    err = set_device_ptr(
        kernelA,
        0,
        foreign_addr);

    printf("set_foreign_device_pointer=%d\n", err);

    if (err != CL_SUCCESS) {
        printf("B5-C6b=CONTROLLED_API_REJECTION\n");
        return 20;
    }

    err = clSetKernelArg(
        kernelA,
        1,
        sizeof(outA),
        &outA);

    printf("set_outA=%d\n", err);
    if (err != CL_SUCCESS) return 18;

    size_t global = 1;

    printf("about_to_cross_context_dereference=1\n");

    err = clEnqueueNDRangeKernel(
        qA,
        kernelA,
        1,
        NULL,
        &global,
        NULL,
        0,
        NULL,
        &ev);

    printf("enqueue_kernelA=%d event=%s\n",
           err,
           ev ? "yes" : "no");

    if (err != CL_SUCCESS) {
        printf("B5-C6b=CONTROLLED_ENQUEUE_FAILURE\n");
        return 21;
    }

    err = clWaitForEvents(1, &ev);

    printf("wait_kernelA=%d\n", err);

    if (err != CL_SUCCESS) {
        printf("B5-C6b=CONTROLLED_EVENT_FAILURE\n");
        return 22;
    }

    cl_int event_status = -999;

    err = clGetEventInfo(
        ev,
        CL_EVENT_COMMAND_EXECUTION_STATUS,
        sizeof(event_status),
        &event_status,
        NULL);

    printf("event_status=%d query_err=%d\n",
           event_status,
           err);

    uint32_t observed[4] = {0};

    err = clEnqueueReadBuffer(
        qA,
        outA,
        CL_TRUE,
        0,
        sizeof(observed),
        observed,
        0,
        NULL,
        NULL);

    printf("read_outA=%d\n", err);

    if (err == CL_SUCCESS) {
        for (int i = 0; i < 4; i++) {
            printf(
                "observed_%d=0x%08x expected_ctxB=0x%08x\n",
                i,
                observed[i],
                marker[i]);
        }
    }

    /*
     * Direct ctxB readback control.
     */
    uint32_t verifyB[4] = {0};

    cl_int berr = clEnqueueReadBuffer(
        qB,
        targetB,
        CL_TRUE,
        0,
        sizeof(verifyB),
        verifyB,
        0,
        NULL,
        NULL);

    printf("direct_ctxB_read=%d\n", berr);

    if (berr == CL_SUCCESS) {
        for (int i = 0; i < 4; i++)
            printf(
                "ctxB_control_%d=0x%08x\n",
                i,
                verifyB[i]);
    }

    int leaked = 1;

    for (int i = 0; i < 4; i++) {
        if (observed[i] != marker[i])
            leaked = 0;
    }

    if (ev)
        clReleaseEvent(ev);

    clReleaseKernel(kernelA);
    clReleaseProgram(progA);

    clReleaseMemObject(outA);
    clReleaseMemObject(targetB);

    clReleaseCommandQueue(qB);
    clReleaseCommandQueue(qA);

    clReleaseContext(ctxB);
    clReleaseContext(ctxA);

    if (err != CL_SUCCESS) {
        printf("B5-C6b=POST_EXEC_FAILURE\n");
        return 23;
    }

    if (leaked) {
        printf("B5-C6b=CROSS_CONTEXT_READ_CONFIRMED\n");
        return 40;
    }

    printf("B5-C6b=NO_CROSS_CONTEXT_MARKER_OBSERVED\n");
    return 0;
}
