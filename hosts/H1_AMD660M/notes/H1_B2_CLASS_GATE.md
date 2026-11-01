# H1 B2 — Workgroup / Dispatch Geometry Class Gate

Date: 2026-08-28
Status: CLOSED on H1

## Results

- C1 local 64 / global 64: PASS F0
- C2 local 256 / global 256: PASS F0
- C3 local 511: PASS F0
- C4 exact kernel boundary 512: PASS F0
- C5 kernel boundary +1 (513): CL_INVALID_WORK_GROUP_SIZE, F0
- C6a non-uniform 65/64: CL_INVALID_WORK_GROUP_SIZE, F0
- C6b work_dim=4 with max dimensions=3:
  CL_INVALID_WORK_DIMENSION, F0
- C6c reqd_work_group_size valid control: PASS F0
- C6c reqd_work_group_size mismatch: CL_INVALID_WORK_GROUP_SIZE, F0
- C7a 2D 16x32 = 512: PASS F0
- C7b 2D 16x33 = 528: CL_INVALID_WORK_GROUP_SIZE, F0

## Main observation

CL_DEVICE_MAX_WORK_GROUP_SIZE = 1024
CL_KERNEL_WORK_GROUP_SIZE = 512

The kernel-specific limit is enforced at clEnqueueNDRangeKernel.

Geometry contracts tested on H1 were contained at the API/runtime
enqueue boundary. No evidence justified representation-differential
C8 or containment C9 escalation.

## Class decision

CLOSE on H1.

Retain representative B2 cases for later cross-implementation testing.

Proceed to B3 Combined Resources.
