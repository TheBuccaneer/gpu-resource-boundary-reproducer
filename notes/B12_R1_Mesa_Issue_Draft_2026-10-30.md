# Draft Mesa issue — Rusticl returns an empty export-handle-type list for an exportable semaphore

**Status:** Draft only; not filed or sent. **Suggested Mesa label:** `Rusticl` (per [Mesa contributor guidance](https://docs.mesa3d.org/rusticl.html)).

## Summary

On Rusticl 26.0.8 (`mesa-opencl-icd` 26.0.8-1ubuntu0.3), `clGetSemaphoreInfoKHR(..., CL_SEMAPHORE_EXPORT_HANDLE_TYPES_KHR, ...)` returns `CL_SUCCESS` and a zero-byte list for a binary semaphore successfully created with `CL_SEMAPHORE_HANDLE_SYNC_FD_KHR` in its export-handle-type property. The same object reports `CL_SEMAPHORE_EXPORTABLE_KHR = CL_TRUE`.

## Expected result

The [OpenCL API Specification, §5.13.8](https://registry.khronos.org/OpenCL/specs/unified/refpages/man/html/clGetSemaphoreInfoKHR.html) defines `CL_SEMAPHORE_EXPORT_HANDLE_TYPES_KHR` as the list of handle types that may be used to export the queried semaphore. For this object, the list should contain exactly `CL_SEMAPHORE_HANDLE_SYNC_FD_KHR` (`0x2058`) and have size `sizeof(cl_external_semaphore_handle_type_khr)` (4 bytes on this system). A zero-length list denotes no exportable handle types.

## Minimal valid reproduction

1. On an OpenCL device reporting `cl_khr_semaphore`, `cl_khr_external_semaphore`, `cl_khr_external_semaphore_sync_fd`, and Sync-FD in `CL_DEVICE_SEMAPHORE_EXPORT_HANDLE_TYPES_KHR`, create a single-device context.
2. Create a binary semaphore with properties:

   ```c
   const cl_semaphore_properties_khr props[] = {
       CL_SEMAPHORE_TYPE_KHR, CL_SEMAPHORE_TYPE_BINARY_KHR,
       CL_SEMAPHORE_EXPORT_HANDLE_TYPES_KHR, CL_SEMAPHORE_HANDLE_SYNC_FD_KHR,
       CL_SEMAPHORE_EXPORT_HANDLE_TYPES_LIST_END_KHR, 0
   };
   ```

3. Query `CL_SEMAPHORE_EXPORT_HANDLE_TYPES_KHR` first with `param_value = NULL` to obtain `param_value_size_ret`, then with a 4-byte output buffer. Query `CL_SEMAPHORE_EXPORTABLE_KHR` as a control. Release semaphore and context.

No command queue, kernel, signal, wait, FD export/import, or GPU dispatch is needed.

## Actual result on H1

Two independent executions of the same binary under the same system ICD provenance both returned:

```text
clCreateSemaphoreWithPropertiesKHR=0 object=1
query_export_types_size_status=0 bytes=0
query_export_types_value_status=0 bytes=0 value=0xdeadbeef expected=0x2058
query_exportable_status=0 bytes=4 value=1
clReleaseSemaphoreKHR=0
clReleaseContext=0
```

`0xdeadbeef` was the preinitialized output sentinel and remained unchanged because the reported list was empty. Neither run timed out or crashed.

## Environment and likely cause

- AMD Radeon 660M, RadeonSI/Rembrandt/ACO, OpenCL 3.0.
- System `libRusticlOpenCL.so.1.0.0`, driver 26.0.8-1ubuntu0.3, SHA-256 `8cc0f8a269a310e3e48527c0e029ab84f4d7a27144cd9261a78a478638f7870d`; ocl-icd loader 2.3.4, SHA-256 `3a24c22deedc682eb864596ef301a75d54dea23ca4c35a50deda9480694137c2`.
- `RUSTICL_ENABLE=radeonsi`; no local Mesa or B9-patched library loaded. `/proc/self/maps` in both runs records the system loader and ICD paths.
- In the corresponding Ubuntu Mesa 26.0.8 source package, `src/gallium/frontends/rusticl/api/semaphore.rs:44–47` writes `None` unconditionally for `CL_SEMAPHORE_EXPORT_HANDLE_TYPES_KHR`. The same file accepts the Sync-FD export property at `:117–129` and reports exportability from `sema.handle_type` at `:51–52`. `api/util.rs:110–136,430–445` converts `None` into a zero-byte successful query. This is a high-confidence source-level explanation for the observed output. A candidate correction would return the configured `sema.handle_type` rather than unconditional `None`; it has not been implemented or tested here.

## Scope

This is a stable API metadata/conformance failure (F1). No memory safety, isolation, cross-process effect, or GPU fault has been demonstrated. The relevant OpenCL-CTS checkout inspected at commit `9feccbb8fcb8eefeebf86a48036173a8155f9d21` has positive external-semaphore coverage but does not query the export-handle-type list of an exportable semaphore in the examined semaphore tests. The CTS observation is limited to that commit.

## Local evidence to attach before filing

- `<REPO>/src/b12_c1_t1_2026-10-30.c`
- `<REPO>/artifacts/b12/b12_c1_t1_2026-10-30.log`
- `<REPO>/artifacts/b12/b12_r1_t1_2026-10-30.log`

Before filing, check the current Mesa issue and merge-request tracker for a duplicate or an already merged fix; that current upstream state was not verified in this audit.
