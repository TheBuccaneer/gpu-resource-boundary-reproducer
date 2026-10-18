# FUP-B4-2A/T8 — `cl_mem` Lifetime Evidence Consolidation

Date: 2026-10-17  
Scope: static evidence review only; no build, OpenCL call, GPU execution, test modification, commit, push, or disclosure  
Worktree: `fup-b4-2a-clmem-lifetime` at `e61234fa5af40480fd663e136b9cb3efbd75099a`

T9 verification update: 2026-10-17; immutable H1 T6/T7 evidence copies
transferred and independently checked.

## Decision

**FUP-B4-2A is CONFIRMED within the supplied H1 run evidence on the two tested
Rusticl revisions.** In all six C1 trials, the memory-object destructor callback
was observed before a subsequent event query returned `CL_SUBMITTED`; the event
reached `CL_COMPLETE` only later. C0 and C2 passed in both runs.

The source mechanism is independently **CONFIRMED** at both exact Mesa commits:
the kernel command upgrades weak memory-object references to strong `Arc`
references captured by its one-shot `EventSig`; the queue consumes and drops
that closure at command submission, not fence completion. The ordinary kernel
path does not attach a completion cleanup callback. The cleanup infrastructure
introduced for B4 exists at the fixed revision, but `clEnqueueNDRangeKernel`
continues to use `create_and_queue`, which supplies `cleanup = None`.

This establishes an OpenCL object-lifetime/ordering violation. It does **not**
establish that the physical shader was still accessing storage at callback
time, that the backing allocation was reclaimed, or that corruption occurred.

## Classification vocabulary

- **CONFIRMED:** directly established by the reviewed source or internally
  complete experimental evidence.
- **SUPPORTED:** strong evidence exists, but an independent input, exhaustive
  search, or causal isolation remains missing.
- **HYPOTHESIS:** plausible mechanism or consequence requiring validation.
- **UNKNOWN:** available evidence is insufficient.
- **REJECTED:** contradicted by the reviewed evidence.

## Evidence boundary

The cloud audit still has no direct filesystem access to H1. T9 closed the
transfer gap by creating immutable evidence copies and manifests on H1, then
transferring the resulting archive for an independent check. Consequently:

- **VERIFIED within the supplied H1 evidence chain:** the on-H1 verification
  output reports all 24 originals and all 24 copies as matching their
  manifests. The transferred archive has the expected outer hash, and all 24
  transferred files independently match `COPIES.sha256`.
- **CONFIRMED by direct byte comparison:** the transferred T6 and T7 stdout
  and stderr files are byte-identical to the corresponding sections of the
  previously reviewed terminal transcripts. Both raw exit files contain
  `10`; both raw kernel captures contain `-- No entries --`.
- **CONFIRMED from transferred raw artifacts:** the six C1 records, both
  control summaries, runtime identities, loader paths, and reference binary
  hashes used below. The T7 run-local manifest also independently matches all
  five transferred T7 run files it covers.
- **CONFIRMED independently:** exact Rusticl source at both Mesa commits, the
  normative OpenCL text, the T1 oracle implementation in this worktree, and
  the inspected CTS snapshot.

This classification does not claim that the cloud auditor directly observed
the original H1 filesystem at acquisition time. It records a complete
hash-linked chain from the on-H1 original/copy checks to the independently
verified transferred bytes.

Cloud-review input transcript hashes (these identify only the supplied
terminal transcript copies):

| Input | SHA-256 |
| --- | --- |
| T6 terminal transcript | `f639e4e947df09ee675c603cc531be1c1eba59f440a515a9117534f7ff482d1e` |
| T7 terminal transcript | `3ef24552b86bce6ef993ee51e64219d28d2dfb9219019ff7e377f46cd560fcd3` |

### T9 transfer and manifest verification

| Artifact | SHA-256 |
| --- | --- |
| Transferred T9 archive | `9056c3dfae0804d25b25aa81b0b4495f679be5fa9fac3452c01932ae5ac6bea4` |
| `MANIFEST.tsv` | `d55a7ed60d8b049243e740be9c262a3d81d76088da2f90bd74ff376906c0de39` |
| `ORIGINALS.sha256` | `85bbd2e4f5791e51ffe7ee716dd83b2d7a719540dfba2e431403a532a081f29c` |
| `COPIES.sha256` | `9b4667d07d7994c9b999c805c21bc0f99849693fb48bdcf3e19922e1853c5132` |
| `METADATA.sha256` | `6b0e75c9ed8ddfc66ea85ba4f3600d4a552f80bf97b06e6785cf828343893cd5` |

The manifest contains 24 data rows, matching the 24 entries in each of the
original and copy checksum lists. The transferred references independently
reproduce these hashes:

| Reference artifact | SHA-256 |
| --- | --- |
| T1 source | `bdf2948b6329b302130f2912983e8054b7e1ae34af0b53756b6fd4968bfb985a` |
| T1 binary | `33938bf32401d4022b0305f71f3a4e3b06d3e7d8bfa9f1b6b4f9447375bca44c` |
| Baseline Rusticl | `b410ce2948d0c834348710f2403b82aa07f149d469e8c31c3f932aab15ea4aef` |
| Fixed-revision Rusticl | `2e6a566496bc57f6d9e7459c5ab0fe058a5ab004d1114916476efaf06a866daf` |
| Local `libdrm` | `12aab45c6d1e23a3e1b7988b50c31220e77836fa4d5ad1d7998df3a4cd11c790` |
| Local `libdrm_amdgpu` | `3db52cf8007d56c2e51c23c189f7e69aaba66b68faa6e0701c6f83cec317d79d` |
| ICD loader `libOpenCL.so.1` | `3a24c22deedc682eb864596ef301a75d54dea23ca4c35a50deda9480694137c2` |

No content from the confidential Mesa issue was consulted or copied into this
report. B4 facts below come from the local research record and the public fix
commits.

## T1 oracle review

The approved source is
`src/fup-b4-2a-t1/fup_b4_2a_cl_mem_lifetime.c`, SHA-256
`bdf2948b6329b302130f2912983e8054b7e1ae34af0b53756b6fd4968bfb985a`.
The H1 binary used in both runs was reported as
`33938bf32401d4022b0305f71f3a4e3b06d3e7d8bfa9f1b6b4f9447375bca44c`.

The oracle is causally narrow:

1. C0 completes the kernel first, verifies that the destructor has not run
   while the explicit reference remains, releases that reference, and then
   requires destructor and completion callbacks (`run_c0`, lines 601–653).
2. C1 enqueues the kernel, releases the application's final explicit `cl_mem`
   reference, flushes, observes the destructor flag with acquire semantics,
   and queries the still-retained event (`run_c1_trial`, lines 691–776).
3. A C1 `FAIL` requires that the destructor was already observed before the
   query and that the query successfully returned a status greater than
   `CL_COMPLETE` (`observe_c1_order`, lines 656–688).
4. C2 verifies ordinary callback delivery for an object with no queued user
   (`run_c2`, lines 779–805).

The destructor and event callbacks perform only atomic state updates. The
event is retained through the oracle and cleanup. The query occurs after an
acquire observation of the destructor flag. The result therefore demonstrates
API-model ordering, while deliberately making no claim about instantaneous
physical shader execution.

## Experimental evidence review

Both runs used `4,000,000` iterations, three C1 trials, and a `15,000 ms`
per-stage timeout. `RUSTICL_DEBUG=sync` was absent, all three preflight GPU-busy
samples were zero, and no matching AMDGPU/DRM fault, reset, or timeout appeared
in either captured kernel log.

| Run | Mesa revision | Trial | Status queried after destructor | Query | Destructor sequence | Completion sequence | Final status | Result |
| --- | --- | ---: | --- | ---: | ---: | ---: | --- | --- |
| T6 | `3821bf78426f5515d26047e49164beedb8ecd18d` | 1 | `2` (`CL_SUBMITTED`) | `CL_SUCCESS` | 1 | 2 | `CL_COMPLETE` | FAIL |
| T6 | `3821bf78426f5515d26047e49164beedb8ecd18d` | 2 | `2` (`CL_SUBMITTED`) | `CL_SUCCESS` | 1 | 2 | `CL_COMPLETE` | FAIL |
| T6 | `3821bf78426f5515d26047e49164beedb8ecd18d` | 3 | `2` (`CL_SUBMITTED`) | `CL_SUCCESS` | 1 | 2 | `CL_COMPLETE` | FAIL |
| T7 | `772c8bba74f6b197e88054669ded5861a8a6a293` | 1 | `2` (`CL_SUBMITTED`) | `CL_SUCCESS` | 1 | 2 | `CL_COMPLETE` | FAIL |
| T7 | `772c8bba74f6b197e88054669ded5861a8a6a293` | 2 | `2` (`CL_SUBMITTED`) | `CL_SUCCESS` | 1 | 2 | `CL_COMPLETE` | FAIL |
| T7 | `772c8bba74f6b197e88054669ded5861a8a6a293` | 3 | `2` (`CL_SUBMITTED`) | `CL_SUCCESS` | 1 | 2 | `CL_COMPLETE` | FAIL |

Every C1 record additionally reports `query_error=0`, `cleanup_terminal=1`,
`final_query_error=0`, `destructor_seen=1`, and
`complete_callback_seen=1`. Both summaries report
`c0=PASS c1_pass=0 c1_fail=3 c1_inconclusive=0 c2=PASS`; both processes exit
with the probe's defined failure code `10`.

The sequence statement must remain precise: the host first observed that the
destructor callback had started and then queried `CL_SUBMITTED`. The trace does
not sample the event status inside the callback itself. It does prove that the
command had not reached `CL_COMPLETE` after callback observation.

### Controls and provenance

| Property | T6 baseline | T7 official-fix revision |
| --- | --- | --- |
| C0 | PASS; completion sequence 1, destructor sequence 2 | PASS; completion sequence 1, destructor sequence 2 |
| C2 | PASS | PASS |
| Driver string | `26.3.0-devel (git-3821bf7842)` | `26.3.0-devel (git-772c8bba74)` |
| Loaded path | `install/baseline/.../libRusticlOpenCL.so.1` | `install/fixed/.../libRusticlOpenCL.so.1` |
| Rusticl SHA-256 | `b410ce2948d0c834348710f2403b82aa07f149d469e8c31c3f932aab15ea4aef` | `2e6a566496bc57f6d9e7459c5ab0fe058a5ab004d1114916476efaf06a866daf` |
| Device | AMD Radeon 660M / Rembrandt / PCI identity established in T5 | same |
| Process result | exit 10 | exit 10 |

T9 independently verifies these transferred H1 raw-file hashes:

| Run file | SHA-256 |
| --- | --- |
| `t6-preflight-provenance.txt` | `c8e2bbbd10d0e2d9550f7983cc65fc3bf02548800feeb220d3437ead81e048ba` |
| `t6-baseline-stdout.txt` | `87b5fa3454479fd7779457b2853e6a0666ff77ec2823e493bcb3fe598315eeeb` |
| `t6-baseline-stderr.txt` | `d81e7642c085710c893cb62f8c9ee8f44a4136d08c446fb0f7fc7427d458c821` |
| `t6-baseline-exit.txt` | `917df3320d778ddbaa5c5c7742bc4046bf803c36ed2b050f30844ed206783469` |
| `t6-baseline-kernel.txt` | `19c4e28b2f54ea8f217db4e622b0a5c758efa7b77ce2dc0394b6d159568c0b2e` |
| `t6-baseline-loader.41596` | `3a95df75b123be31b85f6a850c37b01a025c4c86d3e2ca728187c02bec941d5c` |
| `t7-fixed-stdout.txt` | `c55948d6095683acc08a2a04328b82c921ed8e429f3dfa2481660331213e4d31` |
| `t7-fixed-stderr.txt` | `d81e7642c085710c893cb62f8c9ee8f44a4136d08c446fb0f7fc7427d458c821` |
| `t7-fixed-exit.txt` | `917df3320d778ddbaa5c5c7742bc4046bf803c36ed2b050f30844ed206783469` |
| `t7-fixed-kernel.txt` | `19c4e28b2f54ea8f217db4e622b0a5c758efa7b77ce2dc0394b6d159568c0b2e` |
| `t7-fixed-loader.43841` | `2ad152e8a1cfbaaca12a8f3c194b4609a7c2e58d82f316520e87dbc8b4735ea0` |

The identical patched-libclc warning and optional loader-symbol lookup
diagnostics occurred on both sides. Both OpenCL processes nevertheless reached
all controls and produced their defined exit result. These diagnostics are
recorded limitations; they do not explain a difference because the observed
behavior is identical.

## Source-level root cause

The following chain is present at fixed revision `772c8bba`; the relevant
kernel, queue, and memory-object logic is identical at baseline `3821bf` except
for an unrelated local-size index correction in `core/kernel.rs`.

### 1. Kernel objects retain `cl_mem` arguments only weakly

`clSetKernelArg` obtains a temporary `Arc<Buffer>` from the API handle, then
stores `Arc::downgrade(&buffer)` as `KernelArgValue::Buffer`:

- [`api/kernel.rs:454–466`](https://github.com/chaotic-cx/mesa-mirror/blob/772c8bba74f6b197e88054669ded5861a8a6a293/src/gallium/frontends/rusticl/api/kernel.rs#L454-L466)
- [`core/kernel.rs:45–68`](https://github.com/chaotic-cx/mesa-mirror/blob/772c8bba74f6b197e88054669ded5861a8a6a293/src/gallium/frontends/rusticl/core/kernel.rs#L45-L68)

This correctly avoids treating `clSetKernelArg` itself as a lasting retain.
The temporary `Arc` from `arc_from_raw` increments the strong count and drops
when argument setup returns; `ArcedCLObject::release` decrements the strong
count for `clReleaseMemObject`:

- [`api/icd.rs:346–400`](https://github.com/chaotic-cx/mesa-mirror/blob/772c8bba74f6b197e88054669ded5861a8a6a293/src/gallium/frontends/rusticl/api/icd.rs#L346-L400)
- [`api/memory.rs:2394–2411`](https://github.com/chaotic-cx/mesa-mirror/blob/772c8bba74f6b197e88054669ded5861a8a6a293/src/gallium/frontends/rusticl/api/memory.rs#L2394-L2411)

### 2. Enqueue upgrades `Weak` to `Arc` inside a one-shot work closure

`Kernel::launch_with_args` upgrades buffer and image arguments and places the
strong references in `buffer_arcs` / `image_arcs`. Those maps are captured by
the returned `FnOnce` `EventSig`:

- [`core/kernel.rs:1672–1736`](https://github.com/chaotic-cx/mesa-mirror/blob/772c8bba74f6b197e88054669ded5861a8a6a293/src/gallium/frontends/rusticl/core/kernel.rs#L1672-L1736)
- [`core/kernel.rs:1821–1845`](https://github.com/chaotic-cx/mesa-mirror/blob/772c8bba74f6b197e88054669ded5861a8a6a293/src/gallium/frontends/rusticl/core/kernel.rs#L1821-L1845)

`clEnqueueNDRangeKernel` passes that closure through the ordinary
`create_and_queue` path:

- [`api/kernel.rs:803–818`](https://github.com/chaotic-cx/mesa-mirror/blob/772c8bba74f6b197e88054669ded5861a8a6a293/src/gallium/frontends/rusticl/api/kernel.rs#L803-L818)

### 3. The event owns the closure only until submission work is called

`create_and_queue` is a wrapper that calls `create_and_queue_with_cleanup`
with `cleanup = None`; `Event::new` stores the work closure in
`EventMutState.work`:

- [`api/event.rs:170–215`](https://github.com/chaotic-cx/mesa-mirror/blob/772c8bba74f6b197e88054669ded5861a8a6a293/src/gallium/frontends/rusticl/api/event.rs#L170-L215)
- [`core/event.rs:49–57, 270–305`](https://github.com/chaotic-cx/mesa-mirror/blob/772c8bba74f6b197e88054669ded5861a8a6a293/src/gallium/frontends/rusticl/core/event.rs#L49-L57)

The queue worker invokes `Event::call`; that method removes the closure with
`lock.work.take()`, invokes the `FnOnce`, and then sets the event to
`CL_SUBMITTED`. When the closure returns, its captured maps and their strong
`Arc<Buffer>` references are dropped:

- [`core/event.rs:440–460`](https://github.com/chaotic-cx/mesa-mirror/blob/772c8bba74f6b197e88054669ded5861a8a6a293/src/gallium/frontends/rusticl/core/event.rs#L440-L460)
- [`core/queue.rs:441–496`](https://github.com/chaotic-cx/mesa-mirror/blob/772c8bba74f6b197e88054669ded5861a8a6a293/src/gallium/frontends/rusticl/core/queue.rs#L441-L496)

For the tested top-level buffer, after the application releases its explicit
handle, this captured `Arc` is the only visible object-level strong reference
that bridges enqueue to command processing. The kernel object's stored value
is weak. The destructor callback registration itself does not retain the
object.

### 4. Last object reference triggers the destructor callback

`MemBase::drop` takes and calls the registered callbacks. Therefore dropping
the final `Arc<Buffer>` also drops its embedded `MemBase` and invokes the
callback:

- [`api/memory.rs:483–496`](https://github.com/chaotic-cx/mesa-mirror/blob/772c8bba74f6b197e88054669ded5861a8a6a293/src/gallium/frontends/rusticl/api/memory.rs#L483-L496)
- [`core/memory.rs:1244–1250`](https://github.com/chaotic-cx/mesa-mirror/blob/772c8bba74f6b197e88054669ded5861a8a6a293/src/gallium/frontends/rusticl/core/memory.rs#L1244-L1250)

The EventSig has already bound Gallium resources before returning. Backend
objects may independently retain physical storage, but that does not retain
the OpenCL `MemBase` object or postpone its destructor callback. This
distinction explains why the OpenCL lifetime violation can be confirmed while
physical reclamation or corruption remains unknown.

### 5. Completion is signaled later

The queue flushes submitted commands and sends their fence and events to a
separate signal thread. Only after `fence.wait()` succeeds does that thread
call `Event::signal`, which transitions through `CL_RUNNING`, invokes any
cleanup callback, and sets `CL_COMPLETE`:

- [`core/queue.rs:339–355, 501–525`](https://github.com/chaotic-cx/mesa-mirror/blob/772c8bba74f6b197e88054669ded5861a8a6a293/src/gallium/frontends/rusticl/core/queue.rs#L339-L355)
- [`core/event.rs:406–425`](https://github.com/chaotic-cx/mesa-mirror/blob/772c8bba74f6b197e88054669ded5861a8a6a293/src/gallium/frontends/rusticl/core/event.rs#L406-L425)

This establishes the problematic boundary:

```text
KernelArgValue::Buffer(Weak)
  -> enqueue upgrades Weak to Arc
  -> Arc captured by EventSig work closure
  -> queue worker consumes EventSig while submitting work
  -> closure drops; final Arc may drop MemBase
  -> destructor callback
  -> fence wait completes later
  -> event CL_COMPLETE callback
```

### Other possible strong references

- The application API handle is a strong reference, but C1 intentionally and
  validly releases its final explicit reference after enqueue.
- The temporary `Arc` used by `clSetKernelArg` is not retained; the kernel
  stores `Weak` by design.
- Duplicate uses of the same buffer within one launch are deduplicated in
  `buffer_arcs`; this changes the count, not the lifetime boundary.
- Sub-buffers and images can introduce different ownership relationships and
  were not exercised by T1. No claim is made about them.
- Gallium resource references can protect backing storage independently of
  the OpenCL object. Their exact fence-lifetime behavior is outside the proven
  object-lifetime trace.

## Why the official B4 fix does not cover this path

Between the two audited revisions:

- `api/kernel.rs`, `core/queue.rs`, and `core/memory.rs` are byte-identical.
- The only change in `core/kernel.rs` is an unrelated `grid[0]` to `grid[i]`
  local-size correction.
- `api/event.rs` and `core/event.rs` add an optional cleanup closure executed
  just before completion.
- `api/memory.rs` moves `clEnqueueSVMFree` lifetime work from the submission
  closure into that completion cleanup closure.

The fixed SVM path explicitly calls `create_and_queue_with_cleanup`:

- [`api/memory.rs:2611–2627`](https://github.com/chaotic-cx/mesa-mirror/blob/772c8bba74f6b197e88054669ded5861a8a6a293/src/gallium/frontends/rusticl/api/memory.rs#L2611-L2627)

The kernel path still calls `create_and_queue`, which supplies no cleanup.
Thus the B4 infrastructure is reusable but is not automatically applied to
ordinary kernel-argument retention. The T7 result matches this source-level
prediction.

## OpenCL specification invariant

The official OpenCL documentation source was reviewed at Khronos commit
`f03cc607100cc8212cb6aff1d92938448c9c1be0`.

`clReleaseMemObject` states:

> After the memobj reference count becomes zero and commands queued for
> execution on a command-queue(s) that use memobj have finished, the memory
> object is deleted.

[`opencl_runtime_layer.asciidoc:5914–5920`](https://github.com/KhronosGroup/OpenCL-Docs/blob/f03cc607100cc8212cb6aff1d92938448c9c1be0/api/opencl_runtime_layer.asciidoc#L5914-L5920)

`clSetMemObjectDestructorCallback` states that the callback receives the
memory object being deleted, that it is no longer valid when called, and that
the registered callbacks are called before resources are freed and the object
is deleted:

[`opencl_runtime_layer.asciidoc:5923–5955`](https://github.com/KhronosGroup/OpenCL-Docs/blob/f03cc607100cc8212cb6aff1d92938448c9c1be0/api/opencl_runtime_layer.asciidoc#L5923-L5955)

The required invariant is therefore:

```text
last application reference released
  AND every queued command using memobj finished
  -> destructor callbacks / object deletion may occur
```

In every C1 trial, the destructor callback was already observable while the
kernel event subsequently remained `CL_SUBMITTED`. In the OpenCL execution
model the command had not finished. **The invariant is violated.** This is
independent of whether hardware happened to finish individual memory accesses
before the event transition.

## B4 versus FUP-B4-2A

| Dimension | B4 SVM lifetime | FUP-B4-2A `cl_mem` lifetime |
| --- | --- | --- |
| API transition | Explicit `clEnqueueSVMFree` command | Implicit deletion after final `clReleaseMemObject` |
| Object/data representation | SVM allocation and pointer | Ordinary OpenCL buffer object passed as kernel argument |
| Immediate source defect | Free work executed as submission-time Event work | Strong `Arc<Buffer>` retained only by submission-time Event work |
| Required safe boundary | Dependencies complete before free command acts | All queued users finish before object deletion |
| Official B4 fix | Moves SVM free to event cleanup near completion | Does not attach cleanup retention to kernel events |
| Dynamic ordering evidence | Callback/free while dependency `CL_SUBMITTED` | Destructor callback observed before queried event remained `CL_SUBMITTED` |
| Strongest demonstrated impact | Cross-lifetime memory-integrity violation within one context (existing B4 record) | Object-lifetime/ordering violation; no physical corruption demonstrated |

**CONFIRMED common architectural theme:** submission and completion are
distinct lifetime boundaries, but both original paths released host-side
lifetime state at submission.

**CONFIRMED distinct code-level mechanisms:** B4 placed an explicit SVM-free
operation in the wrong event phase; FUP-B4-2A drops implicit kernel-argument
object retention when the submission closure is consumed. They are siblings,
not duplicate observations of one API call.

**SUPPORTED generalization:** the evidence broadens B4 from one explicit
SVM-free command to a second, ordinary `cl_mem` retention path. It supports a
mechanism-oriented Rusticl lifetime class. It does not generalize across OpenCL
implementations, devices, processes, or security principals.

## CTS coverage review

A repository-wide static search was performed on OpenCL-CTS commit
`401a093ddef5cb29930d2522fba1501642c53767` (2026-10-16).

- The memory-object destructor test checks callback delivery and reverse
  registration order after releasing otherwise unused buffers/images:
  [`test_mem_objects.cpp:29–116`](https://github.com/KhronosGroup/OpenCL-CTS/blob/401a093ddef5cb29930d2522fba1501642c53767/test_conformance/api/test_mem_objects.cpp#L29-L116).
- `retain_mem_object_set_kernel_arg` correctly checks that merely setting a
  kernel argument does not retain the memory object, but it never enqueues the
  kernel:
  [`test_retain.cpp:235–270`](https://github.com/KhronosGroup/OpenCL-CTS/blob/401a093ddef5cb29930d2522fba1501642c53767/test_conformance/api/test_retain.cpp#L235-L270).
- No source file containing `clSetMemObjectDestructorCallback` also contained
  a kernel-enqueue operation in this snapshot.

**SUPPORTED CTS gap:** the inspected snapshot appears not to test destructor
ordering after a command using the object has been enqueued. This is stronger
than inference from one file, but remains `SUPPORTED` until the CTS maintainers
confirm scope and generated/indirect coverage is exhaustively excluded. No CTS
test is proposed for submission before an independently reviewed oracle and
coordinated disclosure decision.

## Scientific contribution

The additional path materially increases the scientific value of B4 because:

1. it was predicted from a reference-lifetime audit and then reproduced with
   a specification-derived oracle;
2. it affects a different API and lifetime transition than SVM free;
3. it survives the official SVM-specific fix exactly as the source model
   predicts; and
4. it identifies a reusable architectural distinction—submission is not a
   valid generic object-lifetime boundary.

The result supports a focused Rusticl lifetime case study and a broader
argument about completion-aware resource retention. It is not yet sufficient
by itself for a multi-implementation OpenCL security study or a journal-scale
generality claim. Bug count alone does not determine publication strength.

## Explicit non-claims

- No physical GPU access was proven to be active at callback time.
- No backing-storage reclamation, use-after-free, reuse, or corruption was
  demonstrated for FUP-B4-2A.
- No cross-context, cross-process, cross-user, tenant, privilege, or
  confidentiality boundary was crossed.
- No exploitability or exploit chain is claimed.
- No behavior is claimed for images, sub-buffers, other devices, or other
  OpenCL implementations.
- T6/T7 do not isolate the causal effect of one patch across the 793-commit
  revision distance. They establish that the behavior exists at both exact
  endpoints.
- No claim is made that the loader's optional-symbol diagnostics or the
  patched-libclc warning are harmless in general; neither prevented these
  runs from reaching their controls and defined result.
- No confidential Mesa issue content is authorized for publication.

## Open questions

| Question | Status | Required evidence |
| --- | --- | --- |
| Are the supplied transcript copies byte-identical to the H1 raw files? | CONFIRMED for stdout/stderr; evidence chain VERIFIED | T9 transfer, manifests, independent hashes, and direct byte comparison |
| Does the callback precede ongoing physical shader memory access? | UNKNOWN | A separately approved, non-destructive physical-execution oracle |
| Can early object deletion lead to backing-memory reuse or corruption? | UNKNOWN | Separate risk review and explicit experimental GO; not implied by T6/T7 |
| Does a minimal completion-retention patch eliminate the violation? | HYPOTHESIS | Source review, isolated proof-of-fix, then controlled differential |
| Is CTS coverage definitively absent? | SUPPORTED gap | Maintainer-level or exhaustive generated-test review |
| Does the class occur in other implementations? | UNKNOWN | Authorized cross-implementation conformance study |

## Status after T9

**The independent evidence check recommended after T8 is complete.** The T6
and T7 raw evidence, runtime references, manifests, and earlier transcript
copies are mutually consistent. The T9 evidence chain is classified
**VERIFIED**, with no discrepancy found.

A minimal proof-of-fix is now technically justified. Its design
question should be narrow: can the strong buffer/image references captured for
a kernel command be moved or duplicated into completion cleanup so that the
OpenCL objects remain alive through fence completion without changing
`clSetKernelArg` retention semantics? Source review must first account for
normal completion, error signaling, queue teardown, images, sub-buffers, and
resource cycles. Any build or GPU validation requires a new explicit GO.

**T9 STOP:** no proof-of-fix, build, GPU run, commit, push, or external report
was performed. Any such continuation requires a new explicit GO.
