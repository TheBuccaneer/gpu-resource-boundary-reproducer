# H1-B1-LDS-01 — Static Local-Memory Boundary

## Test metadata

- Date: 2026-08-26
- Hardware: H1 — AMD Radeon 660M / Rembrandt
- OpenCL implementation: Rusticl
- Mesa: 26.0.3-1ubuntu1
- Kernel: 7.0.0-30-generic
- Backend: radeonsi + ACO
- Bug class: B1 Resource Capacity
- Secondary class: B9 Integer / Arithmetic Backend
- Abstraction level: L0/L2 -> L4
- Kernel complexity: K0
- Method: M5 Constraint-guided boundary testing

## Device contract

CL_DEVICE_LOCAL_MEM_SIZE = 65536 bytes

## Hypothesis

Values at or below the advertised device limit should be accepted.

A resource requirement above the device limit should be rejected in a
controlled manner rather than terminating the client process.

## Results

| LOCAL_INTS | Bytes | clBuildProgram | clCreateKernel | Exit | Result |
|---:|---:|---:|---|---:|---|
| 16383 | 65532 | CL_SUCCESS | CL_SUCCESS | 0 | PASS |
| 16384 | 65536 | CL_SUCCESS | CL_SUCCESS | 0 | PASS |
| 16385 | 65540 | CL_SUCCESS | SIGFPE before return | 136 | INTERESTING |

## Interpretation

The advertised local-memory boundary is exact:

- limit - 4 bytes succeeds
- exact limit succeeds
- limit + 4 bytes terminates the invoking process

The failure occurs during clCreateKernel after successful source-program
creation and build.

This reproduces the previously identified seed bug under the current H1
baseline using Linux kernel 7.0.0-30-generic.

## Classification

- Failure class: F2 — originating client process terminates
- Security status: robustness / availability defect; no cross-client impact established
- Gate: INTERESTING
- Bug-class decision: EXPAND B1

## Raw evidence

results/b1/H1_B1_LDS_boundary_2026-08-26.txt
