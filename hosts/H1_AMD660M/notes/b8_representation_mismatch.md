# B8 – Representation Mismatch: Abschluss H1

> Consolidation evidence note, 2026-10-30: source and IL inputs are present; durable paired stdout was not located in the examined local evidence directories. Empirical rows below remain report-backed, not independently raw-log-verified by the consolidation.

Datum: 2026-10-17
Status: CLOSE auf H1, kein eigenständiger B8-Bug nachgewiesen
Hardware: AMD Radeon 660M (Rembrandt), radeonsi/ACO
Messumgebung: Rusticl/Mesa 26.0.8-1ubuntu0.3
Source-Audit: Mesa 26.2.0 (nicht identisch mit Messumgebung)

## Forschungsfrage

Führen semantisch äquivalente OpenCL-C- und SPIR-V-IL-Programme
zu unterschiedlichen Validierungs-, Ressourcen- oder Fehlerzuständen?

## Empirische Ergebnisse

| Stufe | Experiment | Ergebnis |
|---|---|---|
| C0 | IL-Capability und Toolchain | PASS |
| C1 | add1, Source vs. IL | Beide korrekt, Kernelmetadaten identisch |
| C2 | Dynamic Local, 4096 B | Beide PASS, Accounting 4096 B |
| C3 | Dynamic Local, 65532 B | Beide PASS, Accounting 65532 B |
| C4 | Dynamic Local, 65536 B | Beide PASS, Accounting 65536 B |
| C5 | Dynamic Local, 65537 B | Beide bis einschließlich Kernelargumente und Metadatenabfrage erfolgreich; kein Dispatch |
| C6 | Compiler-/Validierungspfad | Implementierungsasymmetrie gefunden, kein Bug nachgewiesen |

Bei C5 meldeten beide Repräsentationen 65537 B bei
CL_DEVICE_LOCAL_MEM_SIZE = 65536 B. Der Probe-Guard verhinderte
den Enqueue. Die tatsächliche Fehlerbehandlung beim Dispatch
wurde hier nicht untersucht. Frühere B1-Befunde begründen
diese Sicherheitsbegrenzung.

## Source-Audit: Mesa 26.2.0

OpenCL C:
  Program::new -> SPIRVBin::from_clc
  -> clc_compile_c_to_spirv
  -> clc_c_to_spirv
  -> clc_compile_to_llvm_module
  -> llvm_mod_to_spirv

SPIR-V IL:
  Program::from_spirv
  -> SPIRVBin::clone_on_validate
  -> clc_validate_spirv

Debug-Standardwerte:
  allow_invalid_spirv = false
  validate_spirv = false

Die explizite SPIR-V-Tools-Validierung unterscheidet sich
zwischen den Pfaden. Der Source-Compiler besitzt eigene
Kompilierungs- und Übersetzungsprüfungen. Daraus folgt
weder automatisch ein Validierungsbypass noch ein
Spezifikationsverstoß.

Der Audit betrifft Mesa 26.2.0. Eine Übertragung der
internen Implementierungsdetails auf Mesa 26.0.8
ist ohne Versionsvergleich nicht belegt.

## Reproduktionsartefakte

Source:
  probes/add1.cl
  probes/dynamic_local.cl

SPIR-V:
  artifacts/add1.spv
  artifacts/b8_c2_dynamic_local.spv

Probes:
  src/b8_c1_source_il_metadata.c
  src/b8_c2_source_il_probe.c

Die C2-SPIR-V-Datei bestand eine externe spirv-val-Prüfung.
C5 verwendete einen expliziten Pre-Dispatch-Guard.

## Wissenschaftliche Entscheidung

B8 wird auf H1 als untersuchter Negativbefund geschlossen.

Keine nachgewiesene Divergenz bei den getesteten
gültigen Source-/IL-Paaren.
Keine nachgewiesene Security-Boundary-Verletzung.
Keine unabhängige Paper-Novelty aus B8 ableitbar.

C7 wird ohne konkrete Interaktionshypothese nicht erweitert.
C8 ist durch die paarweisen Vergleiche abgedeckt.
C9 wird mangels reproduzierbarem Sicherheitsbefund
nicht eskaliert.

Weitere Untersuchungen nur bei neuer spezifikationsrelevanter
Hypothese oder gezieltem Versionsvergleich.
