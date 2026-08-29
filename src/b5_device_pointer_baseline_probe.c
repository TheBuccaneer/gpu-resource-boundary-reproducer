#define CL_TARGET_OPENCL_VERSION 300

#include <CL/cl.h>
#include <CL/cl_ext.h>

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

typedef cl_int (CL_API_CALL *pfn_set_device_ptr_t)(
    cl_kernel,
    cl_uint,
    cl_mem_device_address_ext);

static const char *src =
"__kernel void raw_rw(__global uint *p, __global uint *out)\n"
"{\n"
"    if (get_global_id(0) == 0) {\n"
"        uint v = p[0];\n"
"        out[0] = v;\n"
"        p[0] = 0x55667788u;\n"
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
        4096,
        NULL,
        &err);

    printf("clCreateBufferWithProperties(raw)=%d\n", err);
    if (!raw_buf || err != CL_SUCCESS) return 8;

    uint32_t init = 0x11223344u;

    err = clEnqueueWriteBuffer(
        q,
        raw_buf,
        CL_TRUE,
        0,
        sizeof(init),
        &init,
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

    cl_mem_device_address_ext addr = 0;

    err = clGetMemObjectInfo(
        raw_buf,
        CL_MEM_DEVICE_ADDRESS_EXT,
        sizeof(addr),
        &addr,
        NULL);

    printf("CL_MEM_DEVICE_ADDRESS_EXT_query=%d\n", err);
    printf("device_address=0x%" PRIx64 "\n",
           (uint64_t)addr);

    if (err != CL_SUCCESS || addr == 0) return 11;

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
        prog, "raw_rw", &err);

    printf("clCreateKernel=%d\n", err);
    if (!kernel || err != CL_SUCCESS) return 14;

    err = set_device_ptr(kernel, 0, addr);

    printf("clSetKernelArgDevicePointerEXT=%d\n", err);
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

    uint32_t out = 0;

    err = clEnqueueReadBuffer(
        q,
        out_buf,
        CL_TRUE,
        0,
        sizeof(out),
        &out,
        0,
        NULL,
        NULL);

    printf("clEnqueueReadBuffer(out)=%d\n", err);
    if (err != CL_SUCCESS) return 19;

    uint32_t after = 0;

    err = clEnqueueReadBuffer(
        q,
        raw_buf,
        CL_TRUE,
        0,
        sizeof(after),
        &after,
        0,
        NULL,
        NULL);

    printf("clEnqueueReadBuffer(raw_after)=%d\n", err);
    if (err != CL_SUCCESS) return 20;

    printf("expected_read=0x11223344\n");
    printf("observed_read=0x%08x\n", out);

    printf("expected_after=0x55667788\n");
    printf("observed_after=0x%08x\n", after);

    clReleaseEvent(ev);
    clReleaseKernel(kernel);
    clReleaseProgram(prog);
    clReleaseMemObject(out_buf);
    clReleaseMemObject(raw_buf);
    clReleaseCommandQueue(q);
    clReleaseContext(ctx);

    if (out != 0x11223344u) {
        printf("B5-C1=FAIL_READ\n");
        return 30;
    }

    if (after != 0x55667788u) {
        printf("B5-C1=FAIL_WRITE\n");
        return 31;
    }

    printf("B5-C1=PASS\n");
    return 0;
}
