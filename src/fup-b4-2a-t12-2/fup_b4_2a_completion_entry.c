#define _POSIX_C_SOURCE 200809L
#define CL_TARGET_OPENCL_VERSION 120

#include <CL/cl.h>

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef FUP_SOURCE_SHA256
#define FUP_SOURCE_SHA256 "UNSET"
#endif

#define DEFAULT_ITERATIONS 4000000u
#define DEFAULT_TRIALS 3u
#define DEFAULT_TIMEOUT_MS 15000u
#define DEFAULT_PLATFORM_SUBSTR "rusticl"
#define DEFAULT_DEVICE_SUBSTR "AMD Radeon 660M"

#define CALLBACK_DESTRUCTOR_STARTED (1u << 0)
#define CALLBACK_COMPLETE_STARTED   (1u << 1)

#define EXIT_OBSERVED_PASS 0
#define EXIT_OBSERVED_FAIL 10
#define EXIT_INCONCLUSIVE 20
#define EXIT_USAGE 64

enum test_result {
    TEST_PASS,
    TEST_FAIL,
    TEST_INCONCLUSIVE,
};

struct options {
    cl_uint iterations;
    unsigned trials;
    unsigned timeout_ms;
    const char *platform_substr;
    const char *device_substr;
    const char *runtime_label;
    int oracle_self_test;
};

/*
 * Callback user_data is deliberately heap allocated and retained until
 * process exit. This makes timeout/error paths safe even if an implementation
 * invokes a registered callback late.
 */
struct callback_state {
    atomic_int normal_completion_published;
    atomic_int destructor_saw_normal_completion;
    atomic_uint flags;
    atomic_uint sequence;
    atomic_uint destructor_sequence;
    atomic_uint complete_sequence;
    atomic_int complete_callback_status;
};

struct runtime {
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
    cl_program program;
    cl_kernel kernel;
};

static const char *kernel_source =
    "__kernel void bounded_buffer_use(__global volatile uint *p, uint iterations)\n"
    "{\n"
    "    uint x = p[0] ^ 0x9e3779b9u;\n"
    "    for (uint i = 0; i < iterations; ++i) {\n"
    "        uint old = p[0];\n"
    "        x = (x ^ old ^ i) * 1664525u + 1013904223u;\n"
    "        p[0] = x;\n"
    "    }\n"
    "}\n";

static const char *result_name(enum test_result result)
{
    switch (result) {
    case TEST_PASS:
        return "PASS";
    case TEST_FAIL:
        return "FAIL";
    default:
        return "INCONCLUSIVE";
    }
}
static uint64_t monotonic_ms(void)
{
    struct timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
        return 0;
    return (uint64_t)ts.tv_sec * 1000u + (uint64_t)ts.tv_nsec / 1000000u;
}

static void sleep_one_ms(void)
{
    struct timespec requested = { .tv_sec = 0, .tv_nsec = 1000000L };

    while (nanosleep(&requested, &requested) != 0 && errno == EINTR)
        ;
}

static int parse_u32(const char *text, unsigned *value)
{
    char *end = NULL;
    unsigned long parsed;

    errno = 0;
    parsed = strtoul(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' || parsed == 0 ||
        parsed > UINT32_MAX)
        return 0;
    *value = (unsigned)parsed;
    return 1;
}

static int contains_case_insensitive(const char *haystack, const char *needle)
{
    size_t needle_len = strlen(needle);

    if (needle_len == 0)
        return 1;
    for (; *haystack != '\0'; ++haystack) {
        size_t i;

        for (i = 0; i < needle_len; ++i) {
            unsigned char h = (unsigned char)haystack[i];
            unsigned char n = (unsigned char)needle[i];

            if (h == '\0' || tolower(h) != tolower(n))
                break;
        }
        if (i == needle_len)
            return 1;
    }
    return 0;
}

static int option_list_contains_sync(const char *value)
{
    const char *p = value;

    if (value == NULL)
        return 0;
    while (*p != '\0') {
        const char *start;
        size_t len;

        while (*p == ',' || isspace((unsigned char)*p))
            ++p;
        start = p;
        while (*p != '\0' && *p != ',' && !isspace((unsigned char)*p))
            ++p;
        len = (size_t)(p - start);
        if (len == 4 && strncmp(start, "sync", 4) == 0)
            return 1;
    }
    return 0;
}

static void usage(const char *argv0)
{
    fprintf(stderr,
            "Usage: %s --runtime-label LABEL [options]\n"
            "       %s --oracle-self-test\n"
            "  --iterations N       bounded loop count (default %u)\n"
            "  --trials N           C1 repetitions (default %u)\n"
            "  --timeout-ms N       per-stage timeout (default %u)\n"
            "  --platform-substr S  required platform substring (default %s)\n"
            "  --device-substr S    required device substring (default %s)\n",
            argv0, argv0, DEFAULT_ITERATIONS, DEFAULT_TRIALS, DEFAULT_TIMEOUT_MS,
            DEFAULT_PLATFORM_SUBSTR, DEFAULT_DEVICE_SUBSTR);
}

static int parse_options(int argc, char **argv, struct options *options)
{
    int i;

    options->iterations = DEFAULT_ITERATIONS;
    options->trials = DEFAULT_TRIALS;
    options->timeout_ms = DEFAULT_TIMEOUT_MS;
    options->platform_substr = DEFAULT_PLATFORM_SUBSTR;
    options->device_substr = DEFAULT_DEVICE_SUBSTR;
    options->runtime_label = NULL;
    options->oracle_self_test = 0;

    for (i = 1; i < argc; ++i) {
        const char *name = argv[i];
        const char *value;
        unsigned parsed;

        if (strcmp(name, "--oracle-self-test") == 0) {
            options->oracle_self_test = 1;
            continue;
        }
        if (i + 1 >= argc) {
            usage(argv[0]);
            return 0;
        }
        value = argv[++i];
        if (strcmp(name, "--iterations") == 0) {
            if (!parse_u32(value, &parsed))
                return 0;
            options->iterations = (cl_uint)parsed;
        } else if (strcmp(name, "--trials") == 0) {
            if (!parse_u32(value, &parsed))
                return 0;
            options->trials = parsed;
        } else if (strcmp(name, "--timeout-ms") == 0) {
            if (!parse_u32(value, &parsed))
                return 0;
            options->timeout_ms = parsed;
        } else if (strcmp(name, "--platform-substr") == 0) {
            options->platform_substr = value;
        } else if (strcmp(name, "--device-substr") == 0) {
            options->device_substr = value;
        } else if (strcmp(name, "--runtime-label") == 0) {
            options->runtime_label = value;
        } else {
            usage(argv[0]);
            return 0;
        }
    }

    if (!options->oracle_self_test &&
        (options->runtime_label == NULL || options->runtime_label[0] == '\0')) {
        fprintf(stderr, "--runtime-label is required\n");
        usage(argv[0]);
        return 0;
    }
    return 1;
}

static struct callback_state *new_callback_state(void)
{
    struct callback_state *state = calloc(1, sizeof(*state));

    if (state == NULL)
        return NULL;
    atomic_init(&state->normal_completion_published, 0);
    atomic_init(&state->destructor_saw_normal_completion, -1);
    atomic_init(&state->flags, 0);
    atomic_init(&state->sequence, 0);
    atomic_init(&state->destructor_sequence, 0);
    atomic_init(&state->complete_sequence, 0);
    atomic_init(&state->complete_callback_status, INT32_MIN);
    return state;
}

/* The destructor callback performs only C11 atomic operations. */
static void CL_CALLBACK destructor_callback(cl_mem memobj, void *user_data)
{
    struct callback_state *state = user_data;
    unsigned sequence;
    int saw_normal_completion;

    (void)memobj;
    /* First relevant instrumentation action in this callback. */
    saw_normal_completion = atomic_load_explicit(
        &state->normal_completion_published, memory_order_acquire);
    atomic_store_explicit(&state->destructor_saw_normal_completion,
                          saw_normal_completion, memory_order_release);
    atomic_fetch_or_explicit(&state->flags, CALLBACK_DESTRUCTOR_STARTED,
                             memory_order_release);
    sequence = atomic_fetch_add_explicit(&state->sequence, 1,
                                         memory_order_relaxed) + 1;
    atomic_store_explicit(&state->destructor_sequence, sequence,
                          memory_order_release);
}

/* The event callback likewise performs only C11 atomic operations. */
static void CL_CALLBACK complete_callback(cl_event event,
                                          cl_int event_command_status,
                                          void *user_data)
{
    struct callback_state *state = user_data;
    unsigned sequence;

    (void)event;
    atomic_store_explicit(&state->complete_callback_status,
                          event_command_status, memory_order_relaxed);
    if (event_command_status == CL_COMPLETE) {
        atomic_store_explicit(&state->normal_completion_published, 1,
                              memory_order_release);
    }
    atomic_fetch_or_explicit(&state->flags, CALLBACK_COMPLETE_STARTED,
                             memory_order_release);
    sequence = atomic_fetch_add_explicit(&state->sequence, 1,
                                         memory_order_relaxed) + 1;
    atomic_store_explicit(&state->complete_sequence, sequence,
                          memory_order_release);
}

static int callback_seen(const struct callback_state *state, unsigned flag)
{
    return (atomic_load_explicit(&state->flags, memory_order_acquire) & flag) != 0;
}

static int wait_for_callback(const struct callback_state *state,
                             unsigned flag,
                             unsigned timeout_ms)
{
    uint64_t deadline = monotonic_ms() + timeout_ms;

    while (monotonic_ms() <= deadline) {
        if (callback_seen(state, flag))
            return 1;
        sleep_one_ms();
    }
    return callback_seen(state, flag);
}

static cl_int query_event_status(cl_event event, cl_int *status)
{
    return clGetEventInfo(event, CL_EVENT_COMMAND_EXECUTION_STATUS,
                          sizeof(*status), status, NULL);
}

static int wait_for_event_terminal(cl_event event,
                                   unsigned timeout_ms,
                                   cl_int *status,
                                   cl_int *query_error)
{
    uint64_t deadline = monotonic_ms() + timeout_ms;

    do {
        *query_error = query_event_status(event, status);
        if (*query_error != CL_SUCCESS || *status <= CL_COMPLETE)
            return *query_error == CL_SUCCESS;
        sleep_one_ms();
    } while (monotonic_ms() <= deadline);

    *query_error = query_event_status(event, status);
    return *query_error == CL_SUCCESS && *status <= CL_COMPLETE;
}

static int get_platform_string(cl_platform_id platform,
                               cl_platform_info key,
                               char *buffer,
                               size_t size)
{
    return clGetPlatformInfo(platform, key, size, buffer, NULL) == CL_SUCCESS;
}

static int get_device_string(cl_device_id device,
                             cl_device_info key,
                             char *buffer,
                             size_t size)
{
    return clGetDeviceInfo(device, key, size, buffer, NULL) == CL_SUCCESS;
}

static int select_device(const struct options *options,
                         cl_platform_id *selected_platform,
                         cl_device_id *selected_device)
{
    cl_platform_id *platforms = NULL;
    cl_uint platform_count = 0;
    cl_int err;
    cl_uint p;

    err = clGetPlatformIDs(0, NULL, &platform_count);
    if (err != CL_SUCCESS || platform_count == 0)
        return 0;
    platforms = calloc(platform_count, sizeof(*platforms));
    if (platforms == NULL)
        return 0;
    err = clGetPlatformIDs(platform_count, platforms, NULL);
    if (err != CL_SUCCESS) {
        free(platforms);
        return 0;
    }

    for (p = 0; p < platform_count; ++p) {
        cl_device_id *devices = NULL;
        cl_uint device_count = 0;
        char platform_name[512] = {0};
        cl_uint d;

        if (!get_platform_string(platforms[p], CL_PLATFORM_NAME,
                                 platform_name, sizeof(platform_name)) ||
            !contains_case_insensitive(platform_name,
                                       options->platform_substr))
            continue;
        err = clGetDeviceIDs(platforms[p], CL_DEVICE_TYPE_GPU,
                             0, NULL, &device_count);
        if (err != CL_SUCCESS || device_count == 0)
            continue;
        devices = calloc(device_count, sizeof(*devices));
        if (devices == NULL)
            continue;
        err = clGetDeviceIDs(platforms[p], CL_DEVICE_TYPE_GPU,
                             device_count, devices, NULL);
        if (err != CL_SUCCESS) {
            free(devices);
            continue;
        }
        for (d = 0; d < device_count; ++d) {
            char device_name[512] = {0};

            if (get_device_string(devices[d], CL_DEVICE_NAME,
                                  device_name, sizeof(device_name)) &&
                contains_case_insensitive(device_name,
                                          options->device_substr)) {
                *selected_platform = platforms[p];
                *selected_device = devices[d];
                free(devices);
                free(platforms);
                return 1;
            }
        }
        free(devices);
    }

    free(platforms);
    return 0;
}

static void print_runtime_provenance(const struct options *options,
                                     const struct runtime *runtime)
{
    char platform_name[512] = {0};
    char platform_vendor[512] = {0};
    char platform_version[512] = {0};
    char device_name[512] = {0};
    char device_version[512] = {0};
    char driver_version[512] = {0};
    char opencl_c_version[512] = {0};

    get_platform_string(runtime->platform, CL_PLATFORM_NAME,
                        platform_name, sizeof(platform_name));
    get_platform_string(runtime->platform, CL_PLATFORM_VENDOR,
                        platform_vendor, sizeof(platform_vendor));
    get_platform_string(runtime->platform, CL_PLATFORM_VERSION,
                        platform_version, sizeof(platform_version));
    get_device_string(runtime->device, CL_DEVICE_NAME,
                      device_name, sizeof(device_name));
    get_device_string(runtime->device, CL_DEVICE_VERSION,
                      device_version, sizeof(device_version));
    get_device_string(runtime->device, CL_DRIVER_VERSION,
                      driver_version, sizeof(driver_version));
    get_device_string(runtime->device, CL_DEVICE_OPENCL_C_VERSION,
                      opencl_c_version, sizeof(opencl_c_version));

    printf("PROVENANCE source_sha256=%s\n", FUP_SOURCE_SHA256);
    printf("PROVENANCE runtime_label=%s\n", options->runtime_label);
    printf("PROVENANCE platform_name=%s\n", platform_name);
    printf("PROVENANCE platform_vendor=%s\n", platform_vendor);
    printf("PROVENANCE platform_version=%s\n", platform_version);
    printf("PROVENANCE device_name=%s\n", device_name);
    printf("PROVENANCE device_version=%s\n", device_version);
    printf("PROVENANCE driver_version=%s\n", driver_version);
    printf("PROVENANCE opencl_c_version=%s\n", opencl_c_version);
    printf("CONFIG iterations=%u trials=%u timeout_ms=%u\n",
           options->iterations, options->trials, options->timeout_ms);
}

static int initialize_runtime(const struct options *options,
                              struct runtime *runtime)
{
    cl_int err;

    memset(runtime, 0, sizeof(*runtime));
    if (!select_device(options, &runtime->platform, &runtime->device)) {
        fprintf(stderr, "No matching GPU platform/device found\n");
        return 0;
    }
    runtime->context = clCreateContext(NULL, 1, &runtime->device,
                                       NULL, NULL, &err);
    if (runtime->context == NULL || err != CL_SUCCESS) {
        fprintf(stderr, "clCreateContext=%d\n", err);
        return 0;
    }
    runtime->queue = clCreateCommandQueue(runtime->context, runtime->device,
                                          0, &err);
    if (runtime->queue == NULL || err != CL_SUCCESS) {
        fprintf(stderr, "clCreateCommandQueue=%d\n", err);
        return 0;
    }
    runtime->program = clCreateProgramWithSource(runtime->context, 1,
                                                  &kernel_source, NULL, &err);
    if (runtime->program == NULL || err != CL_SUCCESS) {
        fprintf(stderr, "clCreateProgramWithSource=%d\n", err);
        return 0;
    }
    err = clBuildProgram(runtime->program, 1, &runtime->device,
                         NULL, NULL, NULL);
    if (err != CL_SUCCESS) {
        size_t log_size = 0;
        char *log = NULL;

        clGetProgramBuildInfo(runtime->program, runtime->device,
                              CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        if (log_size != 0)
            log = calloc(log_size + 1, 1);
        if (log != NULL) {
            clGetProgramBuildInfo(runtime->program, runtime->device,
                                  CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
            fprintf(stderr, "OpenCL build log:\n%s\n", log);
            free(log);
        }
        fprintf(stderr, "clBuildProgram=%d\n", err);
        return 0;
    }
    runtime->kernel = clCreateKernel(runtime->program,
                                     "bounded_buffer_use", &err);
    if (runtime->kernel == NULL || err != CL_SUCCESS) {
        fprintf(stderr, "clCreateKernel=%d\n", err);
        return 0;
    }
    return 1;
}

static void release_runtime(struct runtime *runtime)
{
    if (runtime->kernel != NULL)
        clReleaseKernel(runtime->kernel);
    if (runtime->program != NULL)
        clReleaseProgram(runtime->program);
    if (runtime->queue != NULL)
        clReleaseCommandQueue(runtime->queue);
    if (runtime->context != NULL)
        clReleaseContext(runtime->context);
}

static int prepare_buffer(struct runtime *runtime,
                          struct callback_state *state,
                          cl_mem *buffer)
{
    cl_uint seed = 0x12345678u;
    cl_int err;

    *buffer = clCreateBuffer(runtime->context,
                             CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
                             sizeof(seed), &seed, &err);
    if (*buffer == NULL || err != CL_SUCCESS) {
        fprintf(stderr, "clCreateBuffer=%d\n", err);
        return 0;
    }
    err = clSetMemObjectDestructorCallback(*buffer,
                                           destructor_callback, state);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "clSetMemObjectDestructorCallback=%d\n", err);
        clReleaseMemObject(*buffer);
        *buffer = NULL;
        return 0;
    }
    return 1;
}

static int enqueue_kernel(struct runtime *runtime,
                          cl_mem buffer,
                          cl_uint iterations,
                          struct callback_state *state,
                          cl_event *event)
{
    size_t global_size = 1;
    cl_int err;

    err = clSetKernelArg(runtime->kernel, 0, sizeof(buffer), &buffer);
    if (err == CL_SUCCESS)
        err = clSetKernelArg(runtime->kernel, 1,
                             sizeof(iterations), &iterations);
    if (err == CL_SUCCESS)
        err = clEnqueueNDRangeKernel(runtime->queue, runtime->kernel,
                                     1, NULL, &global_size, NULL,
                                     0, NULL, event);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "kernel setup/enqueue=%d\n", err);
        return 0;
    }
    err = clSetEventCallback(*event, CL_COMPLETE,
                             complete_callback, state);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "clSetEventCallback=%d\n", err);
        return 0;
    }
    return 1;
}

static void print_callback_state(const char *control,
                                 unsigned trial,
                                 const struct callback_state *state)
{
    if (state == NULL) {
        printf("ORDER control=%s trial=%u state=unavailable\n",
               control, trial);
        return;
    }

    unsigned flags = atomic_load_explicit(&state->flags,
                                           memory_order_acquire);
    unsigned destructor_sequence =
        atomic_load_explicit(&state->destructor_sequence,
                             memory_order_acquire);
    unsigned complete_sequence =
        atomic_load_explicit(&state->complete_sequence,
                             memory_order_acquire);
    int callback_status =
        atomic_load_explicit(&state->complete_callback_status,
                             memory_order_acquire);
    int normal_completion_published =
        atomic_load_explicit(&state->normal_completion_published,
                             memory_order_acquire);
    int destructor_saw_normal_completion =
        atomic_load_explicit(&state->destructor_saw_normal_completion,
                             memory_order_acquire);

    printf("ORDER control=%s trial=%u destructor_started=%u "
           "destructor_sequence=%u complete_callback_started=%u "
           "complete_sequence=%u complete_callback_status=%d "
           "normal_completion_published=%d "
           "destructor_saw_normal_completion=%d\n",
           control, trial,
           !!(flags & CALLBACK_DESTRUCTOR_STARTED), destructor_sequence,
           !!(flags & CALLBACK_COMPLETE_STARTED), complete_sequence,
           callback_status, normal_completion_published,
           destructor_saw_normal_completion);
}

static enum test_result run_c0(struct runtime *runtime,
                               const struct options *options)
{
    struct callback_state *state = new_callback_state();
    cl_mem buffer = NULL;
    cl_event event = NULL;
    cl_int status = INT32_MIN;
    cl_int query_error = CL_SUCCESS;
    cl_int err;
    enum test_result result = TEST_INCONCLUSIVE;

    if (state == NULL || !prepare_buffer(runtime, state, &buffer) ||
        !enqueue_kernel(runtime, buffer, options->iterations, state, &event))
        goto out;
    err = clFlush(runtime->queue);
    if (err != CL_SUCCESS) {
        printf("CONTROL=C0 RESULT=INCONCLUSIVE reason=clFlush error=%d\n", err);
        goto out;
    }
    if (!wait_for_event_terminal(event, options->timeout_ms,
                                 &status, &query_error) ||
        status != CL_COMPLETE) {
        printf("CONTROL=C0 RESULT=INCONCLUSIVE reason=event_not_complete "
               "query_error=%d event_status=%d\n", query_error, status);
        goto out;
    }
    if (callback_seen(state, CALLBACK_DESTRUCTOR_STARTED)) {
        printf("CONTROL=C0 RESULT=INCONCLUSIVE "
               "reason=destructor_with_explicit_reference\n");
        goto out;
    }
    err = clReleaseMemObject(buffer);
    buffer = NULL;
    if (err != CL_SUCCESS ||
        !wait_for_callback(state, CALLBACK_DESTRUCTOR_STARTED,
                           options->timeout_ms) ||
        !wait_for_callback(state, CALLBACK_COMPLETE_STARTED,
                           options->timeout_ms)) {
        printf("CONTROL=C0 RESULT=INCONCLUSIVE "
               "reason=callback_timeout release_error=%d\n", err);
        goto out;
    }

    result = TEST_PASS;
    printf("CONTROL=C0 RESULT=PASS event_status=%d\n", status);

out:
    print_callback_state("C0", 0, state);
    if (buffer != NULL)
        clReleaseMemObject(buffer);
    if (event != NULL)
        clReleaseEvent(event);
    return result;
}

static enum test_result observe_c1_order(cl_event event,
                                         const struct callback_state *state,
                                         unsigned timeout_ms,
                                         cl_int *observed_status,
                                         cl_int *query_error)
{
    uint64_t deadline = monotonic_ms() + timeout_ms;

    while (monotonic_ms() <= deadline) {
        int destructor_before;
        int destructor_after;

        destructor_before = callback_seen(state, CALLBACK_DESTRUCTOR_STARTED);
        *query_error = query_event_status(event, observed_status);
        destructor_after = callback_seen(state, CALLBACK_DESTRUCTOR_STARTED);

        if (*query_error != CL_SUCCESS)
            return TEST_INCONCLUSIVE;
        if (destructor_before) {
            if (*observed_status > CL_COMPLETE)
                return TEST_FAIL;
            return TEST_INCONCLUSIVE;
        }
        if (*observed_status < CL_COMPLETE)
            return TEST_INCONCLUSIVE;
        if (*observed_status == CL_COMPLETE) {
            if (!destructor_after)
                return TEST_PASS;
            return TEST_INCONCLUSIVE;
        }
        sleep_one_ms();
    }
    return TEST_INCONCLUSIVE;
}

static enum test_result classify_c1_result(
    enum test_result direct_result,
    int cleanup_terminal,
    cl_int final_event_status,
    cl_int final_query_error,
    int destructor_seen,
    int complete_callback_seen,
    cl_int complete_callback_status,
    int normal_completion_published,
    int destructor_saw_normal_completion)
{
    if (direct_result == TEST_FAIL)
        return TEST_FAIL;

    if (cleanup_terminal && final_event_status == CL_COMPLETE &&
        final_query_error == CL_SUCCESS && destructor_seen &&
        complete_callback_seen &&
        complete_callback_status == CL_COMPLETE &&
        normal_completion_published == 1 &&
        destructor_saw_normal_completion == 1)
        return TEST_PASS;

    return TEST_INCONCLUSIVE;
}

static int run_oracle_self_test(void)
{
    struct {
        const char *name;
        enum test_result expected;
        enum test_result direct;
        int terminal;
        cl_int final_status;
        cl_int final_query_error;
        int destructor_seen;
        int complete_seen;
        cl_int complete_status;
        int published;
        int destructor_saw;
    } cases[] = {
        {
            "FAIL_direct_nonterminal_event",
            TEST_FAIL, TEST_FAIL, 1, CL_COMPLETE, CL_SUCCESS,
            1, 1, CL_COMPLETE, 1, 1,
        },
        {
            "PASS_destructor_acquired_normal_completion",
            TEST_PASS, TEST_INCONCLUSIVE, 1, CL_COMPLETE, CL_SUCCESS,
            1, 1, CL_COMPLETE, 1, 1,
        },
        {
            "INCONCLUSIVE_missing_marker",
            TEST_INCONCLUSIVE, TEST_INCONCLUSIVE, 1, CL_COMPLETE,
            CL_SUCCESS, 1, 1, CL_COMPLETE, 1, 0,
        },
        {
            "INCONCLUSIVE_negative_completion",
            TEST_INCONCLUSIVE, TEST_INCONCLUSIVE, 1, -5,
            CL_SUCCESS, 1, 1, -5, 0, 0,
        },
        {
            "INCONCLUSIVE_missing_callback",
            TEST_INCONCLUSIVE, TEST_INCONCLUSIVE, 1, CL_COMPLETE,
            CL_SUCCESS, 1, 0, CL_COMPLETE, 1, 1,
        },
    };
    size_t i;

    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        enum test_result actual = classify_c1_result(
            cases[i].direct, cases[i].terminal, cases[i].final_status,
            cases[i].final_query_error, cases[i].destructor_seen,
            cases[i].complete_seen, cases[i].complete_status,
            cases[i].published, cases[i].destructor_saw);

        printf("SELFTEST case=%s expected=%s actual=%s\n",
               cases[i].name, result_name(cases[i].expected),
               result_name(actual));
        if (actual != cases[i].expected) {
            printf("FUP-B4-2A-T12.2-SELFTEST=FAIL\n");
            return EXIT_OBSERVED_FAIL;
        }
    }

    printf("FUP-B4-2A-T12.2-SELFTEST=PASS\n");
    return EXIT_OBSERVED_PASS;
}

static enum test_result run_c1_trial(struct runtime *runtime,
                                     const struct options *options,
                                     unsigned trial)
{
    struct callback_state *state = new_callback_state();
    cl_mem buffer = NULL;
    cl_event event = NULL;
    cl_int observed_status = INT32_MIN;
    cl_int query_error = CL_SUCCESS;
    cl_int final_status = INT32_MIN;
    cl_int final_query_error = CL_SUCCESS;
    cl_int release_error;
    cl_int flush_error;
    enum test_result result = TEST_INCONCLUSIVE;
    enum test_result direct_result;
    int terminal;
    int destructor_seen;
    int complete_callback_seen;
    int normal_completion_published;
    int destructor_saw_normal_completion;
    cl_int complete_callback_status;

    if (state == NULL || !prepare_buffer(runtime, state, &buffer) ||
        !enqueue_kernel(runtime, buffer, options->iterations, state, &event))
        goto out;

    /* This drops the application's last explicit cl_mem reference. */
    release_error = clReleaseMemObject(buffer);
    buffer = NULL;
    if (release_error != CL_SUCCESS) {
        printf("CONTROL=C1 TRIAL=%u RESULT=INCONCLUSIVE "
               "reason=clReleaseMemObject error=%d\n", trial, release_error);
        goto out;
    }

    /* Submission is explicit; no blocking wait is used for the oracle. */
    flush_error = clFlush(runtime->queue);
    if (flush_error != CL_SUCCESS) {
        printf("CONTROL=C1 TRIAL=%u RESULT=INCONCLUSIVE "
               "reason=clFlush error=%d\n", trial, flush_error);
        goto out;
    }

    result = observe_c1_order(event, state, options->timeout_ms,
                              &observed_status, &query_error);
    direct_result = result;

    /* Bounded cleanup/validation. This does not participate in the oracle. */
    terminal = wait_for_event_terminal(event, options->timeout_ms,
                                       &final_status, &final_query_error);
    destructor_seen = wait_for_callback(state, CALLBACK_DESTRUCTOR_STARTED,
                                        options->timeout_ms);
    complete_callback_seen =
        wait_for_callback(state, CALLBACK_COMPLETE_STARTED,
                          options->timeout_ms);
    normal_completion_published = atomic_load_explicit(
        &state->normal_completion_published, memory_order_acquire);
    destructor_saw_normal_completion = atomic_load_explicit(
        &state->destructor_saw_normal_completion, memory_order_acquire);
    complete_callback_status = atomic_load_explicit(
        &state->complete_callback_status, memory_order_acquire);
    result = classify_c1_result(
        direct_result, terminal, final_status, final_query_error,
        destructor_seen, complete_callback_seen, complete_callback_status,
        normal_completion_published, destructor_saw_normal_completion);

    if (result == TEST_FAIL) {
        printf("CONTROL=C1 TRIAL=%u RESULT=FAIL "
               "oracle=destructor_before_event_completion "
               "observed_event_status=%d query_error=%d "
               "cleanup_terminal=%d final_event_status=%d "
               "final_query_error=%d destructor_seen=%d "
               "complete_callback_seen=%d\n",
               trial, observed_status, query_error, terminal, final_status,
               final_query_error, destructor_seen, complete_callback_seen);
    } else if (result == TEST_PASS) {
        printf("CONTROL=C1 TRIAL=%u RESULT=PASS "
               "oracle=destructor_acquired_normal_completion_marker "
               "observed_event_status=%d query_error=%d "
               "cleanup_terminal=%d final_event_status=%d "
               "final_query_error=%d destructor_seen=%d "
               "complete_callback_seen=%d complete_callback_status=%d "
               "normal_completion_published=%d "
               "destructor_saw_normal_completion=%d\n",
               trial, observed_status, query_error, terminal, final_status,
               final_query_error, destructor_seen, complete_callback_seen,
               complete_callback_status, normal_completion_published,
               destructor_saw_normal_completion);
    } else {
        printf("CONTROL=C1 TRIAL=%u RESULT=INCONCLUSIVE "
               "reason=no_direct_violation_or_synchronized_positive_order "
               "observed_event_status=%d query_error=%d "
               "cleanup_terminal=%d final_event_status=%d "
               "final_query_error=%d destructor_seen=%d "
               "complete_callback_seen=%d complete_callback_status=%d "
               "normal_completion_published=%d "
               "destructor_saw_normal_completion=%d\n",
               trial, observed_status, query_error, terminal, final_status,
               final_query_error, destructor_seen, complete_callback_seen,
               complete_callback_status, normal_completion_published,
               destructor_saw_normal_completion);
    }

out:
    print_callback_state("C1", trial, state);
    if (buffer != NULL)
        clReleaseMemObject(buffer);
    if (event != NULL)
        clReleaseEvent(event);
    return result;
}

static enum test_result run_c2(struct runtime *runtime,
                               const struct options *options)
{
    struct callback_state *state = new_callback_state();
    cl_mem buffer = NULL;
    cl_int err;
    enum test_result result = TEST_INCONCLUSIVE;

    if (state == NULL || !prepare_buffer(runtime, state, &buffer))
        goto out;
    err = clReleaseMemObject(buffer);
    buffer = NULL;
    if (err == CL_SUCCESS &&
        wait_for_callback(state, CALLBACK_DESTRUCTOR_STARTED,
                          options->timeout_ms)) {
        result = TEST_PASS;
        printf("CONTROL=C2 RESULT=PASS\n");
    } else {
        printf("CONTROL=C2 RESULT=INCONCLUSIVE "
               "reason=destructor_timeout release_error=%d\n", err);
    }

out:
    print_callback_state("C2", 0, state);
    if (buffer != NULL)
        clReleaseMemObject(buffer);
    return result;
}

int main(int argc, char **argv)
{
    struct options options;
    struct runtime runtime;
    enum test_result c0;
    enum test_result c2;
    enum test_result overall = TEST_PASS;
    unsigned pass_count = 0;
    unsigned fail_count = 0;
    unsigned inconclusive_count = 0;
    unsigned trial;

    setvbuf(stdout, NULL, _IOLBF, 0);
    if (!parse_options(argc, argv, &options))
        return EXIT_USAGE;

    if (options.oracle_self_test)
        return run_oracle_self_test();

    if (option_list_contains_sync(getenv("RUSTICL_DEBUG"))) {
        printf("FUP-B4-2A-T12.2=INCONCLUSIVE "
               "reason=RUSTICL_DEBUG_sync_masks_oracle\n");
        return EXIT_INCONCLUSIVE;
    }
    printf("PROVENANCE rusticl_debug_sync=0\n");

    if (!initialize_runtime(&options, &runtime)) {
        release_runtime(&runtime);
        printf("FUP-B4-2A-T12.2=INCONCLUSIVE "
               "reason=runtime_initialization\n");
        return EXIT_INCONCLUSIVE;
    }
    print_runtime_provenance(&options, &runtime);

    c0 = run_c0(&runtime, &options);
    if (c0 != TEST_PASS)
        overall = TEST_INCONCLUSIVE;

    for (trial = 1; trial <= options.trials; ++trial) {
        enum test_result c1 = run_c1_trial(&runtime, &options, trial);

        if (c1 == TEST_FAIL)
            ++fail_count;
        else if (c1 == TEST_PASS)
            ++pass_count;
        else
            ++inconclusive_count;
    }

    c2 = run_c2(&runtime, &options);
    if (c2 != TEST_PASS)
        overall = TEST_INCONCLUSIVE;

    if (c0 == TEST_PASS && c2 == TEST_PASS) {
        if (fail_count != 0)
            overall = TEST_FAIL;
        else if (inconclusive_count != 0)
            overall = TEST_INCONCLUSIVE;
        else
            overall = TEST_PASS;
    }

    printf("SUMMARY c0=%s c1_pass=%u c1_fail=%u c1_inconclusive=%u c2=%s\n",
           result_name(c0), pass_count, fail_count, inconclusive_count,
           result_name(c2));
    printf("FUP-B4-2A-T12.2=%s\n", result_name(overall));

    release_runtime(&runtime);
    if (overall == TEST_FAIL)
        return EXIT_OBSERVED_FAIL;
    if (overall == TEST_INCONCLUSIVE)
        return EXIT_INCONCLUSIVE;
    return EXIT_OBSERVED_PASS;
}
