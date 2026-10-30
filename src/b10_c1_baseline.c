#define CL_TARGET_OPENCL_VERSION 300
#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *source =
    "__kernel void b10_add_one(__global uint *words) { words[1] += 1u; }\n";

static int check(cl_int err, const char *step)
{
    printf("%s=%d\n", step, err);
    return err == CL_SUCCESS;
}

static int platform_string(cl_platform_id platform, cl_platform_info key,
                           const char *label)
{
    char value[256] = { 0 };
    cl_int err = clGetPlatformInfo(platform, key, sizeof(value), value, NULL);
    if (!check(err, label)) return 0;
    printf("%s_value=%s\n", label, value);
    return 1;
}

static int device_string(cl_device_id device, cl_device_info key,
                         const char *label)
{
    char value[256] = { 0 };
    cl_int err = clGetDeviceInfo(device, key, sizeof(value), value, NULL);
    if (!check(err, label)) return 0;
    printf("%s_value=%s\n", label, value);
    return 1;
}

static void print_loaded_icd(void)
{
    FILE *maps = fopen("/proc/self/maps", "r");
    if (!maps) {
        perror("/proc/self/maps");
        return;
    }
    char line[1024];
    while (fgets(line, sizeof(line), maps)) {
        if (strstr(line, "libRusticlOpenCL.so")) {
            char *path = strstr(line, "/usr/");
            if (path) printf("loaded_icd=%s", path);
            break;
        }
    }
    fclose(maps);
}

int main(int argc, char **argv)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    if (argc != 2 || strcmp(argv[1], "--run-c1") != 0) {
        fprintf(stderr, "Usage: %s --run-c1\n", argv[0]);
        return 2;
    }

    cl_int err;
    cl_uint nplatforms = 0;
    if (!check(clGetPlatformIDs(0, NULL, &nplatforms), "clGetPlatformIDs(count)")
        || nplatforms != 1) {
        printf("platform_count=%u; expected exactly one\n", nplatforms);
        return 3;
    }
    cl_platform_id platform = NULL;
    if (!check(clGetPlatformIDs(1, &platform, NULL), "clGetPlatformIDs(select)"))
        return 3;
    if (!platform_string(platform, CL_PLATFORM_NAME, "CL_PLATFORM_NAME")
        || !platform_string(platform, CL_PLATFORM_VERSION, "CL_PLATFORM_VERSION"))
        return 3;

    cl_device_id device = NULL;
    if (!check(clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL),
               "clGetDeviceIDs(GPU)"))
        return 4;
    if (!device_string(device, CL_DEVICE_NAME, "CL_DEVICE_NAME")
        || !device_string(device, CL_DEVICE_VERSION, "CL_DEVICE_VERSION")
        || !device_string(device, CL_DRIVER_VERSION, "CL_DRIVER_VERSION"))
        return 4;
    print_loaded_icd();

    cl_command_queue_properties supported = 0;
    if (!check(clGetDeviceInfo(device, CL_DEVICE_QUEUE_ON_HOST_PROPERTIES,
                               sizeof(supported), &supported, NULL),
               "CL_DEVICE_QUEUE_ON_HOST_PROPERTIES"))
        return 4;
    printf("host_queue_properties=0x%llx\n", (unsigned long long)supported);

    cl_context context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    if (!check(err, "clCreateContext") || !context) return 5;
    cl_command_queue queue =
        clCreateCommandQueueWithProperties(context, device, NULL, &err);
    if (!check(err, "clCreateCommandQueueWithProperties") || !queue) return 5;
    cl_command_queue_properties active = ~0UL;
    if (!check(clGetCommandQueueInfo(queue, CL_QUEUE_PROPERTIES,
                                     sizeof(active), &active, NULL),
               "CL_QUEUE_PROPERTIES"))
        return 5;
    printf("active_queue_properties=0x%llx\n", (unsigned long long)active);
    if (active & CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE) {
        fprintf(stderr, "C1 requires an in-order queue\n");
        return 5;
    }

    const cl_uint before[4] = { 0xa5a5a5a5u, 42u, 0x5a5a5a5au, 0x12345678u };
    const cl_uint expected[4] = { 0xa5a5a5a5u, 43u, 0x5a5a5a5au, 0x12345678u };
    cl_uint after[4] = { 0, 0, 0, 0 };
    cl_mem buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                   sizeof(before), NULL, &err);
    if (!check(err, "clCreateBuffer") || !buffer) return 6;
    cl_program program = clCreateProgramWithSource(context, 1, &source,
                                                   NULL, &err);
    if (!check(err, "clCreateProgramWithSource") || !program) return 6;
    err = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    if (!check(err, "clBuildProgram")) {
        char log[4096] = { 0 };
        if (clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG,
                                  sizeof(log), log, NULL) == CL_SUCCESS)
            printf("build_log=%s\n", log);
        return 7;
    }
    cl_kernel kernel = clCreateKernel(program, "b10_add_one", &err);
    if (!check(err, "clCreateKernel") || !kernel) return 7;
    if (!check(clSetKernelArg(kernel, 0, sizeof(buffer), &buffer),
               "clSetKernelArg(buffer)"))
        return 7;

    cl_event write_event = NULL, kernel_event = NULL, read_event = NULL;
    if (!check(clEnqueueWriteBuffer(queue, buffer, CL_FALSE, 0,
                                    sizeof(before), before, 0, NULL,
                                    &write_event), "clEnqueueWriteBuffer"))
        return 8;
    const size_t one = 1;
    if (!check(clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &one, &one,
                                      0, NULL, &kernel_event),
               "clEnqueueNDRangeKernel"))
        return 8;
    if (!check(clEnqueueReadBuffer(queue, buffer, CL_FALSE, 0,
                                   sizeof(after), after, 0, NULL,
                                   &read_event), "clEnqueueReadBuffer"))
        return 8;
    if (!check(clFlush(queue), "clFlush")) return 9;
    if (!check(clWaitForEvents(1, &read_event), "clWaitForEvents(read)"))
        return 9;
    if (!check(clFinish(queue), "clFinish")) return 9;

    cl_event events[3] = { write_event, kernel_event, read_event };
    const char *names[3] = { "write", "kernel", "read" };
    for (int i = 0; i < 3; ++i) {
        cl_int status = 999;
        if (!check(clGetEventInfo(events[i], CL_EVENT_COMMAND_EXECUTION_STATUS,
                                  sizeof(status), &status, NULL),
                   "clGetEventInfo(status)"))
            return 10;
        printf("%s_event_status=%d\n", names[i], status);
        if (status != CL_COMPLETE) return 10;
    }

    for (int i = 0; i < 4; ++i)
        printf("word[%d]=0x%08x expected=0x%08x\n", i, after[i], expected[i]);
    int pass = memcmp(after, expected, sizeof(after)) == 0;
    printf("C1_RESULT=%s\n", pass ? "PASS" : "FAIL");

    clReleaseEvent(read_event);
    clReleaseEvent(kernel_event);
    clReleaseEvent(write_event);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseMemObject(buffer);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    return pass ? 0 : 11;
}
