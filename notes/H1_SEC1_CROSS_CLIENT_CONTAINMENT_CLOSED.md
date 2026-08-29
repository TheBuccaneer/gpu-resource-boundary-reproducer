# H1 SEC-1 — Cross-Client Failure Containment

**Status:** H1/Rusticl-local CLOSED — FINDING  
**Generalization:** delegated to DIFF-1

## Question

Can a failure triggered by one OpenCL process observably affect an independent,
benign OpenCL client sharing the same GPU?

## Platform

H1:
- AMD Rembrandt
- Rusticl / radeonsi
- AMDGPU
- 65536 B reported local memory
- Linux / KDE / Wayland

Final V2 campaign condition:
- one active display
- three connected outputs disabled

## Trigger

Established B3 trigger:

    b3_multi_local_probe 32767 32769

Both local-memory objects are individually valid. Their raw sum is 65536 B,
while effective aligned LDS allocation exceeds the dispatchable capacity.

The resulting dispatch can produce an AMDGPU compute-ring timeout.

## Victim

The independent victim runs in a separate process with its own OpenCL context
and queue.

`sec1_victim_heartbeat_v2` repeatedly:
- launches a benign 64-work-item kernel,
- waits for completion,
- reads 64 result words,
- validates every word,
- records API errors and total latency.

V2 additionally records mismatch count, first mismatch, zero count,
previous-pattern count and the first eight returned words.

## Cold-Boot V2 Campaign

Ten independent boots were tested.

Exactly one B3 trigger was executed per boot.

| Trial | Recovery | Victim outcome | Max heartbeat |
|------:|----------|----------------|--------------:|
| 01 | FULL_GPU_RESET | survives, 75/75 correct | 920.52 ms |
| 02 | RING_RESET_ONLY | survives, 73/73 correct | 5.59 ms |
| 03 | RING_RESET_ONLY | survives, 73/73 correct | 6.33 ms |
| 04 | RING_RESET_ONLY | survives, 64/64 correct | 1999.85 ms |
| 05 | RING_RESET_ONLY | survives, 72/72 correct | 4.44 ms |
| 06 | RING_RESET_ONLY | survives, 73/73 correct | 5.75 ms |
| 07 | RING_RESET_ONLY | survives, 73/73 correct | 6.51 ms |
| 08 | FULL_GPU_RESET | WRONG_RESULT, context loss, abort | 3159.56 ms |
| 09 | RING_RESET_ONLY | survives, 73/73 correct | 5.80 ms |
| 10 | RING_RESET_ONLY | survives, 73/73 correct | 5.32 ms |

Descriptive totals:
- 8/10 ring-reset-only recoveries
- 2/10 MODE2 full-GPU resets
- 9/10 victim processes survived
- 3/10 trials showed >=100 ms victim latency excursions
- 665 V2 heartbeats
- 664 correct
- 1 V2 WRONG_RESULT

These are descriptive campaign counts, not stable probability estimates.

## Successful Ring Recovery

Successful compute-ring recovery generally preserved victim correctness and
context survival.

However, recovery was not always transparent.

Trial 04 produced a 1999.85 ms victim heartbeat while returning a correct
result and preserving the victim context.

Earlier SEC-1 replication runs independently showed approximately
1.84–2.01 s victim stalls during successful ring recovery.

Therefore successful ring-level recovery can preserve correctness while still
causing a transient cross-client availability impact.

## Full-GPU Recovery Outcomes

Failed ring recovery can escalate to MODE2 full-GPU reset.

Observed victim outcomes were heterogeneous.

### A — Survive with correct results

Cold-Boot V2 Trial 01:
- MODE2 full-GPU reset
- victim exit 0
- 75/75 correct
- max heartbeat 920.52 ms

A full GPU reset therefore does not deterministically terminate an independent
OpenCL client.

### B — Abort while an in-flight operation remains blocked

In the earlier SEC-1 replacement run around 19:19, the victim remained blocked
in the completion path. A watchdog reported approximately 2 s in clFinish and
the heartbeat never returned an application result before context loss.

### C — Successful API completion with incorrect application result, then abort

Observed independently in:
1. the earlier DISP1 full-reset run
2. Cold-Boot V2 Trial 08

In both observations the heartbeat returned with:

    enqueue_err=0
    finish_err=0
    read_err=0
    result_ok=0

before subsequent context loss / abort.

Thus the incorrect application-level result observation was reproduced across
two independent full-reset events.

This is not classified here as an OpenCL conformance violation or as a proven
cross-client integrity vulnerability.

## V2 All-Zero Observation

Trial 08 returned:

    mismatch_count=64
    zero_count=64
    all_zero=1
    stale_prev_full=0
    actual0_7=0,0,0,0,0,0,0,0

All 64 result words were zero.

The expected first word was 17.

The immediately preceding successful heartbeat pattern was not present.
Therefore the simple explanation that the read merely returned the complete
previous heartbeat result is inconsistent with this observation.

The current evidence does not distinguish between:
- a dispatch that never completed while the read observed reset/reinitialized
  storage, and
- storage being modified or cleared during MODE2 recovery.

A canary-before-dispatch experiment is required to distinguish these cases.

## Watchdog Interpretation

The 2000 ms clFinish watchdog is not the primary availability oracle.

Examples:
- Trial 04: 1999.85 ms total heartbeat, no watchdog
- Trial 08: 3159.56 ms total heartbeat, no watchdog

Primary availability metric: heartbeat `elapsed_ms`.

## Kernel Attribution

The process named in the compute-ring timeout varied across full-reset cases.

Both the B3 trigger and the independent victim have been named in different
events.

No causal interpretation is made from the process name alone.

AMDGPU "innocent" means that the context was not blamed for causing the reset;
it does not mean that the context was unaffected.

## Strongest Supported Finding

A failure induced by one OpenCL client can produce observable effects in an
independent OpenCL client sharing the same GPU.

Successful ring-level recovery can preserve victim correctness and context
survival while causing substantial transient latency.

When ring recovery fails and escalates to MODE2 device-wide recovery, victim
outcomes are heterogeneous: the client may survive correctly, remain blocked
until context loss, or return an incorrect application-level result before
subsequent abort.

## Non-Claims

SEC-1 does not establish:
- an OpenCL specification violation
- a general cross-client integrity vulnerability
- deterministic victim termination after full reset
- deterministic wrong results after full reset
- stable probabilities from N=10
- causal dependence on a particular compute-ring identifier
- cross-vendor or cross-implementation generality

## Closure

SEC-1 is CLOSED for H1/Rusticl-local investigation.

Further identical H1 cold-boot repetitions are not currently justified.

Generalization is delegated to DIFF-1.
