# H1-B1-DYN-03 — Dynamic Local Memory Boundary - 4 B

## Metadata

- Date: 2026-08-26
- Hardware: H1 — AMD Radeon 660M / Rembrandt
- OpenCL implementation: Rusticl
- Mesa: 26.0.3-1ubuntu1
- Linux kernel: 7.0.0-30-generic
- Bug class: B1 Resource Capacity
- Stage: C3 Boundary - delta
- Kernel complexity: K1

## Device contract

CL_DEVICE_LOCAL_MEM_SIZE = 65536 bytes

## Test

Dynamic local-memory allocation:

- requested: 65532 bytes
- distance to advertised limit: -4 bytes
- last valid int index: 16382

The kernel writes and reads canaries at both the first and the
last valid int of the requested allocation.

No out-of-bounds access is performed.

## Expected

The request is below the advertised device limit and should execute
successfully.

CL_KERNEL_LOCAL_MEM_SIZE must account for at least the requested
dynamic local-memory allocation.

## Observed

- clSetKernelArg(local,65532): CL_SUCCESS
- CL_KERNEL_LOCAL_MEM_SIZE: 65532 bytes
- clEnqueueNDRangeKernel: CL_SUCCESS
- clFinish: CL_SUCCESS
- first canary: 0x11111111
- last canary: 0x22222222
- exit: 0

## Classification

- Result: PASS
- Failure class: F0
- requested/reported accounting: 65532 / 65532 bytes
- full requested range exercised successfully
- Decision: continue to C4 exact boundary

## Raw evidence

results/b1/H1_B1_DYN_65532_boundary_minus4_2026-08-26.txt
