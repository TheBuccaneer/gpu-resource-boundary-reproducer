# H1 SEC-1 FUP1 — Canary V3

**Status:** COMPLETED — ORACLE VALIDATED, TARGET FAILURE NOT OBSERVED

## Goal

Distinguish future WRONG_RESULT cases between:

- retained pre-dispatch contents,
- all-zero data,
- stale previous heartbeat data,
- partial/mixed data,
- other corruption.

## V3 oracle

Before every heartbeat kernel dispatch, the output buffer is synchronously
initialized with the non-zero pattern:

    0xC0DE0000 ^ gid

The readback oracle additionally records:

- canary_count
- canary_full
- zero_count
- all_zero
- previous-heartbeat matches

## Baseline

30-second V3 baseline:

- 150/150 heartbeats correct
- 0 CANARY_FAIL
- 0 WRONG_RESULT
- all canary writes returned CL_SUCCESS
- max heartbeat latency 3.78 ms

The V3 oracle itself is therefore operational under normal execution.

## Five cold-boot trials

Exactly one established B3 trigger was executed per independent boot.

Results:

- 5/5 RING_RESET_ONLY
- all victim processes survived
- 356/356 heartbeats correct
- 0 WRONG_RESULT
- 0 CANARY_FAIL
- one substantial latency excursion: 1972.83 ms

No MODE2 full-reset / WRONG_RESULT event occurred in this five-trial campaign.

## Interpretation

The canary oracle is validated but the target failure required for mechanism
classification was not observed.

Therefore the earlier V2 all-zero WRONG_RESULT remains mechanistically
unresolved.

The 0/5 full-reset observation is not interpreted as evidence that V3 prevents
full resets. V3 changes the victim command stream by adding a blocking
pre-dispatch buffer write and may alter scheduling/timing.

## Decision

Do not continue identical H1 cold-boot repetitions solely to wait for another
WRONG_RESULT.

Retain V3 for future targeted reproductions or DIFF-1.
