# H1 B2 C0 — Workgroup / Dispatch Capability Inventory

Date: 2026-08-28
Status: COMPLETE

## Device

- Device: AMD Radeon 660M / radeonsi / ACO
- OpenCL: 3.0
- Mesa/Rusticl: 26.0.3-1ubuntu1
- CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS: 3
- CL_DEVICE_MAX_WORK_GROUP_SIZE: 1024
- CL_DEVICE_MAX_WORK_ITEM_SIZES: [1024, 1024, 1024]
- CL_DEVICE_NON_UNIFORM_WORK_GROUP_SUPPORT: false
- CL_DEVICE_PREFERRED_WORK_GROUP_SIZE_MULTIPLE: 64

## Kernel-specific properties

Minimal B2 kernel:

- CL_KERNEL_WORK_GROUP_SIZE: 512
- CL_KERNEL_COMPILE_WORK_GROUP_SIZE: [0, 0, 0]
- CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE: 64
- CL_KERNEL_LOCAL_MEM_SIZE: 0
- CL_KERNEL_PRIVATE_MEM_SIZE: 0

## Important relation

CL_KERNEL_WORK_GROUP_SIZE (512)
<
CL_DEVICE_MAX_WORK_GROUP_SIZE (1024)

This is spec-permitted and is not classified as a bug.

It provides a clean future differential:

local_work_size = 513

- within per-dimension device limit: yes
- within device total WG limit: yes
- above kernel-specific WG limit: yes

Therefore the expected failure for that future case is
CL_INVALID_WORK_GROUP_SIZE.

No claim is made yet about why the kernel-specific limit is 512.

## Non-uniform workgroups

Not supported on H1.

A global size not divisible by an explicitly specified local size is
therefore expected to be rejected according to the applicable OpenCL
work-group rules.

## Gate

C0: PASS

Proceed to C1 valid baseline.
