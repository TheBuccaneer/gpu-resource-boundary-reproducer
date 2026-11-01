# H2 OpenCL C0 Capabilities

Stand 31.10.2026. Quelle: [OpenCL-Query-Rohdaten](inventory/2026-10-31_C0_release/opencl_query.stdout), [clinfo --list](inventory/2026-10-31_C0_release/clinfo_list.stdout), Header und Bibliothekshashes im selben Datensatz. **Nachgewiesen sind erfolgreiche Enumeration und Info-Abfragen; keine funktionalen Kernel-, Compiler-, SVM-, Queue- oder Interop-Tests.**

Das eigene Werkzeug ruft ausschließlich `clGetPlatformIDs`, `clGetDeviceIDs`, `clGetPlatformInfo`, `clGetDeviceInfo` und `clGetExtensionFunctionAddressForPlatform` auf. Es erzeugt weder Kontext, Queue, Speicherobjekt noch Programm. Der Loader darf intern Geräte initialisieren; „rein abfragend“ behauptet keine vollständige Nebenwirkungsfreiheit innerhalb eines Treibers. `clinfo` wurde nur mit `--help`, `--version` und `--list` verwendet, nicht im vollständigen Modus mit möglichen zusätzlichen Laufzeitprüfungen.

## Plattform und Bibliotheken

Genau eine Plattform: **NVIDIA CUDA**, Vendor NVIDIA Corporation, `FULL_PROFILE`, `OpenCL 3.0 CUDA 13.0.100`. Genau ein Gerät: NVIDIA GeForce RTX 3090, Device-Version `OpenCL 3.0 CUDA`, Treiber 580.178.04. PCI-Bus-Info `[domain=0,bus=1,device=0,function=0]` stimmt mit NVML/sysfs überein.

ICD-Datei `/etc/OpenCL/vendors/nvidia.icd` enthält `libnvidia-opencl.so.1`. Tatsächliche amd64-Auflösung im Queryprozess:

| Rolle / realer Pfad | SHA-256 |
|---|---|
| Loader `/usr/lib/x86_64-linux-gnu/libOpenCL.so.1.0.0` | `3a24c22deedc682eb864596ef301a75d54dea23ca4c35a50deda9480694137c2` |
| ICD `/usr/lib/x86_64-linux-gnu/libnvidia-opencl.so.580.178.04` | `5903293b9b29770f37ba8b1a95459e09bd1469634bec5e0dded190e78d1faee8` |
| CUDA-Treiber `/usr/lib/x86_64-linux-gnu/libcuda.so.580.178.04` | `dcdf334aeb177c95b3385b797c9b871d03a287895713c6d107f79096b0985aa9` |

Mappings stehen unter `loaded_library_maps`, Suchpfade unter `runtime_environment.json`. i386-Bibliotheken sind installiert, aber nicht mit dem 64-Bit-Abfrageprozess gleichzusetzen. Ein Loaderexport oder nicht-null Extension-Funktionszeiger beweist keinen gültigen Gerätepfad. Insbesondere werden Core-Symbole am Loader getrennt von Extension-Funktionsadressen erfasst.

## Abgefragte Grenzen und optionale Features

| Eigenschaft | Rückgabe / C0-Bedeutung |
|---|---|
| OpenCL C | Legacy-String 1.2; `OPENCL_C_ALL_VERSIONS`: 1.0/1.1/1.2/**3.0**. Kein Widerspruch durch unterschiedliche Query-Semantik unterstellen |
| Compiler / Linker | Jeweils verfügbar=1 beworben; Source-Build noch nicht geprüft |
| OpenCL-C-Features | `__opencl_c_fp64`, `__opencl_c_images`, `__opencl_c_int64`, `__opencl_c_3d_image_writes` |
| IL | `IL_VERSION` leer; `ILS_WITH_VERSION` liefert -30. `cl_khr_il_program` fehlt, KHR-Einstiegspunkt null. Kein beworbener SPIR-V-Kernel-IL-Pfad |
| SVM | Bitmaske **1** = coarse-grain buffer; fine-grain buffer/system und atomics nicht beworben |
| Local Memory | **49152 Byte (48 KiB)**, Typ `CL_LOCAL`; keine direkte Übernahme der H1-65536-Byte-Grenzen |
| Globale Speichergrenzen | 25272516608 Byte global, max. Allokation 6318129152 Byte, Constant Buffer 65536 Byte |
| Workgroups | Max. 1024; max. Dimensionen 3; pro Achse `[1024,1024,64]`, bevorzugtes Vielfaches 32. Kernelbezogene Grenzen benötigen später ein Programm und sind offen |
| Host Queue | Bitmaske **3**: Out-of-Order + Profiling beworben; In-Order Basis vorhanden. Events/Callbacks/User-Events als API-Verträge, Laufzeitverhalten ungeprüft |
| Device Queue | Eigenschaften 0, max. Queues/Events 0, Device Enqueue 0; nicht beworben |
| Atomics | Memory-Capabilities 17 = relaxed + work-group; Fence-Capabilities 19 = relaxed + acquire/release + work-group |
| Nichtuniforme Gruppen / Generic AS | Beide 0 |
| Command Buffer / Mutable Dispatch | Extension-Namen fehlen, Einstiegspunkte null, abgefragte Selektoren -30; B6 derzeit N.A. |
| Buffer device address | `cl_ext_buffer_device_address` fehlt, `clGetMemObjectDeviceAddressEXT` null; H1-B5-Pfad N.A. |
| Partitionierung | Max-Subdevices=1, Properties nur Terminator 0, Affinity=0. Kein auswählbares Partitionsschema; kein Subdevice erzeugt |
| Konformität | Gerät meldet `v2025-04-04-00`; das ist eine Selbstangabe, kein lokal ausgeführter CTS-Pass |

Die vollständige Extension-Liste und ihre gemeldeten Versionsnummern sind im JSON erhalten, einschließlich fp64, 32-/64-Bit-Atomics, GL sharing, PCI/UUID und NVIDIA-spezifischer Optionen. Die Plattform meldet die aufgelisteten Erweiterungen mit 1.0.0; diese gemeldeten Nummern sind keine unabhängig validierte Einhaltung sämtlicher zugehöriger Query-Verträge.

## Externer Speicher und Semaphore

Beworben: `cl_khr_external_memory`, `cl_khr_external_memory_opaque_fd`, `cl_khr_semaphore`, `cl_khr_external_semaphore`, `cl_khr_external_semaphore_opaque_fd`. Geräte- und Plattformabfrage zum Speicherimport melden **0x2060 = opaque FD**. Semaphoreimport meldet **0x2055 = opaque FD**. Sync-FD wird nicht beworben und darf nicht aus dem H1-B12-Fall übernommen werden.

Geräteabfragen `CL_DEVICE_SEMAPHORE_TYPES_KHR` (0x204c) und `CL_DEVICE_SEMAPHORE_EXPORT_HANDLE_TYPES_KHR` (0x204e), sowie die jeweiligen Plattformabfragen 0x2036 und 0x2038, liefern **-30 / CL_INVALID_VALUE**. Export ist damit **unklar**, nicht als funktional unterstützt oder sicher fehlend bewiesen. Einträge für Creation/Info/Export-Funktionen sind auflösbar, aber nicht aufgerufen. Keine Semaphore oder externe Allokation angelegt. Die verwendeten Header und [gepinnten Khronos-Dokumente](research/sources/SPEC_SOURCES.json) sind archiviert; der vollständige normative Abgleich einschließlich Query-Tabellen/Versionshistorie steht noch aus. Diese C0-Rückgaben sind kein vorweggenommener NVIDIA-Fehler- oder Sicherheitsbefund.

GL sharing ist lediglich beworben. Aktiver Desktop genügt nicht als Beleg für passende GL-Kontext-/Gerätezuordnung. Externer Import, Semaphore-Objektmetadaten und tatsächliche Interoperabilität bleiben gesonderte Gates.

## Vulkan-Voraussetzungen für alternative OpenCL-Laufzeiten

[Abfrage](inventory/2026-10-31_C0_release/vulkan_query.stdout) über einen eigens erstellten Instance-/Physical-Device-Queryhelfer: **kein VkDevice, keine Queue und kein Submit**. NVIDIA meldet Vulkan **1.4.312**, `shaderInt8`, `bufferDeviceAddress`, `scalarBlockLayout`, `timelineSemaphore`, `variablePointers` und `variablePointersStorageBuffer` jeweils true; außerdem `VK_KHR_shader_non_semantic_info`, `VK_KHR_external_memory_fd` und `VK_KHR_shader_subgroup_rotate`. Shared Memory 49152 Byte, Workgroup-Invocations 1024, 3D-Image-Limit 16384.

Das erfüllt mehrere notwendige Bedingungen für clspv/Zink; die volle Feature-/Format-/Profilprüfung einer ausgewählten Buildversion wurde nicht ausgeführt. Subgroup-Operationen und sämtliche Zink-Formatanforderungen sind nicht vollständig abgeglichen. Ein ebenfalls enumeriertes Softwaregerät darf nicht versehentlich als RTX-3090-Nachweis herangezogen werden. Vulkan-Fähigkeiten beweisen keine entsprechenden OpenCL-Erweiterungen.

Übertragbarkeit je Forschungsfrage: [B1–B12-Matrix](opencl/H2_B1_B12_CAPABILITY_MATRIX.md).
