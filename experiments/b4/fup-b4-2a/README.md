# FUP-B4-2A integrated research record

Status: **CLOSED** (2026-10-18)

This directory indexes the immutable experimental evidence for the ordinary
`cl_mem` lifetime follow-up to B4. Sources, patches, scripts, and reports stay
in the established OpenCL2 project structure; the two archives here preserve
the complete transferred H1 evidence without duplicating their expanded binary
contents in the working tree.

## Canonical project files

| Purpose | Project-relative path |
| --- | --- |
| Original T1 reproducer and provenance gate | `src/fup-b4-2a-t1/` |
| Hardened completion-entry oracle | `src/fup-b4-2a-t12-2/` |
| Offline completion-order evaluator | `scripts/fup_b4_2a_completion_order_oracle.py` |
| PoF patch | `patches/b4/fup_b4_2a_clmem_terminal_retention_pof.patch` |
| T8 evidence consolidation | `notes/FUP_B4_2A_T8_EVIDENCE_CONSOLIDATION.md` |
| T10 PoF analysis | `notes/FUP_B4_2A_T10_MINIMAL_PROOF_OF_FIX.md` |
| T13 final closure | `notes/FUP_B4_2A_FINAL_CLOSURE.md` |
| Integrated artifact manifest | `experiments/b4/fup-b4-2a/INTEGRATION_SHA256SUMS` |

All paths in this document are relative to the OpenCL2 repository root unless
explicitly described as historical H1 paths.

## Evidence archives

### T9

`archives/fup-b4-2a-t9-evidence.tar.xz`

SHA-256:
`9056c3dfae0804d25b25aa81b0b4495f679be5fa9fac3452c01932ae5ac6bea4`

This archive contains the original T6 and T7 stdout, stderr, exit-code,
loader, kernel-journal, and provenance files. It also contains the exact T1
source/binary, the baseline and fixed Rusticl libraries and ICD files, the ICD
loader, and the local libdrm libraries used for the evidence chain. Its internal
`COPIES.sha256`, `ORIGINALS.sha256`, `MANIFEST.tsv`, and `METADATA.sha256`
were verified during T9 and again during T13.

### T13

`archives/fup-b4-2a-t13-evidence.tar.xz`

SHA-256:
`3fd8b58887059c5945103f3011d3d098c1b08b8108d4ceab085852f20621eac9`

This archive contains the T10 patch, T11 build and isolation provenance, T12
raw run, T12.1 evaluator outputs, T12.2 source/build/self-test evidence, and
the complete T12.3 preflight and final raw evidence. Its 56 original evidence
files and nested manifests were independently verified during T13.

Keep the archives compressed. Expanded copies are useful for inspection but
are not additional canonical evidence.

## Exact research revisions

- OpenCL2 integration branch base: `e61234fa5af40480fd663e136b9cb3efbd75099a`
- Mesa experimental baseline: `3821bf78426f5515d26047e49164beedb8ecd18d`
- Mesa official B4 SVM-fix comparison and PoF base:
  `772c8bba74f6b197e88054669ded5861a8a6a293`
- PoF patch SHA-256:
  `3476b8284d5c8efd0c95971bc896938917f80cbc9fe76846c1dc2fbbae997a48`
- PoF patch ID: `d8f1eca02cedad5772b2ad3b3fd4b76c896050c0`

The final scientific classification and its limits are authoritative in
`notes/FUP_B4_2A_FINAL_CLOSURE.md`.

## Historical H1 paths and relocatability

The H1 experiments used `/home/rock/projects/fup-b4-2a-h1` and Mesa source
worktrees below `/home/rock/projects/OpenCL2/vendor-src/`. Meson and Ninja
metadata, loader logs, provenance records, and some manifests intentionally
retain these absolute paths. Moving the build directories does not make them
relocatable.

For later reproduction:

1. treat the recorded source revisions, patch, build options, dependency
   hashes, and raw logs as the portable record;
2. create a fresh Mesa worktree from the owning Mesa Git repository at
   `772c8bba`, then apply the verified patch;
3. configure a new isolated build/install prefix rather than reusing moved
   Meson/Ninja directories;
4. select ICD and Rusticl libraries per process; never modify the system ICD;
5. coordinate exclusive research use of the shared H1 GPU with B8.

## Confidentiality and integration state

The archives and reports are research evidence, not public disclosure
material. Review them before any publication and do not copy confidential Mesa
issue content into public files.

This integration is intentionally uncommitted. No B8 file, system library,
system ICD, Mesa installation, or existing branch was modified to create it.
Original source evidence must not be deleted until a destination copy and an
independent backup both pass their manifests.
