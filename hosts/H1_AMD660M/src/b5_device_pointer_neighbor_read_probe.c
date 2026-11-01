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
"__kernel void raw_read(__global uint *p, __global uint *out)\n"
"{\n"
"    if (get_global_id(0) == 0)\n"
"        out[0] = p[0];\n"
"}\n";

static cl_int get_addr(
    cl_mem mem,
    cl_mem_device_address_ext *addr)
{
    return clGetMemObjectInfo(
        mem,
        CL_MEM_DEVICE_ADDRESS_EXT,
        sizeof(*addr),
        addr,
        NULL);
}

int main(void)
{
    setbuf(stdout, NULL);

    cl_int err;
    cl_uint np = 0;

    cl_platform_id platform = NULL;
    cl_device_id dev = NULL;
    cl_context ctx = NULL;
    cl_command_queue q = NULL;

    cl_mem a = NULL;
    cl_mem b = NULL;
    cl_mem out = NULL;

    cl_program prog = NULL;
    cl_kernel kernel = NULL;
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

    a = clCreateBufferWithProperties(
        ctx,
        props,
        CL_MEM_READ_WRITE,
        BYTES,
        NULL,
        &err);

    printf("create_A=%d\n", err);
    if (!a || err != CL_SUCCESS) return 8;

    b = clCreateBufferWithProperties(
        ctx,
        props,
        CL_MEM_READ_WRITE,
        BYTES,
        NULL,
        &err);

    printf("create_B=%d\n", err);
    if (!b || err != CL_SUCCESS) return 9;

    cl_mem_device_address_ext addr_a = 0;
    cl_mem_device_address_ext addr_b = 0;

    err = get_addr(a, &addr_a);
    printf("query_A=%d\n", err);
    if (err != CL_SUCCESS) return 10;

    err = get_addr(b, &addr_b);
    printf("query_B=%d\n", err);
    if (err != CL_SUCCESS) return 11;

    cl_mem_device_address_ext derived =
        addr_a + BYTES;

    printf("A_base=0x%" PRIx64 "\n",
           (uint64_t)addr_a);

    printf("A_end=0x%" PRIx64 "\n",
           (uint64_t)derived);

    printf("B_base=0x%" PRIx64 "\n",
           (uint64_t)addr_b);

    printf("derived_equals_B_base=%d\n",
           derived == addr_b);

    if (derived != addr_b) {
        printf("B5-C5b=LAYOUT_INCONCLUSIVE\n");
        return 20;
    }

    /*
     * Give the two objects deliberately different data.
     */
    uint32_t a_last = 0xaaaaaaaa;
    uint32_t b_first = 0xcafebabe;

    err = clEnqueueWriteBuffer(
        q, a, CL_TRUE,
        BYTES - sizeof(uint32_t),
        sizeof(a_last),
        &a_last,
        0, NULL, NULL);

    printf("write_A_last=%d\n", err);
    if (err != CL_SUCCESS) return 12;

    err = clEnqueueWriteBuffer(
        q, b, CL_TRUE,
        0,
        sizeof(b_first),
        &b_first,
        0, NULL, NULL);

    printf("write_B_first=%d\n", err);
    if (err != CL_SUCCESS) return 13;

    uint32_t out_init = 0xdeadbeef;

    out = clCreateBuffer(
        ctx,
        CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
        sizeof(out_init),
        &out_init,
        &err);

    printf("create_out=%d\n", err);
    if (!out || err != CL_SUCCESS) return 14;

    prog = clCreateProgramWithSource(
        ctx, 1, &src, NULL, &err);

    printf("create_program=%d\n", err);
    if (!prog || err != CL_SUCCESS) return 15;

    err = clBuildProgram(
        prog, 1, &dev, NULL, NULL, NULL);

    printf("build_program=%d\n", err);
    if (err != CL_SUCCESS) return 16;

    kernel = clCreateKernel(
        prog, "raw_read", &err);

    printf("create_kernel=%d\n", err);
    if (!kernel || err != CL_SUCCESS) return 17;

    /*
     * Deliberately use the value derived as A+BYTES.
     * Numerically this has already been proven equal to B_base.
     */
    err = set_device_ptr(
        kernel,
        0,
        derived);

    printf("set_derived_pointer=%d\n", err);
    if (err != CL_SUCCESS) return 18;

    err = clSetKernelArg(
        kernel,
        1,
        sizeof(out),
        &out);

    printf("set_out=%d\n", err);
    if (err != CL_SUCCESS) return 19;

    size_t global = 1;

    err = clEnqueueNDRangeKernel(
        q, kernel, 1,
        NULL,
        &global,
        NULL,
        0, NULL,
        &ev);

    printf("enqueue_kernel=%d\n", err);
    if (err != CL_SUCCESS) return 21;

    err = clWaitForEvents(1, &ev);

    printf("wait_kernel=%d\n", err);
    if (err != CL_SUCCESS) return 22;

    uint32_t observed = 0;

    err = clEnqueueReadBuffer(
        q,
        out,
        CL_TRUE,
        0,
        sizeof(observed),
        &observed,
        0, NULL, NULL);

    printf("read_out=%d\n", err);
    if (err != CL_SUCCESS) return 23;

    uint32_t verify_b = 0;

    err = clEnqueueReadBuffer(
        q,
        b,
        CL_TRUE,
        0,
        sizeof(verify_b),
        &verify_b,
        0, NULL, NULL);

    printf("read_B_direct=%d\n", err);
    if (err != CL_SUCCESS) return 24;

    printf("A_last_marker=0x%08x\n", a_last);
    printf("B_first_marker=0x%08x\n", b_first);

    printf("derived_pointer_read=0x%08x\n",
           observed);

    printf("B_direct_read=0x%08x\n",
           verify_b);

    clReleaseEvent(ev);
    clReleaseKernel(kernel);
    clReleaseProgram(prog);

    clReleaseMemObject(out);
    clReleaseMemObject(b);
    clReleaseMemObject(a);

    clReleaseCommandQueue(q);
    clReleaseContext(ctx);

    if (observed != 0xcafebabeu) {
        printf("B5-C5b=VALUE_MISMATCH\n");
        return 30;
    }

    if (verify_b != 0xcafebabeu) {
        printf("B5-C5b=B_CONTROL_MISMATCH\n");
        return 31;
    }

    printf("B5-C5b=NEIGHBOR_READ_CONFIRMED\n");
    return 0;
}
