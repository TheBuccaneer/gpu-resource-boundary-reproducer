# FUP-B4-2A / T1 — `cl_mem` lifetime reproducer

Status: source implemented; intentionally not built or executed.

This standalone C11/OpenCL 1.2 probe tests only the ordering question from
FUP-B4-2A. It does not attempt memory reuse, corruption, GPU reset, or
cross-context behavior. It is isolated in the dedicated
`fup-b4-2a-clmem-lifetime` worktree and remains unbuilt.

## Controls and result semantics

- C0 enqueues and completes the bounded kernel before releasing the final
  explicit `cl_mem` reference. The destructor callback must then run.
- C1 enqueues the kernel, releases the final explicit `cl_mem` reference, and
  flushes the queue. The host observes the callback through a C11 atomic and
  only then queries the still-retained kernel event.
- C2 releases a buffer that has no queued user and checks ordinary callback
  delivery.

The destructor and `CL_COMPLETE` event callbacks perform only C11 atomic
operations. They make no OpenCL calls, perform no allocation, and emit no I/O.
The event remains explicitly retained by the host throughout C1.

Result meanings are deliberately conservative:

- `FAIL` (exit 10): after an acquire observation that the destructor callback
  started, `clGetEventInfo` successfully returned `CL_QUEUED`, `CL_SUBMITTED`,
  or `CL_RUNNING`. This is the positive lifetime-violation oracle.
- `PASS` (exit 0): C0 and C2 passed, and every C1 trial observed
  `CL_COMPLETE` while the destructor callback had not started on either side
  of the status query. This means no violation was observed; it is not a proof
  that no narrow race exists.
- `INCONCLUSIVE` (exit 20): the callback was first observed but the subsequent
  event query already returned a terminal state, a timeout/error occurred, a
  control failed, or `RUSTICL_DEBUG=sync` was active.
- Exit 64 denotes invalid invocation.

An event status greater than `CL_COMPLETE` establishes that the OpenCL command
had not completed in the API execution model. It does not independently prove
that a physical shader memory access was occurring at that instant.

## Mandatory H1 provenance gate

`h1_provenance_gate.sh` T1.1 separates three questions that must not be
conflated:

1. Is the selected Rusticl *candidate file* the expected binary?
2. Do the ICD configuration and loader environment statically point toward
   that candidate without an unexplained contradiction?
3. Was that exact file actually loaded by the OpenCL process?

The first two questions can be checked without OpenCL execution. The third
cannot be established from a file path or `.icd` file alone. Consequently the
static gate writes a manifest but deliberately returns exit 20 with
`INCONCLUSIVE_RUNTIME_BINDING_PENDING`. A future separately authorized step
must supply process-derived loader evidence before runtime provenance can be
classified as confirmed.

Before any later build, run the static gate on H1 with explicit values:

```sh
RUSTICL_LIBRARY=/absolute/path/to/libRusticlOpenCL.so.1 \
RUSTICL_ICD_FILE=/absolute/path/to/rusticl.icd \
EXPECTED_RUSTICL_SHA256=<independently-recorded-sha256> \
MESA_SOURCE=/absolute/path/to/exact/mesa-checkout \
EXPECTED_MESA_COMMIT=<exact-40-hex-commit> \
B7_STATE=absent \
RUNTIME_LABEL=mesa-baseline-b7-absent \
PROVENANCE_OUT=/absolute/new/path/prebuild-provenance.txt \
./h1_provenance_gate.sh
```

`PROVENANCE_OUT` must be outside the Mesa checkout and must not already exist.
`MESA_SOURCE` may be a normal checkout or a linked Git worktree. Use
`B7_STATE=present` as an expected value for a source tree containing the local
B7 ownership patch; the value is not accepted as evidence by itself.

The gate verifies and records:

1. the Reproducer against the fixed approved T1 SHA-256
   `bdf2948b6329b302130f2912983e8054b7e1ae34af0b53756b6fd4968bfb985a`;
2. candidate-library and ICD-file paths and SHA-256 values;
3. whether the ICD uses an absolute path or an ambiguous dynamic-loader
   basename, plus the presence and hashes of `OCL_ICD_VENDORS`,
   `LD_LIBRARY_PATH`, and `LD_PRELOAD` settings;
4. Mesa worktree root, per-worktree Git directory, common Git directory, exact
   HEAD, branch or detached state, staged/unstaged diff hashes, and the full
   porcelain status including untracked files;
5. the B7 state derived from forward and reverse application checks using the
   independently pinned patch SHA-256
   `418b0c8870544f4de777b10ec7938d80437789dc9beb3ee8dadc42065b5a32a5`,
   not from `B7_STATE` alone;
6. whether local changes are clean or byte-identical to the approved B7 patch;
7. that `RUSTICL_DEBUG=sync` is not active.

Any untracked file, unexplained tracked change, B7 mismatch, hash mismatch, or
ICD contradiction remains `INCONCLUSIVE`. For such Git deviations the manifest
still preserves the exact status evidence before the gate exits.

For reference, the existing OpenCL2 evidence records these two local
Mesa-26.2 build hashes:

- unpatched B7 baseline: `262fb1e449da668a6a97dd88be69529ce178fd5bd27914ba32eb16155c03e409`
- B7 proof-of-fix: `8edfdb04646bfb8bbe9e0efddb02a375b988b6566500931a5c40f8522a9ed423`

Those hashes must not be reused for a new B4-fixed build. A new binary needs
its own independently recorded expected library hash and label.

## Build and run (not authorized in this task)

After a separate build GO, compute the source hash and embed it in the binary:

```sh
source_sha256=$(sha256sum fup_b4_2a_cl_mem_lifetime.c | awk '{print $1}')
cc -std=c11 -O2 -Wall -Wextra -Wpedantic \
  -DFUP_SOURCE_SHA256="\"$source_sha256\"" \
  fup_b4_2a_cl_mem_lifetime.c -lOpenCL -o fup_b4_2a_cl_mem_lifetime
```

Before execution, rerun the static provenance gate with
`FUP_BINARY=/absolute/path/to/fup_b4_2a_cl_mem_lifetime` and a fresh
`PROVENANCE_OUT`. This records both source and binary SHA-256, but it still
does not prove which Rusticl file an OpenCL process loads. Process-derived
loader evidence and GPU execution both require later, separate authorization.
After those gates, use a unique runtime label matching the manifest:

```sh
./fup_b4_2a_cl_mem_lifetime \
  --runtime-label mesa-baseline-b7-absent \
  --iterations 4000000 --trials 3 --timeout-ms 15000
```

The defaults select a platform containing `rusticl` and a GPU device
containing `AMD Radeon 660M`. Selection can be overridden explicitly with
`--platform-substr` and `--device-substr`.

The controlled comparison should use separate manifests for the experimental
baseline `3821bf78426f5515d26047e49164beedb8ecd18d` and the B4-fixed revision
`772c8bba74f6b197e88054669ded5861a8a6a293`. The historical `mesa-26.2.0` tag
is not a substitute for either exact revision.
