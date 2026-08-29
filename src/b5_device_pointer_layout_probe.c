#define CL_TARGET_OPENCL_VERSION 300

#include <CL/cl.h>
#include <CL/cl_ext.h>

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

#define BYTES 4096

static int get_addr(
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

    cl_mem a = NULL;
    cl_mem b = NULL;

    err = clGetPlatformIDs(0, NULL, &np);
    printf("clGetPlatformIDs=%d platforms=%u\n", err, np);
    if (err != CL_SUCCESS || !np)
        return 2;

    err = clGetPlatformIDs(1, &platform, NULL);
    printf("clGetPlatformIDs(select)=%d\n", err);
    if (err != CL_SUCCESS)
        return 3;

    err = clGetDeviceIDs(
        platform,
        CL_DEVICE_TYPE_ALL,
        1,
        &dev,
        NULL);

    printf("clGetDeviceIDs=%d\n", err);
    if (err != CL_SUCCESS)
        return 4;

    ctx = clCreateContext(
        NULL, 1, &dev,
        NULL, NULL, &err);

    printf("clCreateContext=%d\n", err);
    if (!ctx || err != CL_SUCCESS)
        return 5;

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
    if (!a || err != CL_SUCCESS)
        return 6;

    b = clCreateBufferWithProperties(
        ctx,
        props,
        CL_MEM_READ_WRITE,
        BYTES,
        NULL,
        &err);

    printf("create_B=%d\n", err);
    if (!b || err != CL_SUCCESS)
        return 7;

    cl_mem_device_address_ext addr_a = 0;
    cl_mem_device_address_ext addr_b = 0;

    err = get_addr(a, &addr_a);
    printf("query_A=%d\n", err);
    if (err != CL_SUCCESS)
        return 8;

    err = get_addr(b, &addr_b);
    printf("query_B=%d\n", err);
    if (err != CL_SUCCESS)
        return 9;

    uint64_t A = (uint64_t)addr_a;
    uint64_t B = (uint64_t)addr_b;
    uint64_t A_end = A + BYTES;

    printf("A_base=0x%" PRIx64 "\n", A);
    printf("A_end=0x%" PRIx64 "\n", A_end);

    printf("B_base=0x%" PRIx64 "\n", B);
    printf("B_end=0x%" PRIx64 "\n", B + BYTES);

    if (B >= A) {
        printf("B_minus_A=%" PRIu64 "\n", B - A);
        printf("B_minus_A_end=%" PRIu64 "\n",
               B >= A_end ? B - A_end : 0);
    } else {
        printf("A_minus_B=%" PRIu64 "\n", A - B);
    }

    printf("B_equals_A_end=%d\n",
           B == A_end);

    printf("A_equals_B_end=%d\n",
           A == B + BYTES);

    printf("same_address=%d\n",
           A == B);

    clReleaseMemObject(b);
    clReleaseMemObject(a);
    clReleaseContext(ctx);

    printf("B5-C5a=PASS\n");
    return 0;
}
