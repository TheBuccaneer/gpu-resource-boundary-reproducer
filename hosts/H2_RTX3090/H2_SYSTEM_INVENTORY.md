# H2 Systeminventar

Stand: **31.10.2026**, direkte Abfrage des verfügbaren Linux-Hosts. Maßgeblicher Datensatz: [C0 release](inventory/2026-10-31_C0_release/collection.json). Das ist keine bloße Remote-Anleitung. `systemd-detect-virt` meldet `none` (Exit 1 ist dafür normal); diese Erkennung ersetzt keine physische Gehäuseinspektion.

## Hardware

| Merkmal | Beobachtung / Beleg |
|---|---|
| CPU | AMD Ryzen Threadripper 3970X, x86_64, 1 Sockel, 32 Kerne, 64 Threads; `cpu.stdout`, `cpu_topology.stdout` |
| RAM | Linux MemTotal 134911287296 Byte = ca. 125,65 GiB; nominell etwa 128 GiB, DIMM-Bestückung ohne privilegierte SMBIOS-Abfrage nicht bestätigt |
| GPU | Genau ein PCI-Display/3D-Gerät: GA102 / RTX 3090, Ampere, CC 8.6, `10de:2204`, Gigabyte Subsystem `1458:4044` |
| GPU-Speicher | NVML 24576 MiB; OpenCL global 25272516608 Byte, maximale Einzelallokation 6318129152 Byte. Physischer VRAM und runtimeverfügbare Speichergröße unterscheiden sich |
| PCIe | `0000:01:00.0`; maximal 16 GT/s ×16 (Gen4), während der sysfs-Abfrage 5 GT/s ×16. NVML meldete zeitversetzt Gen1 ×16. Dynamischer Leerlaufzustand, kein Bandbreitentest oder Fehlerbeleg |
| NUMA | CPU: ein Knoten 0, CPUs 0–63. GPU sysfs `numa_node=-1` (keine ausgewiesene Zuordnung), `local_cpulist=0-63`; nicht als explizite GPU-NUMA-0-Affinität ausgeben |
| Weitere NVIDIA-Funktion | `0000:01:00.1`, GA102 HDMI/DP-Audio `10de:1aef`; dieselbe Karte, keine zweite GPU |
| RTX 5060 Ti | Nicht in vollständigem PCI-Inventar, sysfs-GPU-Inventar oder NVML sichtbar. Keine zweite physisch enumerierte GPU auf diesem Host nachgewiesen |
| Software-Vulkan-Gerät | llvmpipe/lavapipe CPU-Gerät, keine zweite physische GPU |

Quellen: [PCI/sysfs](inventory/2026-10-31_C0_release/pci_sysfs.json), [NVML](inventory/2026-10-31_C0_release/nvidia_detail.stdout), [vollständiges PCI-Inventar](inventory/2026-10-31_C0_release/pci.stdout). Nicht enumerierte, abgeschaltete oder ausgesteckte Hardware lässt sich damit nicht ausschließen. Für eine anderswo verfügbare 5060 Ti fehlen Hostidentität und eigener Datensatz; vorläufig **H3-Kandidat**, kein bereits vermessener H3-Host.

## Betriebssystem und Kernel

Ubuntu 26.04.1 LTS (Resolute Raccoon), Kernel `7.0.0-42-generic`, x86_64; vollständiger `uname`-String und OS-Datei im Datensatz. Die komplette lesbare [Kernelkonfiguration](inventory/2026-10-31_C0_release/config-7.0.0-42-generic) und ihre Prüfsumme sind archiviert. Wesentliche Optionen: SMP/NUMA, PREEMPT_DYNAMIC und PREEMPT_LAZY, HZ_1000, MODULES, MODVERSIONS, AMD_IOMMU; `DRM_NOUVEAU=m`, `DRM_NOUVEAU_SVM` aus. Das sind Buildoptionen, keine Aussage über aktive IOMMU-Isolation oder geladene Nouveau-Module. `/proc/cmdline` ist in `files.json` enthalten.

## NVIDIA: aktiv gegenüber installiert

Aktiv: `nvidia`, `nvidia_modeset`, `nvidia_drm`, `nvidia_uvm`; PCI-Treiber der GPU ist `nvidia`. `/proc/driver/nvidia/version` identifiziert **NVIDIA Open Kernel Module 580.178.04**. Das Open-Kernel-Modul ist vom proprietären OpenCL-Userspace zu unterscheiden. Nouveau ist als Modul verfügbar, aber nicht geladen oder an die Karte gebunden. Die Anzeige verwendet die RTX 3090 (`Display Active: Enabled`, `Display Attached: Yes`); spätere Verfügbarkeitsversuche hätten daher eine konkrete Desktop-Abhängigkeit.

Installiert sind u. a. `nvidia-driver-580-open`, `nvidia-dkms-580-open`, `libnvidia-compute-580`, `libnvidia-gl-580` in Version `580.178.04-0ubuntu0.26.04.1`. Kernelmodulpakete für -38 und -42 sind installiert; ältere Einträge mit Status `rc` sind nur Konfigurationsreste. `nvidia-settings` 580.95.05 ist kein Beleg für eine abweichende aktive Treiberversion. Der vollständige Paketstatus ist in `packages.stdout` erhalten.

CUDA-Treiber-API: `cuDriverGetVersion` liefert bei Erfolg **13000 (13.0)**, ohne `cuInit`; NVML meldet ebenfalls CUDA 13.0. Das installierte Toolkit ist davon verschieden: `nvcc` **12.4.131**, `nvidia-cuda-toolkit` `12.4.131~12.4.1-8`, `libcudart12` `12.4.127~12.4.1-8`. Die OpenCL-Plattform trägt den String `CUDA 13.0.100`; auch das ist keine installierte Toolkitversion. `/usr/local/cuda/bin/nvcc` wurde nicht gefunden.

## Toolchain und weitere Grafikkomponenten

| Komponente | Installiert / abfragbar |
|---|---|
| GCC | 15.2.0 |
| Clang | Standard 21.1.8; zusätzlich clang-20 20.1.8 installiert |
| LLVM | `llvm-config-20` 20.1.8 und `llvm-config-21` 21.1.8; unversioniertes `llvm-config` fehlt. LLVM 21 enthält NVPTX und SPIRV im Target-Verzeichnis |
| Python | 3.14.4 |
| Rust/Cargo/Meson | Nicht über die abgefragten PATH-Befehle verfügbar; keine entsprechenden regulären Pakete im Inventar nachgewiesen. Kein vollständiger Ausschluss privater Toolchains |
| Build | CMake 4.2.3, Ninja 1.13.2, GNU Make 4.4.1 |
| OpenCL Header | `/usr/include/CL`, Paketstand `3.0~2025.07.22-2build1`, Header-Prüfsummen in `files.json` |
| Loader | ocl-icd 2.3.4-1, amd64 und i386 installiert; der Abfrageprozess ist amd64 |
| Vulkan | Loader/Dev 1.4.341.0-1; RTX 3090 meldet API 1.4.312, Softwaregerät 1.4.335; nur physische Geräteabfragen durchgeführt |
| Mesa | 26.0.8-1ubuntu0.3, GL/EGL/Gallium/Vulkan-Pakete vorhanden; daraus folgt keine installierte Rusticl-Laufzeit |
| Alternative OpenCL-Runtimes | Nur NVIDIA-ICD registriert und enumeriert. PoCL/Rusticl/clvk nicht als Plattform nachgewiesen; ergänzende Bibliothekssuche in `alternative_runtime_candidates.json` |

Alle Befehle einschließlich fehlender Tools und Rückgabestatus: [commands.json](inventory/2026-10-31_C0_release/commands.json). Die Bibliothekssuche umfasst `/usr/lib`, `/usr/local/lib`, `/opt`, `/usr/local/cuda`, nicht beliebige private Verzeichnisse. Geladene OpenCL-Bibliotheken sind durch Prozess-Mappings belegt; installierte Vulkan-ICDs zusätzlich durch ihre JSON-Dateien, erfolgreiche Vulkan-Enumeration und Hashes dokumentiert.

## Repository und Integrität

Maßgeblicher Arbeitsort: `/home/rock/projects/gpu-resource-boundary-reproducer/`, Branch `main`, Origin `https://github.com/TheBuccaneer/gpu-resource-boundary-reproducer.git`. Zu Beginn der Prüfung dieses Verzeichnisses sauber; `git pull --ff-only` meldete `Already up to date`. Synchronisierter Commit: **3cd984d3e7cd8b71a2670c8b599c31d65872a078**.

Vor Bekanntgabe dieses Pfads wurde im zunächst leeren Chat-Arbeitsordner ein zusätzlicher Clone angelegt. Die dort erzeugten frühen C0-Dateien wurden nur in das zuvor nicht vorhandene H2-Unterverzeichnis des maßgeblichen Projekts kopiert. Der zusätzliche Clone bleibt erhalten; seine Dateien sind keine zweite Hostmessung. Alle weiteren Arbeiten erfolgen im genannten Projekt. Keine vorhandenen Änderungen überschrieben, kein Stash, Reset oder erzwungenes Checkout.

[SHA256SUMS](inventory/2026-10-31_C0_release/SHA256SUMS) deckt die Rohdaten und die bei der Erhebung verwendeten Skripte ab; `library_hashes.json` enthält aufgelöste Pfade und SHA-256 der installierten/geladenen Bibliotheken. [notes/REPOSITORY_PROVENANCE.json](notes/REPOSITORY_PROVENANCE.json) dokumentiert die Abschlussprüfung. Die bestehenden H1-Dateien wurden anschließend bytegleich nach `../H1_AMD660M/` verschoben. Der ursprüngliche Abschlussnachweis in dieser Datei und `REPOSITORY_PROVENANCE.json` beschreibt die H2-Vorbereitung vor der Verzeichnisneuordnung; der separate Verschiebenachweis im Repositorystamm dokumentiert den neuen Pfad.
