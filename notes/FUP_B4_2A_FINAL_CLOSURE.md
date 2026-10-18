# FUP-B4-2A — Final Consolidation and Closure

Date: 2026-10-18

Final status: **CLOSED**

Proof-of-fix status: **FIX_CONFIRMED for the tested ordinary-buffer `clEnqueueNDRangeKernel` path only**

## Classification

- **CONFIRMED:** directly established by source, specification text, or hash-verified experimental evidence.
- **SUPPORTED:** strongly supported, but not dynamically demonstrated for every relevant path.
- **UNKNOWN:** the available evidence is insufficient for a claim.

## 1. Executive decision

**CONFIRMED:** Rusticl at Mesa revisions `3821bf78426f5515d26047e49164beedb8ecd18d` and `772c8bba74f6b197e88054669ded5861a8a6a293` can invoke the destructor callback of an ordinary OpenCL buffer while a previously enqueued kernel that uses the buffer still has event status `CL_SUBMITTED`.

**CONFIRMED:** the official SVM-free fix in `772c8bba` does not correct this separate ordinary-`cl_mem` retention path. The result does not show a regression in that fix; it shows that the fix has narrower scope.

**CONFIRMED for the tested path and executions:** the isolated proof-of-fix (PoF), based on `772c8bba`, changed the observed result from three direct violations to three synchronized positive completion observations. In every PoF trial, normal completion was published by the completion callback before the destructor callback's first relevant instrumented observation.

**UNKNOWN:** physical memory corruption, cross-process disclosure, privilege escalation, exploitability, and correctness across all Rusticl commands and all OpenCL object types were not demonstrated.

FUP-B4-2A can therefore be closed as a confirmed implementation finding with a narrowly validated PoF. Upstream acceptance, broad regression coverage, and cross-implementation generalization are separate future activities.

## 2. Research question and hypothesis

Research question: is the confirmed B4 SVM-free defect an isolated API-path error, or does Rusticl release other command-owned memory references at submission rather than terminal command completion?

Hypothesis: after `clSetKernelArg` stores a non-owning reference, the kernel-enqueue path temporarily upgrades buffer and image arguments to strong references, but releases them when submission work is dropped. If the application has already released its last `cl_mem` reference, this permits object deletion and its destructor callback before the kernel command reaches a terminal event status.

The source audit predicted this behavior before dynamic validation. T6 and T7 confirmed it for an ordinary buffer. This source-to-prediction-to-experiment sequence is the central scientific value of FUP-B4-2A.

## 3. Specification-based lifetime invariant

**CONFIRMED:** the OpenCL specification requires deletion of a memory object only after both conditions hold:

1. its reference count has reached zero; and
2. commands queued for execution that use the object have finished.

A memory-object destructor callback is tied to that deletion. Consequently, observing the destructor callback followed by a successful query showing the using kernel event still at `CL_QUEUED`, `CL_SUBMITTED`, or `CL_RUNNING` is direct evidence of a lifetime violation. It is not necessary to prove that a physical GPU memory access was occurring at the exact observation instant.

## 4. Evidence-chain verification

### 4.1 Supplied evidence archives

| Archive | SHA-256 | Verification |
| --- | --- | --- |
| T9 evidence archive | `9056c3dfae0804d25b25aa81b0b4495f679be5fa9fac3452c01932ae5ac6bea4` | **CONFIRMED:** valid XZ/TAR; all 24 copied files matched `COPIES.sha256`; internal manifests verified |
| T13 evidence archive | `3fd8b58887059c5945103f3011d3d098c1b08b8108d4ceab085852f20621eac9` | **CONFIRMED:** valid XZ/TAR; all 56 evidence files matched the archive manifest and `ORIGINALS.sha256` |

The T13 metadata hashes were:

- `PATHS.txt`: `11141343b26e419de4ef6c688e95061b2fcf674ccb2c358c36ca9562f46b0b9f`
- `MANIFEST.tsv`: `cb25818d3790e5c09e758b9b7169ac6687c9b316e7ea73caee803d92c752775c`
- `ORIGINALS.sha256`: `6a6e85f8019d2cc5d733da77bd97a42ef9b229f3cfb22b6c79542f631902d3b8`

**CONFIRMED:** nested T12, T12.1, T12.2 Phase A, and T12.3 manifests were independently checked by mapping their recorded H1 paths to the archived copies. The H1 copies of the PoF patch, hardened source, and offline evaluator are byte-identical to the Cloud research copies.

The archives are transferred, hash-linked copies of H1 evidence. This verifies integrity from the supplied archives onward; it does not claim an independent second acquisition from the H1 machine.

### 4.2 Phase consolidation

| Phase | Evidence-backed result | Classification |
| --- | --- | --- |
| T6 | Baseline: C0 PASS, C1 3/3 FAIL, C2 PASS; destructor observed while the kernel event remained `CL_SUBMITTED`; exit 10 | **CONFIRMED** |
| T7 | Official B4-fix revision: same 3/3 direct violations and passing controls; exit 10 | **CONFIRMED** |
| T9 | Original T6/T7 stdout, stderr, exit, loader, journal, and provenance artifacts copied and hash-verified | **CONFIRMED** |
| T10 | Minimal terminal-retention PoF designed on `772c8bba`; six Rusticl files changed | **CONFIRMED** as source artifact; dynamic efficacy not yet established at T10 |
| T11 | PoF built and installed in isolation; source patch identity, build provenance, ELF dependencies, and isolation manifests checked | **CONFIRMED** |
| T12 | Original T1 oracle: C0/C2 PASS; 3/3 C1 INCONCLUSIVE, with normal completion sequence point 1 and destructor sequence point 2 | **CONFIRMED**; supportive rather than strict fix proof |
| T12.1 | Offline evaluator: T6/T7 FAIL; T12 INCONCLUSIVE because T1 set callback-start flags before assigning sequence points | **CONFIRMED** |
| T12.2 | Hardened Release/Acquire completion-entry oracle created separately; synthetic FAIL/PASS/INCONCLUSIVE cases passed; original T1 preserved | **CONFIRMED** |
| T12.3 | PoF plus hardened oracle: C0 PASS, C1 3/3 PASS, C2 PASS, exit 0; exact PoF runtime loaded; no relevant kernel-journal entry | **CONFIRMED for tested path/executions** |

### 4.3 Exact revisions and principal binaries

| Role | Revision or SHA-256 |
| --- | --- |
| Experimental Mesa baseline | `3821bf78426f5515d26047e49164beedb8ecd18d` |
| Official SVM-fix comparison revision | `772c8bba74f6b197e88054669ded5861a8a6a293` |
| Distance between revisions | 793 commits; therefore not a one-change causal differential |
| Original T1 source | `bdf2948b6329b302130f2912983e8054b7e1ae34af0b53756b6fd4968bfb985a` |
| Original T1 binary | `33938bf32401d4022b0305f71f3a4e3b06d3e7d8bfa9f1b6b4f9447375bca44c` |
| Baseline Rusticl library | `b410ce2948d0c834348710f2403b82aa07f149d469e8c31c3f932aab15ea4aef` |
| Official-fix Rusticl library | `2e6a566496bc57f6d9e7459c5ab0fe058a5ab004d1114916476efaf06a866daf` |
| PoF patch | `3476b8284d5c8efd0c95971bc896938917f80cbc9fe76846c1dc2fbbae997a48` |
| PoF patch ID | `d8f1eca02cedad5772b2ad3b3fd4b76c896050c0` |
| PoF build-tree Rusticl library | `fd339c54ca88dda30d350a7d3e5c8eaa0de2a52e488f6d24c25297456d35da12` |
| PoF installed Rusticl library used in T12/T12.3 | `32d421f38d94d630623904a21443aeb6d9aa675abe07068b9b355a1cae624937` |
| PoF ELF build ID | `1ba5b03b6b45184b24f1c754a383e731ce4f5e05` |
| Hardened T12.2 source | `c67153e3b70577aec4ffbe94251de7835b93e9541dee8aadce20a98cb84d3a56` |
| Hardened H1 binary | `e0813b186cf61a7e8388a4053a6f21cfd45d7a6be07bc87e471b41f4cf3dc73d` |
| T12.1 offline evaluator | `10a5122268f9eb626d97da31e996d2712dd820526f82249f8666a1247658adf3` |
| `libdrm.so` used for comparison | `12aab45c6d1e23a3e1b7988b50c31220e77836fa4d5ad1d7998df3a4cd11c790` |
| `libdrm_amdgpu.so` | `3db52cf8007d56c2e51c23c189f7e69aaba66b68faa6e0701c6f83cec317d79d` |
| OpenCL ICD loader | `3a24c22deedc682eb864596ef301a75d54dea23ca4c35a50deda9480694137c2` |

The installed PoF library differs bytewise from its build-tree counterpart because installation removes the build RUNPATH. Their ELF build IDs and required-library lists match. This is understood provenance, not evidence of source divergence.

## 5. Root-cause analysis

### 5.1 Affected ordinary-`cl_mem` path

**CONFIRMED by source:** `clSetKernelArg` preserves Rusticl's intended weak-retention behavior. At enqueue time, the kernel path upgrades buffer and image arguments to strong `Arc` references for command use. Before the PoF, those strong references reside in the submission work closure. The event path drops that closure after command submission rather than terminal completion.

This conflates two milestones:

- dependency and command submission; and
- completion of the command's use of its memory objects.

If the application has released its final external reference, dropping the submission closure can delete the memory object and invoke its destructor while the kernel event remains nonterminal.

### 5.2 Relation to original B4

**SUPPORTED as a shared architectural class:** original B4 and FUP-B4-2A are distinct API mechanisms but share a submission-versus-completion lifetime error pattern.

- Original B4: host-side SVM free performed too early for an explicit SVM-free command.
- FUP-B4-2A: strong references for ordinary kernel memory arguments released with submission work.

**CONFIRMED:** the official B4 fix adds a cleanup mechanism and moves SVM freeing onto it. It does not attach terminal retention to the ordinary kernel buffer/image argument path. This explains why T7 still fails.

## 6. Proof-of-fix design

The PoF introduces terminal-release state separate from the existing submission work and SVM cleanup state:

1. the kernel enqueue path clones and retains strong buffer and image references;
2. those references are stored in a terminal-release closure owned by the event;
3. when `Event::set_status` receives `CL_COMPLETE` or a negative terminal status, it first publishes the terminal status;
4. it notifies waiters and invokes event callbacks;
5. only then does it invoke and drop terminal-release state.

**CONFIRMED by source:** this ordering prevents the retained memory objects' destructor callbacks from running before the associated event becomes terminal. It is stronger than merely waiting for a GPU fence: terminal status publication and callback ordering are part of the release boundary.

**CONFIRMED by T12.3 for the tested path:** the completion callback published a normal-completion marker with release semantics. The destructor callback's first relevant action acquired and observed that marker in all three trials. Each trial also reported terminal event status, both callbacks, normal completion, completion sequence 1, and destructor sequence 2.

The PoF's concrete source paths are:

| File | Relevant PoF responsibility |
| --- | --- |
| `src/gallium/frontends/rusticl/api/event.rs` | introduces `create_and_queue_with_terminal_release` and passes terminal-release state into the event |
| `src/gallium/frontends/rusticl/api/kernel.rs` | obtains retained arguments for zero-work and dispatched kernels and selects the terminal-release enqueue path |
| `src/gallium/frontends/rusticl/api/queue.rs` | adapts the affected event-construction call site to the extended interface |
| `src/gallium/frontends/rusticl/core/event.rs` | stores `TerminalReleaseSig`; `Event::set_status` takes it only for a terminal status and invokes it after status publication and callbacks |
| `src/gallium/frontends/rusticl/core/kernel.rs` | factors `retain_mem_objects`, retains buffer/image `Arc` maps, and constructs the terminal-release closure |
| `src/gallium/frontends/rusticl/core/queue.rs` | supplies negative terminal status on additional queue/signal failure paths so retained resources can be released correctly |

## 7. Limited static regression review

| Path or concern | Static finding | Status |
| --- | --- | --- |
| Normal kernel completion | Terminal status is published before terminal-retained references are released; event callbacks precede release | **CONFIRMED by source; dynamically confirmed for one ordinary-buffer path** |
| Work-closure error | Error is converted to a negative terminal status before terminal release | **SUPPORTED; not dynamically tested** |
| Dependency error | Negative status propagates through the terminal-status path before release | **SUPPORTED; not dynamically tested** |
| Fence/device failure | Error status is set before terminal release | **SUPPORTED; not dynamically tested** |
| Queue input-channel failure | PoF marks affected queued events `CL_OUT_OF_HOST_MEMORY`, enabling terminal release | **SUPPORTED; not dynamically tested** |
| Signal-channel failure | PoF sets `CL_OUT_OF_RESOURCES` before terminal release | **SUPPORTED; not dynamically tested** |
| Worker panic | normal Rust unwinding reaches a `QueueEvent` drop path that supplies an error state | **SUPPORTED**; process abort and foreign-runtime catastrophes remain **UNKNOWN** |
| Event callbacks | callbacks run after terminal status publication and before terminal-retained objects are dropped | **CONFIRMED by source** |
| Buffer arguments | retained by the PoF | **CONFIRMED by source and tested for an ordinary buffer** |
| Image arguments | included in the retained image map | **CONFIRMED by source; dynamic correction UNKNOWN** |
| Sub-buffers | a sub-allocation owns a reference to its parent allocation, so retaining the sub-buffer keeps the parent reachable | **SUPPORTED by source; dynamic correction UNKNOWN** |
| Zero-global-work kernel command | uses terminal argument retention in the PoF | **SUPPORTED by source; dynamic correction UNKNOWN** |
| Samplers and raw SVM arguments | not part of this ordinary buffer/image retention change | **UNKNOWN outside this finding's scope** |
| Reference cycle | terminal closure owns memory objects; reviewed memory objects do not own the event/queue back | **SUPPORTED: no cycle identified** |
| Unresolved dependency | retained references remain while the command is nonterminal | **CONFIRMED and required behavior**, not by itself a leak |
| Enqueue/in-flight overhead | additional `Arc` clones and retained maps extend lifetime and add limited bookkeeping | **SUPPORTED** |
| `clSetKernelArg` semantics | weak storage behavior is unchanged | **CONFIRMED by source** |
| Original B4 SVM cleanup | remains on the existing cleanup path; ordinary kernel retention uses separate terminal-release state | **CONFIRMED by source; no adverse interaction identified** |

No substantial new correctness defect was found in this bounded review. The queue-error additions broaden the patch beyond the single happy path because terminal release must also be guaranteed on failures; these paths require maintainer review and targeted tests before an upstream-quality claim.

## 8. Experimental comparison

| Property | Baseline `3821bf` | Official B4-fix revision `772c8bba` | `772c8bba` + PoF |
| --- | --- | --- | --- |
| Runtime binding | exact baseline library verified by loader evidence | exact official-fix library verified | exact installed PoF library verified |
| C0 | PASS | PASS | PASS |
| C1 | 3/3 FAIL | 3/3 FAIL | 3/3 PASS with hardened oracle |
| C2 | PASS | PASS | PASS |
| Critical observation | destructor, then event `CL_SUBMITTED` | destructor, then event `CL_SUBMITTED` | normal-completion marker observed at destructor entry; event terminal |
| Kernel journal | no relevant entries | no relevant entries | no relevant entries |
| Interpretation | confirmed lifetime violation | violation remains outside SVM fix | fix confirmed for tested path and executions |

The baseline and official-fix builds are separated by 793 commits. Their equal failure is evidence that the finding exists at both exact revisions, not a narrow causal attribution to the SVM patch. The PoF comparison is narrow because it applies one identified patch to `772c8bba`.

The OpenCL runtime named the device “AMD Radeon 660M,” while PCI enumeration described Rembrandt “[Radeon 680M].” Both map to PCI `0000:04:00.0`; the evidence therefore identifies one physical H1 GPU despite the two labels.

## 9. Scientific contribution, novelty, and generalizability

**CONFIRMED contribution:** FUP-B4-2A establishes a second lifetime violation, on a different OpenCL API path, predicted from the architectural mechanism of original B4 and confirmed by controlled testing.

**SUPPORTED novelty:** the pair provides evidence for a Rusticl lifetime error class centered on releasing host-side ownership at submission rather than terminal command completion. This is scientifically stronger than counting two unrelated bugs because it links specification invariant, source mechanism, prediction, experiment, and repair.

**SUPPORTED publication value:** this is a strong implementation case study and a meaningful component of a broader OpenCL security/runtime paper.

**UNKNOWN generalizability:** no cross-implementation evidence shows that other OpenCL runtimes share this mechanism. The evidence alone does not support a journal-scale claim across vendors or runtime architectures.

## 10. Security impact and explicit non-claims

**CONFIRMED:** premature deletion crosses an OpenCL object-lifetime boundary and can make command execution outlive the runtime object intended to retain its backing resource.

**SUPPORTED:** depending on allocator reuse and backend behavior, this class can plausibly cause stale-resource access, use-after-free-like behavior, corruption, or process/device instability.

The following remain **UNKNOWN** and must not be claimed:

- actual physical GPU access after deletion in these trials;
- memory corruption or data disclosure;
- cross-process, cross-user, or tenant impact;
- privilege escalation or reliable exploitation;
- applicability beyond the tested H1 configuration;
- applicability to every buffer, image, sub-buffer, error, teardown, or command path.

No exploit development was performed.

## 11. Limits of the proof-of-fix

- Dynamic proof is limited to the tested `clEnqueueNDRangeKernel` path, an ordinary buffer, H1, one kernel shape, 4,000,000 iterations, and three PoF trials.
- Images, sub-buffers, zero-work commands, negative completion, queue teardown, dependency failure, and exceptional worker termination were reviewed statically but not dynamically validated.
- The hardened marker proves a synchronized positive callback-entry ordering for these executions. It does not define the only legal callback schedule for all conformant implementations.
- The test environment emitted Rusticl's warning that patched Mesa libclc was not detected. Passing C0/C2 controls and the direct ordering observations reduce concern for this specific result, but the warning remains part of provenance.
- T11's build/install logs contain no warning/error/failure lines. The Meson setup log contains one generic `add_languages` warning that was not counted by the earlier build-diagnostics file; it does not invalidate the build but is recorded here for completeness.
- The PoF is research code, not an upstream-reviewed production patch.

## 12. Potential CTS extension

**SUPPORTED gap:** the inspected CTS snapshot tests destructor callbacks for unused objects and tests that `clSetKernelArg` itself does not retain a memory object, but no located test combines a queued kernel use with final host release and destructor-order validation. A complete CTS-coverage claim requires broader maintainer review.

A robust candidate test would:

1. enqueue a harmless kernel that uses a buffer behind an unsignaled user-event dependency;
2. release the application's final buffer reference;
3. verify that the destructor callback does not run while the dependency is unsignaled;
4. signal the dependency, wait for terminal command completion, and require eventual destructor execution;
5. include positive functional and destructor controls.

The negative oracle—destructor observed while the using event is nonterminal—is specification-based and portable. A rule that the OpenCL completion callback must always execute before the destructor callback should not be made a universal conformance requirement without specification confirmation; the T12.2 marker is a strong positive oracle for this PoF, not necessarily the only conformant scheduling order.

## 13. Coordinated upstream recommendation

1. Coordinate privately with Mesa/Rusticl maintainers and reference the existing confidential process without copying confidential issue content into a public repository.
2. Provide the minimal reproducer, exact revisions and hashes, the direct T6/T7 lifetime oracle, the PoF, and the bounded T12.3 result.
3. Ask maintainers whether terminal resource retention should be generalized in event infrastructure or kept in the kernel path. The research PoF's queue failure handling deserves explicit design review.
4. Add focused unit/CTS-style tests for normal completion and carefully selected negative terminal paths before proposing production acceptance.
5. Keep physical-corruption and security-boundary claims out of the report unless new evidence establishes them.
6. Have a human author the final upstream report and patch rationale. If a Mesa commit is derived from this AI-generated PoF, Mesa's repository instructions require the exact trailer `Generated-by: LLM`.

No public report, patch submission, or external message was made during T13.

## 14. Durable artifacts

The following must be retained:

- T9 and T13 evidence archives with their outer SHA-256 values;
- original T6/T7 raw stdout, stderr, exit, loader, journal, and provenance files;
- T10 PoF report and exact patch;
- PoF Mesa source worktree state or a reproducible patch-plus-base representation;
- T11 setup/build/install/provenance and isolation logs;
- T12 raw run and manifest;
- T12.1 evaluator, input hashes, and output classifications;
- T12.2 hardened source, README, source archive, H1 build provenance, self-tests, and binary hash;
- T12.3 preflight, stdout/stderr, exit, loader, kernel journal, provenance, integrity checks, and manifests;
- this closure report;
- the exact toolchain and local `libdrm` provenance recorded by T11.

Generated Mesa build directories are useful for short-term reproduction but are not a substitute for source revision, patch, options, dependency hashes, and logs.

## 15. Future integration plan for OpenCL2

Nothing is moved or deleted in T13. A later, separately approved integration should proceed as follows:

1. **Freeze both locations.** Record `git status`, worktree lists, absolute paths, and a fresh manifest for `/home/rock/projects/fup-b4-2a-h1` and `/home/rock/projects/OpenCL2`. Confirm that B8 is idle and identify all B8-owned files before writes.
2. **Choose repository destinations.** Suggested layout:
   - `notes/FUP_B4_2A_*` for audit, PoF, and closure reports;
   - `patches/b4/` for the exact PoF patch and provenance;
   - `src/fup-b4-2a-*` for original and hardened reproducers;
   - `scripts/` for the offline evaluator;
   - `results/b4/fup-b4-2a/` for compact, immutable logs and manifests;
   - large evidence archives under an explicitly approved artifact policy rather than silently adding them to Git.
3. **Preserve, do not relocate, live worktrees first.** The PoF Mesa worktree is linked to another Git repository. Copying its directory does not create an independent valid worktree. Preserve the base commit plus exact patch; if a new worktree is needed, create it through `git worktree add` from the owning Mesa repository and reapply the verified patch.
4. **Treat build trees as path-bound.** Meson/Ninja metadata contains absolute paths under `/home/rock/projects/fup-b4-2a-h1` and the Mesa worktrees. Do not move build directories and call them reproducible. Keep them in place temporarily or rebuild later into new isolated paths after separate approval.
5. **Copy immutable evidence.** Copy files without changing originals, generate destination hashes, compare them to the verified source manifests, and retain a source-to-destination mapping. Do not normalize line endings or regenerate logs.
6. **Integrate research sources separately from generated binaries.** Preserve binary hashes in provenance. Decide explicitly whether binaries belong in Git, release storage, or a research archive.
7. **Recheck B8 and the main checkout.** Confirm B8 source/binary hashes, system ICD hash, system Rusticl hash, branch/worktree states, and unrelated user changes before and after integration.
8. **Review for confidentiality.** Ensure no content from the confidential Mesa issue enters public files. Keep any private disclosure bundle outside the public repository.
9. **Commit only after review.** Stage only the approved B4 paths, inspect the staged diff and manifests, and use a dedicated commit. Do not push without separate authorization.
10. **Retire temporary paths only later.** Delete no worktree, build, or evidence directory until the integrated copy and an independent backup both pass their manifests and the owning Git repository confirms safe worktree removal.

## 16. Closure criteria

| Criterion | Result |
| --- | --- |
| Specification oracle established | **CONFIRMED** |
| Baseline violation reproduced | **CONFIRMED, 3/3** |
| Official SVM-fix revision assessed | **CONFIRMED, violation remains 3/3** |
| Original raw evidence verified | **CONFIRMED** |
| Root cause linked to source | **CONFIRMED** |
| Minimal PoF built in isolation | **CONFIRMED** |
| PoF ordering validated with hardened oracle | **CONFIRMED for tested path, 3/3** |
| Material static regression blocker found | **No** |
| Untested paths clearly bounded | **Yes** |
| Public/upstream action performed | **No** |
| B8 modified | **No evidence of modification by this work** |

**Final classification: FUP-B4-2A is CLOSED.**

Closure means that the research question has a defensible answer, the evidence chain is verified, and the tested path has a narrowly confirmed PoF. It does not mean that the PoF is production-ready, upstream-accepted, or validated across all paths and implementations.
