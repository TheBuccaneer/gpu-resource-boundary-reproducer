# H1 O2 — Future Work

Current status:

**O2 H1 CLOSED — SENSITIVITY-LIMITED / INCONCLUSIVE FOR
RESET-SPECIFIC LDS CLEARING; NO POSITIVE POST-RESET LEAKAGE EVIDENCE**

## 1. Reason for closure on H1

The current H1 experiment cannot distinguish reset-specific LDS clearing from
ordinary disappearance of observable remanence.

The relevant timing relationship is:

    Victim/Re-prime
        ->
    complete intervention / recovery
        ->
    Reader

The entire Victim/Re-prime-to-Reader interval must remain inside a range in
which the LDS oracle has independently demonstrated sensitivity.

On H1:

- immediate V3 controls recover the complete victim pattern;
- the B3 recovery path produces an approximately 3.9-second interval;
- an approximately matched no-reset interval is already completely negative.

Therefore the current B3 recovery duration itself exceeds the useful
sensitivity range of the oracle.

Starting the reader quickly *after recovery* does not solve this problem,
because the relevant remanence age begins at Victim/Re-prime, not at recovery
completion.

## 2. Mandatory prerequisite for reopening O2

O2 should be reopened on another platform or recovery path only if at least
one of the following is true:

1. the complete fault/recovery sequence from Victim/Re-prime to Reader can
   finish inside the empirically demonstrated LDS-sensitive interval;

2. the target platform exhibits LDS remanence that remains observable for
   substantially longer than the complete recovery duration;

3. a different hardware/driver mechanism provides a direct measurement of
   LDS sanitization during recovery without depending on long-lived
   post-recovery remanence.

This is not merely an optimization.

It is a prerequisite for a discriminating reset-specific LDS experiment.

## 3. Required experimental structure when reopened

Preserve the paired design:

1. Create victim and reader protection domains/contexts before the experiment
   where appropriate.
2. Fully prebuild the reader.
3. Victim primes a unique LDS pattern.
4. Perform a PRECHECK.
5. Reject the trial if PRECHECK is negative.
6. Re-prime the victim after PRECHECK.
7. Apply exactly one intervention.
8. Perform POSTCHECK.
9. Run a no-reset control matched for the complete
   Victim/Re-prime-to-Reader interval.
10. Classify recovery type separately.

Relevant recovery classes must not be pooled blindly:

- successful compute-ring recovery;
- failed ring recovery followed by wider device reset;
- other driver/device recovery paths.

## 4. Timing requirement

For every platform, first establish the oracle's own sensitivity distribution
without reset.

Do not assume a universal LDS lifetime.

Determine empirically whether residue remains recoverable over the interval
needed by the candidate recovery path.

Only then run the reset differential.

A useful reset experiment requires overlap between:

    empirically sensitive LDS interval

and:

    complete Victim/Re-prime -> recovery -> Reader interval

If these intervals do not overlap, the reset experiment is
non-discriminating and should not be expanded merely by increasing N.

## 5. Cleaner / isolation tracing

Where supported, continue to record direct AMDGPU or equivalent cleanup /
isolation events.

Negative trace capture should be described neutrally, for example:

    "No cleaner/isolation events were captured in the selected interval."

Do not convert an empty or negative trace into a general claim that no
cleanup mechanism could have executed unless the tracing pipeline and
mechanism coverage independently justify that stronger conclusion.

## 6. Cross-platform differential work

The unresolved scientific question is appropriate for DIFF-1 or equivalent
cross-platform testing.

High-value candidate platforms are those with either:

- substantially longer observable LDS remanence;
- substantially faster fault recovery;
- a different GPU reset architecture;
- or direct sanitization instrumentation.

The principal differential question is:

    Does a platform exist for which the LDS oracle remains sensitive across
    the complete recovery path?

Only such a platform can provide a meaningful post-reset residue differential
using this oracle design.

## 7. B3 trigger provenance

When O2 is repeated, record the exact trigger variant.

The H1 O2 run used:

    32768 + 32769 = 65537 B

This is the plain over-limit B3 trigger.

Do not conflate it with the alignment-hidden B3 case:

    32767 + 32769

whose raw/API-visible total is 65536 B but whose aligned effective footprint
crosses the device limit.

For O2, the trigger's role is fault generation; nevertheless exact provenance
must remain explicit.

## 8. V2/V3 probe lesson

The V2 paired probe produced 10/10 PRECHECK failures.

V3 removed additional host-buffer work from the timing-sensitive
victim-to-reader interval and subsequently recovered the expected LDS pattern
in three consecutive valid controls.

This supports treating small host-side changes as potential perturbations in
the LDS oracle.

It does NOT prove that the removed `memset()` was uniquely or causally
responsible for every V2 failure.

Future probes should therefore minimize host work between Victim/Re-prime and
Reader and preserve exact source/binary provenance.

## 9. Reopen criterion

Do not reopen H1 merely to collect more long-delay zero observations.

Reopen O2 only when a materially different experimental condition removes the
sensitivity limitation, such as:

- faster complete recovery;
- longer-lived remanence;
- a different GPU/driver;
- or direct reset-time LDS-state instrumentation.

Until then:

**O2 H1 remains closed, while the general post-reset LDS question remains open
for differential hardware evaluation.**
