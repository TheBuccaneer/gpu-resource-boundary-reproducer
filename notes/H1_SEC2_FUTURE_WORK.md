# SEC-2 — Future Work / Deferred Extensions

**Status:** DEFERRED after H1 mechanism closure  
**Date:** 2026-08-29  
**Purpose:** Preserve high-value follow-up experiments without keeping the H1 SEC-2 block open.

The current H1 mechanism characterization is closed. The items below are only to be resumed when they materially strengthen a paper, disclosure, cross-device claim, or security-boundary claim.

---

## 1. Priority A — Cross-process P3 with direct cleaner tracing

### Why

Current H1 result:

```text
P3 exact recovery = 0/61
```

This is meaningful empirical negative evidence, but not a formal process-isolation proof.

The most valuable follow-up is no longer "run more blind P3 trials". It is to combine P3 with the now-validated AMDGPU tracepoints.

### Future design

Use:

```text
Process A:
Victim
→ clFinish
→ remain alive

Process B:
Reader
```

Trace simultaneously:

```text
amdgpu_isolation
amdgpu_cleaner_shader
```

Questions:

1. Does every A→B process transition emit isolation/cleaner events under `enforce_isolation=1`?
2. Does `enforce_isolation=0` remove those events?
3. Can P3 ever recover the exact victim pattern when no cleaner occurs?
4. Is the observed P3 `0/61` explained directly by process-boundary cleaner execution?

### Statistical planning

Do not choose a large N arbitrarily.

If the objective is a zero-success upper bound, predefine the desired 95% upper limit.

Approximate rule-of-three examples for zero observed successes:

- n=61 → upper bound ≈ 4.9%
- n=100 → ≈ 3%
- n=150 → ≈ 2%
- n=200 → ≈ 1.5%

Use an exact binomial interval in final analysis.

### Value

**Very high** if SEC-2 is used for a security-boundary or mitigation-evaluation contribution.

---

## 2. Priority A — P4 cross-user / container boundary

### Why

P3 tests another process under the same user. A stronger protection-domain study would test:

- another Unix user;
- optionally a container / namespace boundary where the same physical GPU is accessible.

### Key question

Does AMDGPU process isolation behave identically across:

- P3 same-user process boundary;
- P4 cross-user boundary;
- containerized clients sharing the same GPU?

### Required controls

- exact same victim/reader geometry;
- direct cleaner trace;
- explicit `enforce_isolation=0` vs `1`;
- preserve process lifetime until reader completion;
- record DRM nodes and user identity;
- avoid interpreting a negative result as universal cross-tenant safety.

### Value

**Very high** for a standalone SEC-2/security paper.  
Lower priority if SEC-2 remains only a supporting extension around B4.

---

## 3. Priority A/B — Second AMD GPU / DIFF-1

### Why

All current SEC-2 claims are H1-specific.

A second AMDGPU generation would greatly strengthen external validity.

### Minimum useful replication

Repeat only the most informative core controls:

1. null/background reader;
2. immediate same-context positive control;
3. manual cleaner NOOP vs CLEAN;
4. fully prebuilt context-switch baseline;
5. trace-correlated cleaner experiment;
6. P3 if feasible.

Do **not** repeat the entire historical exploratory chain unless the second platform behaves differently.

### Candidate comparisons

- newer RDNA generation;
- discrete AMD GPU vs current Rembrandt APU;
- different Mesa/Rusticl version if hardware is unavailable.

### Value

**High for journal generalization.**

---

## 4. Priority B — Cleaner scope across CUs

### Question

Does one manual `run_cleaner_shader` invocation clear observable LDS state across all compute units, or only the CUs/rings affected by recent work?

### Possible design

1. Victim writes distinguishable per-group/per-CU-like tags.
2. Execute victim geometry across all available CUs.
3. Invoke cleaner once.
4. Probe recovery distribution by workgroup.
5. Compare NOOP vs CLEAN.

### Caveat

OpenCL does not provide a portable guarantee that one workgroup is pinned to a specific CU. Any CU-localization method must be justified rather than assumed.

### Value

**Medium-high** for mechanism depth.  
Not necessary for the existing H1 closure.

---

## 5. Priority B — Resolve the five `NEG + NO_CLEANER` cases

Final H1 contingency:

```text
POS + CLEANER_BETWEEN = 0
POS + NO_CLEANER      = 35
NEG + CLEANER_BETWEEN = 20
NEG + NO_CLEANER      = 5
```

The current cleaner window ends at the host's reader-enqueue timestamp.

Possible explanations for the five negative/no-pre-enqueue-cleaner runs:

1. cleaner after host enqueue but before actual GPU execution;
2. physical LDS non-reuse;
3. scheduler placement onto different resources;
4. another state-destruction mechanism.

### Available tracepoints

```text
gpu_scheduler/drm_sched_job_queue
gpu_scheduler/drm_sched_job_run
gpu_scheduler/drm_sched_job_done
amdgpu/amdgpu_sched_run_job
```

These contain fence identifiers; DRM scheduler queue/run events also expose `client_id`.

### Deferred experiment

Join the reader submission using:

```text
fence_context:fence_seqno
```

from queue → run → AMDGPU run → done, then test whether a cleaner lies between victim completion and the actual reader GPU job.

### Stop rule

Only resume this if a reviewer or later result makes it important to explain **all** negatives.

The current primary causal direction does not depend on resolving these five cases.

### Value

**Medium.**

---

## 6. Priority B — Isolation mode without cleaner, if supported

The AMDGPU implementation/documentation should be rechecked on the exact target kernel before use.

If the per-device isolation interface supports a mode that preserves isolation/serialization while disabling cleaner execution, test:

```text
no isolation / no cleaner
vs.
isolation + cleaner
vs.
isolation without cleaner
```

### Research question

Is the loss of LDS remanence caused by:

- cleaner execution itself;
- scheduling serialization;
- or both?

### Required validation

Before any run:

1. verify accepted sysfs values on the exact kernel;
2. verify kernel source/docs for the semantic meaning;
3. use `amdgpu_cleaner_shader` tracepoint to confirm whether cleaner execution actually occurs.

### Value

**High mechanistic value** if the mode is available and semantics are confirmed.

---

## 7. Priority B — Larger manual cleaner replication

Current intervention:

```text
NOOP  = 10/10 full recovery
CLEAN = 10/10 no recovery
```

This is already strong.

If final publication review benefits from larger N, repeat:

```text
NOOP n≥30
CLEAN n≥30
```

with:

- same binary;
- alternating or randomized condition order;
- direct cleaner trace enabled;
- exact same geometry;
- recorded action latency.

### Value

**Low-to-medium incremental value** because the existing intervention is already deterministic.

Do not spend time here unless needed for final paper polish.

---

## 8. Priority C — Controlled DRM-client interference

The desktop system normally has many DRM clients, including examples observed during H1 work:

- KWin
- Xwayland
- Plasma
- Firefox
- Discord
- other KDE processes

An open DRM render node does not prove active GPU submission.

### Possible later experiment

Compare:

1. normal desktop session;
2. reduced/no-browser desktop;
3. minimal TTY/headless-like environment if practical.

Trace:

```text
amdgpu_isolation
amdgpu_cleaner_shader
drm_sched_job_*
```

Question:

Does unrelated GPU-client activity explain nonstationary cleaner frequency under `enforce_isolation=1`?

### Value

**Medium**, mostly for explaining variability.  
Not necessary for the main H1 cleaner result.

---

## 9. Priority C — Process-transition neutral hop

If a direct process-boundary mechanism test is desired:

```text
Process A:
Victim → clFinish

Process B:
LDS-free neutral kernel → clFinish

Process A:
Reader
```

Compare:

```text
enforce=0
enforce=1
```

and trace cleaner/isolation events.

### Why useful

This isolates a host-process/DRM-client transition without allowing Process B's own kernel to overwrite LDS via `__local` use.

### Important requirement

The neutral kernel must retain:

```text
CL_KERNEL_LOCAL_MEM_SIZE = 0
```

and use matched NDRange geometry.

### Value

**High** if the process boundary becomes a central paper claim.

---

## 10. Priority C — CU/resource reuse localization

The current study infers physical LDS reuse from the strong blockwise pattern and scheduler-dependent recovery.

A later mechanism study could try to localize resource reuse more directly.

Possible approaches:

- encode distinguishable group tags;
- study complete 8192-word block recovery per group;
- correlate with available hardware/scheduler metadata;
- avoid assuming portable CU IDs where OpenCL does not expose them.

### Value

**Medium**.

Useful for a deep GPU-memory-mechanism paper, not necessary for the current security-extension closure.

---

## 11. Priority C — Version differential

The broader project already uses source-level Mesa/Rusticl analysis.

For SEC-2, a version differential could compare:

- current Mesa/Rusticl;
- another available Mesa release;
- kernel versions with/without relevant process-isolation implementation differences.

### Minimal core

Do not repeat every exploratory SEC-2 probe.

Repeat:

- manual cleaner control;
- fully prebuilt context control;
- trace-correlated cleaner run;
- P3 if changed behavior appears.

### Value

**Medium-high** if behavior changes by release.

---

## 12. SEC-3 — Post-reset LDS

Deferred security extension:

> Test whether an F5/GPU reset affects LDS remanence and whether residual local state remains observable after reset/recovery.

Only pursue after the main B-series and SEC-2/B4 narrative are stable.

### Caution

Reset semantics can change device/runtime state substantially, so this must be treated as a separate experiment rather than folded into the clean SEC-2 mechanism chain.

### Value

**Exploratory / conditional.**

---

## 13. SEC-4 — Global-memory remanence

Deferred extension:

> Characterize whether analogous residual-state behavior appears in global/device allocations and across allocation/process boundaries.

This is a distinct memory region and should not be conflated with LDS.

### Value

**Exploratory / conditional.**

---

## 14. Paper-level decision tree

### If SEC-2 remains a supporting section

Stop after H1 closure unless one of these is cheap:

- second AMD platform core replication;
- trace-backed P3.

Do not reopen detailed scheduler forensics.

### If SEC-2 becomes a major journal contribution

Priority order:

1. trace-backed P3;
2. P4 cross-user/container;
3. second AMD GPU / DIFF-1;
4. isolation-without-cleaner mode if supported;
5. process-transition neutral hop;
6. cleaner scope across CUs;
7. resolve remaining negative/no-pre-enqueue-cleaner cases.

### If a reviewer challenges the five negative/no-cleaner cases

Resume fence-level scheduler attribution only then.

---

## 15. Explicitly parked questions

Do not forget these, but do not keep H1 open for them:

- Is P3 `0/61` fully explained by automatic process-boundary cleaner execution?
- Does P4 behave the same as P3?
- Does a second AMD GPU reproduce the cleaner↔LDS relationship?
- Is there a supported isolation mode that separates serialization from cleaner execution?
- Does a cleaner after host reader enqueue explain the remaining five negative/no-pre-enqueue-cleaner cases?
- Does manual cleaner execution cover all CUs deterministically?
- How much of `enforce_isolation=1` nonstationarity comes from unrelated DRM clients?
- Do Mesa/kernel versions materially change the behavior?
- What happens to LDS after GPU reset?
- Is there analogous remanence in global memory?

---

## 16. Re-entry criterion

SEC-2 should only be reopened if at least one of the following is true:

1. the final paper needs a stronger process/tenant security claim;
2. journal reviewers require cross-device generalization;
3. a second platform produces contradictory behavior;
4. the five unexplained negative/no-pre-enqueue-cleaner cases become central;
5. AMDGPU mode semantics offer a clean serialization-vs-cleaner experiment;
6. SEC-2 is promoted from supporting extension to a primary paper contribution.

Otherwise H1 remains **CLOSED**.
