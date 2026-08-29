# SEC-2 / H1 — LDS Remanence and AMDGPU Isolation

**Status:** CLOSED for H1 mechanism characterization  
**Date:** 2026-08-29  
**Scope:** AMD Rembrandt / Rusticl (`radeonsi`) / Linux AMDGPU  
**Security-extension classification:** SEC-2  
**Oracle classification:** O2 / intentionally undefined read of uninitialized OpenCL `__local` memory  
**Conformance claim:** None. This block characterizes residual state and isolation behavior; it is not an OpenCL specification violation claim.

---

## 1. Research question

SEC-2 asks whether residual GPU-local state in AMD LDS remains observable across OpenCL scheduling/protection-domain transitions, and which AMDGPU mechanisms suppress or eliminate that state.

The central distinction is between:

- OpenCL-level abstractions such as command queues and contexts;
- host-process / DRM-client transitions;
- AMDGPU's process-isolation and cleaner-shader mechanisms.

The final H1 result is that observable LDS remanence is robust without cleanup, while direct kernel trace evidence links AMDGPU cleaner execution to disappearance of the victim pattern.

---

## 2. Platform H1

- OS: Ubuntu
- Kernel during final SEC-2 runs: `7.0.0-30-generic`
- OpenCL: Rusticl / Mesa
- Runtime enablement: `RUSTICL_ENABLE=radeonsi`
- GPU family: AMD Rembrandt
- OpenCL device name: `AMD Radeon 660M`
- PCI ID: `1002:1681`
- Compute units: 6
- `CL_DEVICE_LOCAL_MEM_SIZE`: 65536 B
- AMDGPU sysfs controls:
  - `/sys/class/drm/card1/device/enforce_isolation`
  - `/sys/class/drm/card1/device/run_cleaner_shader`
- Default per-device isolation state observed during baseline work:
  - `enforce_isolation=0`

Note: `lspci` naming may differ from the OpenCL device string. For reporting, use Rembrandt + PCI ID + OpenCL device name.

---

## 3. Victim/reader oracle

### Victim

The victim kernel uses:

```c
volatile __local uint scratch[8192];
```

This is 32768 B LDS, intentionally below the 64 KiB device local-memory limit.

The victim writes a deterministic pattern:

```text
0xA5C30000 XOR index
```

and consumes local state into a global sink so the writes cannot be trivially optimized away.

### Reader

The reader uses the same 32768 B `volatile __local` array but intentionally does **not** initialize it. It copies the observed values to global memory and is built with `-cl-opt-disable`.

This is intentionally O2 / undefined behavior. The reader is a residual-state oracle, not a conformance test.

### Sampling geometry

- local size: 256
- groups: 96 = 6 CUs × 16
- words per group: 8192
- total samples: 786432 32-bit words
- output size: 3 MiB

Primary metrics:

- `exact_matches`
- `range_matches`
- `longest_exact_run`

Canonical full positive:

```text
exact_matches=786432
range_matches=786432
longest_exact_run=8192
```

Canonical negative:

```text
exact_matches=0
range_matches=0
longest_exact_run=0
```

---

## 4. Basic sensitivity controls

### 4.1 Null/background reader

A reader without a preceding victim produced:

```text
exact=0
range=0
longest=0
```

This establishes that the characteristic victim pattern is not appearing as ordinary background noise.

### 4.2 Immediate same-context / same-queue positive control

Sequence:

```text
Victim
→ clFinish
→ Reader
```

produced complete recovery:

```text
exact=786432
longest=8192
```

The oracle is therefore highly sensitive when physical LDS state is immediately reused.

---

## 5. Timing characterization

Early delay experiments showed:

- 0 ms: consistently positive
- nanosleep 10 ms and above: negative
- busy-wait 1 ms: 3/10 positive
- busy-wait 2 ms: 1/10 positive
- busy-wait 5 ms and 10 ms: 0/10

However, later P2 positives were observed with victim→reader host gaps of roughly 1.3–1.8 ms, while negative runs overlapped the same timing region.

Therefore the data do **not** support a deterministic "LDS lifetime" or "half-life" claim.

Defensible interpretation:

> Elapsed host time influences the opportunity for scheduler/reuse changes, but recoverability is governed primarily by physical LDS/CU reuse and GPU scheduling state rather than a fixed time threshold.

---

## 6. Cross-context P2 characterization

True P2 keeps the victim context alive while a second OpenCL context performs the reader.

Earlier series:

- 8/61 positive

Later instrumented series:

- 7/30 positive

Positive runs were often massive rather than weak/random, including complete or large block recovery with:

```text
longest_exact_run=8192
```

A broad descriptive aggregate is 15/91 positive across those two series, but it must **not** be presented as an i.i.d. population leak rate. These are repeated device trials on one machine and one scheduling environment.

The measured host-side victim→reader gap did not significantly separate positives from negatives.

---

## 7. Cross-process P3 characterization

A separate victim process wrote the LDS pattern, called `clFinish`, printed readiness, and kept its OpenCL state alive while a second process launched the reader.

Observed:

```text
P3 exact recovery: 0/61
```

Minor isolated range noise did not reconstruct the exact victim pattern.

This is meaningful empirical negative evidence on H1, but it is **not** a formal proof that the host-process boundary is always safe or that all cross-process leakage is impossible.

---

## 8. Manual AMDGPU cleaner — strongest intervention control

A same-context / same-queue sequence was used:

```text
Victim
→ clFinish
→ sysfs action
→ Reader
```

### 8.1 NOOP control

The `run_cleaner_shader` sysfs node was opened and closed without writing.

Result:

```text
10/10 complete recovery
exact=786432
longest=8192
```

### 8.2 CLEAN condition

Writing `0` to:

```text
/sys/class/drm/card1/device/run_cleaner_shader
```

produced:

```text
10/10 negative
exact=0
range=0
longest=0
```

The manual cleaner therefore deterministically eliminated observable LDS remanence in this control, whereas the otherwise-identical sysfs-open control preserved it.

This is a core mechanistic result.

---

## 9. `enforce_isolation` — early A/B/A evidence

An early P2 series showed:

```text
enforce=0 → 7/30 positive
enforce=1 → 0/30 positive
enforce=0 → 5/30 positive
```

This reversible A/B/A pattern established that `enforce_isolation` materially affects recoverability.

Later experiments showed that the mechanism is more subtle than "every OpenCL context switch cleans LDS"; the final interpretation must therefore be based on the cleaner trace evidence below.

---

## 10. Provenance correction: invalid early "dual-queue" batch

An early experiment was mistakenly interpreted as a dual-command-queue test.

Later source and binary provenance inspection showed that this source/binary still used a single queue and retained the `SEC2-SAME-PROCESS` label.

Therefore:

- the earlier alleged dual-queue results are **reclassified as single-queue data**;
- they must not be used as queue-switch evidence;
- the provenance correction must remain explicit in any final paper/repository notes.

A verified true dual-queue implementation was subsequently built and tested.

---

## 11. True dual-queue characterization

Verified structure:

```text
same OpenCL context
Victim on qA
→ clFinish(qA)
→ Reader on qB
```

### `enforce=0`

```text
30/30 positive
```

Almost all runs were complete recovery; partial examples still contained full 8192-word exact runs.

### `enforce=1`

```text
24/30 positive
```

This showed that a queue transition is not itself a deterministic LDS-clearing boundary.

---

## 12. Identical-binary same-queue vs queue-switch

A stronger control always created qA and qB and varied only where the reader was submitted.

Under `enforce=1`:

```text
same   = 20/30 positive
switch = 19/30 positive
```

Paired discordance:

```text
same-only   = 7
switch-only = 6
```

McNemar exact:

```text
p = 1.0
```

Conclusion:

> No measurable queue-switch-specific isolation effect was demonstrated.

---

## 13. Context-switch experiments and confounder correction

### 13.1 Initial context-switch design

The first design created substantial reader-side OpenCL state after the victim. This made `same` vs `switch` differ in more than the actual reader context.

Those results are not suitable for a clean context-boundary claim.

### 13.2 Pre-created-context design

Both contexts existed before the victim, but reader program/kernel/buffer construction still occurred after the victim.

Under `enforce=1` this produced an apparently strong:

```text
same   = 10/30
switch = 1/30
```

with McNemar exact approximately `p=0.0117`.

This attractive result was **not accepted as causal evidence** because post-victim reader construction still differed by context.

### 13.3 Fully prebuilt-reader context control

Both contexts, queues, reader programs, reader kernels, reader output buffers, and reader arguments were fully prepared before the victim.

After victim completion, the experiment only selected reader A or reader B.

#### `enforce=0`

```text
same   = 30/30
switch = 30/30
both   = 30 pairs
```

#### `enforce=1`

```text
same   = 20/30
switch = 15/30
both        = 8
same-only   = 12
switch-only = 7
neither     = 3
```

McNemar exact was approximately:

```text
p ≈ 0.36
```

Conclusion:

> The cleanest experiment does not support OpenCL context identity itself as the causal isolation boundary.

---

## 14. Neutral-hop experiment

A neutral kernel was added to distinguish:

- direct Victim→Reader;
- extra GPU work in the same context (`hopA`);
- extra GPU work in another OpenCL context (`hopB`).

The neutral kernel was verified with:

```text
CL_KERNEL_LOCAL_MEM_SIZE = 0
```

### 14.1 Geometry provenance correction

The first neutral-hop implementation accidentally launched only one neutral workgroup.

This was identified during external review and corrected.

The final neutral-hop uses the same NDRange geometry as victim/reader:

```text
96 workgroups
local size 256
```

The original one-workgroup result must not be used as the final geometry-controlled evidence.

### 14.2 Corrected matched-geometry result, `enforce=0`

```text
direct = 30/30 positive
hopA   = 30/30 positive
hopB   = 30/30 positive
```

Recovery was almost uniformly complete:

```text
exact=786432
longest=8192
```

Therefore:

> Without isolation/cleanup, even a same-geometry, LDS-free intervening GPU job does not eliminate the observed residual LDS pattern, whether submitted through the same or another OpenCL context.

### 14.3 Corrected matched-geometry result, `enforce=1`

```text
direct = 21/30 positive
hopA   = 19/30 positive
hopB   = 24/30 positive
```

No convincing mode-specific difference was found.

This further weakened the hypothesis that an OpenCL context hop itself is the important cleanup trigger.

---

## 15. Timing instrumentation of neutral hops

The neutral-hop probe was instrumented to separate:

- enqueue time;
- `clFinish` wait;
- total hop time.

A later 30×hopA + 30×hopB series under `enforce=1` produced:

```text
hopA = 30/30 positive
hopB = 29/30 positive
```

The only negative in that series coincided with an approximately 11.6 ms `clFinish` wait.

This showed strong run-to-run nonstationarity and demonstrated that long stalls are **not** a general deterministic explanation for LDS loss.

Timing was therefore demoted from mechanism evidence to a secondary scheduling signal.

---

## 16. Direct AMDGPU tracepoint ground truth

The kernel exposes:

```text
amdgpu/amdgpu_cleaner_shader
amdgpu/amdgpu_isolation
```

### 16.1 Manual cleaner trace calibration

A manual write to `run_cleaner_shader` produced:

```text
amdgpu_isolation
amdgpu_cleaner_shader
```

with the cleaner event following the isolation event by roughly 26 µs in the calibration run.

Counts:

```text
cleaner_events=1
isolation_events=1
```

This validates the tracepoint as a direct sensor for the cleaner mechanism.

### 16.2 Clock calibration

The harness emits absolute `CLOCK_MONOTONIC` timestamps and ftrace was configured with the `mono` trace clock.

A `trace_marker` calibration showed approximately:

```text
clock_marker_delta_us = 66.8
```

This is sufficiently small for the measured victim→reader windows, which are commonly hundreds of microseconds or longer.

A useful calibration run also showed a cleaner approximately 128 µs **before** victim completion, followed by complete recovery. This confirms why only cleaner events occurring after the victim are relevant.

---

## 17. Final cleaner ↔ LDS correlation experiment

Thirty `hopA` and thirty `hopB` runs were performed under `enforce_isolation=1`.

For each run the trace was checked for `amdgpu_cleaner_shader` events specifically in the interval:

```text
victim_done_mono_ns
    →
reader_enqueue_mono_ns
```

Observed contingency table:

| LDS result | Cleaner between victim and reader enqueue | No cleaner in window |
|---|---:|---:|
| Positive | 0 | 35 |
| Negative | 20 | 5 |

Equivalent counts:

```text
POS + CLEANER_BETWEEN = 0
POS + NO_CLEANER      = 35
NEG + CLEANER_BETWEEN = 20
NEG + NO_CLEANER      = 5
```

Fisher exact:

```text
p ≈ 1.27 × 10^-11
```

Key observations:

1. **20/20 runs with a cleaner in the relevant pre-reader window were negative.**
2. **0/35 positive runs contained a cleaner in that window.**
3. Without an observed pre-enqueue cleaner, recovery remained possible in 35/40 runs.
4. Five negative runs had no cleaner in the current pre-enqueue window. These may reflect:
   - physical LDS non-reuse / different scheduling;
   - a cleaner after host reader enqueue but before actual GPU execution;
   - another residual-state destruction mechanism.

The five cases are not required to support the primary causal direction: observed cleaner-before-reader was sufficient for loss in all measured cases.

---

## 18. Scheduler trace follow-up

The kernel also exposes:

```text
amdgpu_sched_run_job
gpu_scheduler/drm_sched_job_queue
gpu_scheduler/drm_sched_job_run
gpu_scheduler/drm_sched_job_done
```

These contain fence identifiers and, for DRM scheduler events, `client_id`.

A final calibration run with:

```text
exact_matches=0
```

showed multiple AMDGPU isolation/cleaner events in the victim→reader interval, including cleaner activity immediately before the reader-side host timestamp region.

This further supports the cleaner interpretation.

Full fence-level attribution of the reader job was considered possible but was **not required to close H1 SEC-2**, because the stronger causal direction had already been established by:

- deterministic manual intervention;
- direct cleaner trace calibration;
- 20/20 cleaner-window negatives;
- 0/35 cleaner-window positives.

---

## 19. Strongest defensible H1 conclusions

### Strong

1. Residual victim LDS state is reproducibly observable by a subsequent intentionally-uninitialized `__local` reader on H1.

2. The oracle is specific: the null/background reader does not reproduce the victim pattern.

3. Without cleanup, full LDS-pattern recovery survives:
   - same queue;
   - a different command queue;
   - a fully pre-existing second OpenCL context;
   - a same-geometry, LDS-free intervening GPU job in the same context;
   - a same-geometry, LDS-free intervening GPU job in another OpenCL context.

4. OpenCL command-queue identity is not an observed security-isolation boundary.

5. OpenCL context identity is not an observed deterministic LDS-clearing boundary on H1.

6. A manual AMDGPU cleaner invocation deterministically removes the observable victim residue in the tested control:
   - NOOP: 10/10 full recovery
   - CLEAN: 10/10 no recovery

7. The AMDGPU cleaner tracepoint was independently validated using the manual cleaner.

8. In the final trace-correlated series, every run with an observed cleaner between victim completion and reader enqueue was negative:
   - 20/20 cleaner-window runs negative
   - 0/35 positive runs with cleaner in that window
   - Fisher exact ≈ `1.27 × 10^-11`

9. Therefore the disappearance of the victim pattern is directly and strongly associated with the AMDGPU cleaner mechanism, not merely with elapsed time or OpenCL context/queue switching.

---

## 20. Claims that must NOT be made

Do not claim:

- an OpenCL specification violation from SEC-2;
- that reading uninitialized `__local` memory is defined behavior;
- that OpenCL contexts are formal security protection domains;
- that command queues are formal security protection domains;
- that `enforce_isolation=1` cleans after every GPU job;
- that every OpenCL context switch triggers a cleaner;
- that every negative read is necessarily caused by a cleaner;
- that LDS has a deterministic persistence time such as 1–5 ms;
- that P3 `0/61` proves cross-process safety;
- that the effect generalizes to all AMD GPUs, Mesa versions, operating systems, vendors, or OpenCL implementations;
- a single pooled "leak probability" from heterogeneous repeated H1 series.

---

## 21. Reviewer-facing interpretation

Recommended core wording:

> On H1, observable LDS remanence persists across OpenCL queue and context changes in the absence of cleanup. A manually invoked AMDGPU cleaner deterministically eliminates the recovered pattern, and kernel trace correlation shows that every trial containing a cleaner event between victim completion and reader enqueue lost the victim pattern. Conversely, no positive recovery trial contained such a cleaner event in that interval. The data therefore support the AMDGPU cleaner mechanism, rather than OpenCL queue or context identity, as the relevant state-sanitization mechanism on this platform.

Mandatory caveat:

> The reader intentionally observes uninitialized OpenCL local memory and therefore constitutes an undefined-behavior residual-state oracle rather than a conformance test.

---

## 22. H1 closure decision

**SEC-2 mechanism characterization on H1 is CLOSED.**

Reason:

The block now contains:

- a sensitive positive control;
- a null/background control;
- queue and context controls;
- corrected source/binary provenance;
- a matched-geometry neutral-hop control;
- a deterministic manual cleaner intervention;
- direct kernel cleaner/isolation tracepoints;
- user/kernel monotonic clock calibration;
- a trace-correlated 60-run experiment with a highly asymmetric contingency table;
- explicit identification of remaining negative-without-pre-enqueue-cleaner cases.

Further fence-level scheduler forensics would refine the five remaining negative/no-pre-enqueue-cleaner cases but is not necessary to support the primary H1 mechanistic conclusion.

Future extension work is tracked separately in `H1_SEC2_FUTURE_WORK.md`.
