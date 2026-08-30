# OpenCL2 Project State

Updated: 2026-08-30

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

## Next regular block

### B7 — Object / Context Ownership

**NEXT**

Use specification-defined ownership/context-validation questions.

Do not turn B7 into another B5 raw-device-pointer exploration unless a result
specifically requires that comparison.

## Documentation rule

For each meaningful closed block preserve:

- question / hypothesis
- platform
- oracle
- experiments and controls
- raw provenance
- findings
- discarded hypotheses / confounders
- strongest supported claims
- explicit non-claims
- future-work / reopen criteria

## Immediate next action

Inspect the existing B7 definition and construct the B7 C0 capability /
baseline gate before running adversarial ownership tests.

## Exact handoff checkpoint — 2026-08-30

B7 has been opened, but no B7 runtime test has been executed yet.

Existing source:

    src/b7_context_ownership_probe.c

Existing binary:

    probes/b7_context_ownership_probe

Current pre-self-check source SHA256:

    bfafa8b08bc2b198ab1e0204a606f90d49f326dbc53155caa81dfec73970c135

Current pre-self-check binary SHA256:

    a9488588444759c5dc36bd42227c707df0c02e35f224717bec5b54eb8990507f

The probe already implements B7 N1-N5 foreign-context tests, but the currently
built version does not yet include N1-N5 in the final PASS/FAIL return oracle.

Next action:

1. Patch B7 probe so N1-N5 are stored separately as n1...n5.
2. Require every value to equal CL_INVALID_CONTEXT (-34).
3. Emit:
   - B7-C0-METADATA
   - B7-C0-OWNERSHIP
   - B7-C0-FUNCTIONAL
   - B7-C0
4. Rebuild and record new SHA256 values.
5. Execute the first B7-C0 runtime test.

Do not reopen O2 before this.
O2 H1 is closed as sensitivity-limited.

Paper gates to revisit after the structured H1 classes:

- Rusticl/radeonsi local-memory enforcement source audit.
- Minimal cross-stack B1/B3 comparison, ideally Intel NEO or pocl.

## Exact handoff checkpoint — 2026-08-30

B7 has been opened, but no B7 runtime test has been executed yet.

Existing source:

    src/b7_context_ownership_probe.c

Existing binary:

    probes/b7_context_ownership_probe

Current pre-self-check source SHA256:

    bfafa8b08bc2b198ab1e0204a606f90d49f326dbc53155caa81dfec73970c135

Current pre-self-check binary SHA256:

    a9488588444759c5dc36bd42227c707df0c02e35f224717bec5b54eb8990507f

The probe already implements B7 N1-N5 foreign-context tests, but the currently
built version does not yet include N1-N5 in the final PASS/FAIL return oracle.

Next action:

1. Patch B7 probe so N1-N5 are stored separately as n1...n5.
2. Require every value to equal CL_INVALID_CONTEXT (-34).
3. Emit:
   - B7-C0-METADATA
   - B7-C0-OWNERSHIP
   - B7-C0-FUNCTIONAL
   - B7-C0
4. Rebuild and record new SHA256 values.
5. Execute the first B7-C0 runtime test.

Do not reopen O2 before this.
O2 H1 is closed as sensitivity-limited.

Paper gates to revisit after the structured H1 classes:

- Rusticl/radeonsi local-memory enforcement source audit.
- Minimal cross-stack B1/B3 comparison, ideally Intel NEO or pocl.

## Exact handoff checkpoint — 2026-08-30

B7 has been opened, but no B7 runtime test has been executed yet.

Existing source:

    src/b7_context_ownership_probe.c

Existing binary:

    probes/b7_context_ownership_probe

Current pre-self-check source SHA256:

    bfafa8b08bc2b198ab1e0204a606f90d49f326dbc53155caa81dfec73970c135

Current pre-self-check binary SHA256:

    a9488588444759c5dc36bd42227c707df0c02e35f224717bec5b54eb8990507f

The probe already implements B7 N1-N5 foreign-context tests, but the currently
built version does not yet include N1-N5 in the final PASS/FAIL return oracle.

Next action:

1. Patch B7 probe so N1-N5 are stored separately as n1...n5.
2. Require every value to equal CL_INVALID_CONTEXT (-34).
3. Emit:
   - B7-C0-METADATA
   - B7-C0-OWNERSHIP
   - B7-C0-FUNCTIONAL
   - B7-C0
4. Rebuild and record new SHA256 values.
5. Execute the first B7-C0 runtime test.

Do not reopen O2 before this.
O2 H1 is closed as sensitivity-limited.

Paper gates to revisit after the structured H1 classes:

- Rusticl/radeonsi local-memory enforcement source audit.
- Minimal cross-stack B1/B3 comparison, ideally Intel NEO or pocl.

## Exact handoff checkpoint — 2026-08-30

B7 has been opened, but no B7 runtime test has been executed yet.

Existing source:

    src/b7_context_ownership_probe.c

Existing binary:

    probes/b7_context_ownership_probe

Current pre-self-check source SHA256:

    bfafa8b08bc2b198ab1e0204a606f90d49f326dbc53155caa81dfec73970c135

Current pre-self-check binary SHA256:

    a9488588444759c5dc36bd42227c707df0c02e35f224717bec5b54eb8990507f

The probe already implements B7 N1-N5 foreign-context tests, but the currently
built version does not yet include N1-N5 in the final PASS/FAIL return oracle.

Next action:

1. Patch B7 probe so N1-N5 are stored separately as n1...n5.
2. Require every value to equal CL_INVALID_CONTEXT (-34).
3. Emit:
   - B7-C0-METADATA
   - B7-C0-OWNERSHIP
   - B7-C0-FUNCTIONAL
   - B7-C0
4. Rebuild and record new SHA256 values.
5. Execute the first B7-C0 runtime test.

Do not reopen O2 before this.
O2 H1 is closed as sensitivity-limited.

Paper gates to revisit after the structured H1 classes:

- Rusticl/radeonsi local-memory enforcement source audit.
- Minimal cross-stack B1/B3 comparison, ideally Intel NEO or pocl.
