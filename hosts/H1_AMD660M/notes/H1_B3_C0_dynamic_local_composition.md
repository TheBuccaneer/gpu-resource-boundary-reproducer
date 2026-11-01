# H1 B3 C0 — Dynamic Local-Memory Composition

Date: 2026-08-28
Status: PASS / F0

Device local memory: 65536 B
Device max work-group size: 1024

Kernel before dynamic local args:
- CL_KERNEL_LOCAL_MEM_SIZE = 0
- CL_KERNEL_WORK_GROUP_SIZE = 512

After A = 4096 B:
- CL_KERNEL_LOCAL_MEM_SIZE = 4096
- CL_KERNEL_WORK_GROUP_SIZE = 512

After B = 8192 B:
- CL_KERNEL_LOCAL_MEM_SIZE = 12288
- CL_KERNEL_WORK_GROUP_SIZE = 512

Observation:
For this configuration, the queried local-memory requirement equals
the raw sum of the two dynamic local-memory argument sizes.

This equality is not generalized as a universal oracle: implementations
may include alignment or other implementation-defined accounting.
For later tests, values >= the requested semantic requirement may be valid.

No dispatch occurred in C0.

Gate: proceed to B3-C1 valid multi-local dispatch.
