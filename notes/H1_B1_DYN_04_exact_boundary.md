# H1-B1-DYN-04 — Dynamic Local Memory Exact Boundary

## Metadata

- Date: 2026-08-26
- Hardware: H1 — AMD Radeon 660M / Rembrandt
- OpenCL implementation: Rusticl
- Mesa: 26.0.3-1ubuntu1
- Linux kernel: 7.0.0-30-generic
- Bug class: B1 Resource Capacity
- Stage: C4 Exact Boundary
- Kernel complexity: K1

## Device contract

CL_DEVICE_LOCAL_MEM_SIZE = 65536 bytes

## Test

Dynamic local-memory allocation exactly equal to the advertised
device local-memory capacity.

- requested: 65536 bytes
- reported: 65536 bytes
- number of int elements: 16384
- last valid int index: 16383

The kernel writes and reads canaries at both the first and the last
valid int of the requested allocation.

No out-of-bounds access is performed.

## Expected

The advertised device limit should remain usable at the exact boundary.

## Observed

- clSetKernelArg(local,65536): CL_SUCCESS
- CL_KERNEL_LOCAL_MEM_SIZE: 65536 bytes
- clEnqueueNDRangeKernel: CL_SUCCESS
- clFinish: CL_SUCCESS
- first canary: 0x11111111
- last canary: 0x22222222
- exit: 0

## Classification

- Result: PASS
- Failure class: F0
- requested/reported accounting: 65536 / 65536 bytes
- entire requested range exercised successfully
- advertised local-memory limit is usable at the exact boundary
- Decision: continue to C5 Boundary + delta

## Comparison with static-local path

At the same H1 device limit:

- static local, 65532 B: PASS
- static local, 65536 B: PASS
- dynamic local, 65532 B: PASS
- dynamic local, 65536 B: PASS

The first differential point to test next is the smallest controlled
request above the advertised limit.

## Raw evidence

results/b1/H1_B1_DYN_65536_exact_boundary_2026-08-26.txt
