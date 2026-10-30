#define CL_TARGET_OPENCL_VERSION 300
#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    int negative = argc == 2 && strcmp(argv[1], "--negative") == 0;
    int positive = argc == 2 && strcmp(argv[1], "--positive") == 0;
    if (!negative && !positive) {
        fprintf(stderr, "Usage: %s --positive|--negative\n", argv[0]);
        return 2;
    }
    printf("mode=%s\n", negative ? "negative" : "positive");

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
    cl_command_queue queue =
        clCreateCommandQueueWithProperties(context, device, NULL, &err);
    if (!check(err, "clCreateCommandQueueWithProperties") || !queue) return 5;
    cl_command_queue_properties props = ~0UL;
    if (!check(clGetCommandQueueInfo(queue, CL_QUEUE_PROPERTIES,
                                     sizeof(props), &props, NULL),
               "CL_QUEUE_PROPERTIES"))
        return 5;
    printf("queue_properties=0x%llx\n", (unsigned long long)props);
    if (props & CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE) return 5;

    cl_mem buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                   4 * sizeof(cl_uint), NULL, &err);
    if (!check(err, "clCreateBuffer") || !buffer) return 6;
    cl_event gate = clCreateUserEvent(context, &err);
    if (!check(err, "clCreateUserEvent") || !gate) return 6;

    const cl_uint pattern = 0xb10b10b1u;
    cl_event fill_event = NULL;
    if (!check(clEnqueueFillBuffer(queue, buffer, &pattern, sizeof(pattern),
                                   0, 4 * sizeof(cl_uint), 1, &gate,
                                   &fill_event), "clEnqueueFillBuffer"))
        return 7;

    cl_uint output[4] = { 0xdeadbeefu, 0xdeadbeefu,
                          0xdeadbeefu, 0xdeadbeefu };
    cl_event read_event = NULL;
    if (positive) {
        if (!check(clEnqueueReadBuffer(queue, buffer, CL_FALSE, 0,
                                       sizeof(output), output, 0, NULL,
                                       &read_event), "clEnqueueReadBuffer"))
            return 7;
    }
    if (!check(clFlush(queue), "clFlush")) return 7;

    cl_int gate_before = 999, fill_before = 999;
    if (!event_status(gate, "gate_before", &gate_before)
        || !event_status(fill_event, "fill_before", &fill_before))
        return 8;
    if (gate_before != CL_SUBMITTED
        || !(fill_before == CL_QUEUED || fill_before == CL_SUBMITTED))
        return 8;
    if (positive) {
        cl_int read_before = 999;
        if (!event_status(read_event, "read_before", &read_before)) return 8;
        if (!(read_before == CL_QUEUED || read_before == CL_SUBMITTED))
            return 8;

        err = clSetUserEventStatus(gate, CL_RUNNING);
        printf("clSetUserEventStatus(CL_RUNNING)=%d expected=%d\n",
               err, CL_INVALID_VALUE);
        if (err != CL_INVALID_VALUE) return 9;
        cl_int gate_after_invalid = 999;
        if (!event_status(gate, "gate_after_invalid", &gate_after_invalid)
            || gate_after_invalid != CL_SUBMITTED)
            return 9;

        if (!check(clSetUserEventStatus(gate, CL_COMPLETE),
                   "clSetUserEventStatus(CL_COMPLETE)"))
            return 10;
        err = clSetUserEventStatus(gate, CL_COMPLETE);
        printf("clSetUserEventStatus(second_call)=%d expected=%d\n",
               err, CL_INVALID_OPERATION);
        if (err != CL_INVALID_OPERATION) return 10;
        if (!check(clWaitForEvents(1, &read_event), "clWaitForEvents(read)"))
            return 10;
        if (!check(clFinish(queue), "clFinish")) return 10;

        cl_int gate_final = 999, fill_final = 999, read_final = 999;
        if (!event_status(gate, "gate_final", &gate_final)
            || !event_status(fill_event, "fill_final", &fill_final)
            || !event_status(read_event, "read_final", &read_final))
            return 11;
        if (gate_final != CL_COMPLETE || fill_final != CL_COMPLETE
            || read_final != CL_COMPLETE)
            return 11;
        for (int i = 0; i < 4; ++i) {
            printf("word[%d]=0x%08x expected=0x%08x\n",
                   i, output[i], pattern);
            if (output[i] != pattern) return 11;
        }
        printf("T2_POSITIVE_RESULT=PASS\n");
        clReleaseEvent(read_event);
        clReleaseEvent(fill_event);
        clReleaseEvent(gate);
        clReleaseMemObject(buffer);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return 0;
    }

    if (!check(clSetUserEventStatus(gate, -1),
               "clSetUserEventStatus(-1)"))
        return 12;
    err = clWaitForEvents(1, &fill_event);
    printf("clWaitForEvents(fill)=%d expected=%d\n",
           err, CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST);
    if (err != CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST) return 12;
    printf("T2_NEGATIVE_RESULT=PASS\n");
    /* A terminated command may make later context API behavior
       implementation-defined. End this process without further OpenCL calls. */
    return 0;
}
