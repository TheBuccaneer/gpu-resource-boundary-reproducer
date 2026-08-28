# H1 B1 Resource Capacity — Final Gate

Status: CLOSED on H1
Date: 2026-08-28

## Primary finding

AMD Radeon 660M / GFX10.3

CL_DEVICE_LOCAL_MEM_SIZE:
65536 B

Dynamic local memory:

65536 B -> PASS / F0
65537 B -> F2 + F5

Smallest failing delta:
+1 byte

## Root cause

The per-workgroup LDS capacity invariant is not revalidated when
dynamically sized OpenCL local-memory requirements are incorporated
at dispatch time.

For 65537 B, Mesa 26.2 runtime instrumentation directly observed:

variable_shared_size = 65537
static shared size    = 0
total shared size     = 65537
LDS blocks            = 129
device limit          = 65536
RSRC2 before          = 0x0000008a
RSRC2 after           = 0x0040808a

The dispatch reaches the GPU and is followed by a compute scheduler
timeout and successful compute-ring recovery instead of controlled
OpenCL resource failure.

## Empirical version scope

Confirmed:

- mesa-opencl-icd 26.0.3-1ubuntu1
- locally built Mesa 26.2.0 release, b_ndebug=true

## Failure scope

Confirmed:
- F2 originating process failure
- F5 compute-ring / engine reset

Not established:
- F4 cross-client impact
- F6 unrelated GPU/display failure
- F7 cross-context leakage/corruption
- F8 stronger memory-safety violation

No hardware-level zero-occupancy mechanism is claimed.

## Decision

CLOSE B1 for H1 discovery.

Future work:
cross-implementation and cross-hardware reproduction.

Next structured class:
B2 Workgroup / Dispatch Geometry.
