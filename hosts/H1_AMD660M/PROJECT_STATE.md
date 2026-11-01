# OpenCL2 Project State

Updated: **2026-10-30**. Scope: **H1 research consolidation**, not new experiments. Newer class closures take precedence over earlier plans and phase gates. Detailed evidence, version scope, oracle, counts, fix status, and limitations are in the [H1 evidence matrix](docs/H1_EVIDENCE_MATRIX_2026-10-30.md).

## Platform and interpretation

H1 is AMD Radeon 660M / Rembrandt / GFX10.3, Linux AMDGPU, Mesa Rusticl / RadeonSI / ACO, OpenCL 3.0. Historical campaigns used different kernels and Mesa builds; they must not be pooled into one runtime configuration. Original work used system Mesa 26.0.3 and isolated Mesa 26.2.0 builds; later B7/B8/B10/B12 work records system Mesa 26.0.8-1ubuntu0.3. B12 process maps and hashes identify the system ICD explicitly. Local B4/B7/B9 patch builds are distinct configurations.

An advertised extension is not a functional proof. A local proof of fix is not upstream acceptance. Same-process OpenCL context access is not evidence of cross-process or cross-user access. Failure levels describe observations; they do not by themselves establish a security vulnerability.

## B1–B12 status

| Class | Current H1 disposition | Supported result |
|---|---|---|
| B1 Resource capacity | **CLOSED — finding** | A +1-byte dynamic-local-memory excess reaches dispatch and is followed by process failure and successful compute-ring recovery; source/instrumentation support a missing capacity recheck. Max observed class F5, also F2. |
| B2 Workgroup/dispatch geometry | **CLOSED — positive validation control** | Tested kernel-specific geometry and dimension errors are rejected with specified API errors (F0); valid controls PASS. |
| B3 Combined resources | **CLOSED H1 — finding** | Raw over-limit and alignment-hidden local-memory cases reach dispatch. Do not conflate the raw sum with aligned footprint. F2/F5 observed; same accounting family as B1, not automatically another independent root cause. |
| B4 SVM/lifetime | **CLOSED H1 — finding and follow-ups** | Same-queue free before prior GPU completion permits cross-lifetime integrity corruption. Project classification F8, restricted to intra-context temporal integrity. Callback and NULL-callback baseline/fix arms each have 5/5 results. Later local lifetime follow-up is closed with a path-limited proof of fix; disclosure-sensitive details are held from this public upload. |
| B5 Raw device-pointer authority | **CLOSED — characterization** | Foreign raw-address reads across two contexts in one process were observed on 26.0.3 and 26.2.0 (5/5 each). The dereference is outside the extension's defined usage; no formal protection-domain breach or conformance violation is established. |
| B6 Mutable-state revalidation | **DEFERRED H1** | Required command-buffer/mutable-dispatch capability was absent in the recorded H1 configuration. No negative result about an implemented path. |
| B7 Object/context ownership | **CLOSED H1 — finding; local fix confirmed** | Ordinary foreign-context `cl_mem` is accepted and modified through the other context in the same process/device. Matched local 26.2.0 patch rejects at `clSetKernelArg` while controls PASS. No cross-process claim. |
| B8 Representation mismatch | **CLOSED — bounded negative result** | Tested valid Source/IL pairs agree on data and metadata. The 65537-byte case was stopped before enqueue. A compiler-path asymmetry is an audit observation, not a demonstrated bypass. |
| B9 Integer/arithmetic backend | **Finding and root cause documented; local mitigation confirmed; upstream open** | Backend arithmetic finding, separate host OOM, and two observed compute-ring timeout/reset events must remain distinct. The arithmetic defect was not causally isolated as the reason for those timeouts; no cross-client impact proved. The 30 October local mitigation was checked without GPU dispatch and has limited driver/path scope. Detailed unpublished material is held for disclosure review. |
| B10 Synchronization/state | **CLOSED H1 — F0 / bounded negative result** | In-Order, cross-queue, user-event, two named CTS marker tests, and a foreign-event FillBuffer case behaved as expected. Valid controls PASS; specified error cases F0. No general race-safety claim. |
| B11 Multi-device/ICD identity | **DEFERRED H1** | Recorded configuration has one GPU/device, one ICD, and no subdevice partitioning; the required identity differential was unavailable. |
| B12 External memory/semaphore/interop | **CLOSED H1 — F1** | Exportable Sync-FD binary semaphore returns `CL_SUCCESS` with an empty object export-type list in 2/2 runs on system Rusticl 26.0.8. Matching package source explains the metadata inconsistency. No GPU dispatch or demonstrated security-boundary violation. External memory and GL-sharing execution paths were not evaluated. |

B13 (error propagation) and B14 (version regression) remain cross-cutting axes. **Next regular platform: H2, NVIDIA RTX 3090**, with capability gates and selected differential controls; [roadmap](docs/H2_ROADMAP.md). No H2 experiments have started.

## Important closure boundaries

- **B1/B3 recovery:** compute-ring timeout and successful ring reset are engine-level observations. Do not describe every event as a full-device reset. No hardware zero-occupancy mechanism is claimed by the final B1 closure.
- **B4:** the original SVM causal patch corrected the tested path. The later ordinary-object follow-up must not be confused with either the original SVM result or an upstream-complete fix. The discarded cross-queue run missing `clFlush(q2)` remains excluded.
- **B7:** cite `results/b7/20261003_082740_mesa262_valid_baseline_vs_b7fix/`; the 08:25 library-mismatched run is discarded.
- **B8:** source audit uses Mesa 26.2.0, runtime observations use 26.0.8. Durable paired stdout is not present in the local B8 evidence examined for this consolidation; source and IL inputs are present.
- **B9:** the 17 October statement that no GPU recovery was evidenced is superseded by the 20 October addendum describing two 18 October compute-ring recoveries. The later mitigation checks were compile-only; do not turn that limited statement into a claim that no earlier B9 runtime dispatch occurred.
- **B10:** one run per core mode and two selected CTS tests are not a complete CTS pass. The CTS logs do not independently record the loaded ICD path or kernel journal.
- **B12:** the CTS query gap is scoped to commit `9feccbb8fcb8eefeebf86a48036173a8155f9d21` and the examined semaphore test directories. No statement about every current CTS version is supported.

## Supplementary security/containment studies

| Study | H1 closure and limits |
|---|---|
| SEC-1 / O1 | **CLOSED, cross-client availability finding.** An independent victim can stall during successful ring recovery; failed ring recovery can escalate to MODE2 full-device recovery with heterogeneous victim outcomes. The N=10 campaign is descriptive, not a probability estimate. Canary V3 and GUI-less controls are retained. |
| SEC-2 | **CLOSED, LDS-remanence mechanism characterization.** Observable patterns persist without cleanup; cleaner execution correlates strongly with pattern loss. The reader intentionally reads uninitialized local memory, so this is not an OpenCL conformance oracle or proof of an OS isolation breach. |
| O2 | **CLOSED H1, sensitivity-limited/inconclusive.** No positive post-reset leakage evidence. The matched no-reset delay is also negative; reset-specific clearing cannot be inferred. |

## Canonical record and publication state

Use the class links in [the evidence matrix](docs/H1_EVIDENCE_MATRIX_2026-10-30.md). [Quality review](docs/H1_QUALITY_REVIEW_2026-10-30.md) records missing evidence and historical interpretation conflicts. New B4-follow-up/B9 detailed evidence remains local pending disclosure review; a complete local index is maintained outside the upload allowlist. No source evidence, worktree, build, archive, or dump was deleted. Public push requires explicit approval.
