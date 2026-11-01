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

static int get_platform_string(cl_platform_id platform, cl_platform_info key,
                               const char *label)
{
    char value[256] = { 0 };
    cl_int err = clGetPlatformInfo(platform, key, sizeof(value), value, NULL);
    if (!check(err, label)) return 0;
    printf("%s_value=%s\n", label, value);
    return 1;
}

static int get_device_string(cl_device_id device, cl_device_info key,
                             const char *label)
{
    char value[256] = { 0 };
    cl_int err = clGetDeviceInfo(device, key, sizeof(value), value, NULL);
    if (!check(err, label)) return 0;
    printf("%s_value=%s\n", label, value);
    return 1;
}

static int event_status(cl_event event, const char *label, cl_int *status)
{
    cl_int err = clGetEventInfo(event, CL_EVENT_COMMAND_EXECUTION_STATUS,
                                sizeof(*status), status, NULL);
    if (!check(err, "clGetEventInfo(status)")) return 0;
    printf("%s=%d\n", label, *status);
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
    if (argc != 2 || strcmp(argv[1], "--run-t1") != 0) {
        fprintf(stderr, "Usage: %s --run-t1\n", argv[0]);
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
    if (!get_platform_string(platform, CL_PLATFORM_NAME, "CL_PLATFORM_NAME")
        || !get_platform_string(platform, CL_PLATFORM_VERSION,
                                "CL_PLATFORM_VERSION"))
        return 3;

    cl_device_id device = NULL;
    if (!check(clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL),
               "clGetDeviceIDs(GPU)"))
        return 4;
    if (!get_device_string(device, CL_DEVICE_NAME, "CL_DEVICE_NAME")
        || !get_device_string(device, CL_DEVICE_VERSION, "CL_DEVICE_VERSION")
        || !get_device_string(device, CL_DRIVER_VERSION, "CL_DRIVER_VERSION"))
        return 4;
    print_loaded_icd();

    cl_context context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    if (!check(err, "clCreateContext") || !context) return 5;
    cl_command_queue producer_queue =
        clCreateCommandQueueWithProperties(context, device, NULL, &err);
    if (!check(err, "clCreateCommandQueueWithProperties(producer)")
        || !producer_queue) return 5;
    cl_command_queue consumer_queue =
        clCreateCommandQueueWithProperties(context, device, NULL, &err);
    if (!check(err, "clCreateCommandQueueWithProperties(consumer)")
        || !consumer_queue) return 5;

    cl_command_queue_properties producer_props = ~0UL, consumer_props = ~0UL;
    if (!check(clGetCommandQueueInfo(producer_queue, CL_QUEUE_PROPERTIES,
                                     sizeof(producer_props), &producer_props,
                                     NULL), "CL_QUEUE_PROPERTIES(producer)")
        || !check(clGetCommandQueueInfo(consumer_queue, CL_QUEUE_PROPERTIES,
                                        sizeof(consumer_props), &consumer_props,
                                        NULL), "CL_QUEUE_PROPERTIES(consumer)"))
        return 5;
    printf("producer_queue_properties=0x%llx\n",
           (unsigned long long)producer_props);
    printf("consumer_queue_properties=0x%llx\n",
           (unsigned long long)consumer_props);
    if ((producer_props | consumer_props) & CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE)
        return 5;

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

    cl_event gate = clCreateUserEvent(context, &err);
    if (!check(err, "clCreateUserEvent(gate)") || !gate) return 8;
    cl_event producer_event = NULL, consumer_event = NULL, read_event = NULL;
    if (!check(clEnqueueWriteBuffer(producer_queue, buffer, CL_FALSE, 0,
                                    sizeof(before), before, 1, &gate,
                                    &producer_event),
               "clEnqueueWriteBuffer(producer)"))
        return 8;
    if (!check(clFlush(producer_queue), "clFlush(producer)")) return 9;

    const size_t one = 1;
    if (!check(clEnqueueNDRangeKernel(consumer_queue, kernel, 1, NULL,
                                      &one, &one, 1, &producer_event,
                                      &consumer_event),
               "clEnqueueNDRangeKernel(consumer)"))
        return 9;
    if (!check(clEnqueueReadBuffer(consumer_queue, buffer, CL_FALSE, 0,
                                   sizeof(after), after, 0, NULL,
                                   &read_event), "clEnqueueReadBuffer"))
        return 9;
    if (!check(clFlush(consumer_queue), "clFlush(consumer)")) return 9;

    cl_int gate_before = 999, producer_before = 999, consumer_before = 999;
    if (!event_status(gate, "gate_before", &gate_before)
        || !event_status(producer_event, "producer_before", &producer_before)
        || !event_status(consumer_event, "consumer_before", &consumer_before))
        return 10;
    if (gate_before != CL_SUBMITTED
        || !(producer_before == CL_QUEUED || producer_before == CL_SUBMITTED)
        || !(consumer_before == CL_QUEUED || consumer_before == CL_SUBMITTED)) {
        fprintf(stderr, "Command advanced before user-event gate opened\n");
        return 10;
    }

    if (!check(clSetUserEventStatus(gate, CL_COMPLETE),
               "clSetUserEventStatus(CL_COMPLETE)"))
        return 11;
    if (!check(clWaitForEvents(1, &read_event), "clWaitForEvents(read)"))
        return 11;
    if (!check(clFinish(producer_queue), "clFinish(producer)")) return 11;
    if (!check(clFinish(consumer_queue), "clFinish(consumer)")) return 11;

    cl_event events[4] = { gate, producer_event, consumer_event, read_event };
    const char *names[4] = { "gate_final", "producer_final",
                             "consumer_final", "read_final" };
    for (int i = 0; i < 4; ++i) {
        cl_int status = 999;
        if (!event_status(events[i], names[i], &status) || status != CL_COMPLETE)
            return 12;
    }

    for (int i = 0; i < 4; ++i)
        printf("word[%d]=0x%08x expected=0x%08x\n", i, after[i], expected[i]);
    int pass = memcmp(after, expected, sizeof(after)) == 0;
    printf("T1_RESULT=%s\n", pass ? "PASS" : "FAIL");

    clReleaseEvent(read_event);
    clReleaseEvent(consumer_event);
    clReleaseEvent(producer_event);
    clReleaseEvent(gate);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseMemObject(buffer);
    clReleaseCommandQueue(consumer_queue);
    clReleaseCommandQueue(producer_queue);
    clReleaseContext(context);
    return pass ? 0 : 13;
}
