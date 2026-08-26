# B5 — Raw Device-Pointer Authority

## Target

Extension:
cl_ext_buffer_device_address

H1 support:
yes, version 1.0.2

## Security question

Which authority does possession of a raw OpenCL device address provide, and
how are buffer boundaries, lifetime, context ownership and device isolation
enforced when that address is dereferenced by a kernel?

## Specification baseline

A buffer device address is obtained from a buffer created with:

CL_MEM_DEVICE_PRIVATE_ADDRESS_EXT = CL_TRUE

The address is queried using:

CL_MEM_DEVICE_ADDRESS_EXT

A device pointer is passed to a global pointer kernel argument using:

clSetKernelArgDevicePointerEXT

Offsets into the buffer are permitted when naturally aligned for the
argument type.

Passing an invalid device-pointer value to
clSetKernelArgDevicePointerEXT is explicitly permitted. Therefore successful
argument-setting is NOT by itself evidence of a validation bug.

The security-relevant operation is dereferencing the address during kernel
execution.

## Test ladder

B5.1  valid base pointer
B5.2  valid in-buffer offset
B5.3  final valid boundary access
B5.4  out-of-buffer address
B5.5  misaligned address
B5.6  pointer after buffer release
B5.7  stale pointer and address reuse
B5.8  multiple buffers / aliasing
B5.9  context/device ownership
B5.10 cross-context / cross-process isolation

## Escalation rule

B5.1-B5.3 establish expected semantics.

B5.4-B5.9 are only interpreted using explicit specification rules and
observed failure containment.

B5.10 is attempted only if earlier results provide a concrete reason to
investigate isolation impact.

## Relevant failure classes

F0 controlled OpenCL/runtime failure
F1 unexpected success/result
F2 originating client crash
F4 second client affected
F5 GPU reset
F7 cross-context corruption/leakage
F8 stronger memory-safety/isolation impact
