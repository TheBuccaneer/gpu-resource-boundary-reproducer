# Seed Bug: OpenCL local-memory overcommit reaches zero-wave ACO state

## Summary

A formally valid OpenCL kernel whose static local-memory requirement exceeds
the device limit by only 4 bytes is accepted through program creation and
build, but causes the host process to terminate during `clCreateKernel`.

The failure is reproducible through both OpenCL C source ingestion and
SPIR-V IL ingestion.

## Device

- GPU: AMD Radeon 660M / Rembrandt
- OpenCL implementation: Rusticl + radeonsi + ACO
- Reported device local memory: 65536 bytes
- Kernel local-memory boundary:
  - 65536 bytes: accepted
  - 65540 bytes: process termination

## Confirmed runtimes

### Mesa 26.0.3

Source path:

- `clCreateProgramWithSource`: success
- `clBuildProgram`: success
- `clCreateKernel`: host-process SIGFPE
- exit status: 136

Symbolized root cause:

- `aco::get_addr_regs_from_waves(..., waves=0)`
- `aco_live_var_analysis.cpp`
- division by zero in register-demand calculation

### Mesa 26.2.0 debugoptimized

Source path:

- `clCreateProgramWithSource`: success
- `clBuildProgram`: success
- `clCreateKernel`: host-process SIGABRT
- failure triggered by assertion in
  `si_calculate_needed_lds_size`
- assertion:
  `lds_size <= lds_size_per_workgroup`

This is a debug assertion and does not represent controlled OpenCL error
handling.

### Mesa 26.2.0 release

Source path, 65536 bytes:

- `clCreateProgramWithSource`: success
- `clBuildProgram`: success
- `clCreateKernel`: success
- `CL_KERNEL_LOCAL_MEM_SIZE = 65536`
- exit status: 0

Source path, 65540 bytes:

- `clCreateProgramWithSource`: success
- `clBuildProgram`: success
- `clCreateKernel`: SIGFPE
- exit status: 136

SPIR-V IL path, 65540 bytes:

- `clCreateProgramWithIL`: success
- `clBuildProgram`: success
- `clCreateKernel`: SIGFPE
- exit status: 136

The SPIR-V module was independently validated with:

`spirv-val --target-env opencl1.2`

and preserves the full 16385-element Workgroup array.

## Root cause

The relevant ACO control flow computes a suitable wave count from
per-workgroup LDS usage.

For an infeasible local-memory request:

`lds_per_workgroup > lds_limit`

integer division yields:

`lds_limit / lds_per_workgroup = 0`

which can reduce the number of workgroups to zero.

This propagates to:

`program->num_waves = 0`

and is later passed to register-demand code that divides physical register
counts by `waves`.

Thus the dangerous backend state is:

`waves == 0`

followed by an unchecked division.

## Representation result

The same failure occurs for:

- OpenCL C source
- validated SPIR-V IL

Therefore this trigger does not support a Source-vs-IL validation asymmetry.
Both ingestion paths converge on the same downstream radeonsi/ACO failure.

## Prior art

Mesa issue #9731 reported a similar zero-wave divide-by-zero in ACO in 2023.
That report was closed after the reporter concluded that the triggering
GLSL-to-SPIR-V compilation was invalid and wrote:

"Issue found to be in glsl->spirv compilation, disregard."

The current reproducer differs in that:

- the OpenCL source is valid,
- the SPIR-V module is independently validated,
- the trigger is tied exactly to the advertised device local-memory limit,
- the failure is reproduced on Mesa 26.0.3 and Mesa 26.2.0 release.

Mesa issue #9058 concerns a different Rusticl/NIR local-memory accounting
problem and was fixed separately.

The dangerous ACO zero-wave control flow predates Mesa MR !16039 and remains
present in Mesa 26.2.0 source. The same unguarded sink was also observed in
current upstream source during the August 2026 source audit.

## Security / claim boundary

This seed bug currently establishes:

- a device-resource feasibility failure,
- missing failure containment,
- host-process termination during kernel creation.

It does NOT currently establish:

- cross-process impact,
- GPU reset,
- system-wide denial of service,
- privilege escalation,
- memory corruption,
- data leakage,
- a CVE-worthy security impact.

The demonstrated failure domain is the invoking client process.

## Research role

This bug should be treated as a motivating seed example for a broader study of
device-resource validation and failure containment in GPU compilation stacks.

The intended broader research direction is to derive kernels from backend
resource constraints and test boundary states such as:

- feasible,
- exactly saturated,
- minimally infeasible,
- zero-occupancy,
- alignment/granularity boundaries,
- register-pressure boundaries,
- interacting resource limits.

The seed bug alone is not considered sufficient for a full paper.

## Frozen artifact hashes

507da62971ea160f27929ad9f33990c4c21e14572269c2c66b3a79c0ffdae7f0  probes/local_limit.cl
2800f860112efdd7759deb4cd55f5ca893d18e7cc542d4bfc75bc203e1d5b926  src/local_limit_probe.c
da3b0c51c2d0cb24f9a7c26f6dc1ca314c44b224c38433094b27e9fe0691ccd5  src/il_local_limit_probe.c
19a715e80dccc7ef694e81e4f12037e6affde384e15a26bc7b091b7bb060a2da  artifacts/local_limit_65540.spv
