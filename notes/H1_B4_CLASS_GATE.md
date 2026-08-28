# H1 B4 Class Gate — SVM Lifetime / Ordering

Date: 2026-08-28
Device: AMD Radeon 660M / Rembrandt
Implementation: Rusticl / radeonsi / ACO
OpenCL: 3.0 device, evaluated against current unified OpenCL 3.1.1 specification

## Decision

B4: CLOSE — CONFIRMED FINDING

Highest observed impact:
F8 — temporal cross-lifetime memory-integrity violation within one OpenCL context.

Not demonstrated:
- cross-context corruption
- cross-process corruption
- privilege-boundary escape
- information disclosure across security principals

Therefore do not classify as F7.

## Specification oracle

OpenCL clEnqueueSVMFree specifies that events in event_wait_list
need to complete before clEnqueueSVMFree can be executed.

For a non-NULL pfn_free_func, the implementation calls that callback
to free the supplied SVM pointers.

Observed Rusticl behavior violates this ordering requirement:
the SVM-free callback executes while the dependency kernel event
still reports CL_SUBMITTED.

## Capability baseline

H1 supports:
- coarse-grain buffer SVM
- clSVMAlloc / clSVMFree
- clSetKernelArgSVMPointer
- clEnqueueSVMMap / Unmap / Memcpy / MemFill / Free

Fine-grain buffer/system SVM and SVM atomics are unavailable.

## B4-C1 — valid baseline

Valid map/write/unmap -> kernel -> map/read lifecycle.

Result:
PASS / F0

Expected incremented data observed.

## B4-C2 — repeated valid lifetime baseline

16 valid allocation/use/free cycles.

Result:
PASS / F0

Observed deterministic immediate VA reuse:
cycle 1-15 reused the preceding SVM address.

reuse_previous_count=15

This is not itself a defect; it established the reuse baseline.

## B4-C3 — event-ordered clEnqueueSVMFree

Sequence:
kernel K using SVM A
-> clEnqueueSVMFree(A, wait_list={K})
-> wait for free event
-> read independent cl_mem output

Expected sum:
524800

Observed:
0

Result:
FAIL / F1

The application does not dereference A after its lifetime ends.

## B4-C3a — control without enqueued free

Sequence:
kernel K
-> explicitly wait for K
-> read output
-> clSVMFree

Expected:
524800

Observed:
524800

Result:
PASS / F0

This validates the kernel, data initialization and output path.

## B4-C3b — direct ordering oracle

clEnqueueSVMFree uses kernel K as an explicit event dependency.
The free callback queries K's execution status when invoked.

Mesa 26.0.3:
callback_kernel_status = 2 (CL_SUBMITTED)

Mesa 26.2:
callback_kernel_status = 2 (CL_SUBMITTED)

Result:
CONFIRMED ORDERING VIOLATION

The host-side SVM-free operation is executed before the dependency
kernel has completed.

## Source-level root-cause evidence

Rusticl queue worker distinguishes same-queue dependencies from
cross-queue/user-event dependencies.

For same-queue dependency:
    dep.status()

rather than:
    dep.wait()

is used before calling the dependent Event work closure.

Event work closures execute during submission; hardware completion
is signaled later through the fence/signal path.

clEnqueueSVMFree implements freeing as host-side Event work.

Therefore normal same-queue GPU ordering assumptions are incorrectly
applied to a host-side lifetime operation: the SVM-free work executes
after kernel submission rather than after kernel completion.

SVM kernel arguments are stored as raw SVM pointer values.

During dispatch Rusticl resolves SVM allocations and creates device
resource references, including copy_svm_to_dev() / PipeResourceOwned.

The exact lower-level BO/GPUVM remap mechanism responsible for the
observed cross-lifetime write is not claimed as fully root-caused yet.

## B4-C3c2 — host-side ABA/reallocation control

Sequence:
A -> long-running K_A
-> premature callback/free
-> B allocated immediately at same VA
-> initialize B host-side
-> wait for K_A

Observed:
callback at ~0.5 ms while K_A remained CL_SUBMITTED
same_addr=1
B remained unchanged

Result:
ACTIVE_KERNEL_NO_CORRUPTION

This initially demonstrated that simple host-side reuse alone was
insufficient to expose integrity corruption.

## B4-C3d2 — device-side new-lifetime access

Sequence:

1. Allocate SVM A.
2. Initialize A.
3. Launch long-running kernel K_A using A.
4. Enqueue clEnqueueSVMFree(A, wait_list={K_A}).
5. Rusticl invokes the free callback while K_A is CL_SUBMITTED.
6. Callback frees A using clSVMFree.
7. Allocate new SVM object B.
8. B receives the same virtual address as A.
9. Initialize:
       B[0] = 0xbbbbbbbb
       B[1] = 0xcccccccc
10. While K_A is still CL_SUBMITTED, enqueue K_B using only B.
11. K_A later writes:
       0xa5a5a5a5
12. K_B reads B[0] and writes B[1].

Observed:
B_device_read = 0xa5a5a5a5
B0_final      = 0xa5a5a5a5
B1_final      = 0xeeeeeeee

Expected:
B_device_read = 0xbbbbbbbb
B0_final      = 0xbbbbbbbb
B1_final      = 0xeeeeeeee

Result:
TEMPORAL CROSS-LIFETIME MEMORY-INTEGRITY VIOLATION

## Mesa 26.0.3 reproducibility

5 independent processes.

All 5/5:
- callback_old_status = CL_SUBMITTED
- same_addr = 1
- old kernel still CL_SUBMITTED before B kernel
- B_device_read = 0xa5a5a5a5
- B0_final = 0xa5a5a5a5
- B1_final = 0xeeeeeeee
- probe exit = 38

Result:
5/5 reproduced.

Result-file SHA256:

855734be87a0dda16071354b6c0f5db54effa394789fc9c5cf19987cf93b7d5b
830521395d79c664e46a9c3214563123fb4116d65438ce58e5c9ec73fef22d15
6b66df002209c156b4a92e633c680a20133c7010b9e26b3500f3754be58ab202
91daffeaccf326129d98e4adda45d7e8954bb7e4f6ccbba4f58a98c9415f2539
a82cd0fcd6c22eb8813cae231d39e6cbb5420a9abfe3eaded8ad56db94abc92d

## B4-C3d3 — no-write causal control

The old kernel remains long-running and reads A but its final write
to A[0] is removed.

All other relevant lifetime and reuse conditions remain.

Mesa 26.0.3 observed:
callback_old_status = CL_SUBMITTED
same_addr = 1
old kernel active during B setup

B_device_read = 0xbbbbbbbb
B0_final      = 0xbbbbbbbb
B1_final      = 0xeeeeeeee

Result:
CONTROL PASS

This rules against simple stale initial contents as the explanation
for the C3d2 value.

Source SHA256:
3e220fb6481a57499f0fae9f361ca216a3e95d874802ed44b822fa97152f09a4

Binary SHA256:
1e26885dc05eef3fcb1a026a9bf3bf880c651f662e6bdebb82af662a84c80b56

Result SHA256:
a00463fb226482e6af01dfb3a15a5605cff55d55301bc24be855536a7dcaeb29

## Mesa 26.2 reproduction

Mesa Rusticl library SHA256:
262fb1e449da668a6a97dd88be69529ce178fd5bd27914ba32eb16155c03e409

libdrm SHA256:
12aab45c6d1e23a3e1b7988b50c31220e77836fa4d5ad1d7998df3a4cd11c790

libdrm_amdgpu SHA256:
3db52cf8007d56c2e51c23c189f7e69aaba66b68faa6e0701c6f83cec317d79d

C3d2 was reproduced independently 5/5.

All 5/5:
- callback_old_status = CL_SUBMITTED
- same_addr = 1
- old kernel still CL_SUBMITTED before B kernel
- B_device_read = 0xa5a5a5a5
- B0_final = 0xa5a5a5a5
- B1_final = 0xeeeeeeee
- probe exit = 38

Result hashes:

606019fc28f3da9781e5f92c2d36fc98d5d781e767992b09919f4447f1c09989
a455169db334f9f9b0ff8c34c75e4fe5a87fcaaedcc7404c72d2a13e2a1cfc2f
734c8d64f001805e59cfcf83abc6e89969a12d67356b7935dea9bf16918be373
44cbf83aacaa98d8807ff8ca5db58f8a319748ceff15eb9f82bb143310931b17
c42a9dcd483dcca77f26ea4b6fef29463eb9610431aa9da2ec7d4f3c197997b8

## Mesa 26.2 no-write control

Observed:
callback_old_status = CL_SUBMITTED
same_addr = 1
old kernel active during B setup

B_device_read = 0xbbbbbbbb
B0_final      = 0xbbbbbbbb
B1_final      = 0xeeeeeeee

Result:
CONTROL PASS

Result SHA256:
a04fffbf16d616226f1eb6c413c1a2836fe7764cc6af2e7d2e40cbd3cec6a7e1

## Causal interpretation

Evidence supports the following causal chain:

K_A is submitted using SVM allocation A
    ->
clEnqueueSVMFree explicitly depends on K_A
    ->
Rusticl executes its host-side free callback before K_A completes
    ->
A's OpenCL SVM lifetime terminates prematurely
    ->
a new valid allocation B reuses the same SVM virtual address
    ->
B is initialized while K_A remains active
    ->
K_A later performs its final write associated with the terminated A lifetime
    ->
the new B lifetime subsequently observes that written value

Removing only the old kernel's final SVM write removes the corruption
on both tested Mesa versions.

This provides causal evidence for a temporal cross-lifetime integrity
violation rather than merely stale initialization data.

## Enforcement-stage analysis

Contract:
event dependency for SVM lifetime termination

Knowledge time:
enqueue-time

Expected enforcement:
queue/event scheduling before execution of SVM-free work

Observed enforcement:
missing for same-queue dependency completion

Failure propagation:

API
 -> queue worker
 -> kernel submitted
 -> dependency status checked but not waited
 -> host-side SVM-free callback executed
 -> allocation lifetime ends
 -> VA reused
 -> predecessor GPU work remains active
 -> late predecessor write becomes visible in successor allocation

## Failure classification

Primary contract failure:
F1 — incorrect execution / specification violation

Highest observed downstream impact:
F8 — temporal memory-integrity / memory-safety violation
within one OpenCL context

Not F7:
no cross-context or cross-process corruption/leakage demonstrated.

## Security interpretation

This is stronger than a correctness-only bug because a logically
terminated allocation can affect a subsequent valid allocation.

However, current evidence is intra-context.

Do not currently claim:
- cross-process memory corruption
- cross-context information disclosure
- privilege escalation
- sandbox escape
- arbitrary address corruption

Security-boundary exploitability remains unestablished.

## Version status

Affected and reproduced:
- Mesa/Rusticl 26.0.3
- Mesa/Rusticl 26.2 release build

The relevant Rusticl event/SVM-free source pattern is present in both.

## B4 gate

B4 status:
CLOSE / CONFIRMED

Reason:
The class produced a specification-defined, reproducible,
cross-version finding with direct ordering oracle, source-level
mechanism evidence, observable incorrect computation, temporal
memory-integrity impact, and causal controls.

Further allocator pressure / artificial BO-recycling escalation is
not justified without a separate hypothesis.

Next structured class:
B5 — Raw Device-Pointer Authority
