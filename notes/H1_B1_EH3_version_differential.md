# H1 B1 Evidence Hardening EH3 — Mesa Version Differential

Date: 2026-08-26

## Versions

Compared source trees:
- Mesa 26.0.3
- Mesa 26.2.0

## Main B1 capacity path

The dynamic-local-memory capacity path is functionally equivalent
between Mesa 26.0.3 and Mesa 26.2.0.

### Rusticl

Both versions:

1. initialize variable_local_size from static local memory;
2. align the current offset for KernelArgValue::LocalMem;
3. add the requested dynamic local-memory size;
4. subtract static_local_size before dispatch;
5. pass variable_local_size to ctx.launch_grid().

No device-specific per-workgroup capacity validation was observed
between incorporation of the dynamic local-memory requirement and
launch_grid().

### Gallium forwarding

Both versions assign:

pipe_grid_info.variable_shared_mem = variable_local_mem

and forward the value to the driver's launch_grid implementation.

### radeonsi

Both versions compute:

shared_size =
    shader->info.shared_size + variable_shared_size

For GFX7+:

lds_blocks =
    align(shared_size, 512) >> 9

Both versions then contain:

assert(lds_blocks <= 0xFF)

and encode the value into COMPUTE_PGM_RSRC2.LDS_SIZE.

The examined dynamic OpenCL path does not compare this total against
the device's lds_size_per_workgroup limit.

For the H1 boundary:

65536 B -> 128 encoded 512-byte units
65537 B -> 66048 B after alignment -> 129 units

Thus the code mechanism relevant to the B1 +1-byte finding is present
in both examined versions.

## Scope

Empirical F5 reproduction is currently established on the installed
mesa-opencl-icd 26.0.3-1ubuntu1 stack.

Mesa 26.2.0 is currently supported by source-level/white-box evidence,
not yet by an empirical F5 reproduction.

## Separate version difference

The clSetKernelArg MemLocal zero-size check differs:

Mesa 26.0.3 rejects arg_size == 0 with CL_INVALID_ARG_SIZE.

In the examined Mesa 26.2.0 source, KernelArgType::MemLocal has an
empty validation branch.

This is recorded as a separate B14/C6 regression candidate and is not
part of the current B1 capacity finding.

## Evidence

Artifacts:
- artifacts/eh3/H1_B1_Mesa_26.0.3_rootcause_extract.txt
- artifacts/eh3/H1_B1_Mesa_26.2.0_rootcause_extract.txt
- artifacts/eh3/H1_B1_Mesa_26.0.3_vs_26.2.0_rootcause.diff
