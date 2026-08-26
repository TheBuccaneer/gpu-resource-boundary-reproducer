# H1 B1 Evidence Hardening EH1 — Build Provenance and Assertions

Date: 2026-08-26

## Empirically tested OpenCL stack

Package:
- mesa-opencl-icd:amd64 26.0.3-1ubuntu1

Rusticl binary:
- /usr/lib/x86_64-linux-gnu/libRusticlOpenCL.so.1.0.0
- Build ID: c91ab80de48efc31006854b340c98704c0049da7
- SHA256: 17c9c4ad5aef76d8da15eb8edb283d8cf908c288a01778a246aa65246cd7cae9

The binary contains Mesa 26.0.3 build/version strings.

## Distinction from system DRI Mesa

At test time the system also contained:

- libgl1-mesa-dri:amd64 26.0.8-1ubuntu0.3
- mesa-libgallium:amd64 26.0.8-1ubuntu0.3

However:

- mesa-opencl-icd does not depend on mesa-libgallium;
- libRusticlOpenCL.so does not list libgallium as DT_NEEDED;
- LD_DEBUG tracing during Rusticl device enumeration did not show
  libgallium-26.0.8 being loaded.

The empirical B1 OpenCL result is therefore attributed to the
mesa-opencl-icd 26.0.3-1ubuntu1 Rusticl/radeonsi stack, not to the
separately installed DRI/Gallium 26.0.8 package.

## C assert evidence

In the production Rusticl binary:

- the exact string "lds_blocks <= 0xFF" was not found;
- no __assert_fail string was found;
- no __assert_fail dynamic reference was found.

This is strong binary-level evidence that classical C assertions were
disabled in the production OpenCL build.

This does not affect the B1 +1-byte root cause:

65537 B -> 129 encoded LDS units

and:

129 <= 255

Therefore even an active:

assert(lds_blocks <= 0xFF)

would not reject this request.
