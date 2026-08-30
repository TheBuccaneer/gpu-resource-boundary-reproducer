# H1 O2 — Post-Reset LDS Leakage

Status:

**O2 H1 CLOSED — SENSITIVITY-LIMITED / INCONCLUSIVE FOR
RESET-SPECIFIC LDS CLEARING; NO POSITIVE POST-RESET LEAKAGE EVIDENCE**

## 1. Research question

O2 asks whether LDS residue written before an AMDGPU compute fault/recovery
can remain observable after recovery by another OpenCL context.

The intended differential is:

    victim LDS prime
        ->
    fault / recovery
        ->
    independent reader

versus a matched no-reset control.

This is an observational security/reliability oracle involving reads from
uninitialized `__local` storage. It is not an OpenCL conformance test.

## 2. H1 platform

Test platform:

- AMD Rembrandt GPU
- Rusticl / radeonsi
- OpenCL 3.0
- device-reported local memory: 65536 B
- `enforce_isolation=0`

Victim and reader geometry:

- LDS object: 8192 `uint` words
- LDS footprint: 32768 B
- local work-group size: 256
- groups: `compute_units * 16`
- victim pattern: `0xA5C30000 XOR index`

The reader checks:

- exact matches
- range matches
- longest exact run

A complete recovery corresponds to:

    exact_matches = 786432
    range_matches = 786432
    longest_exact_run = 8192

on H1.

## 3. Paired sensitivity-gate design

The final O2 probe was based on the SEC-2 prebuilt-reader design.

Two OpenCL contexts are created before the victim executes:

- Context A: victim
- Context B: reader

The Context-B reader program, kernel, queue, and output buffer are fully
constructed before the victim dispatch.

The final V3 trial sequence is:

1. Victim A primes LDS.
2. Reader B performs a PRECHECK.
3. If PRECHECK observes no victim residue, the trial is rejected and no
   fault trigger is executed.
4. If PRECHECK succeeds, Victim A re-primes LDS.
5. The intervention is performed.
6. Reader B performs the POSTCHECK.

The PRECHECK itself consumes LDS and therefore changes device state.
For this reason Victim A is explicitly re-run after PRECHECK so that the
known victim pattern is re-established immediately before the intervention.

## 4. Probe development and sensitivity validation

Earlier O2 probe variants were diagnostically useful but were not used as
the final sensitivity evidence.

In particular, the V2 paired-control calibration produced:

    10 / 10 PRECHECK failures

despite no captured cleaner/isolation events.

V2 contained additional host work in the timing-sensitive victim-to-reader
interval, including a multi-MiB host `memset()`.

V3 removed that unnecessary host-buffer touch.

Removing that work was followed by restoration of the expected sensitivity,
but the available data do NOT prove that the `memset()` alone caused the V2
failures. V2 is therefore treated as a probe-perturbation observation rather
than a causal mechanism result.

The final V3 no-trace sensitivity evidence is located in:

    results/o2/20260830_154442_v3_no_trace_control/

Observed sequence:

- Trial 1:
  - PRECHECK FAIL
  - trial rejected

- Trials 2–4:
  - PRECHECK PASS
  - PRE exact matches = 786432 / 786432
  - victim re-prime successful
  - POST exact matches = 786432 / 786432

Thus:

    valid PRECHECK trials = 3
    POST positive among valid trials = 3 / 3

Immediate post-reprime POST gaps were approximately:

    4–5 us

These runs demonstrate that the final V3 oracle can recover the complete
victim pattern when the post-reprime interval remains short.

The earlier directory:

    results/o2/20260830_154226_paired_control_calibration/

contains the V2 10/10 PRECHECK-failure series and is NOT the evidence source
for the final 3/3 V3 sensitivity claim.

## 5. B3 fault/recovery trial

The O2 reset experiment used:

    ./probes/b3_multi_local_probe 32768 32769

This is the plain over-limit B3 variant:

    32768 + 32769 = 65537 B

It must not be confused with the separate alignment-hidden B3 case in which
the raw/API-visible sum is still 65536 B.

For O2 the B3 probe is used only as an established compute-fault trigger.

Result directory:

    results/o2/20260830_154734_v3_b3_reset/

Before the fault:

    O2-PRECHECK=PASS
    PRE exact_matches=786432
    PRE range_matches=786432
    PRE longest_exact_run=8192

Re-prime:

    O2-REPRIME clEnqueueNDRangeKernel=0
    O2-REPRIME clFinish=0
    O2-REPRIME=PASS

B3 process:

    b3_exit=134

Kernel recovery evidence:

    ring comp_1.3.0 timeout
    Starting comp_1.3.0 ring reset
    Ring comp_1.3.0 reset succeeded
    device wedged, but recovered through reset

The O2 process survived:

    o2_exit=0

Post-recovery reader:

    victim_to_reader_gap_us=3875404

    O2-POST exact_matches=0
    O2-POST range_matches=0
    O2-POST longest_exact_run=0
    O2-POSTCHECK=NO_EXACT_MATCH

Therefore no victim LDS residue was observed after this successful
compute-ring recovery.

## 6. Matched no-reset delay control

A no-reset control repeated the same:

    PRECHECK
        ->
    victim re-prime
        ->
    delay
        ->
    POSTCHECK

sequence, replacing B3/recovery with a host delay chosen to approximate the
B3-recovery interval.

Result directory:

    results/o2/20260830_154928_v3_matched_delay/

Before the delay:

    O2-PRECHECK=PASS
    PRE exact_matches=786432
    PRE range_matches=786432
    PRE longest_exact_run=8192

Re-prime:

    O2-REPRIME=PASS

Post-reprime interval:

    victim_to_reader_gap_us=3919560

Post-delay reader:

    O2-POST exact_matches=0
    O2-POST range_matches=0
    O2-POST longest_exact_run=0
    O2-POSTCHECK=NO_EXACT_MATCH

Thus an approximately matched no-reset interval also eliminated observable
victim residue.

## 7. Sensitivity limitation

The key O2 comparison is therefore:

| Condition | Valid PRE | Intervention | Re-prime -> POST | POST |
|-----------|-----------|--------------|------------------|------|
| Immediate V3 control | full | none | ~4–5 us | full |
| B3 recovery | full | compute-ring timeout/recovery | 3,875,404 us | 0 |
| Matched delay | full | no reset | 3,919,560 us | 0 |

The matched-delay result is decisive for interpretation.

The B3 trial and the no-reset delay trial are both outside the short interval
in which this H1 LDS oracle was demonstrated to retain sensitivity.

Prior SEC-2 experiments showed observable LDS residue only over short
microsecond-to-low-millisecond intervals and explicitly did not establish a
fixed deterministic LDS lifetime.

Consequently, the approximately 3.9-second O2 interval is a
**sensitivity gap** for this oracle.

The experiment therefore has no discriminating power between:

- reset-specific LDS clearing;
- ordinary disappearance of remanence over the long interval;
- intervening GPU scheduling / CU reuse;
- or another unobserved state transition.

The data do not identify which of these mechanisms caused the disappearance.

This must not be described as a measured deterministic "LDS lifetime" or
"remanence floor".

## 8. Cleaner / isolation trace

AMDGPU trace collection was enabled for:

    amdgpu_cleaner_shader
    amdgpu_isolation

For the selected post-reprime windows of both the B3 and matched-delay runs:

    no cleaner/isolation events were captured

The corresponding trace artifacts contain no such events.

This is recorded as an observation only.

It does NOT establish:

- that no cleaner mechanism could have executed by any path;
- that cleaner execution is impossible with `enforce_isolation=0`;
- or that the disappearance of LDS residue has been mechanistically
  explained.

The O2 conclusion does not depend on the negative trace result.

## 9. Interpretation

The successful B3 compute-ring recovery was followed by complete loss of
observable victim LDS residue.

However, an approximately matched no-reset delay produced the same result.

Therefore the disappearance cannot be attributed specifically to the reset.

The experiment does NOT provide evidence that the compute-ring reset actively
clears LDS.

Equally, no post-reset victim residue was observed, so the experiment provides
no positive evidence for post-reset LDS leakage on H1.

The central limitation is structural:

    the available H1 B3 timeout/recovery path lasts far longer than the
    empirically demonstrated sensitive interval of the LDS-remanence oracle.

Host-side changes that only reduce latency after recovery cannot solve this
problem, because remanence age is measured from Victim/Re-prime to Reader,
not from recovery completion to Reader.

## 10. Strongest supported claim

On H1, a paired sensitivity-gated LDS oracle recovered the complete victim
pattern immediately before intervention. After a B3-induced compute-ring
timeout followed by successful ring recovery, no victim residue was observed.
However, an otherwise matched no-reset control with approximately the same
3.9-second post-reprime interval was equally negative. The current H1 oracle
therefore cannot distinguish reset-specific LDS clearing from ordinary loss
of observable remanence over the much longer interval, and no positive
post-reset LDS leakage evidence was obtained.

## 11. Non-claims

O2 H1 does NOT establish:

- that compute-ring reset actively clears LDS;
- that AMDGPU recovery sanitizes LDS;
- that reset provides a security isolation guarantee;
- that the matched-delay loss is caused solely by elapsed time;
- a deterministic LDS-remanence lifetime;
- that scheduling/CU reuse caused the negative result;
- that absence of captured cleaner trace events proves absence of every
  possible cleanup mechanism;
- that post-reset LDS disclosure is impossible;
- that F7 is impossible;
- that a different fault/recovery class would behave identically;
- that a full-device reset would behave identically;
- that another AMD GPU or another OpenCL implementation would behave
  identically;
- an OpenCL conformance property.

In particular, the experiment does not test reset behavior inside an interval
where the H1 LDS oracle is known to remain sensitive, because the available
B3 recovery path itself already exceeds that interval.

## 12. Closure decision

Further repetitions of the same H1 experiment would add additional
long-delay null observations but would not remove the structural sensitivity
gap.

The O2 question therefore cannot be resolved further on H1 using the current
combination of:

- B3 recovery latency;
- LDS remanence behavior;
- and the current reader oracle.

No further H1 GPU trials are required for this block.

Final H1 status:

**O2 H1 CLOSED — SENSITIVITY-LIMITED / INCONCLUSIVE FOR
RESET-SPECIFIC LDS CLEARING; NO POSITIVE POST-RESET LEAKAGE EVIDENCE**
