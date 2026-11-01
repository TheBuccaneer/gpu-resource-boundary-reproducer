/*
 * SEC-1 -- Cross-Client Failure Containment
 * Process V: independent, harmless OpenCL "victim" heartbeat.
 *
 * Design goals (see OpenCL2_EXTENSIONS.md, SEC-1):
 *   - separate process, own OpenCL context/queue
 *   - trivial kernel, no __local boundary cases
 *   - continuous sequence number
 *   - CLOCK_MONOTONIC timestamps (comparable across processes on the
 *     same host -- no IPC / clock handshake with Process A required)
 *   - explicit enqueue/finish/read return codes
 *   - pause/hang/error/process-end detection
 *   - tests whether the *same* long-lived context/queue keeps working
 *     after an external B1/B3 F5 event, and separately whether a fresh
 *     context in the same process recovers
 *   - phase-agnostic: this binary does not know about BASELINE / ARMED /
 *     POST. Phase attribution is done offline, by joining this log's
 *     mono_ns column against Process A's own start/exit timestamps
 *     (also CLOCK_MONOTONIC). Keeping V phase-blind means the exact
 *     same, unmodified victim binary is used in every run.
 *
 * Known limitation (documented, not hidden): clFinish() is a blocking
 * call that cannot be cancelled from inside this process. The watchdog
 * thread can only *observe and report* a suspected hang (main thread
 * still blocked past HANG_WARN_MS); it cannot interrupt it. True hang
 * recovery requires an external observer (e.g. process-liveness check
 * via PID) once the log stream itself goes silent.
 *
 * Build:
 *   gcc -O2 -Wall -Wextra -std=c11 -pthread \
 *       src/sec1_victim_heartbeat.c -lOpenCL -o sec1_victim_heartbeat
 *
 * Usage:
 *   ./sec1_victim_heartbeat [run_label] [interval_ms] [max_iterations]
 *     run_label      free-text tag written into RUN_START (default: unlabeled)
 *     interval_ms    target time between heartbeats (default: 200)
 *     max_iterations 0 = run until SIGINT/SIGTERM (default: 0)
 *
 * Stop cleanly with SIGINT or SIGTERM (logs RUN_END, releases cleanly).
 * A crash-class signal (SIGABRT/SIGSEGV/SIGBUS/SIGFPE) is caught, the
 * last known seq is logged via async-signal-safe writes, then the
 * process exits with 128+signum -- it does not attempt to continue.
 */

#define _POSIX_C_SOURCE 200809L
#define CL_TARGET_OPENCL_VERSION 300
#include <CL/cl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <stdatomic.h>

#define HB_GLOBAL_SIZE        64
#define DEFAULT_INTERVAL_MS   200
#define HANG_WARN_MS          2000   /* first watchdog warning threshold        */
#define HANG_WARN_REPEAT_MS   2000   /* repeat interval while still blocked     */
#define WATCHDOG_POLL_MS      200
#define REINIT_AFTER_FAILS    5      /* consecutive bad heartbeats -> fresh ctx */

/* ---------------------------------------------------------------- */
/* Kernel source is read from disk, same convention as the B1/B3    */
/* probes, so this file can sit next to them under src/ + probes/.  */
/* ---------------------------------------------------------------- */

static char *read_file(const char *path, size_t *size_out)
{
    FILE *f = fopen(path, "rb");
    if (!f)
        return NULL;

    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    rewind(f);

    char *buf = malloc((size_t)n + 1);
    if (!buf) {
        fclose(f);
        return NULL;
    }

    if (fread(buf, 1, (size_t)n, f) != (size_t)n) {
        free(buf);
        fclose(f);
        return NULL;
    }

    buf[n] = '\0';
    fclose(f);
    *size_out = (size_t)n;
    return buf;
}

/* ---------------------------------------------------------------- */
/* Clocks                                                           */
/* ---------------------------------------------------------------- */

static long long mono_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

static double wall_s(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

/* ---------------------------------------------------------------- */
/* Logging: single mutex, unbuffered stdout, one line per event.    */
/* Rohlogs -- no summarization, one machine-parseable line per row. */
/* ---------------------------------------------------------------- */

static pthread_mutex_t g_log_mtx = PTHREAD_MUTEX_INITIALIZER;

static void log_line(const char *fmt, ...)
{
    va_list ap;
    pthread_mutex_lock(&g_log_mtx);
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    fflush(stdout);
    pthread_mutex_unlock(&g_log_mtx);
}

/* ---------------------------------------------------------------- */
/* Signal handling                                                  */
/*                                                                    */
/* graceful_handler: SIGINT/SIGTERM -> just set a flag, main loop    */
/* finishes the current iteration and exits cleanly (RUN_END).       */
/*                                                                    */
/* crash_handler: SIGABRT/SIGSEGV/SIGBUS/SIGFPE -> the process state  */
/* may be corrupted, so this handler uses ONLY async-signal-safe      */
/* primitives (write(), no malloc/printf/strsignal) to record the     */
/* last known sequence number before exiting.                        */
/* ---------------------------------------------------------------- */

static volatile sig_atomic_t g_stop_requested = 0;
static volatile sig_atomic_t g_last_seq_sig = -1;

static int async_safe_utoa(char *dst, unsigned long v)
{
    char tmp[24];
    int n = 0;
    if (v == 0) {
        dst[0] = '0';
        return 1;
    }
    while (v > 0) {
        tmp[n++] = (char)('0' + (v % 10));
        v /= 10;
    }
    for (int i = 0; i < n; i++)
        dst[i] = tmp[n - 1 - i];
    return n;
}

static int async_safe_copy(char *dst, const char *src)
{
    int n = 0;
    while (src[n]) {
        dst[n] = src[n];
        n++;
    }
    return n;
}

static void graceful_handler(int sig)
{
    (void)sig;
    g_stop_requested = 1;
}

static void crash_handler(int sig)
{
    char buf[160];
    int len = 0;

    len += async_safe_copy(buf + len, "SIGNAL sig=");
    len += async_safe_utoa(buf + len, (unsigned long)sig);
    len += async_safe_copy(buf + len, " last_seq=");
    if (g_last_seq_sig < 0)
        len += async_safe_copy(buf + len, "NA");
    else
        len += async_safe_utoa(buf + len, (unsigned long)g_last_seq_sig);
    len += async_safe_copy(buf + len, "\n");

    ssize_t written = write(STDOUT_FILENO, buf, (size_t)len);
    (void)written; /* best effort; nothing sane to do if this fails */

    _exit(128 + sig);
}

static void install_signal_handlers(void)
{
    struct sigaction sa_graceful;
    memset(&sa_graceful, 0, sizeof(sa_graceful));
    sa_graceful.sa_handler = graceful_handler;
    sigaction(SIGINT, &sa_graceful, NULL);
    sigaction(SIGTERM, &sa_graceful, NULL);

    struct sigaction sa_crash;
    memset(&sa_crash, 0, sizeof(sa_crash));
    sa_crash.sa_handler = crash_handler;
    sigaction(SIGABRT, &sa_crash, NULL);
    sigaction(SIGSEGV, &sa_crash, NULL);
    sigaction(SIGBUS, &sa_crash, NULL);
    sigaction(SIGFPE, &sa_crash, NULL);
}

/* ---------------------------------------------------------------- */
/* Watchdog thread: observes, cannot cancel clFinish().              */
/* ---------------------------------------------------------------- */

static atomic_int      g_running = 1;
static atomic_int      g_in_finish = 0;
static atomic_llong    g_finish_start_mono_ns = 0;
static atomic_long     g_seq_atomic = -1;

static void sleep_ms(long ms)
{
    struct timespec ts;
    ts.tv_sec  = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
}

static void *watchdog_thread(void *arg)
{
    (void)arg;
    long long next_warn_ns = -1;

    while (atomic_load(&g_running)) {
        sleep_ms(WATCHDOG_POLL_MS);

        if (atomic_load(&g_in_finish)) {
            long long start = atomic_load(&g_finish_start_mono_ns);
            long long elapsed = mono_ns() - start;

            if (elapsed >= (long long)HANG_WARN_MS * 1000000LL) {
                long long now = mono_ns();
                if (next_warn_ns < 0 || now >= next_warn_ns) {
                    long seq = atomic_load(&g_seq_atomic);
                    log_line("WATCHDOG seq=%ld state=BLOCKED_IN_FINISH elapsed_ms=%.1f\n",
                             seq, elapsed / 1e6);
                    next_warn_ns = now + (long long)HANG_WARN_REPEAT_MS * 1000000LL;
                }
            }
        } else {
            next_warn_ns = -1;
        }
    }
    return NULL;
}

/* ---------------------------------------------------------------- */
/* OpenCL context lifecycle                                          */
/* ---------------------------------------------------------------- */

typedef struct {
    cl_platform_id   platform;
    cl_device_id     device;
    cl_context       context;
    cl_command_queue queue;
    cl_program       program;
    cl_kernel        kernel;
    cl_mem           out_buf;
} cl_handles_t;

static int init_context(cl_handles_t *h, int generation, const char *kernel_path)
{
    memset(h, 0, sizeof(*h));
    cl_int err;

    err = clGetPlatformIDs(1, &h->platform, NULL);
    log_line("INIT gen=%d clGetPlatformIDs=%d\n", generation, err);
    if (err != CL_SUCCESS) return 0;

    err = clGetDeviceIDs(h->platform, CL_DEVICE_TYPE_GPU, 1, &h->device, NULL);
    log_line("INIT gen=%d clGetDeviceIDs=%d\n", generation, err);
    if (err != CL_SUCCESS) return 0;

    h->context = clCreateContext(NULL, 1, &h->device, NULL, NULL, &err);
    log_line("INIT gen=%d clCreateContext=%d\n", generation, err);
    if (err != CL_SUCCESS) return 0;

    h->queue = clCreateCommandQueueWithProperties(h->context, h->device, NULL, &err);
    log_line("INIT gen=%d clCreateCommandQueueWithProperties=%d\n", generation, err);
    if (err != CL_SUCCESS) return 0;

    size_t src_size = 0;
    char *src = read_file(kernel_path, &src_size);
    if (!src) {
        log_line("INIT gen=%d read_file(%s)=FAIL\n", generation, kernel_path);
        return 0;
    }

    const char *sources[] = { src };
    h->program = clCreateProgramWithSource(h->context, 1, sources, &src_size, &err);
    log_line("INIT gen=%d clCreateProgramWithSource=%d\n", generation, err);
    if (err != CL_SUCCESS) { free(src); return 0; }

    err = clBuildProgram(h->program, 1, &h->device, NULL, NULL, NULL);
    log_line("INIT gen=%d clBuildProgram=%d\n", generation, err);
    free(src);
    if (err != CL_SUCCESS) return 0;

    h->kernel = clCreateKernel(h->program, "sec1_heartbeat", &err);
    log_line("INIT gen=%d clCreateKernel=%d\n", generation, err);
    if (err != CL_SUCCESS) return 0;

    h->out_buf = clCreateBuffer(h->context, CL_MEM_WRITE_ONLY,
                                 HB_GLOBAL_SIZE * sizeof(cl_uint), NULL, &err);
    log_line("INIT gen=%d clCreateBuffer=%d\n", generation, err);
    if (err != CL_SUCCESS) return 0;

    err = clSetKernelArg(h->kernel, 1, sizeof(cl_mem), &h->out_buf);
    log_line("INIT gen=%d clSetKernelArg(out)=%d\n", generation, err);
    if (err != CL_SUCCESS) return 0;

    return 1;
}

static void release_context(cl_handles_t *h)
{
    if (h->out_buf)  clReleaseMemObject(h->out_buf);
    if (h->kernel)   clReleaseKernel(h->kernel);
    if (h->program)  clReleaseProgram(h->program);
    if (h->queue)    clReleaseCommandQueue(h->queue);
    if (h->context)  clReleaseContext(h->context);
    memset(h, 0, sizeof(*h));
}

/* ---------------------------------------------------------------- */
/* Main                                                              */
/* ---------------------------------------------------------------- */

int main(int argc, char **argv)
{
    setvbuf(stdout, NULL, _IONBF, 0);

    const char *run_label     = (argc > 1) ? argv[1] : "unlabeled";
    long interval_ms          = (argc > 2) ? strtol(argv[2], NULL, 10) : DEFAULT_INTERVAL_MS;
    long max_iterations       = (argc > 3) ? strtol(argv[3], NULL, 10) : 0;
    const char *kernel_path   = (argc > 4) ? argv[4] : "probes/sec1_victim_heartbeat.cl";

    install_signal_handlers();

    log_line("RUN_START label=%s interval_ms=%ld max_iterations=%ld pid=%d wall_s=%.6f mono_ns=%lld\n",
              run_label, interval_ms, max_iterations, (int)getpid(), wall_s(), mono_ns());

    cl_handles_t h;
    int generation = 0;
    if (!init_context(&h, generation, kernel_path)) {
        log_line("FATAL initial context setup failed, aborting\n");
        return 1;
    }

    pthread_t wd_thread;
    atomic_store(&g_running, 1);
    pthread_create(&wd_thread, NULL, watchdog_thread, NULL);

    long seq = 0;
    int consecutive_fail = 0;

    while (!g_stop_requested && (max_iterations == 0 || seq < max_iterations)) {
        seq++;
        atomic_store(&g_seq_atomic, seq);
        g_last_seq_sig = (sig_atomic_t)seq;

        cl_uint seq32 = (cl_uint)seq;
        cl_int err_arg = clSetKernelArg(h.kernel, 0, sizeof(cl_uint), &seq32);

        long long t_start_mono = mono_ns();
        double    t_start_wall = wall_s();

        cl_int err_canary  = CL_SUCCESS;


        cl_int err_enqueue = err_arg;


        cl_int err_finish  = CL_SUCCESS;


        cl_int err_read    = CL_SUCCESS;
        int    result_ok   = 0;

        /* Diagnostic oracle for WRONG_RESULT classification. */
        int     mismatch_count     = 0;
        int     first_mismatch_idx = -1;
        cl_uint first_expected     = 0;
        cl_uint first_actual       = 0;
        int     zero_count         = 0;

        int     prev_pattern_count = 0;

        int     canary_count       = 0;
        cl_uint actual0_7[8]       = {0};

        /*


         * V3: establish a known non-zero state immediately before dispatch.


         */


        cl_uint canary[HB_GLOBAL_SIZE];


        for (int i = 0; i < HB_GLOBAL_SIZE; i++)


            canary[i] = 0xC0DE0000u ^ (cl_uint)i;



        err_canary = clEnqueueWriteBuffer(


            h.queue, h.out_buf, CL_TRUE, 0,


            sizeof(canary), canary, 0, NULL, NULL);



        if (err_canary == CL_SUCCESS && err_arg == CL_SUCCESS) {


            size_t global = HB_GLOBAL_SIZE;


            err_enqueue = clEnqueueNDRangeKernel(h.queue, h.kernel, 1, NULL,


                                                 &global, NULL, 0, NULL, NULL);


        } else if (err_canary != CL_SUCCESS) {


            err_enqueue = err_canary;


        }

        if (err_enqueue == CL_SUCCESS) {
            atomic_store(&g_finish_start_mono_ns, mono_ns());
            atomic_store(&g_in_finish, 1);
            err_finish = clFinish(h.queue);
            atomic_store(&g_in_finish, 0);
        }

        if (err_enqueue == CL_SUCCESS && err_finish == CL_SUCCESS) {
            cl_uint result[HB_GLOBAL_SIZE];
            err_read = clEnqueueReadBuffer(h.queue, h.out_buf, CL_TRUE, 0,
                                            sizeof(result), result, 0, NULL, NULL);
            if (err_read == CL_SUCCESS) {
                /*
                 * Inspect all 64 words rather than stopping at the first
                 * mismatch.  This lets post-processing distinguish:
                 *
                 *   - exact previous-heartbeat contents (stale result),
                 *   - all/mostly zero data,
                 *   - partial writes,
                 *   - other corruption patterns.
                 */
                for (int i = 0; i < HB_GLOBAL_SIZE; i++) {
                    cl_uint expected      = seq32 + (cl_uint)i + 1u;
                    cl_uint prev_expected = seq32 + (cl_uint)i;

                    if (i < 8)
                        actual0_7[i] = result[i];

                    if (result[i] == 0u)
                        zero_count++;

                    if (result[i] == prev_expected)


                        prev_pattern_count++;



                    if (result[i] == canary[i])


                        canary_count++;

                    if (result[i] != expected) {
                        if (first_mismatch_idx < 0) {
                            first_mismatch_idx = i;
                            first_expected = expected;
                            first_actual = result[i];
                        }
                        mismatch_count++;
                    }
                }

                result_ok = (mismatch_count == 0);
            }
        }

        long long t_end_mono = mono_ns();
        double elapsed_ms = (t_end_mono - t_start_mono) / 1e6;

        const char *status;
        if (err_canary != CL_SUCCESS)       status = "CANARY_FAIL";

        else if (err_enqueue != CL_SUCCESS) status = "ENQUEUE_FAIL";
        else if (err_finish != CL_SUCCESS)  status = "FINISH_FAIL";
        else if (err_read != CL_SUCCESS)    status = "READ_FAIL";
        else if (!result_ok)                status = "WRONG_RESULT";
        else                                 status = "OK";

        log_line("HB seq=%ld gen=%d mono_ns=%lld wall_s=%.6f elapsed_ms=%.2f "
                 "canary_err=%d enqueue_err=%d finish_err=%d read_err=%d "
                 "result_ok=%d status=%s\n",
                 seq, generation, t_start_mono, t_start_wall, elapsed_ms,
                 (int)err_canary, (int)err_enqueue, (int)err_finish,
                 (int)err_read, result_ok, status);

        if (strcmp(status, "WRONG_RESULT") == 0) {
            log_line(
                "MISMATCH seq=%ld gen=%d count=%d first_idx=%d "
                "expected=%u actual=%u zero_count=%d "
                "prev_pattern_count=%d stale_prev_full=%d "
                "canary_count=%d canary_full=%d all_zero=%d "
                "actual0_7=%u,%u,%u,%u,%u,%u,%u,%u\n",
                seq, generation,
                mismatch_count,
                first_mismatch_idx,
                (unsigned)first_expected,
                (unsigned)first_actual,
                zero_count,
                prev_pattern_count,

                prev_pattern_count == HB_GLOBAL_SIZE,

                canary_count,

                canary_count == HB_GLOBAL_SIZE,

                zero_count == HB_GLOBAL_SIZE,
                (unsigned)actual0_7[0],
                (unsigned)actual0_7[1],
                (unsigned)actual0_7[2],
                (unsigned)actual0_7[3],
                (unsigned)actual0_7[4],
                (unsigned)actual0_7[5],
                (unsigned)actual0_7[6],
                (unsigned)actual0_7[7]);
        }

        if (strcmp(status, "OK") == 0)
            consecutive_fail = 0;
        else
            consecutive_fail++;

        if (consecutive_fail >= REINIT_AFTER_FAILS) {
            log_line("REINIT_ATTEMPT seq=%ld after_consecutive_fail=%d\n", seq, consecutive_fail);
            release_context(&h);
            generation++;
            int ok = init_context(&h, generation, kernel_path);
            log_line("REINIT_RESULT seq=%ld gen=%d ok=%d\n", seq, generation, ok);
            consecutive_fail = 0;
            if (!ok) {
                log_line("FATAL reinit failed, aborting\n");
                break;
            }
        }

        long long spent_ns = mono_ns() - t_start_mono;
        long long sleep_ns = (long long)interval_ms * 1000000LL - spent_ns;
        if (sleep_ns > 0) {
            struct timespec ts;
            ts.tv_sec  = sleep_ns / 1000000000LL;
            ts.tv_nsec = sleep_ns % 1000000000LL;
            nanosleep(&ts, NULL);
        }
    }

    atomic_store(&g_running, 0);
    pthread_join(wd_thread, NULL);

    release_context(&h);

    log_line("RUN_END last_seq=%ld reason=%s wall_s=%.6f mono_ns=%lld\n",
              seq, g_stop_requested ? "STOP_REQUESTED" : "MAX_ITER_REACHED",
              wall_s(), mono_ns());

    return 0;
}
