# H1 B1 EH5 — Instrumented Mesa 26.2 Runtime Confirmation

Date: 2026-08-28

## Build

Mesa: 26.2.0
Build type: release
b_ndebug: true
Gallium driver: radeonsi
OpenCL frontend: Rusticl

Instrumentation patch SHA256:

7d74100a31a89b08a9532fe718c93cf6c2bf845ae3379f516678b2f3abee686b

Instrumented libRusticlOpenCL:

Build ID:
af712751d070b15e88f9b59dc7851a3d59509bd1

SHA256:
262fb1e449da668a6a97dd88be69529ce178fd5bd27914ba32eb16155c03e409

## 65536-byte control

Advertised per-workgroup local-memory limit:

65536 B

Instrumented runtime state:

variable=65536
static=0
total=65536
lds_blocks=128
limit=65536
rsrc2_before=0x0000008a
rsrc2_after=0x0040008a

Observed:

- clSetKernelArg: CL_SUCCESS
- clEnqueueNDRangeKernel: CL_SUCCESS
- clFinish: CL_SUCCESS
- clEnqueueReadBuffer: CL_SUCCESS
- canaries correct
- process exit: 0

Classification:

F0 / PASS

Control log SHA256:

c41bbf3f9455ac37d72210fd9bb1e8f30bb6984fe6ad823d64d9b010051e8540

## 65537-byte +1 test

Requested dynamic local memory:

65537 B

Last accessed byte:

65536

The kernel does not intentionally access outside the requested
65537-byte local-memory object.

Instrumented runtime state:

variable=65537
static=0
total=65537
lds_blocks=129
limit=65536
rsrc2_before=0x0000008a
rsrc2_after=0x0040808a

Observed:

- clSetKernelArg(local,65537): CL_SUCCESS
- CL_KERNEL_LOCAL_MEM_SIZE: 65537
- clEnqueueNDRangeKernel: CL_SUCCESS
- clFinish: CL_SUCCESS
- OpenCL context subsequently lost
- originating process aborts
- process exit: 134

Kernel:

- ring comp_1.3.1 timeout
- originating dynamic_local_b process identified
- compute-ring reset started
- compute-ring reset succeeded
- device recovered through ring-level recovery

Classification:

B1 Resource Capacity
F2 originating process failure
F5 compute-ring / engine reset

## Device coredump

Size:

6036753 bytes

SHA256:

ff4aa229674ed4dea59f30516c85f0fca22364f105006136ed8967818f882d79

Coredump:

- kernel 7.0.0-30-generic
- process_name dynamic_local_b PID 65947
- timed-out ring comp_1.3.1
- reaches "VRAM lost check is skipped!"
- no IB sections, as expected for the Linux 7.0 AMDGPU coredump format
- page-fault status is zero; no reliable GPU VM fault evidence is claimed

## Conclusion

The source-derived LDS-state prediction is directly confirmed at runtime.

65536 B:
128 LDS units -> valid execution.

65537 B:
129 LDS units -> over the device's 65536-B per-workgroup limit,
yet the state is constructed and submitted.

Mesa 26.2 release therefore empirically reproduces the same B1
failure-containment issue previously observed on the system Mesa
OpenCL 26.0.3 stack.
