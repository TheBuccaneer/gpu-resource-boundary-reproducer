// B12-T1: valid, unsignaled exportable binary semaphore metadata query.
// No command queue, GPU enqueue, signal, wait, or FD export is performed.
#define CL_TARGET_OPENCL_VERSION 300
#include <CL/cl.h>
#include <CL/cl_ext.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool has_extension(const char *list, const char *name)
{
    size_t n = strlen(name);
    const char *p = list;
    while ((p = strstr(p, name))) {
        if ((p == list || p[-1] == ' ') && (p[n] == ' ' || p[n] == '\0'))
            return true;
        p += n;
    }
    return false;
}

static void print_loaded_opencl_libraries(void)
{
    FILE *maps = fopen("/proc/self/maps", "r");
    if (!maps) {
        puts("provenance_maps=unavailable");
        return;
    }
    char line[1024];
    while (fgets(line, sizeof line, maps)) {
        if (strstr(line, "libOpenCL.so") || strstr(line, "libRusticlOpenCL.so"))
            printf("provenance_map=%s", line);
    }
    fclose(maps);
}

int main(void)
{
    cl_int err;
    cl_uint num_platforms = 0, num_devices = 0;
    cl_platform_id platform = NULL;
    cl_device_id device = NULL;
    cl_context context = NULL;
    cl_semaphore_khr sema = NULL;
    int result = 2;
    char platform_name[128] = {0}, device_name[256] = {0};
    char driver[128] = {0}, extensions[8192] = {0};
    size_t handle_bytes = 0;
    cl_external_semaphore_handle_type_khr device_handles[16] = {0};

    err = clGetPlatformIDs(1, &platform, &num_platforms);
    printf("clGetPlatformIDs=%d platforms=%u\n", err, num_platforms);
    if (err || num_platforms != 1) goto done;
    err = clGetPlatformInfo(platform, CL_PLATFORM_NAME,
                            sizeof platform_name, platform_name, NULL);
    printf("clGetPlatformInfo_name=%d value=%s\n", err, platform_name);
    if (err || strcmp(platform_name, "rusticl")) goto done;
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, &num_devices);
    printf("clGetDeviceIDs=%d gpu_devices=%u\n", err, num_devices);
    if (err || num_devices != 1) goto done;

    err = clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof device_name,
                          device_name, NULL);
    printf("device_name_status=%d value=%s\n", err, device_name);
    if (err) goto done;
    err = clGetDeviceInfo(device, CL_DRIVER_VERSION, sizeof driver,
                          driver, NULL);
    printf("driver_version_status=%d value=%s\n", err, driver);
    if (err) goto done;
    err = clGetDeviceInfo(device, CL_DEVICE_EXTENSIONS, sizeof extensions,
                          extensions, NULL);
    printf("device_extensions_status=%d\n", err);
    if (err) goto done;
    printf("extension_semaphore=%d external_semaphore=%d sync_fd=%d\n",
           has_extension(extensions, "cl_khr_semaphore"),
           has_extension(extensions, "cl_khr_external_semaphore"),
           has_extension(extensions, "cl_khr_external_semaphore_sync_fd"));
    if (!has_extension(extensions, "cl_khr_semaphore") ||
        !has_extension(extensions, "cl_khr_external_semaphore") ||
        !has_extension(extensions, "cl_khr_external_semaphore_sync_fd"))
        goto done;

    err = clGetDeviceInfo(device, CL_DEVICE_SEMAPHORE_EXPORT_HANDLE_TYPES_KHR,
                          0, NULL, &handle_bytes);
    printf("export_device_handle_size_status=%d bytes=%zu\n", err, handle_bytes);
    if (err || handle_bytes == 0 ||
        handle_bytes > sizeof device_handles ||
        handle_bytes % sizeof device_handles[0]) goto done;
    err = clGetDeviceInfo(device, CL_DEVICE_SEMAPHORE_EXPORT_HANDLE_TYPES_KHR,
                          sizeof device_handles, device_handles, NULL);
    printf("export_device_handle_values_status=%d", err);
    if (err) { putchar('\n'); goto done; }
    bool has_sync_fd = false;
    for (size_t i = 0; i < handle_bytes / sizeof device_handles[0]; ++i) {
        printf(" value[%zu]=0x%x", i, (unsigned)device_handles[i]);
        if (device_handles[i] == CL_SEMAPHORE_HANDLE_SYNC_FD_KHR)
            has_sync_fd = true;
    }
    putchar('\n');
    if (!has_sync_fd) goto done;

    clCreateSemaphoreWithPropertiesKHR_fn create =
        (clCreateSemaphoreWithPropertiesKHR_fn)
        clGetExtensionFunctionAddressForPlatform(
            platform, "clCreateSemaphoreWithPropertiesKHR");
    clGetSemaphoreInfoKHR_fn info =
        (clGetSemaphoreInfoKHR_fn)
        clGetExtensionFunctionAddressForPlatform(platform, "clGetSemaphoreInfoKHR");
    clReleaseSemaphoreKHR_fn release =
        (clReleaseSemaphoreKHR_fn)
        clGetExtensionFunctionAddressForPlatform(platform, "clReleaseSemaphoreKHR");
    printf("extension_functions=create:%d info:%d release:%d\n",
           create != NULL, info != NULL, release != NULL);
    print_loaded_opencl_libraries();
    if (!create || !info || !release) goto done;

    cl_context_properties context_props[] = {
        CL_CONTEXT_PLATFORM, (cl_context_properties)platform, 0
    };
    context = clCreateContext(context_props, 1, &device, NULL, NULL, &err);
    printf("clCreateContext=%d object=%d\n", err, context != NULL);
    if (err || !context) goto done;

    const cl_semaphore_properties_khr sema_props[] = {
        CL_SEMAPHORE_TYPE_KHR, CL_SEMAPHORE_TYPE_BINARY_KHR,
        CL_SEMAPHORE_EXPORT_HANDLE_TYPES_KHR, CL_SEMAPHORE_HANDLE_SYNC_FD_KHR,
        CL_SEMAPHORE_EXPORT_HANDLE_TYPES_LIST_END_KHR, 0
    };
    sema = create(context, sema_props, &err);
    printf("clCreateSemaphoreWithPropertiesKHR=%d object=%d\n", err,
           sema != NULL);
    if (err || !sema) goto done;

    size_t query_bytes = (size_t)-1;
    cl_int size_status = info(sema, CL_SEMAPHORE_EXPORT_HANDLE_TYPES_KHR,
                              0, NULL, &query_bytes);
    printf("query_export_types_size_status=%d bytes=%zu\n",
           size_status, query_bytes);

    cl_external_semaphore_handle_type_khr queried_type = 0xdeadbeefU;
    size_t value_bytes = (size_t)-1;
    cl_int value_status = info(sema, CL_SEMAPHORE_EXPORT_HANDLE_TYPES_KHR,
                               sizeof queried_type, &queried_type, &value_bytes);
    printf("query_export_types_value_status=%d bytes=%zu value=0x%x expected=0x%x\n",
           value_status, value_bytes, (unsigned)queried_type,
           (unsigned)CL_SEMAPHORE_HANDLE_SYNC_FD_KHR);

    cl_bool exportable = 0xffffffffU;
    size_t exportable_bytes = (size_t)-1;
    cl_int exportable_status = info(sema, CL_SEMAPHORE_EXPORTABLE_KHR,
                                    sizeof exportable, &exportable,
                                    &exportable_bytes);
    printf("query_exportable_status=%d bytes=%zu value=%u\n",
           exportable_status, exportable_bytes, exportable);

    result = size_status == CL_SUCCESS && value_status == CL_SUCCESS &&
             query_bytes == sizeof queried_type &&
             value_bytes == sizeof queried_type &&
             queried_type == CL_SEMAPHORE_HANDLE_SYNC_FD_KHR &&
             exportable_status == CL_SUCCESS &&
             exportable_bytes == sizeof exportable && exportable == CL_TRUE
             ? 0 : 10;
    printf("oracle=%s\n", result == 0 ? "PASS" : "F1_CANDIDATE");

done:
    if (sema) {
        clReleaseSemaphoreKHR_fn release =
            (clReleaseSemaphoreKHR_fn)
            clGetExtensionFunctionAddressForPlatform(
                platform, "clReleaseSemaphoreKHR");
        cl_int cleanup = release ? release(sema) : CL_INVALID_OPERATION;
        printf("clReleaseSemaphoreKHR=%d\n", cleanup);
        if (cleanup != CL_SUCCESS) result = 2;
    }
    if (context) {
        cl_int cleanup = clReleaseContext(context);
        printf("clReleaseContext=%d\n", cleanup);
        if (cleanup != CL_SUCCESS) result = 2;
    }
    printf("exit=%d\n", result);
    return result;
}
