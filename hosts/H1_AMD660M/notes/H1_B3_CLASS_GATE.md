# H1 B3 — Combined Resources — Class Gate

Date: 2026-08-28
Platform: Rusticl / radeonsi / AMD Radeon 660M (GFX10.3)
Kernel: 7.0.0-30-generic

## Class decision

B3 is CLOSED on H1.

Classification:
- Bug class: B3 Combined Resources
- Enforcement span: L3-L6
- Failure class: F2 + F5

## C0 — Capability / composition baseline

Two dynamic `__local` arguments are supported.

Observed:
- device local memory: 65536 B
- 4096 + 8192 B reported as 12288 B
- no failure

Result: PASS / F0.

## C1 — Valid execution baseline

A = 4096 B
B = 8192 B

Both local objects were accessed at first and last byte.
All canaries correct.

Result: PASS / F0.

## C2 — Normal combined load

A = 16384 B
B = 16384 B
Total = 32768 B

Result: PASS / F0.

## C3 — Boundary minus one

A = 32768 B
B = 32767 B
Raw total = 65535 B

Result: PASS / F0.

## C4 — Exact raw boundary

A = 32768 B
B = 32768 B
Raw total = 65536 B

Observed:
- CL_DEVICE_LOCAL_MEM_SIZE = 65536
- CL_KERNEL_LOCAL_MEM_SIZE = 65536
- enqueue / finish / read succeed
- all canaries correct

Result: PASS / F0.

## C5 — Combined plus one

A = 32768 B
B = 32769 B

Each argument individually fits within the advertised 65536-byte device
local-memory limit, but their raw sum is 65537 B.

Observed:
- CL_KERNEL_LOCAL_MEM_SIZE = 65537
- clEnqueueNDRangeKernel = CL_SUCCESS
- clFinish = CL_SUCCESS
- AMDGPU compute-ring timeout
- ring reset succeeded
- originating process terminated with SIGABRT

Result: INTERESTING / F2 + F5.

## C6 — Alignment-hidden capacity violation

Source analysis showed that Rusticl uses different accounting semantics for
the informational local-memory query and the actual dispatch layout.

CL_KERNEL_LOCAL_MEM_SIZE:
- sums raw LocalMem argument sizes

Dispatch layout:
- aligns each LocalMem argument before adding its size
- alignment is up to 128 bytes for the tested arguments

Adversarial case:

A = 32767 B
B = 32769 B

API-visible raw sum:
32767 + 32769 = 65536 B

Thus:
- CL_DEVICE_LOCAL_MEM_SIZE = 65536
- CL_KERNEL_LOCAL_MEM_SIZE = 65536

However, Rusticl's actual layout is:

A offset = 0
A end = 32767
B offset = align(32767, 128) = 32768
B size = 32769

Actual dynamic layout:
32768 + 32769 = 65537 B

The production Mesa 26.0.3 stack accepted this dispatch and produced:
- clEnqueueNDRangeKernel = CL_SUCCESS
- clFinish = CL_SUCCESS
- compute-ring timeout
- ring recovery
- SIGABRT of the originating process

Result: INTERESTING / F2 + F5.

## Instrumented confirmation

Mesa 26.2 release build:
- buildtype: release
- b_ndebug: true

Instrumented libRusticlOpenCL:
- Build ID: af712751d070b15e88f9b59dc7851a3d59509bd1
- SHA256: 262fb1e449da668a6a97dd88be69529ce178fd5bd27914ba32eb16155c03e409

Runtime used local libdrm 2.4.133.

Exact 65536-byte control:

B1_EH5 variable=65536 static=0 total=65536
lds_blocks=128 limit=65536

Execution succeeded.

Alignment-hidden case:

B1_EH5 variable=65537 static=0 total=65537
lds_blocks=129 limit=65536

Despite total > limit:
- dispatch was submitted
- compute ring timed out
- ring reset succeeded
- originating context was lost
- process exited through Mesa's non-robust-context abort path

## Root cause

Rusticl computes the actual dynamic local-memory layout before dispatch,
including per-argument alignment.

The resulting aligned requirement is passed to Gallium as
`variable_shared_mem`.

Radeonsi then computes:

    shared_size = shader->info.shared_size + variable_shared_size
    lds_blocks = align(shared_size, 512) >> 9

For the alignment-hidden reproducer:

    shared_size = 65537
    lds_blocks = 129
    lds_size_per_workgroup = 65536

The dynamic OpenCL path checks only that the encoded LDS block count fits
the register field (`lds_blocks <= 0xFF`). It does not revalidate the
resulting requirement against `lds_size_per_workgroup`.

## Main finding

Rusticl can report a kernel local-memory requirement equal to the
advertised device limit while its aligned dispatch layout actually exceeds
that limit.

The oversized requirement is propagated into radeonsi, encoded and
submitted rather than rejected before device execution.

On the tested GFX10.3 system this causes an AMDGPU compute-ring timeout and
ring-level recovery.

## Security interpretation

Observed:
- F2: originating process terminated
- F5: compute-ring timeout and successful ring reset

Not observed / not established:
- F4 cross-client impact
- F6 display failure
- F7 cross-context corruption or leakage
- F8 stronger memory-safety violation

The current result is therefore primarily an availability and failure-
containment finding.

## Class gate

Decision: CLOSE B3 on H1.

Rationale:
- valid baseline established
- exact boundary established
- smallest relevant excess demonstrated
- compositional case demonstrated
- alignment-hidden case demonstrated
- source-level root cause identified
- instrumented release build confirmed the complete accounting path
- kernel and userspace failure propagation captured
- additional LDS variants are unlikely to materially strengthen the H1 result

Retain representative cases for later cross-implementation testing:
1. 32768 + 32768 — exact valid boundary
2. 32768 + 32769 — raw combined +1
3. 32767 + 32769 — query-valid / aligned-dispatch +1
