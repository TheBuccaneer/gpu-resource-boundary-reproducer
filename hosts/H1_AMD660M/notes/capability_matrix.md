# OpenCL Security Study — Capability Matrix

## H1 — AMD Radeon 660M / Rusticl

### Environment

- Date: 2026-08-26
- GPU: AMD Radeon 660M / Rembrandt
- Backend: radeonsi + ACO
- OpenCL implementation: Rusticl
- Mesa: 26.0.3-1ubuntu1
- Kernel: 7.0.0-30-generic
- OpenCL: 3.0
- Default OpenCL C: 1.2
- SPIR-V IL: 1.0–1.6
- Local memory: 65536 bytes
- Max work-group size: 1024
- Max work-item sizes: 1024 × 1024 × 1024
- Max sub-groups/work-group: 32

### SVM

- Coarse-grained buffer: yes
- Fine-grained buffer: no
- Fine-grained system: no
- SVM atomics: no

### Security-relevant extensions

- cl_ext_buffer_device_address: yes
- cl_khr_external_semaphore: yes
- cl_khr_external_semaphore_sync_fd: yes
- cl_khr_semaphore: yes
- cl_khr_external_memory: not advertised
- cl_khr_command_buffer: not advertised
- cl_khr_command_buffer_mutable_dispatch: not advertised

### Bug-class applicability

| ID | Bug class | H1 status |
|---|---|---|
| B1 | Resource capacity | TEST |
| B2 | Workgroup/dispatch geometry | TEST |
| B3 | Combined resources | TEST |
| B4 | SVM negative/lifetime | TEST — coarse-grained SVM |
| B5 | Raw device-pointer authority | TEST — high priority |
| B6 | Mutable-state revalidation | NOT AVAILABLE |
| B7 | Object/context ownership | TEST |
| B8 | Representation mismatch | TEST |
| B9 | Integer/arithmetic backend | TEST — white-box |
| B10 | Synchronization/state | TEST |
| B11 | Multi-device/ICD identity | LATER |
| B12 | External memory/semaphore | PARTIAL — semaphore only |
| B13 | Error propagation | CROSS-CUTTING |
| B14 | Version regression | CROSS-CUTTING |

### Raw evidence

- `inventory/baseline_rusticl_660m_2026-08-26.txt`
- `inventory/clinfo_rusticl_660m_2026-08-26.txt`
