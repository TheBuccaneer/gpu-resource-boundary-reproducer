# H1-B1-DYN-02 — Dynamic Local-Memory Normal Range

## Metadata

- Date: 2026-08-26
- Hardware: H1 — AMD Radeon 660M / Rembrandt
- OpenCL implementation: Rusticl
- Mesa: 26.0.3-1ubuntu1
- Linux kernel: 7.0.0-30-generic
- Bug class: B1 Resource Capacity
- Stage: C2 Normal Range
- Kernel complexity: K0

## Test

Dynamic `__local` kernel argument with:

- requested local memory: 32768 bytes
- device local-memory limit: 65536 bytes
- utilization: 50 %

## Expected

A value clearly below the advertised device limit should execute successfully.
`CL_KERNEL_LOCAL_MEM_SIZE` must account for at least the requested amount.

## Observed

- clSetKernelArg(local,32768): CL_SUCCESS
- CL_KERNEL_LOCAL_MEM_SIZE: 32768 bytes
- clEnqueueNDRangeKernel: CL_SUCCESS
- clFinish: CL_SUCCESS
- result: 0x12345678
- exit: 0

## Classification

- Result: PASS
- Failure class: F0
- requested/reported accounting: 32768 / 32768 bytes
- Decision: continue to C3 Boundary - delta

## Raw evidence

results/b1/H1_B1_DYN_32768_normal_2026-08-26.txt
