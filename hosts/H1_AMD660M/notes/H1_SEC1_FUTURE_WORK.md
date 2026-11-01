# H1 SEC-1 — Future Work

**Parent:** SEC-1 Cross-Client Failure Containment  
**Status:** H1/Rusticl-local CLOSED — FINDING

## Canary-before-dispatch

Before every victim kernel launch, initialize the output buffer with a known
non-zero canary.

On WRONG_RESULT distinguish:
- expected kernel result
- previous heartbeat result
- retained canary
- all-zero result
- partial/mixed result
- arbitrary data

A retained canary would support "kernel write did not occur".

All-zero output despite a non-zero canary would strengthen the hypothesis that
recovery/reset modifies or reinitializes the observed storage.

## Phase-Separated Timing

Measure separately:
- enqueue duration
- clFinish duration
- read duration
- total heartbeat duration

This is required because total latency excursions are not fully captured by
the current clFinish watchdog.

## GUI-less Control

Run a small separate cold-boot control without KDE/Wayland compositor activity.

Suggested:
- text console or SSH
- display manager stopped
- one B3 trigger per boot
- same V2 victim

Questions:
- does the same compute-ring timeout remain?
- does recovery still escalate to MODE2?
- do victim stalls remain?
- can context loss / WRONG_RESULT still occur?

Keep this dataset separate from the existing 10-trial GUI campaign.

Console-only operation must not automatically be described as fully headless,
because DRM/KMS/display hardware may remain active.

## Additional Victims

Test benign workloads with different characteristics:
- compute-bound
- memory-bound
- buffer transformation
- multiple queues
- longer-running kernel

## Scheduler / Ring Attribution

Where practical, correlate:
- attacker activity
- victim activity
- DRM scheduler entities
- compute-ring assignment
- AMDGPU reset tracepoints

Do not infer causality from the process name printed by the timeout alone.

## DIFF-1

Repeat the canonical SEC-1 experiment where applicable on:
- other AMD hardware / Mesa versions
- NVIDIA OpenCL
- Intel OpenCL
- alternative OpenCL implementations

Unsupported cases should be recorded explicitly.

## Reopen H1 Only If

- a materially different driver/runtime version is tested
- a new oracle distinguishes competing mechanisms
- a missing control such as GUI-less operation is tested
- DIFF-1 produces a concrete new H1 hypothesis
