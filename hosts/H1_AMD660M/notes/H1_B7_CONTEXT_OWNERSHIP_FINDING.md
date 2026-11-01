# H1 B7 — Object / Context Ownership

Status: CLOSED — FINDING / SOURCE-CONFIRMED / FIX-CONFIRMED on H1/Rusticl

## Platform

- AMD Radeon 660M
- Rusticl / radeonsi
- Mesa 26.0.8-1ubuntu0.3
- Linux kernel 7.0.0-38-generic
- same device
- same process
- two distinct OpenCL contexts

## Question

Does the implementation enforce specification-defined context ownership for
ordinary OpenCL memory objects when they are passed as kernel arguments?

## C0 — API ownership validation

Two distinct contexts were created:

- ctxA
- ctxB

Each had its own queue, buffers, program/kernel objects.

Metadata controls confirmed:

- bufA belongs to ctxA
- bufB belongs to ctxB
- queues and kernels belong to their expected contexts

Negative seams:

- foreign ReadBuffer -> CL_INVALID_CONTEXT
- foreign WriteBuffer -> CL_INVALID_CONTEXT
- mixed-context CopyBuffer -> CL_INVALID_CONTEXT
- foreign kernel enqueue through queue from another context -> CL_INVALID_CONTEXT

But:

- clSetKernelArg(kernelB, ..., bufA)
  returned CL_SUCCESS
  where CL_INVALID_CONTEXT was expected

Reproducibility:

5/5 runs reproduced the same result.

All metadata and functional controls passed in all five runs.

Result directory:

results/b7/20261002_202625_c0_n4_repro

## C1 — foreign memory dispatch

Kernel belongs to ctxB.
Target buffer belongs to ctxA.

Precondition:

bufA = 0xA1A2A3A4

Observed:

- clSetKernelArg(kernelB, bufA) = CL_SUCCESS
- clEnqueueNDRangeKernel(qB, kernelB) = CL_SUCCESS
- clFinish(qB) = CL_SUCCESS
- legitimate readback of bufA through qA succeeds
- bufA becomes 0xB7C10001

Classification:

FOREIGN_ACCESS_OBSERVED

Single-run directory:

results/b7/20261002_203201_c1_foreign_mem_dispatch

C1 reproducibility:

5/5 runs:

- set_foreign success
- enqueue_foreign success
- finish_foreign success
- exact foreign marker read back

No relevant AMDGPU timeout/reset/fault messages were observed.

Repro directory:

results/b7/20261002_203518_c1_foreign_mem_repro

## C2 — reverse-direction control

Roles were reversed.

Kernel/program belong to ctxA.
Foreign target buffer belongs to ctxB.

Valid positive control:

kernel(ctxA) -> bufA(ctxA)

Foreign experiment:

kernel(ctxA) -> bufB(ctxB)

Observed:

- foreign target initially 0xB1B2B3B4
- clSetKernelArg = CL_SUCCESS
- enqueue = CL_SUCCESS
- clFinish = CL_SUCCESS
- legitimate readback through qB returns 0xB7C10001

Classification:

FOREIGN_ACCESS_OBSERVED

No relevant AMDGPU timeout/reset/fault messages were observed.

Result directory:

results/b7/20261002_204043_c2_reverse_foreign_mem

## Interpretation

On this H1 Rusticl/radeonsi stack, ordinary cl_mem objects from another
OpenCL context are accepted as kernel arguments and can be dereferenced by
the kernel successfully.

This is not merely a missing error return at clSetKernelArg:
the foreign memory object reaches execution and is modified.

The effect was observed in both context directions.

## Strongest supported claim

Rusticl/radeonsi on H1 fails to enforce specification-defined context
ownership for ordinary cl_mem kernel arguments, permitting a kernel from one
OpenCL context to access and modify a buffer created in another context
within the same process and device.

## Explicit non-claims

Do NOT claim from these experiments alone:

- cross-process access
- OS-process isolation failure
- arbitrary-address access
- access without possession of the cl_mem handle
- cross-device access
- a general GPU protection-domain vulnerability

OpenCL contexts are the tested API ownership boundary here; these experiments
do not by themselves establish that contexts are intended as hostile-process
security domains.

## Relation to B5

B5 observed same-process cross-context behavior through raw device addresses,
where the protection semantics were less clear.

B7 is stronger as a specification/ownership result because it uses ordinary
OpenCL object handles and an explicit context-validation requirement.

## Follow-up outside B7 closure

- Carry the independent-implementation comparison into DIFF-1.
- Preserve the CTS coverage assessment and regression-test proposal with the patch review.

## Source-confirmed mechanism

Exact source corresponding to the installed Ubuntu package
mesa-opencl-icd 26.0.8-1ubuntu0.3 was inspected.

In Rusticl's clSetKernelArg implementation:

- the cl_mem argument is resolved through Buffer::arc_from_raw()
- the resulting object is stored as KernelArgValue::Buffer(Weak<Buffer>)
- no comparison is performed between the memory object's context and
  kernel.prog.context

Kernel::set_kernel_arg() subsequently only replaces the stored argument value
and performs no context validation.

At enqueue time, clEnqueueNDRangeKernel explicitly checks only:

    command_queue.context == kernel.prog.context

It does not validate the contexts of already stored memory arguments.

During Kernel::launch(), stored Buffer arguments are upgraded to strong
references and used through buffer.get_res_for_access() / add_global()
without an intervening kernel-vs-buffer context ownership check.

This source path is consistent with the observed runtime behavior:

foreign cl_mem accepted at clSetKernelArg
    -> stored in kernel state
    -> queue/kernel validation passes
    -> foreign buffer reaches launch
    -> foreign buffer is modified successfully

### Mechanism status

SOURCE-CONFIRMED on Mesa/Rusticl 26.0.8-1ubuntu0.3.

### Root-cause statement

Rusticl 26.0.8 omits the specification-required context validation for
ordinary cl_mem kernel arguments in clSetKernelArg. No later effective
validation prevents the foreign-context buffer from reaching execution.


## Mesa 26.2.0 baseline versus local fix

The controlled differential run is
`results/b7/20261003_082740_mesa262_valid_baseline_vs_b7fix/`.
Both sides used the same boot, AMD Radeon 660M, probe binaries and local
libdrm 2.4.133 libraries. The per-side `meta.txt`, `c0.txt`, `c1.txt`,
`clinfo.txt`, and `kernel.txt` files preserve the runtime evidence.

| Observation | Unpatched baseline | Local B7 fix |
| --- | --- | --- |
| Precheck | PASS | PASS |
| C0 foreign `cl_mem` argument (N4) | `CL_SUCCESS` (0); ownership FAIL | `CL_INVALID_CONTEXT` (-34); ownership PASS |
| C0 metadata / functional controls | PASS / PASS | PASS / PASS |
| C1 foreign buffer | dispatch modified marker to `0xb7c10001`; `FOREIGN_ACCESS_OBSERVED` | rejected at set time with -34; `SET_TIME_REJECT` |
| C0 / C1 process exits | 30 / 0 | 0 / 0 |

The C0 baseline exit 30 is the probe's failing ownership oracle. C1 exits
zero on both sides because it classifies the observed outcome. The patch
changes the predicted ownership decision while the positive controls remain
functional, supporting a causal fix for the tested buffer path.

### Provenance and hashes (SHA-256)

- Mesa upstream base: `3821bf78426f5515d26047e49164beedb8ecd18d`
- Patch: `patches/b7/rusticl_clSetKernelArg_context_validation.patch` — `418b0c8870544f4de777b10ec7938d80437789dc9beb3ee8dadc42065b5a32a5`
- Baseline Rusticl library: `262fb1e449da668a6a97dd88be69529ce178fd5bd27914ba32eb16155c03e409`
- Patched Rusticl library: `8edfdb04646bfb8bbe9e0efddb02a375b988b6566500931a5c40f8522a9ed423`
- Shared `libdrm.so`: `12aab45c6d1e23a3e1b7988b50c31220e77836fa4d5ad1d7998df3a4cd11c790`
- Shared `libdrm_amdgpu.so`: `3db52cf8007d56c2e51c23c189f7e69aaba66b68faa6e0701c6f83cec317d79d`

Patch provenance is recorded in `patches/b7/PROVENANCE.txt`.
The earlier run at
`results/b7/20261003_082503_mesa262_baseline_vs_b7fix/` is discarded:
its runtime did not load the matching local libdrm, and both sides failed
GPU/device initialization (`clGetDeviceIDs=-1`, C0/C1 exit 4). It provides
no baseline-versus-fix evidence.

## Closure

B7 is CLOSED on H1: runtime finding, installed-source mechanism, and local
Mesa 26.2.0 fix differential are confirmed. The local patch is evidence of
fixability, not a claim of upstream acceptance or cross-implementation
validation. Reopen only for a concrete new ownership path or contradictory
evidence; generalization belongs to DIFF-1.
