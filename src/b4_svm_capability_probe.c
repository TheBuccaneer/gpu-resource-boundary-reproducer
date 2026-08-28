#define CL_TARGET_OPENCL_VERSION 300
#include <CL/cl.h>
#include <dlfcn.h>
#include <stdio.h>

static void symbol(void *h, const char *name)
{
    printf("%s=%s\n", name, dlsym(h, name) ? "present" : "missing");
}

int main(void)
{
    setbuf(stdout, NULL);

    cl_int err;
    cl_uint np = 0;

    err = clGetPlatformIDs(0, NULL, &np);
    printf("clGetPlatformIDs=%d platforms=%u\n", err, np);
    if (err != CL_SUCCESS || np == 0)
        return 2;

    cl_platform_id platform = NULL;
    err = clGetPlatformIDs(1, &platform, NULL);
    if (err != CL_SUCCESS)
        return 3;

    cl_uint nd = 0;
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 0, NULL, &nd);
    printf("clGetDeviceIDs=%d devices=%u\n", err, nd);
    if (err != CL_SUCCESS || nd == 0)
        return 4;

    cl_device_id dev = NULL;
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 1, &dev, NULL);
    if (err != CL_SUCCESS)
        return 5;

    char text[512] = {0};

    err = clGetDeviceInfo(dev, CL_DEVICE_NAME,
                          sizeof(text), text, NULL);
    printf("CL_DEVICE_NAME=%s err=%d\n", text, err);

    err = clGetDeviceInfo(dev, CL_DEVICE_VERSION,
                          sizeof(text), text, NULL);
    printf("CL_DEVICE_VERSION=%s err=%d\n", text, err);

    cl_uint address_bits = 0;
    err = clGetDeviceInfo(dev, CL_DEVICE_ADDRESS_BITS,
                          sizeof(address_bits), &address_bits, NULL);
    printf("CL_DEVICE_ADDRESS_BITS=%u err=%d\n",
           address_bits, err);

    cl_device_svm_capabilities caps = 0;
    err = clGetDeviceInfo(dev, CL_DEVICE_SVM_CAPABILITIES,
                          sizeof(caps), &caps, NULL);

    printf("CL_DEVICE_SVM_CAPABILITIES=0x%llx err=%d\n",
           (unsigned long long)caps, err);

    printf("SVM_COARSE_GRAIN_BUFFER=%d\n",
           !!(caps & CL_DEVICE_SVM_COARSE_GRAIN_BUFFER));

    printf("SVM_FINE_GRAIN_BUFFER=%d\n",
           !!(caps & CL_DEVICE_SVM_FINE_GRAIN_BUFFER));

    printf("SVM_FINE_GRAIN_SYSTEM=%d\n",
           !!(caps & CL_DEVICE_SVM_FINE_GRAIN_SYSTEM));

    printf("SVM_ATOMICS=%d\n",
           !!(caps & CL_DEVICE_SVM_ATOMICS));

    void *ocl = dlopen("libOpenCL.so.1", RTLD_NOW | RTLD_LOCAL);
    printf("dlopen_libOpenCL=%s\n", ocl ? "success" : "failed");

    if (ocl) {
        symbol(ocl, "clSVMAlloc");
        symbol(ocl, "clSVMFree");
        symbol(ocl, "clSetKernelArgSVMPointer");
        symbol(ocl, "clEnqueueSVMMap");
        symbol(ocl, "clEnqueueSVMUnmap");
        symbol(ocl, "clEnqueueSVMMemcpy");
        symbol(ocl, "clEnqueueSVMMemFill");
        symbol(ocl, "clEnqueueSVMFree");
        dlclose(ocl);
    }

    if (err != CL_SUCCESS)
        return 6;

    printf("B4-C0=PASS\n");
    return 0;
}
