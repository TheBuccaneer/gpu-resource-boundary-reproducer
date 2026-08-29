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

#define WORDS 1024
#define OFFSET_WORD 4
#define OFFSET_BYTES (OFFSET_WORD * sizeof(uint32_t))

static const char *src =
"__kernel void raw_offset_rw(__global uint *p, __global uint *out)\n"
"{\n"
"    if (get_global_id(0) == 0) {\n"
"        out[0] = p[0];\n"
"        p[0] = 0x66778899u;\n"
"    }\n"
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
        WORDS * sizeof(uint32_t),
        NULL,
        &err);

    printf("clCreateBufferWithProperties(raw)=%d\n", err);
    if (!raw_buf || err != CL_SUCCESS) return 8;

    uint32_t init[WORDS] = {0};

    init[0] = 0x11111111u;
    init[OFFSET_WORD] = 0x22334455u;

    err = clEnqueueWriteBuffer(
        q,
        raw_buf,
        CL_TRUE,
        0,
        sizeof(init),
        init,
        0,
        NULL,
        NULL);

    printf("clEnqueueWriteBuffer(raw_init)=%d\n", err);
    if (err != CL_SUCCESS) return 9;

    uint32_t out_init = 0;

    out_buf = clCreateBuffer(
        ctx,
        CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
        sizeof(out_init),
        &out_init,
        &err);

    printf("clCreateBuffer(out)=%d\n", err);
    if (!out_buf || err != CL_SUCCESS) return 10;

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

    if (err != CL_SUCCESS || base == 0) return 11;

    cl_mem_device_address_ext interior =
        base + OFFSET_BYTES;

    printf("offset_bytes=%zu\n", (size_t)OFFSET_BYTES);
    printf("interior_address=0x%" PRIx64 "\n",
           (uint64_t)interior);

    printf("natural_alignment_uint=%d\n",
           ((uint64_t)interior % sizeof(uint32_t)) == 0);

    prog = clCreateProgramWithSource(
        ctx, 1, &src, NULL, &err);

    printf("clCreateProgramWithSource=%d\n", err);
    if (!prog || err != CL_SUCCESS) return 12;

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

        return 13;
    }

    kernel = clCreateKernel(
        prog, "raw_offset_rw", &err);

    printf("clCreateKernel=%d\n", err);
    if (!kernel || err != CL_SUCCESS) return 14;

    err = set_device_ptr(
        kernel, 0, interior);

    printf("clSetKernelArgDevicePointerEXT(offset)=%d\n", err);
    if (err != CL_SUCCESS) return 15;

    err = clSetKernelArg(
        kernel, 1, sizeof(out_buf), &out_buf);

    printf("clSetKernelArg(out)=%d\n", err);
    if (err != CL_SUCCESS) return 16;

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
    if (err != CL_SUCCESS) return 17;

    err = clWaitForEvents(1, &ev);

    printf("clWaitForEvents=%d\n", err);
    if (err != CL_SUCCESS) return 18;

    uint32_t observed_out = 0;

    err = clEnqueueReadBuffer(
        q,
        out_buf,
        CL_TRUE,
        0,
        sizeof(observed_out),
        &observed_out,
        0,
        NULL,
        NULL);

    printf("clEnqueueReadBuffer(out)=%d\n", err);
    if (err != CL_SUCCESS) return 19;

    uint32_t check[WORDS] = {0};

    err = clEnqueueReadBuffer(
        q,
        raw_buf,
        CL_TRUE,
        0,
        sizeof(check),
        check,
        0,
        NULL,
        NULL);

    printf("clEnqueueReadBuffer(raw_after)=%d\n", err);
    if (err != CL_SUCCESS) return 20;

    printf("expected_read=0x22334455\n");
    printf("observed_read=0x%08x\n",
           observed_out);

    printf("expected_target_after=0x66778899\n");
    printf("observed_target_after=0x%08x\n",
           check[OFFSET_WORD]);

    printf("expected_base_sentinel=0x11111111\n");
    printf("observed_base_sentinel=0x%08x\n",
           check[0]);

    clReleaseEvent(ev);
    clReleaseKernel(kernel);
    clReleaseProgram(prog);
    clReleaseMemObject(out_buf);
    clReleaseMemObject(raw_buf);
    clReleaseCommandQueue(q);
    clReleaseContext(ctx);

    if (observed_out != 0x22334455u) {
        printf("B5-C2=FAIL_OFFSET_READ\n");
        return 30;
    }

    if (check[OFFSET_WORD] != 0x66778899u) {
        printf("B5-C2=FAIL_OFFSET_WRITE\n");
        return 31;
    }

    if (check[0] != 0x11111111u) {
        printf("B5-C2=FAIL_BASE_CORRUPTION\n");
        return 32;
    }

    printf("B5-C2=PASS\n");
    return 0;
}
