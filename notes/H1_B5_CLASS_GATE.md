# H1 B5 Class Gate — Raw Device-Pointer Authority

Date: 2026-08-28

## Outcome

**CLOSE — confirmed security-relevant cross-OpenCL-context disclosure behavior**

Primary empirical classification:

**F7 — cross-context data disclosure within one process under a foreign
raw-device-pointer dereference.**

Important qualification:

The foreign-pointer dereference itself is outside defined
`cl_ext_buffer_device_address` usage. Therefore this result must not by
itself be described as an OpenCL conformance violation.

No cross-process disclosure, privilege escalation, sandbox escape,
arbitrary-address disclosure, or cross-context write has been demonstrated.

---

## Platform

Device:

- AMD Radeon 660M / Rembrandt
- radeonsi
- Rusticl
- ACO
- OpenCL 3.0
- `cl_ext_buffer_device_address` supported
- 64-bit device addresses

Implementations tested:

1. System Mesa/Rusticl 26.0.3
2. Local Mesa/Rusticl 26.2 release build

Mesa 26.2 provenance:

- libRusticlOpenCL.so.1.0.0
  `262fb1e449da668a6a97dd88be69529ce178fd5bd27914ba32eb16155c03e409`
- libdrm.so.2.133.0
  `12aab45c6d1e23a3e1b7988b50c31220e77836fa4d5ad1d7998df3a4cd11c790`
- libdrm_amdgpu.so.1.133.0
  `3db52cf8007d56c2e51c23c189f7e69aaba66b68faa6e0701c6f83cec317d79d`

The local Mesa 26.2 build emitted a warning about the patched Mesa libclc
not being detected. The same BDA behavior is independently reproduced on
the system Mesa 26.0.3 stack, and the investigated BDA path is a
runtime/resource-tracking path rather than a libclc operation. The warning
is retained as provenance but is not currently considered explanatory.

---

# Test sequence

## B5-C0 — Capability inventory

PASS / F0.

Confirmed:

- `cl_ext_buffer_device_address`
- `clSetKernelArgDevicePointerEXT`
- device-private-address buffer creation
- non-zero stable device address within object lifetime

Source:
`e7692adb3794b7b222b1448ff3b6490427b2524f110d5639a749a2bbbd85b309`

Binary:
`c272bacdd4e9d4b4dc735fa879fbe0a2ee90ba56a721c8b523cf4ea821e8dcce`

Result:
`80fa5f6fb0235987e26d234310484df9fab319378e4bf7b84d8dc91dffeb39ea`

---

## B5-C1 — Valid base pointer

PASS / F0.

Kernel successfully read and wrote through the buffer's base device
address.

Expected and observed:

- read `0x11223344`
- write `0x55667788`

Source:
`8aecae3741e6d92440d991738dc85f2f545f6af6955a1d5846b69565b994c694`

Binary:
`bc1b3cbd8b2c8220eefdb05707f88cd118cd2a2b749f359f837bc12fa709abfd`

Result:
`a3aa9705ea138d8c1ba72e1a3039bb6716bf8680441047ac6a1bb69afbfed41e`

---

## B5-C2 — Valid aligned interior pointer

PASS / F0.

Pointer:

`base + 16`

The target word was read and modified correctly while the base sentinel
remained unchanged.

Source:
`614d5f5985d714f45e40d3790883cc6cdde7c66a63d218c63f5d5bb6d0a6e8f4`

Binary:
`21978105fd6358928fb2d2b205df6a0dd1ffa18dbc07754adb4403c46fcbc377`

Result:
`e2daab006a43549653c05c394ba93d45e1ca9f8708b0bf16a34e7b9b6da1e42b`

---

## B5-C3 — Last valid uint

PASS / F0.

4096-byte buffer:

`base + 4092`

was the last naturally aligned valid `uint`.

Observed:

- expected read `0x22334455`
- observed read `0x22334455`
- expected write `0x66778899`
- observed write `0x66778899`
- base sentinel unchanged

Source:
`381961dd37321e64cafcf0fb60b7b4ce1a69e7eb110ab14fb2d3d5517461810b`

Binary:
`0f2cff0ff55751618e5747eca1363fefed048584ebe2e0062ac9c18b5bcde139`

Result:
`5dcce45944e58d110a3a7d85f8803e0e8584993fafe623d3b8cd4db71ec87028`

---

## B5-C4 — Exact end pointer without dereference

PASS / F0.

Pointer:

`base + 4096`

The one-past-end value was passed to the kernel but deliberately not
dereferenced.

Observed:

- `clSetKernelArgDevicePointerEXT = CL_SUCCESS`
- enqueue success
- completion success
- output marker `0x12345678`

This establishes the required distinction between accepting an invalid
pointer value and dereferencing it.

Source:
`8049a73fd5f611be91b856f52a2721465fd36601cbe6ef4a9a9f13ba519f225c`

Binary:
`dc6a90f32ca8afdd8928cfcafa417d6c1c63a04ee3f9f90b7e4935e38c4c8db3`

Result:
`3f157cc066fc42720f89dd1ef03b18868538545b53dacb388b4b642bd6355620`

---

## B5-C5 — Exact-end dereference

Security/containment test, not a conformance oracle.

Pointer:

`base + 4096`

was dereferenced as a `uint`.

Observed:

- setter success
- enqueue success
- event `CL_COMPLETE`
- read returned `0x00000000`
- no process failure
- no GPU reset

Initial result:

`e9c56e5f5e77968ff19ebe6ec10009c3431f38e6afed70f01cea1a601e91eead`

Five independent reproductions were byte-identical:

`cc823305710439c5308680564bda933a0a922a642373b4d9a65010ecadfa49ce`

This established a stable accessible exact-end address but did not alone
establish disclosure.

---

## B5-C5a — Device-private allocation layout

Two 4096-byte device-private buffers A and B were allocated.

Observed:

- `A_base = 0xffff800100600000`
- `A_end  = 0xffff800100601000`
- `B_base = 0xffff800100601000`
- `B_end  = 0xffff800100602000`

Therefore:

`A_end == B_base`

Result:
`068de4859c82d77909a4c266187ef7e3d2a65ac1dea837e6c5dd044cb06a87d9`

---

## B5-C5b — Controlled neighbor-buffer read

A marker was stored in B:

`B[0] = 0xcafebabe`

The kernel received the numerical pointer:

`A_base + 4096`

which had been independently established to equal `B_base`.

Observed:

- derived pointer equals B base
- derived-pointer read = `0xcafebabe`
- direct B read = `0xcafebabe`

Conclusion:

Raw device addresses behave without C-style object provenance. Once the
numerical address is the valid address of B, the kernel accesses B.

This result is expected low-level raw-address behavior and is not itself a
security finding.

Source:
`5aa01744d66c242cd1f5a07c4a7736fc843a04c4a6aeaf5e8d60791cba446bb3`

Binary:
`bb9786669c59201a4b46fb17881de922c81d98ea6bafc31173a8c931e9b81ca2`

Result:
`cb70e3896ed69dcc0b71c7191654f30335803b58b3ae1038b810cd67cf7aa951`

---

## B5-C6a — Cross-context address layout

Two distinct OpenCL contexts were created on the same device.

Alternating allocations produced:

- ctxA A0 `...600000`
- ctxB B0 `...601000`
- ctxA A1 `...602000`
- ctxB B1 `...603000`
- ctxA A2 `...604000`
- ctxB B2 `...605000`
- ctxA A3 `...606000`
- ctxB B3 `...607000`

The corresponding addresses were not equal.

This demonstrated process-wide sequential numerical VA assignment in this
experiment rather than identical per-context address sequences.

Result:
`a8fd34dee3f6e58059ff88cb5643cab63a4b8001346ebc42a6b883373a5593fb`

---

# B5-C6b — Foreign raw device pointer across OpenCL contexts

## Setup

Two genuinely distinct OpenCL contexts were created:

- context A: attacker/reader kernel
- context B: target device-private buffer

Only context B created the target BDA allocation.

Target B contained four markers:

- `0xc0dec0de`
- `0x13579bdf`
- `0x2468ace0`
- `0xfeedface`

Its device address was queried from context B.

The numerical address was then supplied using
`clSetKernelArgDevicePointerEXT` to a kernel belonging exclusively to
context A.

Context A had no device-private buffer at that address.

The context-A kernel dereferenced the foreign raw device address and copied
four words into a normal context-A output buffer.

## Observed behavior

In every confirmed run:

- `contexts_distinct = 1`
- foreign pointer setter returned `CL_SUCCESS`
- kernel enqueue returned `CL_SUCCESS`
- kernel wait returned `CL_SUCCESS`
- event status was `CL_COMPLETE`

Context A recovered:

- `0xc0dec0de`
- `0x13579bdf`
- `0x2468ace0`
- `0xfeedface`

A direct context-B read independently returned the same four values.

## Mesa 26.0.3

Initial source:

`e709840ec89c88b94de34052ba0ca394206e1ef2248a858718f670723d2fb610`

Binary:

`30966a99197e25751116c13e614d52c684e00630dceb4b5ce6b6f0e112520406`

Initial result:

`47a9c57b3f9ec3ae540c60250b784b347d965fcb66fd5d8bca0413d4509710e3`

Independent-process reproduction:

**5 / 5 confirmed**

All five reproduction logs were byte-identical:

`3c633ec17f7e507803680f4d8360c306a3cfaeb5b427105da7b7e362112a74ab`

## Mesa 26.2

Initial result:

`411f30de0d5176763b3efc37cab888c39bff029a6be5fb1c433f0ab3e3fb0e58`

Independent-process reproduction:

**5 / 5 confirmed**

All five reproduction logs were byte-identical:

`9bd96c5b6f0ac88dd2337924708cbb7d9ce5d89f05ebf6968dc61bdf082ec684`

## Cross-version summary

- Mesa 26.0.3: 5/5
- Mesa 26.2: 5/5

Total evidence-hardened reproduction:

**10 / 10**

---

# Source-level mechanism

Rusticl stores BDA tracking state in the OpenCL `Context`.

`find_bda_alloc()` uses:

`self.bda_ptrs`

and resolves the supplied numerical address only against that context's BDA
tracking table.

For a BDA kernel argument, Rusticl nevertheless places the raw address in
the kernel argument:

`exec_builder.add_pointer(*address)`

The address is also added to the temporary BDA list.

Before launch Rusticl resolves BDA resources using approximately:

`filter_map(|address| cl_ctx.find_bda_alloc(ctx.dev, address))`

The source explicitly comments that invalid pointers may legally be passed
but not dereferenced.

A foreign address not known to the current OpenCL context is consequently
not converted into a retained/bound Rusticl Buffer resource.

However, the raw numerical pointer remains present in the shader argument.

The resulting resource list is passed to `launch_grid`.

Empirically, the lower GPU memory stack still permits the context-A shader
to access the live allocation belonging to context B.

## Cross-version source differential

The relevant mechanism is semantically unchanged between Mesa 26.0.3 and
Mesa 26.2:

1. BDA raw value added to the kernel input.
2. Context-local `find_bda_alloc()` used for resource resolution.
3. Unknown BDA dropped by `filter_map`.
4. Resolved resources passed separately to launch.

The observed source differences are primarily shifted line numbers and
unrelated Buffer-argument handling changes.

EH2 artifacts:

Mesa 26.0.3:
`8019084f4cc2c75c8d9bb922d50225e9c95f5cb887ba30f178b9fe89b6c94e8f`

Mesa 26.2:
`47cc058677cf27e3c9fed6b0f1104eacc9c6204654108e711eefe0a435678244`

---

# Security interpretation

## Demonstrated

A raw device pointer obtained from a live device-private buffer in one
OpenCL context can be dereferenced by a kernel executing through another
distinct OpenCL context in the same process.

The foreign context recovers controlled target data exactly.

No OpenCL API error, event failure, process termination, or GPU reset
occurs.

This constitutes an empirical failure of context-level data containment
for this raw-pointer misuse scenario.

## Classification

Primary:

**F7 — cross-context data disclosure**

More precise wording:

**Cross-OpenCL-context data disclosure within one process under a foreign
raw-device-pointer dereference.**

## Not demonstrated

The experiments do NOT establish:

- cross-process memory disclosure
- cross-user disclosure
- privilege escalation
- sandbox escape
- arbitrary physical-memory access
- arbitrary GPU memory scanning
- cross-context write
- persistence after buffer destruction
- access after process separation
- a specification-conforming program triggering the disclosure
- an OpenCL conformance violation

The foreign raw-device-pointer dereference is deliberately outside defined
extension usage.

Therefore vulnerability/exploitability claims must remain narrower than
the empirical containment result.

---

# Root-cause model

Observed enforcement split:

1. API accepts the raw numerical pointer.
2. Kernel argument construction preserves the numerical pointer.
3. Rusticl resource tracking is context-local.
4. A foreign address is not found in the current context's BDA allocation
   table.
5. It is therefore omitted from Rusticl's retained BDA-resource set.
6. The raw address nevertheless reaches the shader.
7. The lower GPU/driver memory environment permits access to the live
   allocation belonging to the other OpenCL context.

This exposes a mismatch between:

- context-local runtime resource authority/accounting

and

- effective raw-address accessibility at execution time.

---

# Paper relevance

B5 strongly supports the paper-wide hypothesis that security-relevant
contracts are enforced at different layers and that enforcement metadata
may diverge from the authority actually exercised by the GPU.

Particularly relevant axes:

## Enforcement stage

Runtime tracks BDA resources context-locally, but effective access is
determined later by the raw GPU address.

## Knowledge time

Rusticl knows at dispatch construction time whether a BDA resolves to a
tracked allocation in the current OpenCL context.

## Containment radius

The misuse crosses an OpenCL context boundary while remaining within the
same host process.

## Systematic root cause

Resource tracking and raw-pointer execution semantics are separated:
failure to resolve the pointer as an authorized/tracked context resource
does not prevent its numerical value from reaching device execution.

---

# Gate

**B5: CLOSE / CONFIRMED SECURITY-RELEVANT FINDING**

No further B5 escalation is required.

In particular:

- no cross-context write test
- no blind GPUVA scanning
- no arbitrary-address probing
- no cross-process escalation as part of B5

Next structured class:

**B7 — Object / Context Ownership**

B7 should focus on specification-defined ownership/context validation using
ordinary OpenCL object handles and APIs, providing a useful contrast to the
raw-address behavior observed in B5.
