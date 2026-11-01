# Gesonderte Entscheidungsvorlage: clvk und PoCL CPU

**Entscheidungsvorschlag:** Für einen späteren Implementierungsvergleich zuerst **clvk** als zusätzlichen OpenCL-GPU-Stack genehmigen; **PoCL CPU** anschließend oder als günstiger umsetzbaren CPU-Kontrollarm ergänzen. Die NVIDIA-C1-Pakete A–F benötigen beide nicht und werden dadurch nicht blockiert. Heute weder Installation, Configure/Build der Runtimes noch deren Enumeration/Tests.

## Wissenschaftliche Rollen und Ausschlussgrenzen

| | clvk | PoCL CPU |
|---|---|---|
| Frage | Bleiben dieselben API-/Validierungsbeobachtungen bei anderem OpenCL-Frontend/Runtime auf derselben RTX3090 bestehen? | Welche Beobachtungen sind host-/API-/LLVM- bzw. runtimebezogen und treten ohne NVIDIA-GPU auf? |
| Unabhängigkeit | Eigenes clvk/clspv-Frontend; NVIDIA-Vulkan-Treiber und Kernelmodul bleiben geteilt | Eigenes PoCL-Frontend auf Threadripper, kein GPU-Unterbau; kein GPU-Isolationskontrollbeweis |
| Relevante erste Fälle | B7 Setter, B2 gültige Geometrie/isolierte Fehler, B10 In-Order/Cross-Queue nach eigenem C0 | Dieselben grundlegenden API-Fälle, CPU-Ausführung separat; keine Gleichsetzung von CPU-Local-Memory mit LDS |
| Einschränkungen | Dokumentiert kein OOO und nur ein Gerät pro Kontext. Shader-SPIR-V statt H1-Kernel-SPIR-V. Kein B4/B5/B6/B12 aus einer Versionszahl ableiten | Optionalfeatures erst am konkreten Build abfragen; SVM/IL/Extensions nicht als bereits vorhanden ausgeben. NVIDIA-spezifische Identitätsgates des jetzigen Reproducers müssen kontrolliert portiert werden |
| Nutzenpriorität | Höher für unabhängigen GPU-Runtimevergleich | Hoher Kontrollnutzen bei geringerer unterer GPU-Komplexität |

Die C0-Auswahl wird damit konkretisiert, nicht um weitere Runtimekandidaten erweitert. Keine zusätzliche Architektur-/Nouveau-/CUDA-Backend-Installation Teil dieser Entscheidung.

## Konkrete Quellen- und Abhängigkeitspins

**clvk:** Commit `87d6dd3c55db4299b6e364bb8d41ba00d6eda7d7`. Im zugehörigen Gitbaum sind die folgenden Gitlinks festgelegt, die bereits statisch erfasst wurden:

| Dependency | SHA |
|---|---|
| clspv | `7ee3a40e652b3649d175758f72d7af98b9450a68` |
| OpenCL-Headers | `386ca390b2f52efeb3e1a55a500690eb8013f60e` |
| SPIRV-Headers | `2b7475f6d664efbad4d4315c94ecec9640831e5b` |
| SPIRV-LLVM-Translator | `c4a6b42303c6b45a64ede3d1c0399dce2d90f3fc` |
| SPIRV-Tools | `ba9a9e32160e2500ce1e48cc563a8fedaf596e6e` |

Zusätzlich pinnt **dieser clspv-Stand** in `deps.json` LLVM auf `71a85ef89fc1997205104b72447e0dc2c4ad83da`, eigene SPIRV-Headers auf `b824a462d4256d720bebb40e78b9eb8f78bbb305` und eigene SPIRV-Tools auf `fb7471844504abb16b732dc4fc0837119a32ec24`. Diese zwei SPIR-V-Abhängigkeitsebenen nicht nach Belieben zusammenlegen. Der separat in C0 erfasste damalige clspv-HEAD `f56e…` ist **nicht** der clvk-Gitlink. Weitere rekursive Gitlinks vor dem Build in einem vollständigen Lockmanifest erfassen; der derzeitige Quellabgleich ist noch kein erfolgreich gebauter Dependency-Lock.

[clvk-Gitbaum](https://github.com/kpet/clvk/tree/87d6dd3c55db4299b6e364bb8d41ba00d6eda7d7), [clspv-Abhängigkeiten](https://github.com/google/clspv/blob/7ee3a40e652b3649d175758f72d7af98b9450a68/deps.json).

**PoCL:** Commit `4549718771fcdae72516599230587ad019ef7066`, CPU-Backend; OpenCL3.1-Option aus. Das gepinnte `cmake/LLVM.cmake` akzeptiert LLVM-Hauptversionen18–23. Daher ist der **bereits im C0 nachgewiesene** LLVM21.1.8 ein begründeter Buildkandidat; Konfiguration/Linkprüfung bleiben offen. Auswahl explizit über `/usr/bin/llvm-config-21`, keine unversionierte Autodetektion. [PoCL LLVM-Prüfung](https://github.com/pocl/pocl/blob/4549718771fcdae72516599230587ad019ef7066/cmake/LLVM.cmake), [Buildoptionen](https://github.com/pocl/pocl/blob/4549718771fcdae72516599230587ad019ef7066/CMakeLists.txt).

## Konkrete spätere Einführungsphasen

1. **Buildvorbereitung ohne Systemänderung:** private Pfade unter `/home/rock/projects/opencl-h2-runtimes/{clvk-87d6dd3,pocl-cpu-4549718}/`, jeweils `src/`, `build/`, `prefix/`, `icd/`, `logs/`. Quellen/Gitlinks prüfen, Lockmanifest und Ressourcenbudget festlegen. Höchstens vier Buildjobs, geplante RSS-Gesamtgrenze16GiB, genügend freier Buildspeicher vor Start gezielt prüfen. Diese Grenzen gelten für den späteren Compilerbuild, nicht für die kleinen C1-Runs.
2. **Dependency-Gate:** aus dem archivierten C0-Paketstatus sind Vulkan-/OpenCL-Dev, LLVM21-Dev, zlib-/zstd-/XML-Dev vorhanden; `libclang-21-dev`, `libclang-cpp21-dev`, `libhwloc-dev` und `libsleef-dev` dort nicht nachgewiesen. Das ist keine heutige Paketneuinventarisierung. Für PoCL insbesondere Clang-Entwicklungsbibliotheken klären; hwloc ist optional. SLEEF-Anforderungen am gewählten Buildpfad prüfen, nicht pauschal aus einem fehlenden Systempaket auf unmöglichen Build schließen.
3. **clvk-Konfiguration:** `CLVK_VULKAN_IMPLEMENTATION=system`, Compiler an, `CLVK_BUILD_TESTS=OFF`, `CLVK_UNIT_TESTING=OFF`, keine CTS-Targets aktivieren; die gepinnte clspv-/LLVM-Kette verwenden. Nie den Systemloader durch die als `libOpenCL.so` gebaute Bibliothek über globale Linkerpfade verdecken. Quell-/Buildlayout in ein nachvollziehbares privates Runtimepaket überführen; `clspv`-Pfad im Wrapper explizit setzen.
4. **PoCL-Konfiguration:** `ENABLE_HOST_CPU_DEVICES=ON`, `ENABLE_CUDA=OFF`, `ENABLE_VULKAN=OFF`, `ENABLE_LEVEL0=OFF`, `ENABLE_REMOTE_CLIENT=OFF`, `ENABLE_REMOTE_SERVER=OFF`, `ENABLE_ICD=ON`, `WITH_LLVM_CONFIG=/usr/bin/llvm-config-21`, `ENABLE_TESTS=OFF`, `ENABLE_EXAMPLES=OFF`, `ENABLE_OPENCL_3_1=OFF`, `ENABLE_UNFINISHED_EXTENSIONS=OFF`; privater Installprefix. Für einen reproduzierbaren Konformitätsvergleich `ENABLE_CONFORMANCE=ON` und dessen verpflichtendes `ENABLE_SLEEF=ON` vorsehen; kein erfolgreicher CTS wird dadurch zugesagt. Wenn diese Konfiguration weitere Abhängigkeiten benötigt, erst den konkreten Diff vorlegen statt stillschweigend Optionen zu lockern.
5. **Keine Paketfreigabe vorwegnehmen:** falls Systempakete erforderlich sind, vor deren Installation exakte Versionen, Abhängigkeits-/Entfernungsdiff einer Simulation und Rückkehrweg vorlegen. Keine pauschale Upstream-Installationszeile mit `sudo apt install` ausführen. Kein NVIDIA-/CUDA-/Mesa-Upgrade ist Ziel dieser Einführung.
6. **ICD-Isolation:** je Runtime absolute Bibliothek im privaten ICD-Verzeichnis; pro Prozess `OCL_ICD_VENDORS` setzen. NVIDIA-Vulkan-ICD für clvk explizit wählen und Geräteidentität prüfen, um lavapipe-Fallback auszuschließen. PoCL für CPU explizit einschränken. Keine Änderung an `/etc/OpenCL/vendors`, `/usr`, globalem `LD_LIBRARY_PATH`, Bootparametern oder Modulen.
7. **Nach eigenem Installations-GO:** zunächst nur die nötige neue Runtime-C0-Abfrage samt Mappings/Hashes. Neue Implementierung ist ein konkreter Anlass für diese begrenzte Abfrage; die bestehende NVIDIA-C0 wird nicht als neue Allgemeininventarisierung wiederholt. Jede Runtime erhält eigene Feature-Matrix. Erst danach separat C1-Ausführung genehmigen und die generischen Testfälle an die geprüfte Zielidentität anpassen.

## Rückkehrweg und Abnahme

Wrapper/Prozess beenden und dessen Auswahlvariablen aufheben. Neue Prefixe zunächst erhalten, aus der Auswahl nehmen; nichts aus `/usr` zurückkopieren, da nichts ersetzt werden darf. Der ursprüngliche NVIDIA-Loader-/ICD-Hash und dessen selektive Identitätsabfrage müssen weiterhin zum C0-Stand passen. Bei Abweichung Einführung stoppen. Für separat genehmigte Paketinstallationen müssen zuvor vorhandene Versionen und ein konkreter Paket-Rollback festgehalten werden; heute gibt es deshalb noch keinen behaupteten ausgeführten Paket-Rollback.

**Abnahme clvk:** kompletter Lock, reproduzierbarer privater Build, tatsächlich NVIDIA3090 statt Softwaregerät, eindeutige Librarymaps, korrekte Profil-/Featureangaben. **Abnahme PoCL:** nur ausgewähltes Threadripper-CPU-Gerät, dokumentierte LLVM-/Konformitätsoptionen, keine zusätzlich aktivierten GPU-/Remote-Backends, eigene Library-/Featureprovenienz. Beide brauchen danach kleine gültige Baselines; eine erfolgreiche Enumeration genügt nicht.

**Freigabeentscheidung:** Installation beider Kandidaten noch nicht genehmigt. clvk priorisieren, wenn zusätzlicher GPU-Stack die nächste Forschungsfrage ist; PoCL CPU bevorzugt vorziehen, wenn zunächst eine günstige API-/Hostkontrolle gebraucht wird. Die endgültige Kostenentscheidung hängt an den konkret fehlenden Buildabhängigkeiten, nicht an einem neuen allgemeinen Kandidatensurvey.
