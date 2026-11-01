#define CL_TARGET_OPENCL_VERSION 300

#include <CL/cl.h>
#include <CL/cl_ext.h>

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int has_extension(cl_device_id dev, const char *needle)
{
    size_t n = 0;
    if (clGetDeviceInfo(dev, CL_DEVICE_EXTENSIONS, 0, NULL, &n) != CL_SUCCESS)
        return 0;

    char *s = calloc(1, n + 1);
    if (!s)
        return 0;

    if (clGetDeviceInfo(dev, CL_DEVICE_EXTENSIONS, n, s, NULL)
        != CL_SUCCESS) {
        free(s);
        return 0;
    }

    int found = 0;
    const char *p = s;

    while ((p = strstr(p, needle)) != NULL) {
        const char left = (p == s) ? ' ' : p[-1];
        const char right = p[strlen(needle)];

        if ((left == ' ' || left == '\0') &&
            (right == ' ' || right == '\0')) {
            found = 1;
            break;
        }

        p += strlen(needle);
    }

    free(s);
    return found;
}

int main(void)
{
    setbuf(stdout, NULL);

    cl_int err;
    cl_uint np = 0;

    cl_platform_id platform = NULL;
    cl_device_id dev = NULL;
    cl_context ctx = NULL;
    cl_mem buf = NULL;

    err = clGetPlatformIDs(0, NULL, &np);
    printf("clGetPlatformIDs=%d platforms=%u\n", err, np);
    if (err != CL_SUCCESS || np == 0)
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

    char name[256] = {0};
    char version[256] = {0};

    clGetDeviceInfo(
        dev, CL_DEVICE_NAME,
        sizeof(name), name, NULL);

    clGetDeviceInfo(
        dev, CL_DEVICE_VERSION,
        sizeof(version), version, NULL);

    printf("device_name=%s\n", name);
    printf("device_version=%s\n", version);

    cl_uint address_bits = 0;

    err = clGetDeviceInfo(
        dev,
        CL_DEVICE_ADDRESS_BITS,
        sizeof(address_bits),
        &address_bits,
        NULL);

    printf("CL_DEVICE_ADDRESS_BITS=%u query_err=%d\n",
           address_bits, err);

    const char *ext_name =
        "cl_ext_buffer_device_address";

    int advertised = has_extension(dev, ext_name);

    printf("extension_%s=%d\n",
           ext_name, advertised);

    void *fn = clGetExtensionFunctionAddressForPlatform(
        platform,
        "clSetKernelArgDevicePointerEXT");

    printf("clSetKernelArgDevicePointerEXT_address=%p\n", fn);
    printf("clSetKernelArgDevicePointerEXT_present=%d\n",
           fn != NULL);

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

    buf = clCreateBufferWithProperties(
        ctx,
        props,
        CL_MEM_READ_WRITE,
        4096,
        NULL,
        &err);

    printf("clCreateBufferWithProperties(device_private)=%d\n",
           err);

    if (!buf || err != CL_SUCCESS)
        return 6;

    size_t prop_size = 0;

    err = clGetMemObjectInfo(
        buf,
        CL_MEM_PROPERTIES,
        0,
        NULL,
        &prop_size);

    printf("CL_MEM_PROPERTIES_size=%zu query_err=%d\n",
           prop_size, err);

    cl_mem_device_address_ext addr1 = 0;
    cl_mem_device_address_ext addr2 = 0;
    size_t addr_size1 = 0;
    size_t addr_size2 = 0;

    err = clGetMemObjectInfo(
        buf,
        CL_MEM_DEVICE_ADDRESS_EXT,
        sizeof(addr1),
        &addr1,
        &addr_size1);

    printf("device_address_query_1=%d\n", err);
    printf("device_address_1=0x%" PRIx64 "\n",
           (uint64_t)addr1);
    printf("device_address_size_1=%zu\n",
           addr_size1);

    if (err != CL_SUCCESS)
        return 7;

    err = clGetMemObjectInfo(
        buf,
        CL_MEM_DEVICE_ADDRESS_EXT,
        sizeof(addr2),
        &addr2,
        &addr_size2);

    printf("device_address_query_2=%d\n", err);
    printf("device_address_2=0x%" PRIx64 "\n",
           (uint64_t)addr2);
    printf("device_address_size_2=%zu\n",
           addr_size2);

    if (err != CL_SUCCESS)
        return 8;

    printf("device_address_nonzero=%d\n",
           addr1 != 0);

    printf("device_address_stable=%d\n",
           addr1 == addr2);

    printf("sizeof_cl_mem_device_address_ext=%zu\n",
           sizeof(cl_mem_device_address_ext));

    clReleaseMemObject(buf);
    clReleaseContext(ctx);

    if (!advertised) {
        printf("B5-C0=FAIL_EXTENSION_NOT_ADVERTISED\n");
        return 20;
    }

    if (!fn) {
        printf("B5-C0=FAIL_API_NOT_RESOLVED\n");
        return 21;
    }

    if (addr1 == 0) {
        printf("B5-C0=FAIL_ZERO_ADDRESS\n");
        return 22;
    }

    if (addr1 != addr2) {
        printf("B5-C0=FAIL_ADDRESS_UNSTABLE\n");
        return 23;
    }

    printf("B5-C0=PASS\n");
    return 0;
}
