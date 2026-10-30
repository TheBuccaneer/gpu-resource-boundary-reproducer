# Research method and classification

Consolidation cutoff: 2026-10-30. This document extracts the operative definitions from the local project masterplan; historical plans and seed-mechanism guesses are not evidence. It does not retrospectively supply missing specification snapshots or raw logs.

## Questions and layers

| Question | Meaning |
|---|---|
| RQ1 | Contract enforcement: which API/runtime/compiler/driver layer enforces a contract? |
| RQ2 | Cross-implementation consistency under the same admissible input and oracle |
| RQ3 | Failure containment and effects beyond the initiating API/process |
| RQ4 | Resource invariants and coupled constraints |
| RQ5 | Source/IL representation differences |
| RQ6 | Stateful security and revalidation after state/ownership changes |
| RQ7 | Recurring mechanisms rather than inflated counts of independent bugs |
| RQ8 | Specific conformance coverage gaps or ambiguities |

Layers: L0 specification/CTS; L1 API; L2 representation; L3 compiler IR; L4 backend; L5 runtime/driver; L6 device/isolation; L7 implementation/version comparison. RQ3 is not a synonym for implementation analysis; RQ6 is not error propagation.

## Gated progression

| Stage | Purpose |
|---|---|
| C0 | Verify capability, applicable contract, implementation path, CTS scope and oracle |
| C1 | Establish a valid baseline and functional controls |
| C2 | Exercise a normal range |
| C3 | Approach the boundary from below |
| C4 | Check the exact boundary |
| C5 | Change one condition by the smallest relevant excess |
| C6 | Examine a specifically motivated adversarial state |
| C7 | Add justified resource/complexity interaction |
| C8 | Compare admissible representations |
| C9 | Assess containment under an explicit protection-domain model |

Not every class reaches every stage. Counts and controls stay attached to exact runtime configurations. Capability advertisement, API function availability, platform prerequisites, and demonstrated functionality are separate gates. A code suspicion is not a reproduced implementation failure. An undefined-behavior residual-state or raw-pointer probe is not a conformance oracle.

## Outcomes

**PASS** denotes a successful valid control. **F0** denotes a specified OpenCL error that correctly contains an invalid/infeasible request. Several historical reports use `PASS / F0` for successful controls; the consolidated matrix distinguishes them without altering historical logs.

| Level | Observation |
|---|---|
| F0 | Clean specification-conforming OpenCL error |
| F1 | Wrong success or unexpected result, stable execution |
| F2 | Initiating process dies |
| F3 | Runtime/compiler component enters an erroneous state |
| F4 | Another OpenCL client is affected |
| F5 | GPU context/engine reset |
| F6 | Other GPU applications/display affected |
| F7 | Cross-context corruption or disclosure observation |
| F8 | Stronger memory-safety/isolation effect under the explicitly stated boundary |

Record the maximum observed level plus the supporting observations, not an assumed exploit severity. B4's project F8 classification is intra-context temporal integrity; B5's historical F7 label describes a same-process out-of-contract read. Neither establishes an OS-process protection-domain violation. Cross-client availability observations belong to the separate SEC-1 campaign and must not be silently imported into every B1/B3/B9 run.

Reproduction requires the source/input identity, actual loaded libraries, specification predicate, controlled state change, raw status/data and cleanup. A local patch differential establishes correction only for the exercised path; unrelated intervening commits cannot be treated as a single-change causal experiment. CTS absence claims require a pinned commit and named inspected test scope.
