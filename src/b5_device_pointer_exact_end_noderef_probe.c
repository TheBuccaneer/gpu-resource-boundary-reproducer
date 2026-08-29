#define CL_TARGET_OPENCL_VERSION 300

#include <CL/cl.h>
#include <CL/cl_ext.h>

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef cl_int (CL_API_CALL *pfn_set_device_ptr_t)(
    cl_kernel,
    cl_uint,
    cl_mem_device_address_ext);

#define BUFFER_BYTES 4096

static const char *src =
"__kernel void accept_invalid(__global uint *p, __global uint *out)\n"
"{\n"
"    /*\n"
"     * Intentionally never dereference p.\n"
"     * cl_ext_buffer_device_address explicitly permits an invalid\n"
"     * pointer value provided the kernel does not dereference it.\n"
"     */\n"
"    if (get_global_id(0) == 0)\n"
"        out[0] = 0x12345678u;\n"
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
    cl_mem raw_buf = NULL;
    cl_mem out_buf = NULL;
    cl_event ev = NULL;

    err = clGetPlatformIDs(0, NULL, &np);
    printf("clGetPlatformIDs=%d platforms=%u\n", err, np);
    if (err != CL_SUCCESS || !np) return 2;

    err = clGetPlatformIDs(1, &platform, NULL);
    printf("clGetPlatformIDs(select)=%d\n", err);
    if (err != CL_SUCCESS) return 3;

    err = clGetDeviceIDs(
        platform, CL_DEVICE_TYPE_ALL, 1, &dev, NULL);

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

    ctx = clCreateContext(
        NULL, 1, &dev, NULL, NULL, &err);

    printf("clCreateContext=%d\n", err);
    if (!ctx || err != CL_SUCCESS) return 6;

    q = clCreateCommandQueueWithProperties(
        ctx, dev, NULL, &err);

    printf("clCreateCommandQueueWithProperties=%d\n", err);
    if (!q || err != CL_SUCCESS) return 7;

    const cl_mem_properties props[] = {
        CL_MEM_DEVICE_PRIVATE_ADDRESS_EXT,
        CL_TRUE,
        0
    };

    raw_buf = clCreateBufferWithProperties(
        ctx,
        props,
        CL_MEM_READ_WRITE,
        BUFFER_BYTES,
        NULL,
        &err);

    printf("clCreateBufferWithProperties(raw)=%d\n", err);
    if (!raw_buf || err != CL_SUCCESS) return 8;

    cl_mem_device_address_ext base = 0;

    err = clGetMemObjectInfo(
        raw_buf,
        CL_MEM_DEVICE_ADDRESS_EXT,
        sizeof(base),
        &base,
        NULL);

    printf("CL_MEM_DEVICE_ADDRESS_EXT_query=%d\n", err);
    printf("base_address=0x%" PRIx64 "\n",
           (uint64_t)base);

    if (err != CL_SUCCESS || base == 0) return 9;

    cl_mem_device_address_ext exact_end =
        base + BUFFER_BYTES;

    printf("buffer_bytes=%d\n", BUFFER_BYTES);
    printf("exact_end_address=0x%" PRIx64 "\n",
           (uint64_t)exact_end);

    printf("distance_from_base=%" PRIu64 "\n",
           (uint64_t)(exact_end - base));

    printf("natural_alignment_uint=%d\n",
           ((uint64_t)exact_end % sizeof(uint32_t)) == 0);

    uint32_t out_init = 0;

    out_buf = clCreateBuffer(
        ctx,
        CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
        sizeof(out_init),
        &out_init,
        &err);

    printf("clCreateBuffer(out)=%d\n", err);
    if (!out_buf || err != CL_SUCCESS) return 10;

    prog = clCreateProgramWithSource(
        ctx, 1, &src, NULL, &err);

    printf("clCreateProgramWithSource=%d\n", err);
    if (!prog || err != CL_SUCCESS) return 11;

    err = clBuildProgram(
        prog, 1, &dev, NULL, NULL, NULL);

    printf("clBuildProgram=%d\n", err);

    if (err != CL_SUCCESS) {
        size_t n = 0;

        clGetProgramBuildInfo(
            prog, dev, CL_PROGRAM_BUILD_LOG,
            0, NULL, &n);

        if (n) {
            char *log = malloc(n + 1);

            if (log) {
                clGetProgramBuildInfo(
                    prog, dev, CL_PROGRAM_BUILD_LOG,
                    n, log, NULL);

                log[n] = '\0';
                printf("build_log:\n%s\n", log);
                free(log);
            }
        }

        return 12;
    }

    kernel = clCreateKernel(
        prog, "accept_invalid", &err);

    printf("clCreateKernel=%d\n", err);
    if (!kernel || err != CL_SUCCESS) return 13;

    err = set_device_ptr(
        kernel, 0, exact_end);

    printf("clSetKernelArgDevicePointerEXT(exact_end)=%d\n",
           err);

    if (err != CL_SUCCESS) {
        printf("B5-C4=STRICT_API_REJECTION\n");
        return 20;
    }

    err = clSetKernelArg(
        kernel, 1, sizeof(out_buf), &out_buf);

    printf("clSetKernelArg(out)=%d\n", err);
    if (err != CL_SUCCESS) return 14;

    size_t global = 1;

    err = clEnqueueNDRangeKernel(
        q,
        kernel,
        1,
        NULL,
        &global,
        NULL,
        0,
        NULL,
        &ev);

    printf("clEnqueueNDRangeKernel=%d\n", err);
    if (err != CL_SUCCESS) return 15;

    err = clWaitForEvents(1, &ev);

    printf("clWaitForEvents=%d\n", err);
    if (err != CL_SUCCESS) return 16;

    uint32_t marker = 0;

    err = clEnqueueReadBuffer(
        q,
        out_buf,
        CL_TRUE,
        0,
        sizeof(marker),
        &marker,
        0,
        NULL,
        NULL);

    printf("clEnqueueReadBuffer(out)=%d\n", err);
    if (err != CL_SUCCESS) return 17;

    printf("expected_marker=0x12345678\n");
    printf("observed_marker=0x%08x\n", marker);

    clReleaseEvent(ev);
    clReleaseKernel(kernel);
    clReleaseProgram(prog);
    clReleaseMemObject(out_buf);
    clReleaseMemObject(raw_buf);
    clReleaseCommandQueue(q);
    clReleaseContext(ctx);

    if (marker != 0x12345678u) {
        printf("B5-C4=FAIL_MARKER\n");
        return 30;
    }

    printf("B5-C4=PASS\n");
    return 0;
}
