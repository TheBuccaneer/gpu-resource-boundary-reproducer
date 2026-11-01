# H1-B1-DYN-05 — Dynamic Local Memory Boundary + 4 B

## Metadata

- Date: 2026-08-26
- Hardware: H1 — AMD Radeon 660M / Rembrandt
- OpenCL implementation: Rusticl
- Mesa: 26.0.3-1ubuntu1
- Linux kernel: 7.0.0-30-generic
- Backend: radeonsi / ACO
- Bug class: B1 Resource Capacity
- Stage: C5 Boundary + delta
- Kernel complexity: K1
- Maximum observed failure class: F5

## Device contract

CL_DEVICE_LOCAL_MEM_SIZE = 65536 bytes

## Test input

Dynamic __local allocation:

- requested: 65540 bytes
- advertised device limit: 65536 bytes
- delta: +4 bytes
- number of requested int elements: 16385
- last requested int index: 16384

The kernel accesses only addresses inside the size requested via
clSetKernelArg. It does not intentionally access beyond the requested
65540-byte local-memory object.

## Prior controls

The same dynamic-local path previously produced:

- 4096 B: PASS
- 32768 B: PASS
- 65532 B: PASS
- 65536 B: PASS

At 65532 B and 65536 B, canaries at both the first and the final valid
int were written and read successfully.

The static-local path also succeeds at 65536 B.

## Expected behavior

The requested per-workgroup local-memory requirement exceeds the
advertised device capacity.

A robust implementation should prevent execution of the infeasible
kernel through a controlled OpenCL error path. The exact validation
stage is part of RQ1 and is not assumed in advance.

A process abort, GPU timeout, or GPU reset is not considered a
controlled failure.

## Observed userspace behavior

Test command terminated with:

    amdgpu: The CS has cancelled because the context is lost.
    This context is guilty of a hard recovery.

Exit status:

    134

This is consistent with SIGABRT.

Because stdout was connected through tee and not explicitly made
unbuffered, the individual stdout API-stage messages were not preserved
before abort. Therefore this run alone does not establish the exact
last userspace API call completed before the failure.

## Observed kernel behavior

At 2026-08-26 10:47:28 the kernel reported:

    Dumping IP State
    AMDGPU device coredump file has been created
    ring comp_1.1.0 timeout, signaled seq=5675, emitted seq=5676
    Process dynamic_local_b pid 48188 thread dynamic_lo:cs0 pid 48190
    Starting comp_1.1.0 ring reset
    Ring comp_1.1.0 reset succeeded
    device wedged, but recovered through reset

The kernel explicitly associates the timeout with the
dynamic_local_boundary test process.

## Device coredump

The kernel reported creation of:

    /sys/class/drm/card1/device/devcoredump/data

An attempted retrieval at approximately 10:54 failed because the
devcoredump entry no longer existed.

No device-coredump artifact was preserved.

## Classification

Confirmed:

- F2: originating client terminates
- F5: GPU compute-ring reset
- compute ring: comp_1.1.0
- reset completed successfully
- device recovered after reset

Not established:

- F4: effect on a second OpenCL client
- F6: effect on other GPU applications or display
- F7: cross-context corruption/leakage
- F8: stronger memory-safety/isolation impact

Firefox/AppArmor messages occurring several seconds after the reset are
not evidence of F6 and are excluded from the failure classification.

## Current security interpretation

This is a strong availability / failure-containment finding.

A request exceeding CL_DEVICE_LOCAL_MEM_SIZE by only four bytes reached
GPU execution far enough to cause a compute-ring timeout and reset,
instead of being contained as a controlled OpenCL resource failure.

This result is not yet labeled a security vulnerability. Cross-client
or broader system impact has not been established.

## Comparison with static-local path

At +4 B above the same advertised limit:

Static __local:
    clCreateKernel -> CPU-side SIGFPE -> F2

Dynamic __local:
    GPU compute-ring timeout -> ring reset -> SIGABRT -> F5

This is an intra-implementation enforcement-path differential for the
same resource boundary.

## Gate

Result: INTERESTING

Decision:
- stop ordinary B1 progression temporarily;
- preserve evidence;
- reproduce under improved logging before any security escalation;
- determine exact userspace failure stage;
- then investigate root cause and version scope.

## Raw evidence

- results/b1/H1_B1_DYN_65540_boundary_plus4_2026-08-26.txt
- results/b1/H1_B1_DYN_65540_kernel_evidence_2026-08-26.txt
- results/b1/H1_B1_DYN_65540_kernel_full_2026-08-26.txt
