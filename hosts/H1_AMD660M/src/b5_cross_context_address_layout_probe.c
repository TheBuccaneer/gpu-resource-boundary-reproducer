#define CL_TARGET_OPENCL_VERSION 300

#include <CL/cl.h>
#include <CL/cl_ext.h>

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

#define NBUF 4
#define BYTES 4096

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

    cl_context ctxA = NULL;
    cl_context ctxB = NULL;

    cl_mem a[NBUF] = {0};
    cl_mem b[NBUF] = {0};

    cl_mem_device_address_ext addrA[NBUF] = {0};
    cl_mem_device_address_ext addrB[NBUF] = {0};

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

    ctxA = clCreateContext(
        NULL, 1, &dev,
        NULL, NULL, &err);

    printf("create_ctxA=%d\n", err);
    if (!ctxA || err != CL_SUCCESS) return 5;

    ctxB = clCreateContext(
        NULL, 1, &dev,
        NULL, NULL, &err);

    printf("create_ctxB=%d\n", err);
    if (!ctxB || err != CL_SUCCESS) return 6;

    printf("contexts_distinct=%d\n", ctxA != ctxB);

    const cl_mem_properties props[] = {
        CL_MEM_DEVICE_PRIVATE_ADDRESS_EXT,
        CL_TRUE,
        0
    };

    /*
     * Alternate allocations between the two contexts.
     * This helps distinguish global from context-local VA assignment.
     */
    for (int i = 0; i < NBUF; i++) {
        a[i] = clCreateBufferWithProperties(
            ctxA,
            props,
            CL_MEM_READ_WRITE,
            BYTES,
            NULL,
            &err);

        printf("create_A%d=%d\n", i, err);
        if (!a[i] || err != CL_SUCCESS) return 10 + i;

        err = get_addr(a[i], &addrA[i]);
        printf("query_A%d=%d\n", i, err);
        if (err != CL_SUCCESS) return 20 + i;

        b[i] = clCreateBufferWithProperties(
            ctxB,
            props,
            CL_MEM_READ_WRITE,
            BYTES,
            NULL,
            &err);

        printf("create_B%d=%d\n", i, err);
        if (!b[i] || err != CL_SUCCESS) return 30 + i;

        err = get_addr(b[i], &addrB[i]);
        printf("query_B%d=%d\n", i, err);
        if (err != CL_SUCCESS) return 40 + i;
    }

    printf("\n=== ADDRESS MAP ===\n");

    for (int i = 0; i < NBUF; i++) {
        printf(
            "A%d=0x%" PRIx64
            " B%d=0x%" PRIx64
            " equal=%d\n",
            i, (uint64_t)addrA[i],
            i, (uint64_t)addrB[i],
            addrA[i] == addrB[i]);
    }

    printf("\n=== DELTAS ===\n");

    for (int i = 1; i < NBUF; i++) {
        printf(
            "A%d_minus_A0=%" PRIu64 "\n",
            i,
            (uint64_t)(addrA[i] - addrA[0]));

        printf(
            "B%d_minus_B0=%" PRIu64 "\n",
            i,
            (uint64_t)(addrB[i] - addrB[0]));
    }

    int all_equal = 1;

    for (int i = 0; i < NBUF; i++) {
        if (addrA[i] != addrB[i])
            all_equal = 0;
    }

    printf("\nall_corresponding_addresses_equal=%d\n",
           all_equal);

    printf("A0_equals_B0=%d\n",
           addrA[0] == addrB[0]);

    printf("A1_equals_B1=%d\n",
           addrA[1] == addrB[1]);

    /*
     * Also check whether any A address numerically aliases
     * a differently-indexed B buffer.
     */
    for (int i = 0; i < NBUF; i++) {
        for (int j = 0; j < NBUF; j++) {
            if (addrA[i] == addrB[j]) {
                printf(
                    "numeric_alias=A%d:B%d"
                    " address=0x%" PRIx64 "\n",
                    i, j, (uint64_t)addrA[i]);
            }
        }
    }

    for (int i = NBUF - 1; i >= 0; i--) {
        clReleaseMemObject(b[i]);
        clReleaseMemObject(a[i]);
    }

    clReleaseContext(ctxB);
    clReleaseContext(ctxA);

    printf("B5-C6a=PASS\n");
    return 0;
}
