# OpenCL2 Project State

Updated: 2026-10-03

## H1 platform

Primary discovery platform:

- AMD Rembrandt
- Rusticl / radeonsi
- OpenCL 3.0
- Linux / AMDGPU

H1 is used for deep discovery and mechanism work.
Cross-platform generalization is deferred to the later differential phase.

## Core bug classes

### B1 — Resource limit / dynamic local memory

**CLOSED — FINDING**

A one-byte local-memory boundary crossing can be accepted through the API and
reach dispatch, followed by an AMDGPU compute-ring timeout/reset.

Use precise recovery wording:

- compute-ring timeout
- compute-ring reset
- ring-level recovery

Do not call every such event an unconditional full GPU reset.

### B2 — Work-group geometry

**CLOSED — POSITIVE CONTROL**

Specification-defined invalid work-group geometry is rejected as expected.

### B3 — Combined local-memory accounting

**CLOSED ON H1 — FINDING**

Combined local-memory accounting exposes a dispatch-boundary problem.

Important distinction:

- plain over-limit trigger: 32768 + 32769 = 65537 B
- alignment-hidden case: raw/API-visible sum can remain 65536 B while the
  aligned effective footprint exceeds the device limit

Do not conflate these variants.

### B4 — SVM lifetime / event-ordered free

**CLOSED ON H1 — FINDING**

A valid event-ordered SVM-free sequence can expose premature callback/free
behavior and same-VA reuse while older GPU work is still relevant.

Source-level Rusticl mechanism documented separately.

### B5 — Raw device pointer / device-address behavior

**CLOSED ON H1 — CHARACTERIZATION**

Same-process cross-context device-address dereference was observed.

Do not describe this alone as a formal cross-context protection-domain
violation.

### B6

**N/A / DEFERRED ON H1**

Re-evaluate on later hardware if the required capability exists.

## Security extensions

### SEC-1 / O1 — Cross-client failure containment

**CLOSED ON H1/RUSTICL — FINDING**

Independent victim clients can experience availability effects from another
client's induced compute failure.

Observed recovery classes include:

- successful compute-ring recovery with victim survival;
- transient victim latency;
- wider recovery/context-loss cases after failed ring recovery.

Generalization belongs to DIFF-1.

Follow-ups completed:

- canary V3 oracle validation
- GUI-less control

### SEC-2 — LDS remanence / isolation

**CLOSED ON H1 — FINDING**

Observable LDS remanence can survive queue/context changes in the absence of
cleanup.

AMDGPU cleaner execution is strongly associated with disappearance of the
victim pattern.

Do not claim a deterministic LDS lifetime.

### O2 — Post-reset LDS leakage

**CLOSED ON H1 — SENSITIVITY-LIMITED / INCONCLUSIVE FOR RESET-SPECIFIC
LDS CLEARING; NO POSITIVE POST-RESET LEAKAGE EVIDENCE**

The B3 recovery interval is much longer than the demonstrated sensitive
interval of the H1 LDS oracle.

A matched no-reset delay loses the victim pattern as well.

Therefore H1 cannot distinguish reset-specific LDS clearing from ordinary
loss of observable remanence over that interval.

The general post-reset LDS question remains open for differential hardware
testing.

## Differential phase

### DIFF-1

**LATER**

Re-run the final scientifically relevant suite across additional OpenCL
implementations / hardware.

N/A capabilities and implementation-specific findings must be revisited there.

## B7 — Object / Context Ownership

**CLOSED ON H1 — FINDING / SOURCE-CONFIRMED / FIX-CONFIRMED**

Ordinary foreign-context `cl_mem` was accepted by Rusticl as a kernel
argument and reached dispatch, modifying the foreign buffer in both tested
context directions. The installed Mesa 26.0.8 source confirms the omitted
context check. A controlled Mesa 26.2.0 baseline versus local-patch run
reproduced acceptance and foreign access on baseline, then rejected the
argument with `CL_INVALID_CONTEXT` on the fix while positive controls passed.

Canonical finding and provenance:
`notes/H1_B7_CONTEXT_OWNERSHIP_FINDING.md` and
`results/b7/20261003_082740_mesa262_valid_baseline_vs_b7fix/`.
The earlier 08:25 run was discarded for a libdrm mismatch. Do not cite it
as fix evidence. The result establishes a same-process, same-device OpenCL
context-ownership violation; it does not establish cross-process access.

## Main-line next blocks

B8 representation mismatch, then B9 integer/arithmetic backend, B10
synchronization/state, and B12 external semaphore/interop as H1 permits.
B4 historical comparison and any remaining B7 patch/CTS follow-up are
separate paper-hardening tasks; they do not reopen the closed H1 B7 block.

## Documentation rule

For each meaningful closed block preserve the question, platform, oracle,
controls, raw provenance, finding, confounders, supported claims, non-claims,
and reopen criteria.
