# H1 B1 Dynamic Local Memory — Exact Byte Boundary

Date: 2026-08-26

## Device
- AMD Radeon 660M / Rembrandt / radeonsi
- OpenCL implementation: Rusticl
- CL_DEVICE_LOCAL_MEM_SIZE: 65536 B

## Exact-boundary control

Requested dynamic local memory: 65536 B
Last accessed byte: 65535

Observed:
- clSetKernelArg: CL_SUCCESS
- CL_KERNEL_LOCAL_MEM_SIZE: 65536
- clEnqueueNDRangeKernel: CL_SUCCESS
- clFinish: CL_SUCCESS
- clEnqueueReadBuffer: CL_SUCCESS
- first canary: 0x11
- last canary: 0x22
- process exit: 0

Classification: PASS / F0.

## Boundary + 1 byte

Requested dynamic local memory: 65537 B
Last accessed byte: 65536

The last access is within the 65537-byte object requested through
clSetKernelArg; the experiment does not intentionally access beyond the
requested local-memory object.

Observed:
- clSetKernelArg(local,65537): CL_SUCCESS
- CL_KERNEL_LOCAL_MEM_SIZE: 65537
- clEnqueueNDRangeKernel: CL_SUCCESS
- clFinish: CL_SUCCESS
- userspace subsequently reports lost context / hard recovery
- process exit: 134

Kernel evidence:
- AMDGPU device coredump created
- compute ring comp_1.1.0 timeout
- offending process associated with dynamic_local_b
- compute-ring reset started
- ring reset succeeded
- device recovered through reset

Classification:
- B1 Resource Capacity
- F2 originating process failure
- F5 GPU compute-ring reset
- smallest observed failing delta: +1 B

## White-box correspondence

radeonsi computes dynamic OpenCL LDS allocation in 512-byte encoded units:

65536 B -> 128 units
65537 B -> align(65537, 512) = 66048 B -> 129 units

Mesa defines the per-workgroup LDS limit as 65536 B on GFX10+.
The dynamic OpenCL path checks that the encoded value fits the register
field, but does not reject total per-workgroup LDS exceeding the hardware
capacity before submission.

The +1-byte empirical cliff therefore matches the identified
dispatch-time validation gap.

## Security interpretation

Confirmed availability / failure-containment issue on H1:
an infeasible dynamic-local-memory request accepted through OpenCL reaches
GPU execution and causes a compute-ring timeout and reset.

No cross-context leakage or memory-safety violation is established.
