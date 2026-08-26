# H1 B1 Evidence Hardening EH4 — AMDGPU Timeout/Reset Semantics

Date: 2026-08-26
Kernel under test: Linux 7.0.0-30-generic

## Relevant kernel path

Linux v7.0 amdgpu_job_timedout() handles a DRM scheduler job timeout.

The relevant sequence is:

1. create an AMDGPU device coredump immediately after the job timeout;
2. attempt soft recovery if supported;
3. report the ring timeout;
4. attempt a per-ring/per-queue reset if supported;
5. if that succeeds, restart the scheduler and report
   "Ring <name> reset succeeded";
6. only if ring-level recovery fails may the path proceed to broader
   GPU recovery.

## H1 +1-byte observation

Observed:

ring comp_1.1.0 timeout
Starting comp_1.1.0 ring reset
Ring comp_1.1.0 reset succeeded
device wedged, but recovered through reset

There was no observed transition in this event to the broader
amdgpu_device_gpu_recover path.

## Interpretation

The H1 event is therefore described as:

- DRM scheduler job timeout on compute ring comp_1.1.0;
- successful compute-ring/per-queue recovery;
- originating OpenCL process abort;
- F5 engine/ring reset.

Use "compute-ring reset", "ring-level recovery", or equivalent precise
wording.

Do not use the unqualified phrase "full GPU reset" for this event.

The DRM "device wedged, but recovered through reset" message does not by
itself establish a full-adapter reset.

## Causality limits

A ring timeout establishes that the submitted GPU job did not complete
within the scheduler timeout.

It does not identify the hardware-level reason for non-completion.

Therefore the timeout alone does not prove:
- zero occupancy;
- inability to allocate LDS;
- a particular scheduler deadlock mechanism.

The B1 LDS causal argument instead relies on the combined evidence:

- 65536-byte paired control: PASS;
- 65537-byte request: timeout/ring reset;
- no intentional access outside the requested local-memory object;
- Rusticl propagation of the dynamic local-memory requirement;
- radeonsi construction of the over-limit LDS resource state;
- absence of per-workgroup capacity revalidation in the examined path.

## Coredump timing

Linux v7.0 explicitly captures the AMDGPU device coredump immediately
after the scheduler timeout and before subsequent reset/recovery attempts.

The captured H1 device coredump is therefore evidence from the
pre-recovery timeout state, subject to the limitations of the Linux 7.0
AMDGPU coredump format.
