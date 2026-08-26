# H1 B1 Evidence Hardening EH5 — Instrumentation Status

Date: 2026-08-26
Status: PREPARED, NOT EXECUTED

No additional GPU reset was performed during EH5 preparation.

## Available Mesa 26.2 builds

Source:
- vendor-src/mesa-26.2.0

Debug-oriented build:
- vendor-src/build-mesa-26.2.0
- prefix: vendor-src/mesa-26.2.0-local
- buildtype: debugoptimized
- gallium-drivers: radeonsi
- gallium-rusticl: true
- Rusticl driver: radeonsi

Release-oriented build:
- vendor-src/build-mesa-26.2.0-release
- prefix: vendor-src/mesa-26.2.0-release-local
- buildtype: release
- b_ndebug: true
- gallium-drivers: radeonsi
- gallium-rusticl: true
- Rusticl driver: radeonsi

Release Rusticl binary:

Build ID:
5894838fe09dd17da25c7cf24529c3aafa7d056c

SHA256:
2edcc7d7978bb264297a34836eefee352f9d432c3ba644d7da6a246dd3d6c895

## Planned instrumentation

Target:
si_switch_compute_shader() in radeonsi.

Record immediately before compute resource-state submission:

- shader->info.shared_size
- variable_shared_size
- total shared_size
- lds_blocks
- device lds_size_per_workgroup
- rsrc2 before LDS_SIZE modification
- rsrc2 after LDS_SIZE modification

## Planned experiment order

1. Instrument and rebuild Mesa 26.2 release configuration.
2. Run 65536-B paired control.
3. Verify expected runtime state and successful execution.
4. Only then run one 65537-B case.
5. During the 65537-B run also capture:
   - instrumented stderr;
   - kernel journal;
   - AMDGPU devcoredump if produced.

This single run is intended to test:
- runtime construction of the predicted LDS state;
- empirical Mesa 26.2 behavior;
- +1-byte reproducibility.

No +2/+3/+4, busy-loop, SIZE_MAX, or unrelated stress tests are planned.
