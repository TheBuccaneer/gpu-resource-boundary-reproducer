# H1 B1 Resource Capacity — Class Gate

Date: 2026-08-26
Decision: CLOSE for H1 discovery; expand later cross-implementation.

## Primary finding

Device advertised per-workgroup local memory:
65536 B.

Dynamic __local:
- 65536 B: PASS / F0
- 65537 B: compute-ring timeout and GPU reset / F2 + F5

Smallest observed failing delta:
+1 byte.

The byte-boundary kernel accesses only byte N-1 of the N-byte local
allocation requested through clSetKernelArg. No intentional access beyond
the requested local-memory object is performed.

## Root cause

Rusticl accepts and stores the dynamic local-memory size without checking
it against the device's per-workgroup local-memory capacity.

The value is propagated as Gallium pipe_grid_info.variable_shared_mem.

radeonsi's dynamic OpenCL LDS path computes the total shared-memory size
and encodes it in COMPUTE_PGM_RSRC2. For GFX10.3:

65536 B -> 128 encoded 512-byte LDS units
65537 B -> rounded to 66048 B -> 129 units

Mesa defines the per-workgroup LDS capacity as 65536 B on GFX10+.
The dynamic path checks only that the encoded value fits its register
field and does not reject the infeasible per-workgroup allocation before
submission.

## Failure containment

Confirmed:
- F2 originating process failure
- F5 GPU compute-ring reset

Not established:
- F4 other OpenCL client impact
- F6 other GPU application/display impact
- F7 cross-context corruption/leakage
- F8 stronger memory-safety violation

## Version scope

The relevant Rusticl and radeonsi code paths are functionally equivalent
in Mesa 26.0.3 and Mesa 26.2.0.

Empirical F5 reproduction was performed on the system Mesa 26.0.3.
The 26.2.0 scope is currently source-level/white-box, not an empirical
F5 reproduction.

## Evidence hashes

591c11e2bf8a7941ac3e5055449bea559f5f9cb30f3441df7a27675789912c8c
  H1_B1_DYN_BYTE_65536_exact_boundary_2026-08-26.txt

321e17f00cc2f365dd2d80b9d68758fe9e70ed56f7ca5d07d3d09ff81ee9973c
  H1_B1_DYN_BYTE_65537_boundary_plus1_2026-08-26.txt

1e2a545d2777ff2f42c621001a02937df6739703ac70ce2b1ef1143006406260
  H1_B1_DYN_BYTE_65537_kernel_evidence_2026-08-26.txt
