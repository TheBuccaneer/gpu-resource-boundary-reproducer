# H2 Implementierungsrecherche

Recherche: **31.10.2026**. Ausschließlich Primärquellen und statische Quelltexte; keine alternative Laufzeit gebaut, installiert oder gestartet. Lokale Beobachtungen stehen im System-/Capability-Bericht. „Plausibel“ bezeichnet eine technische Einschätzung, keinen nachgewiesenen Lauf auf einer dieser GPUs.

## Komponenten und Unabhängigkeit

Der **ICD Loader** verteilt OpenCL-Aufrufe auf Implementierungen. Das **OpenCL-ICD** stellt API, Validierung und Laufzeit bereit; dessen Compiler übersetzt den Kernel. Ein **Vulkan-Treiber** oder eine **CUDA-Driver-API** kann darunter liegen. Das **CUDA Toolkit** ist ein zusätzliches Entwicklungs-/Compilerpaket. Keine dieser Rollen ist austauschbar.

| Implementierung | Wesentlicher Pfad | Geteilte Schichten |
|---|---|---|
| NVIDIA OpenCL | OpenCL → NVIDIA-Compiler/Runtime → NVIDIA-Kernelmodul → Ampere | Geschlossener OpenCL-Userspace, vorhandenes Open-Kernel-Modul |
| Rusticl/Zink | OpenCL → Rusticl/LLVM/NIR → Gallium Zink → NVIDIA Vulkan → Kernelmodul | Rusticl mit H1; NVIDIA-Unterbau mit primärem H2 |
| clvk | OpenCL → clvk/clspv → Shader-SPIR-V → NVIDIA Vulkan → Kernelmodul | Eigenes OpenCL-Frontend/Runtime; untere NVIDIA-Schichten geteilt |
| PoCL CUDA | OpenCL → PoCL/Clang/LLVM NVPTX → CUDA Driver API → Kernelmodul | Eigener OpenCL-Stack; NVIDIA-CUDA-Unterbau geteilt |
| Rusticl/Nouveau direkt | OpenCL → Rusticl → Nouveau-Gallium → nouveau-Kernelmodul | H1-Frontend; GPU-Unterbau verschieden, moderner Direktpfad problematisch |
| Rusticl/Zink/NVK | OpenCL → Rusticl/Zink → NVK/NAK → nouveau-Kernelmodul | Open Source bis zum Treiber, NVIDIA-Firmware bleibt relevant |
| PoCL CPU | OpenCL → PoCL/LLVM → x86-CPU | Keine NVIDIA-GPU-Abhängigkeit |
| PoCL Vulkan | OpenCL → PoCL/clspv → Vulkan → gewählter Treiber | PoCL mit CPU/CUDA; clspv mit clvk |

Mehrere ICDs auf derselben GPU sind kein unabhängiges Betriebssystem-Isolationsdomänenmodell. Unterschiede lassen sich nur der tatsächlich geänderten Schicht zuordnen.

## A — NVIDIA OpenCL

NVIDIA dokumentiert OpenCL-3.0-Konformität ab R465 für x86-Linux/Windows. Auf H2 ist dieser Pfad bereits enumerierbar; die konkreten Optionalfeatures stammen aus den lokalen Abfragen, nicht aus der Versionszahl. Die geschlossene Userspace-Implementierung begrenzt Root-Cause-Instrumentierung; API-Verträge und Differenziale bleiben wissenschaftlich gut beobachtbar. Eine 5060 Ti ist Blackwell/CC 12.0, die 3090 Ampere/CC 8.6; für die 5060 Ti fehlt die lokale OpenCL-/Treiberabfrage. [NVIDIA OpenCL](https://developer.nvidia.com/opencl), [NVIDIA GPU-Tabelle](https://developer.nvidia.com/cuda/gpus).

## B — Mesa Rusticl über Zink auf NVIDIA Vulkan

Die aktuelle Zink-Dokumentation nennt für OpenCL `shaderInt8` und Buffer Device Address; für 3.0 u. a. profilspezifische Speicher-/Bildlimits, für 3.1 zusätzliche Subgroup-Anforderungen. Sie beschreibt bei Image-Unterstützung ein Embedded Profile wegen Zink-intern begrenzter Sampled Images. Nicht das hohe physische Vulkan-Limit allein entscheidet daher über ein Full Profile. H2 meldet die abgefragten Basisfeatures, 48 KiB Shared Memory und 3D-Images bis 16384; vollständige Format-/Subgroup-/Profilprüfung bleibt offen. [Zink-Dokumentation](https://docs.mesa3d.org/drivers/zink.html).

Rusticl-Buildvoraussetzungen laut aktueller Anleitung: Rust ≥1.82, Meson ≥1.7, Bindgen ≥0.71.1, LLVM/Clang ≥15, libclc und passend versionierter SPIRV-LLVM-Translator sowie SPIRV-Tools. Rust, Meson, Bindgen/libclc/Translator sind hier nicht als vollständige Buildkette nachgewiesen. Ein opt-in wie `RUSTICL_ENABLE=zink` und isolierte ICD-Auswahl wäre später erforderlich; heute wurde das nicht gesetzt. [Rusticl-Bauanleitung](https://docs.mesa3d.org/rusticl.html).

Mesa 26.2 implementiert API 3.1 abhängig vom Treiber, doch H2 hat Mesa 26.0.8 und kein Rusticl-ICD. **Empfehlung: einen gepinnten 26.2.x-Stand isoliert evaluieren**, keine Zusage über das danach gemeldete Geräteprofil. Ein historisches absolutes Mindest-Mesa für jede Rusticl/Zink/NVIDIA-Kombination ist hier nicht belegt. [26.2.0-Release Notes](https://docs.mesa3d.org/relnotes/26.2.0.html).

Wichtige Versionsgrenze: Die separat archivierten `VP_ZINK_requirements.json` aus **26.0.8 und 26.2.0** enthalten nur GL-Profile, keine CL-Profile. Die heutige Online-Dokumentation darf nicht rückwirkend als taggenauer CL-Profilnachweis gelten. Der ebenfalls archivierte `zink_screen.c` aus 26.2.0 enthält Compute-Unterstützung, beweist aber keinen erfolgreichen Rusticl-Gerätestart. [Tag 26.2.0 Profil](https://gitlab.freedesktop.org/mesa/mesa/-/blob/mesa-26.2.0/src/gallium/drivers/zink/VP_ZINK_requirements.json), [Tag-Quelltext](https://gitlab.freedesktop.org/mesa/mesa/-/blob/mesa-26.2.0/src/gallium/drivers/zink/zink_screen.c).

Für 3090 somit **plausibel, lokal ungetestet**; für 5060 Ti zusätzlich passende Vulkan-/Treiber-/Firmwareversion zu prüfen. Der besondere Nutzen wäre ein kontrollierter Frontend-/Backend-Vergleich mit H1, keine vollständig unabhängige OpenCL-Implementierung gegenüber Rusticl/H1.

## C — clvk / clspv

Das Projekt bezeichnet sich aktuell als konforme OpenCL-3.0-Implementierung, nutzt clspv und unterstützt ICD-Einbindung. Dokumentierte Einschränkungen: ein Gerät je Kontext, keine Out-of-Order Queues, keine Partitionierung/native kernels; CTS-Buildintegration erfordert zusätzliche Arbeit. Build, Runtime und Compiler sind offen und instrumentierbar. Auf 3090 ist der Vulkan-Unterbau vorhanden; auf 5060 Ti ist Lauffähigkeit plausibel, aber jeweils erst durch eine gepinnte C0/C1-Kette zu belegen. [clvk README](https://github.com/kpet/clvk).

clspv erzeugt SPIR-V für das **Shader**-Ausführungsmodell; dies ist nicht das von H1 verwendete OpenCL-Kernel-SPIR-V. Pointer-/Layout- und Featurebeschränkungen müssen am konkreten Compilerstand geprüft werden. H2 meldet Variable Pointers, Int8/BDA und Non-Semantic-Info, was wesentliche Voraussetzungen begünstigt. Ein beliebiges H1-IL kann dennoch nicht unverändert als gültiger Input angenommen werden. [clspv Sprachabbildung](https://github.com/google/clspv/blob/main/docs/OpenCLCOnVulkan.md).

Öffentliche Konformitätseinträge existieren für clvk auf Intel/AMD unter ChromeOS (z. B. Submission 419), für Rusticl/Zink auf AMD/Intel (437) und für NVIDIA/Ampere. Diese zertifizieren **nicht automatisch** einen selbstgebauten Stand auf der hiesigen 3090 oder eine 5060 Ti. [Khronos Produktregister](https://www.khronos.org/conformance/adopters/conformant-products/opencl).

## D — PoCL mit CUDA/NVPTX

Die PoCL-CUDA-Anleitung kennzeichnet das Backend weiterhin als experimentell; benötigt werden NVPTX in LLVM, CUDA-Header/Driver-API und Toolkit/libdevice. Ihre Test- und Konformitätsangaben sind teilweise historisch (Statusabschnitt 2017, regelmäßiger Toolkit-Test laut Text 11.6), daher kein moderner Ampere-/Blackwell-Nachweis. H2 hat NVPTX und CUDA 12.4; LLVM-Zielversion, libdevice und PoCL-Build bleiben zu verifizieren. Für 3090 als Reserve plausibel; für 5060 Ti ist insbesondere ein Blackwell-tauglicher Compiler-/libdevice-Pfad zusätzlich offen. Architektur-Override allein ist kein Beweis. [PoCL CUDA](https://portablecl.org/docs/html/cuda.html).

Die aktuelle PoCL-README nennt für CUDA einen OpenCL-3.0-Gerätepfad mit geringer Reife und SPIR-V-Input 1.2. Für CPU/x86_64 wird hohe Reife und laufende CTS-Prüfung ausgewiesen. Diese backendabhängigen Angaben ersetzen weder einen lokalen CTS-Lauf noch eine Zusage aller Optionalfeatures. CPU und CUDA desselben PoCL-Stands erlauben hilfreiche Backend-Differentiale, sind aber keine unabhängigen Frontends. [PoCL README](https://github.com/pocl/pocl).

## E — Rusticl mit Nouveau, Ampere und Blackwell

Direktes Rusticl über den älteren Nouveau-Gallium-Treiber und **Rusticl → Zink → NVK → nouveau** sind verschiedene Kandidaten. Nach aktueller Mesa-Dokumentation ist für Turing und neuer seit Mesa 25.1 NVK+Zink der Standard; der alte Nouveau-GL-Treiber wird dort nicht mehr unterstützt. NVK unterstützt inzwischen auch Consumer-Blackwell, nennt mindestens Linux 6.6 und Vulkan-1.4-Konformität für unterstützte GPUs. Daraus folgt keine Rusticl-Konformität oder vollständige direkte Nouveau-Compute-Unterstützung. [Mesa NVK](https://docs.mesa3d.org/drivers/nvk.html).

H2s Kernelversion allein reicht nicht zur Freigabe: nouveau-/GSP-Firmware, passender Mesa-/Kernelstand und KMS-/Displaypfad müssten gemeinsam geprüft werden. An derselben PCI-Funktion kann nicht gleichzeitig der aktive NVIDIA- und nouveau-Treiber gebunden sein. Die NVIDIA Open Kernel Modules sind **nicht** Nouveau. Ein Container ersetzt diesen Kernelwechsel nicht. Für Ampere und Blackwell wäre eine separate Bootumgebung mit eigener C0-Aufnahme die angemessene Variante; die direkte ältere Gallium-Route ist derzeit kein praktikabler primärer Kandidat. Keine Module oder Bootparameter wurden geändert.

## F — weitere verfügbare Laufzeiten

**PoCL CPU** ist die bevorzugte CPU-Kontrolle auf diesem Threadripper. **Rusticl/llvmpipe** oder **clvk/lavapipe** wären mögliche Softwarevergleiche, teilen aber relevante Übersetzungsschichten mit den jeweiligen GPU-Armen und ersetzen die PoCL-Kontrolle nicht. Das enumerierte lavapipe-Gerät ist derzeit nur Vulkan, kein bereits verfügbares OpenCL-ICD.

**PoCL Vulkan** ist laut eigener Dokumentation unvollständig und ohne aktiven Maintainer. Der Text nennt unter anderem fehlende Images/Sampler und Compile/Link-Funktionen sowie problematische Host-Pointer-Pfade. Trotz grundsätzlich vorhandener Vulkan-Hardware ergibt das für dieses Projekt wenig zusätzlichen Erkenntniswert gegenüber clvk. [PoCL Vulkan](https://portablecl.org/docs/html/vulkan.html).

Intel Level Zero/NEO oder AMD-Hardware-Runtimes sind keine zusätzlichen nativen RTX-3090-OpenCL-Implementierungen. CUDA Toolkit, NVK, Vulkan Loader, clspv alleine und SYCL-Compiler sind ebenfalls keine eigenständigen OpenCL-Laufzeiten. Historische oder nicht für diese Plattform belegte Projekte werden nicht als einsatzbereite 2026-Option gezählt.

## Quellenstände, CTS und Unsicherheiten

Die Abrufe inklusive fehlgeschlagener URLs stehen in [SOURCES.json](research/sources/SOURCES.json), Ergänzungen in `SUPPLEMENT.json`/`SPEC_SOURCES.json`. Erste Khronos-Pfadversuche unter `extensions/` ergaben 404; korrekte `api/`-Dokumente wurden am gepinnten Docs-Commit archiviert. Alle lokal gespeicherten Quellen gehen in das Abschlussmanifest ein; unveränderte Drittquelltexte dienen als Nachweis, nicht als Projektcode.

| Upstream | Zum Abruf dokumentierter Head |
|---|---|
| clvk | `87d6dd3c55db4299b6e364bb8d41ba00d6eda7d7` (28.10.2026) |
| clspv | `f56e11463cd5c86035a1663fe5a5e0164400fa7b` (30.10.2026) |
| PoCL | `4549718771fcdae72516599230587ad019ef7066` (30.10.2026) |
| OpenCL-CTS | `406533a0b437f49d2e542bd293789adc7900cbed` (28.10.2026) |
| OpenCL-Docs | `0027b7f1a5e97b1bf5256888757479e012a5cb59` (28.10.2026) |

Head-Metadaten und `main`-Dateiabrufe sind getrennte HTTP-Abfragen, kein atomarer Snapshot. Vor jedem Build exakt einen Commit plus rekursive Dependency-SHAs pinnen; clvk benutzt nicht automatisch den oben separat erfassten aktuellen clspv-Head. Mesa-Tagdateien sind ausdrücklich vom Online-`latest` getrennt.

Der [offizielle OpenCL-CTS](https://github.com/KhronosGroup/OpenCL-CTS) ist für alle Kandidaten als Quelltestbasis verfügbar; der erfasste aktuelle Commit ist ein künftiger gemeinsamer Ausgangspunkt. **Kein CTS wurde gebaut oder ausgeführt**, kein CTS-Gap für H2 nachgewiesen. Die zwei H1-CTS-Tests und der dortige Commit bleiben historische H1-Evidenz. Pro Backend sind Feature-Skipps, Profile, Testauswahl und gegebenenfalls Änderungen offenzulegen.
