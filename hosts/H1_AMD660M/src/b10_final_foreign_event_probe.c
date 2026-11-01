#define CL_TARGET_OPENCL_VERSION 300
#include <CL/cl.h>
#include <stdio.h>
#include <string.h>

static void print_loaded_icd(void)
{
    FILE *maps = fopen("/proc/self/maps", "r");
    if (!maps) return;
    char line[1024];
    while (fgets(line, sizeof(line), maps)) {
        char *path = strstr(line, "/usr/");
        if (strstr(line, "libRusticlOpenCL.so") && path) {
            printf("loaded_icd=%s", path);
            break;
        }
    }
    fclose(maps);
}

int main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    cl_int err = CL_SUCCESS;
    cl_platform_id platform = NULL;
    cl_device_id device = NULL;
    cl_context own = NULL, foreign = NULL;
    cl_command_queue queue = NULL;
    cl_mem buffer = NULL;
    cl_event own_gate = NULL, foreign_gate = NULL, valid_fill = NULL;
    cl_event unexpected_fill = NULL;
    const cl_uint pattern = 0x4b10a55aU;
    cl_uint words[4] = { 0, 0, 0, 0 };
    cl_int status = 99;
    char name[256] = { 0 }, version[256] = { 0 };
    int result = 10;
    int own_gate_completed = 0;

    err = clGetPlatformIDs(1, &platform, NULL);
    printf("clGetPlatformIDs=%d\n", err);
    if (err != CL_SUCCESS) goto cleanup;
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    printf("clGetDeviceIDs(GPU)=%d\n", err);
    if (err != CL_SUCCESS) goto cleanup;
    err = clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(name), name, NULL);
    if (err != CL_SUCCESS) goto cleanup;
    err = clGetDeviceInfo(device, CL_DEVICE_VERSION, sizeof(version), version,
                          NULL);
    if (err != CL_SUCCESS) goto cleanup;
    printf("device=%s\nversion=%s\n", name, version);
    print_loaded_icd();

    own = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    printf("clCreateContext(own)=%d\n", err);
    if (!own || err != CL_SUCCESS) goto cleanup;
    foreign = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    printf("clCreateContext(foreign)=%d\n", err);
    if (!foreign || err != CL_SUCCESS) goto cleanup;
    queue = clCreateCommandQueueWithProperties(own, device, NULL, &err);
    printf("clCreateCommandQueueWithProperties=%d\n", err);
    if (!queue || err != CL_SUCCESS) goto cleanup;
    buffer = clCreateBuffer(own, CL_MEM_READ_WRITE, sizeof(words), NULL, &err);
    printf("clCreateBuffer=%d\n", err);
    if (!buffer || err != CL_SUCCESS) goto cleanup;
    own_gate = clCreateUserEvent(own, &err);
    printf("clCreateUserEvent(own)=%d\n", err);
    if (!own_gate || err != CL_SUCCESS) goto cleanup;
    foreign_gate = clCreateUserEvent(foreign, &err);
    printf("clCreateUserEvent(foreign)=%d\n", err);
    if (!foreign_gate || err != CL_SUCCESS) goto cleanup;

    err = clSetUserEventStatus(own_gate, CL_COMPLETE);
    printf("clSetUserEventStatus(own, complete)=%d\n", err);
    if (err != CL_SUCCESS) goto cleanup;
    own_gate_completed = 1;
    err = clEnqueueFillBuffer(queue, buffer, &pattern, sizeof(pattern), 0,
                              sizeof(words), 1, &own_gate, &valid_fill);
    printf("valid_fill_enqueue=%d\n", err);
    if (err != CL_SUCCESS || !valid_fill) goto cleanup;
    err = clWaitForEvents(1, &valid_fill);
    printf("valid_fill_wait=%d\n", err);
    if (err != CL_SUCCESS) goto cleanup;
    err = clGetEventInfo(valid_fill, CL_EVENT_COMMAND_EXECUTION_STATUS,
                         sizeof(status), &status, NULL);
    printf("valid_fill_status_query=%d status=%d\n", err, status);
    if (err != CL_SUCCESS || status != CL_COMPLETE) goto cleanup;
    err = clEnqueueReadBuffer(queue, buffer, CL_TRUE, 0, sizeof(words), words,
                              0, NULL, NULL);
    printf("valid_read=%d data=%08x,%08x,%08x,%08x\n", err, words[0],
           words[1], words[2], words[3]);
    if (err != CL_SUCCESS) goto cleanup;
    for (size_t i = 0; i < 4; ++i) {
        if (words[i] != pattern) goto cleanup;
    }

    err = clEnqueueFillBuffer(queue, buffer, &pattern, sizeof(pattern), 0,
                              sizeof(words), 1, &foreign_gate,
                              &unexpected_fill);
    printf("foreign_event_fill_enqueue=%d expected=%d output_event=%s\n",
           err, CL_INVALID_CONTEXT, unexpected_fill ? "non-null" : "null");
    if (err != CL_INVALID_CONTEXT || unexpected_fill != NULL) {
        printf("result=ANOMALY_STOP\n");
        (void)clSetUserEventStatus(foreign_gate, CL_COMPLETE);
        return 20;
    }
    result = 0;
    printf("result=PASS_CONTEXT_BOUNDARY\n");

cleanup:
    if (foreign_gate) {
        (void)clSetUserEventStatus(foreign_gate, CL_COMPLETE);
        clReleaseEvent(foreign_gate);
    }
    if (valid_fill) clReleaseEvent(valid_fill);
    if (own_gate) {
        if (!own_gate_completed)
            (void)clSetUserEventStatus(own_gate, CL_COMPLETE);
        clReleaseEvent(own_gate);
    }
    if (buffer) clReleaseMemObject(buffer);
    if (queue) clReleaseCommandQueue(queue);
    if (foreign) clReleaseContext(foreign);
    if (own) clReleaseContext(own);
    if (result != 0) printf("result=SETUP_OR_CONTROL_ERROR\n");
    return result;
}
