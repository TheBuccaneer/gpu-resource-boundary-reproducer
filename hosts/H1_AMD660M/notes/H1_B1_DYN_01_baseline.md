# H1-B1-DYN-01 — Dynamic Local-Memory Baseline

## Metadata

- Date: 2026-08-26
- Hardware: H1 — AMD Radeon 660M / Rembrandt
- OpenCL implementation: Rusticl
- Mesa: 26.0.3-1ubuntu1
- Linux kernel: 7.0.0-30-generic
- Bug class: B1 Resource Capacity
- Stage: C1 Valid Baseline
- Kernel complexity: K0

## Test

Dynamic `__local` kernel argument with:

- requested local memory: 4096 bytes
- device local-memory limit: 65536 bytes

## Expected

A value far below the device limit should execute normally.
`CL_KERNEL_LOCAL_MEM_SIZE` must account for the requested dynamic local memory.

## Observed

- clSetKernelArg(local,4096): CL_SUCCESS
- CL_KERNEL_LOCAL_MEM_SIZE: 4096 bytes
- clEnqueueNDRangeKernel: CL_SUCCESS
- clFinish: CL_SUCCESS
- result: 0x12345678
- exit: 0

## Classification

- Result: PASS
- Failure class: F0
- requested/reported accounting: 4096 / 4096 bytes
- Decision: continue to C2 normal-range testing

## Raw evidence

results/b1/H1_B1_DYN_4096_baseline_2026-08-26.txt
