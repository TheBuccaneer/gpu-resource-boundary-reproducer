Aktuell: **S01 API-only-Pilot vorbereitet**. [Fertiger Build, pending Scope und Einzelanleitung](hosts/H2_RTX3090/s01/H2_S01_API_PILOT_READY.md). Noch keine OpenCL-Ausführung oder tatsächliche Freigabe. Finaler lokaler Workspace wird an das Manifest gebunden; danach Änderungen erneut prüfen.

Aktuell **S01 R3**: [Supervisor-Budgetkorrektur und Nachweise](hosts/H2_RTX3090/s01/H2_S01_R3_SUPERVISOR_FIX.md). Dauerhaftes atomares Ledger vor Kindstart; kein Replay durch anderen Ergebnisordner. S01-Fälle unverändert, keine OpenCL-Ausführung oder Laufgenehmigung.

Aktueller S01-Stand: **R2 nach Daybreak-Review**. [Korrekturen](hosts/H2_RTX3090/s01/REVIEW_R2_CORRECTIONS.md): frisches Clone-Kontrolloracle, striktes geordnetes Logschema und maschinenlesbares Ressourcen-Inconclusive. Weiterhin keine OpenCL-Ausführung.

# Aktuell: H2-S01 Implementierungsvorbereitung

[Daybreak-Übergabe und Offline-Nachweise](hosts/H2_RTX3090/s01/H2_S01_READY_FOR_DAYBREAK.md):11 Fälle implementiert; CPU-End-to-End-Test des Supervisors erfolgreich; keine NVIDIA-/OpenCL-Ausführung. Separater S01-Katalog ist in den Supervisor integriert. Frühere Phasen folgen als Historie.

# Aktuelle Phase: H2 Security Discovery, 31.10.2026

[Security-Discovery-Einstieg](hosts/H2_RTX3090/security_discovery/README.md):16 Literaturprofile,12 Hypothesen, drei konkrete Pläne; Daybreakreview integriert. Drei C1-Auditkorrekturen mit Offline-Nachweisen. Keine GPU-/Runtime-Experimente, keine neue Vulnerability nachgewiesen. Nächste Schritte und Ausführungsgrenzen stehen im neuen Einstieg. Die folgenden Abschnitte dokumentieren die vorherigen Phasen.

# OpenCL2 — aktueller Gesamtstand

Aktualisiert: **31.10.2026**. Gemeinsame Basis: Commit `3cd984d3e7cd8b71a2670c8b599c31d65872a078` auf `main`; danach lokale H2-Vorbereitung und Verzeichnisneuordnung, ohne Commit oder Push.

## H1 — abgeschlossen und getrennt abgelegt

Alle 729 ursprünglichen Repositorydateien liegen jetzt unter **`hosts/H1_AMD660M/`**, einschließlich der ursprünglichen README, des bisherigen PROJECT_STATE, sämtlicher Rohdaten, Reproducer, Fixes, Inventare und Manifeste. Ihr Inhalt ist unverändert; SHA-256 und Pfadzuordnung sind [dokumentiert](notes/H1_RELOCATION_2026-10-31.json).

Für H1 gelten unverändert der [Abschlussstand](hosts/H1_AMD660M/PROJECT_STATE.md), die [Evidenzmatrix](hosts/H1_AMD660M/docs/H1_EVIDENCE_MATRIX_2026-10-30.md), der [Qualitätsreview](hosts/H1_AMD660M/docs/H1_QUALITY_REVIEW_2026-10-30.md) und die [Methodik](hosts/H1_AMD660M/docs/METHODOLOGY.md). Die dortigen Zukunftspläne und ursprünglichen Pfade sind historische Dokumentation. Der aktuelle H2-Stand folgt unten.

## H2 — Vorbereitung abgeschlossen, Experimente offen

| Bereich | Aktueller Stand |
|---|---|
| Hardware | Threadripper 3970X, 32 Kerne/64 Threads; 134911287296 Byte RAM für Linux; eine RTX 3090, Ampere, 24 GiB VRAM |
| OS/Treiber | Ubuntu 26.04.1, Kernel 7.0.0-42, NVIDIA Open Kernel Module und proprietärer OpenCL-Userspace 580.178.04 |
| OpenCL | Eine Plattform/ein GPU-Gerät; 3.0, Local Memory 49152 Byte, max. Workgroup 1024, coarse-grain Buffer-SVM |
| Ausschlüsse aktuell | Kein beworbenes Kernel-SPIR-V-IL, kein Mutable Dispatch, kein H1-Buffer-Device-Address-Pfad, keine Multi-GPU-/Multi-ICD-Konfiguration |
| Offene Queryfrage | Semaphore-Typ-/Exportselektoren liefern -30; kein vorweggenommener Fehler-/Sicherheitsbefund |
| Auswahl | NVIDIA primär; clvk als zusätzliche GPU-Laufzeit; PoCL CPU als Kontrolle; Rusticl/Zink optional |
| Ausführung | Nur System-/Capabilityabfragen; keine GPU-Experimente und keine alternativen Runtimes installiert |

Details: [H2-Plattformakte](hosts/H2_RTX3090/README.md), [B1–B12-Matrix](hosts/H2_RTX3090/opencl/H2_B1_B12_CAPABILITY_MATRIX.md), [Entscheidungsvorlage](hosts/H2_RTX3090/H2_IMPLEMENTATION_SELECTION.md), [Readiness](hosts/H2_RTX3090/H2_C0_READINESS.md).

## H3-Kandidat — RTX 5060 Ti

Auf H2 ist keine 5060 Ti enumeriert. Ein eigenständiger Host ist noch nicht nachgewiesen. Blackwell wäre als Architekturdifferential zu Ampere sinnvoll; die erwarteten 16 GB und alle Laufzeitfähigkeiten müssen am tatsächlichen Gerät bestätigt werden. Bei anderem Rechner erhält sie eine eigene H3-Plattformakte; bei Einbau in denselben Rechner zunächst eine eigene Geräte-/Konfigurationsidentität innerhalb H2.

## H2-C1 — Vertragsprüfung und Experimentvorbereitung

Die [C1-Plattformakte](hosts/H2_RTX3090/c1/README.md) enthält den abgeschlossenen normativen Queryabgleich, die CTS-Zuordnung, 26 vorbereitete C1-Fälle und konkrete Einführungsvorlagen für clvk/PoCL CPU. Die C0-Inventarisierung wurde nicht wiederholt. Alle 729 H1-Dateien und die übergebenen C0-Rohdaten bleiben bytegleich.

Die vier auffälligen Semaphore-Typ-/Exportlistenselektoren sind bei den gemeldeten Erweiterungen gültig. Nicht unterstützter Export wird durch success+leere Liste beschrieben; die protokollierten -30 passen dazu nicht. Unabhängige native Bestätigung und interne Ursache sind offen; keine neue GPU-/Sicherheitskampagne wurde ausgeführt.

## Nächster Arbeitsschritt

Die vorbereiteten Pakete separat freigeben: A gezielte B12-Queries, B B7-Setter ohne Enqueue, C kleine gültige Baselines, D B2, E B10-Grundfälle, F OOO/User-Event-Ergänzungen. Der ausführbare Hostcode und der Einzelfall-Supervisor sind vorbereitet und statisch geprüft, jedoch nicht mit OpenCL ausgeführt. Installation zusätzlicher Implementierungen, Objekterzeugung außerhalb reiner Abfragen, GPU-Ausführung und Sicherheitsexperimente bleiben eigene Freigabeschritte. Kein neuer NVIDIA-Fehler oder Sicherheitsbefund ist nachgewiesen.
