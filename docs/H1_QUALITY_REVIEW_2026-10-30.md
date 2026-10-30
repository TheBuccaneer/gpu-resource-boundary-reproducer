# H1 consistency review and open questions

This is an offline consolidation audit, not an independent reacquisition of every experiment. Sources, archived outputs, hashes and current local report precedence were checked without invoking OpenCL. Existing public history was preserved.

## Resolved interpretation conflicts

| Earlier wording or record | Current treatment |
|---|---|
| Old project state scheduled B8/B9/B10/B12 as future work | Replaced by the 30 October status and closures; B6/B11 explicitly deferred |
| Historical successful controls called `PASS / F0` | Valid successes are PASS; F0 is a clean specified error. Raw reports retain historical notation with a consolidation note. |
| B5 headline suggests a security boundary finding | Current annotation and state restrict it to same-process raw-pointer characterization outside defined extension use; no formal protection-domain breach |
| B1 seed-era zero-occupancy/division model | Final instrumented capacity/recovery finding governs; no hardware zero-occupancy mechanism is claimed |
| B3 raw sum and aligned footprint conflated | Matrix distinguishes 32768+32769 raw excess from 32767+32769 alignment-hidden excess |
| B4 original SVM and later ordinary-object lifetime work conflated | Separate records, distinct oracles/fix scopes; unpublished follow-up details held |
| B4 initial cross-queue timeout | Explicitly discarded because `clFlush(q2)` was omitted; corrected 5/5 control governs |
| B7 08:25 baseline/fix attempt | Discarded for library mismatch; cite 08:27 matched differential |
| B9 17 October statement that GPU recovery was not evidenced | Superseded by 20 October addendum: two 18 October compute-ring timeouts/resets. Cause and cross-client effect remain unproved. |
| B9 mitigation report says no GPU dispatch | Scoped to 30 October mitigation checks, not all historical B9 work |
| B12 C0/C1 historical STOP/unconfirmed wording | Historical snapshots annotated; R1 establishes the current 2/2 F1 result |
| CTS gap treated as timeless/general | B12 gap restricted to `9feccbb8fcb8eefeebf86a48036173a8155f9d21` and named test directories; other unpinned assessments remain incomplete |

## Evidence checks and limits

- The B4 follow-up integration manifest verifies 14/14 entries, including both compressed archives; archived T6/T7 summaries each contain three failures with passing controls, and archived hardened-PoF T12.3 contains three passes. The archives are transferred evidence, not a newly acquired independent device experiment. Their local confidential-coordination instruction governs publication selection.
- The 30 October B9 package verifies 56/56 manifest entries. The matching source/mitigation and compile-only logs are retained locally. No new compilation or GPU test was performed.
- B12 source, two raw logs and recorded runtime hashes match. The matching Ubuntu source-package version provides a high-confidence explanation for the observed metadata response; there is no instrumented binary/source trace or tested correction.
- B8 source and small IL input exist, but durable paired stdout was not found under the local evidence directories examined. The closure remains report-backed. Missing raw output is an evidence-completeness gap, not a reason to create a new experiment in this task.
- B10 core logs exist. CTS logs do not independently include library maps or a kernel journal; the separate ICD baseline supplies context, not per-process proof for every CTS execution. The optional Out-of-Order arm was not run.
- Historical specification references are not all pinned to archived document revisions. Current Khronos `clSetKernelArg` and semaphore-query reference pages were inspected for the consolidation; this does not reconstruct the exact document seen by every earlier campaign.
- Six already tracked devcoredumps exceed 1 MiB. They remain in public history. New large dumps, a roughly 169 MB untracked userspace core, archives and local builds are excluded and preserved.

## Remaining review questions

1. Confirm the disclosure/maintainer status before releasing the new B4-follow-up and B9 technical packages. Public-upstream fixes, issue acceptance and production readiness are not assumed.
2. Before submitting B12, check current Mesa duplicate/fix status and current CTS coverage. The prepared issue is an F1 conformance report; submission is not authorized by this consolidation.
3. Recover original B8 paired logs only if an existing immutable copy is located. Do not substitute fresh results for the historical record.
4. Pin specification versions and missing per-run provenance where existing archived material permits. Leave unknown fields explicit otherwise.
5. Have maintainers/reviewers assess local fix breadth, error/teardown paths and license compatibility. No repository-wide license has been chosen; this remains a reuse/publication question.
6. H2 capability inventory and cross-implementation controls are the next research task after this documentation review and separately approved publication step.

## Paper and journal value

B1/B3 provide a compositional accounting/availability mechanism; original B4 provides temporal integrity plus multiple causal controls; B7 provides an explicit object-ownership contract and a matched local correction. SEC-1 adds independent-client containment evidence, while SEC-2/O2 demonstrate why oracle admissibility and sensitivity limits matter. B12 is a concise deterministic Host-API conformance case and a scoped CTS candidate. B8/B10 and capability-deferred B6/B11 make negative results and selection limits visible.

A full paper can organize these as enforcement families across specification, API, submission, backend and recovery layers. A stronger journal/generalization claim needs H2 or other independent implementations, consistently pinned contracts and measured containment boundaries. Count mechanisms rather than labels; do not upgrade F1/F2, same-process context effects or undefined-behavior probes into security vulnerabilities. The new lifetime follow-up may strengthen the case study after disclosure review, but its unmeasured physical impact stays unclaimed.
