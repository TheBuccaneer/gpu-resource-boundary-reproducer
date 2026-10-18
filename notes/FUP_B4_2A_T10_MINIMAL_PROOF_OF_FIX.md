# FUP-B4-2A/T10 — Minimal `cl_mem` Lifetime Proof-of-Fix

Date: 2026-10-17  
Classification: **READY_FOR_BUILD**  
Scope: static source analysis and isolated proof-of-fix only; no build, OpenCL
call, GPU execution, test modification, commit, push, or disclosure

## Provenance and isolation

- Mesa base: `772c8bba74f6b197e88054669ded5861a8a6a293`
- Detached PoF worktree: `/workspace/mesa-fup-b4-2a-pof-772c8bba`
- Patch: `patches/b4/fup_b4_2a_clmem_terminal_retention_pof.patch`
- Patch SHA-256:
  `3476b8284d5c8efd0c95971bc896938917f80cbc9fe76846c1dc2fbbae997a48`
- Existing OpenCL2/B4 worktrees, B8 files, Mesa libraries, system
  configuration, and the T1 reproducer were not modified.

The PoF modifies only these Mesa files:

1. `src/gallium/frontends/rusticl/api/event.rs`
2. `src/gallium/frontends/rusticl/api/kernel.rs`
3. `src/gallium/frontends/rusticl/api/queue.rs`
4. `src/gallium/frontends/rusticl/core/event.rs`
5. `src/gallium/frontends/rusticl/core/kernel.rs`
6. `src/gallium/frontends/rusticl/core/queue.rs`

## Retention path before the PoF

`clSetKernelArg` temporarily obtains a strong `Arc<Buffer>` or `Arc<Image>`
but stores only a `Weak` reference in `KernelArgValue`. This behavior is
unchanged and remains necessary: setting an argument must not itself retain a
memory object indefinitely.

At enqueue, `Kernel::launch_with_args` upgrades the weak arguments into the
`buffer_arcs` and `image_arcs` maps. Those maps are captured by `EventSig`.
The queue consumes `EventSig` during submission, so the maps and their strong
object references are dropped when submission work returns. Fence completion
and the event's terminal transition happen later.

The official B4 `CleanupSig` cannot directly solve this problem. On successful
execution, `Event::signal` performs this sequence:

```text
set CL_RUNNING
run CleanupSig
set CL_COMPLETE
```

Moving the strong memory-object references into `CleanupSig` would therefore
still permit the destructor callback while the event is nonterminal.

## PoF design

The PoF adds a distinct `TerminalReleaseSig` to `EventMutState`.

For a non-empty NDRange launch:

1. the enqueue snapshot upgrades each distinct buffer and image argument;
2. one set of strong references remains in the submission `EventSig`;
3. cloned strong references are captured by `TerminalReleaseSig`;
4. the event owns the terminal-release closure after enqueue;
5. submission consumes and drops only the `EventSig` references;
6. `Event::set_status` installs `CL_COMPLETE` or a negative terminal status;
7. terminal event callbacks run while the strong references are still held;
8. only then is `TerminalReleaseSig` invoked.

The zero-global-size NDRange path also retains the current buffer and image
arguments until its trivial command reaches a terminal state.

This is stronger than waiting only for the GPU fence. The terminal event status
is installed before release. Consequently, if the terminal token owns the last
strong `cl_mem` reference, `MemBase::drop` and the destructor callback cannot
run while the associated command still reports `CL_QUEUED`, `CL_SUBMITTED`, or
`CL_RUNNING`.

The pre-completion `CleanupSig` behavior is unchanged, preserving the B4 SVM
free ordering.

## Error and teardown paths

| Path | PoF behavior |
| --- | --- |
| Successful fence completion | `CL_RUNNING`, existing cleanup, `CL_COMPLETE`, event callbacks, terminal release |
| Submission work returns an error | Negative status is installed, error callbacks run, then terminal release |
| Dependency has failed | Dependent event receives a negative status before terminal release |
| Device reset detected during flush | Events receive `CL_OUT_OF_RESOURCES` before terminal release |
| Fence wait fails | Events receive `CL_OUT_OF_RESOURCES` before terminal release |
| Queue worker channel is unavailable | Queued events receive `CL_OUT_OF_HOST_MEMORY` before terminal release |
| Signal channel is unavailable | Submitted events receive `CL_OUT_OF_RESOURCES` before terminal release |
| Regular queue destruction | Existing `Queue::drop` flushes pending work; the normal terminal paths release retention |
| Failure before event creation | No command exists; function arguments and their temporary strong references are dropped normally |

On a terminal transition, `Event::set_status` also takes and drops an
unconsumed `work` closure. This avoids retaining kernel arguments until the
event handle is destroyed when a dependency or queue failure prevents
submission.

Terminal release happens outside the event-state mutex and after event
callbacks. A destructor callback may therefore query the associated event
without observing a nonterminal status or contending on that state lock.

## Object coverage

- **Buffers:** retained directly through `Arc<Buffer>`.
- **Images:** retained directly through `Arc<Image>`.
- **Sub-buffers:** the argument's sub-buffer `Arc` is retained; its allocation
  already owns the parent `Mem`, so the parent remains reachable as well.
- **Images with parent memory:** retaining the image retains its allocation and
  parent relationship.

Only the ordinary buffer path has dynamic T6/T7 evidence. Image, sub-buffer,
zero-work, and error-path coverage is statically supported by the shared code
path; no successful dynamic correction is claimed for those cases.

The PoF does not change `KernelArgValue` storage at `clSetKernelArg`, and thus
does not convert weak kernel-argument storage into lasting kernel-object
retention.

## Cycles and lifetime leaks

`TerminalReleaseSig` captures only buffer/image `Arc` maps. Memory objects own
their context and, where applicable, parent memory; they do not own the event
or queue. No new event-memory or queue-memory reference cycle was identified.

Retention is deduplicated by the existing object-pointer keys. Each distinct
memory argument gains one additional strong reference from enqueue until the
terminal transition. On every explicitly handled terminal path the closure is
taken exactly once. If event creation or enqueue validation fails before a
command exists, ordinary Rust drop releases it.

An unrecoverable process abort or an internal panic that prevents any terminal
transition remains outside the guarantee. This is not treated as a normal
OpenCL completion/error path and is not a claim that Rusticl is panic-safe.

## Static verification

- Exact detached base commit confirmed.
- `git diff --check`: PASS.
- Reverse application check of the generated patch: PASS.
- All `Event::new` call sites updated for the additional optional state.
- All explicit terminal-status paths in `core/event.rs` and `core/queue.rs`
  reviewed.
- Later public Mesa history was searched; no equivalent terminal-retention fix
  was found in the inspected mirror head.
- No Rust formatter/compiler is installed in the cloud environment. In
  accordance with the T10 boundary, none was installed and no build was run.

## Remaining validation and risks

1. The patch has not been compiled; type checking and project formatting are
   the first required T11 build checks.
2. The PoF has not been run against T1; elimination of the six observed FAIL
   outcomes remains dynamically unverified.
3. The extra `Arc` clones and maps impose a small enqueue-time and in-flight
   memory cost proportional to the number of distinct buffer/image arguments.
4. Sampler and raw SVM argument lifetime are not changed or claimed by this
   PoF.
5. No claim is made about physical allocation reuse, corruption, other OpenCL
   implementations, or untested object types.

## Decision

**READY_FOR_BUILD.** The source design satisfies the T10 acceptance criterion
for the handled Rusticl command lifecycle: strong `cl_mem` references cannot
be released by this path until the event has first entered `CL_COMPLETE` or a
negative terminal state. The design does not misuse the pre-completion
`CleanupSig`, does not alter `clSetKernelArg` retention, and does not introduce
an identified reference cycle.

This classification means ready for an isolated compile and static toolchain
check, not a confirmed fix. Dynamic proof-of-fix status requires a separate GO
after a successful build.

**T10 STOP:** no build, OpenCL call, GPU run, commit, push, or external report
was performed.
