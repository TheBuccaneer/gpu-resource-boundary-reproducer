Aktuell: **S01 API-only-Pilot vorbereitet**. [Fertiger Build, pending Scope und Einzelanleitung](hosts/H2_RTX3090/s01/H2_S01_API_PILOT_READY.md). Noch keine OpenCL-Ausführung oder tatsächliche Freigabe. Finaler lokaler Workspace wird an das Manifest gebunden; danach Änderungen erneut prüfen.

Aktuell **S01 R3**: [Supervisor-Budgetkorrektur und Nachweise](hosts/H2_RTX3090/s01/H2_S01_R3_SUPERVISOR_FIX.md). Dauerhaftes atomares Ledger vor Kindstart; kein Replay durch anderen Ergebnisordner. S01-Fälle unverändert, keine OpenCL-Ausführung oder Laufgenehmigung.

Aktueller S01-Stand: **R2 nach Daybreak-Review**. [Korrekturen](hosts/H2_RTX3090/s01/REVIEW_R2_CORRECTIONS.md): frisches Clone-Kontrolloracle, striktes geordnetes Logschema und maschinenlesbares Ressourcen-Inconclusive. Weiterhin keine OpenCL-Ausführung.

# Aktuell: H2-S01 Implementierungsvorbereitung

[Daybreak-Übergabe und Offline-Nachweise](hosts/H2_RTX3090/s01/H2_S01_READY_FOR_DAYBREAK.md):11 Fälle implementiert; CPU-End-to-End-Test des Supervisors erfolgreich; keine NVIDIA-/OpenCL-Ausführung. Separater S01-Katalog ist in den Supervisor integriert. Frühere Phasen folgen als Historie.

# Aktuelle Phase: H2 Security Discovery, 31.10.2026

[Security-Discovery-Einstieg](hosts/H2_RTX3090/security_discovery/README.md):16 Literaturprofile,12 Hypothesen, drei konkrete Pläne; Daybreakreview integriert. Drei C1-Auditkorrekturen mit Offline-Nachweisen. Keine GPU-/Runtime-Experimente, keine neue Vulnerability nachgewiesen. Nächste Schritte und Ausführungsgrenzen stehen im neuen Einstieg. Die folgenden Abschnitte dokumentieren die vorherigen Phasen.

# OpenCL2 — Plattformübersicht

Aktualisiert: **31. Oktober 2026**. Das Projekt untersucht OpenCL-Vertragsprüfung, Ressourcenverwaltung und Sicherheitsgrenzen. H1 und H2 sind jetzt vollständig in eigenen Plattformordnern organisiert.

| Plattform | Hardware / Implementierung | Stand | Einstieg |
|---|---|---|---|
| **H1** | AMD Radeon 660M / Rembrandt / Mesa Rusticl, RadeonSI, ACO | Untersuchungen konsolidiert; Befunde, Kontrollen und Einschränkungen dokumentiert | [H1-Projektstand](hosts/H1_AMD660M/PROJECT_STATE.md), [Evidenzmatrix](hosts/H1_AMD660M/docs/H1_EVIDENCE_MATRIX_2026-10-30.md) |
| **H2** | NVIDIA RTX 3090 / Ampere / NVIDIA OpenCL 580.178.04 | C0-Inventarisierung und Implementierungsauswahl abgeschlossen; keine GPU-Experimente | [H2-Übersicht](hosts/H2_RTX3090/README.md), [Readiness](hosts/H2_RTX3090/H2_C0_READINESS.md) |
| **H3-Kandidat** | NVIDIA RTX 5060 Ti, erwartete 16 GB / Blackwell | Auf dem H2-Host nicht gefunden; tatsächlicher Host und Ausstattung unbestätigt | [Bewertung und offene Hardwarefrage](hosts/H2_RTX3090/H2_IMPLEMENTATION_SELECTION.md) |

## Verzeichnisstruktur

```text
.
├── README.md                         aktuelle Plattformübersicht
├── PROJECT_STATE.md                  aktueller Gesamtstand und nächste Schritte
├── hosts/
│   ├── H1_AMD660M/
│   │   ├── PROJECT_STATE.md           unveränderter H1-Abschlussstand
│   │   ├── README.md                  ursprüngliche H1-Dokumentation
│   │   ├── docs/                     Methodik, Evidenzmatrix, Qualitätsreview
│   │   ├── inventory/                H1-System- und OpenCL-Inventare
│   │   ├── notes/                    H1-Klassengates und Abschlüsse
│   │   ├── src/  probes/  patches/    historische Reproducer und lokale Fixes
│   │   ├── scripts/  tools/           H1-Prüf- und Plattformwerkzeuge
│   │   ├── artifacts/  results/       H1-Evidenz und historische Rohdaten
│   │   └── manifests/                unveränderte H1-Auswahl und Prüfsummen
│   └── H2_RTX3090/
│       ├── H2_*.md                   fünf Vorbereitungsberichte
│       ├── inventory/                C0-Rohdaten, Bibliothekshashes, Befehle
│       ├── opencl/                   B1–B12-Capability-Matrix
│       ├── research/                 Primärquellen und Quellenstände
│       ├── scripts/                  reine Inventarisierungswerkzeuge
│       ├── notes/                    Installations-/Rollbackplan und Provenienz
│       └── results/                  für spätere Experimente reserviert
├── notes/                            Nachweis der H1-Verzeichnisverschiebung
└── scripts/                          gemeinsame, rein offline arbeitende Prüfung
```

## H2-C1: aktueller Vorbereitungsschritt

Die [C1-Vertragsprüfung und der Arbeitsplan](hosts/H2_RTX3090/c1/README.md) ergänzen die unveränderten C0-Daten. Die Semaphore-Typ-/Exportlistenqueries sind bei den beworbenen Erweiterungen gültig; fehlender Export würde eine erfolgreiche leere Liste erlauben und erklärt die protokollierten -30 nicht. Native Bestätigung und interne Ursache bleiben offen. Es liegen ein Testkatalog mit 26 C1-Fällen, ein separater B12-Query-Reproducer und eine konkrete clvk-/PoCL-CPU-Entscheidungsvorlage vor. Reproducer wurden nur als CPU-Hostcode kompiliert, nicht mit OpenCL ausgeführt.

## Aktueller Forschungsstand

**H1:** B1/B3 dokumentieren Ressourcenaccounting, B4 eine begrenzte Lifetime-/Integritätsfrage, B7 Objekt-/Kontextzuordnung und B12 einen F1-Metadatenbefund. B2/B8/B10 liefern Kontrollen bzw. begrenzte negative Ergebnisse; B6/B11 wurden mangels Voraussetzungen zurückgestellt. Die [H1-Evidenzmatrix](hosts/H1_AMD660M/docs/H1_EVIDENCE_MATRIX_2026-10-30.md) und der [Qualitätsreview](hosts/H1_AMD660M/docs/H1_QUALITY_REVIEW_2026-10-30.md) sind für genaue Geltungsbereiche verbindlich. Nicht jede B-Klasse ist eine unabhängige Sicherheitslücke.

**H2:** Der untersuchte Host hat einen Threadripper 3970X mit 32 Kernen/64 Threads, etwa 126 GiB Linux-nutzbaren RAM und genau eine enumerierte RTX 3090 mit 24 GiB VRAM. NVIDIA OpenCL meldet 3.0, 48 KiB Local Memory, Workgroups bis 1024 und coarse-grain Buffer-SVM. Kernel-SPIR-V-IL und Mutable Dispatch werden nicht beworben. Semaphore-Typ-/Exportabfragen benötigen noch einen Vertragsabgleich. [Systeminventar](hosts/H2_RTX3090/H2_SYSTEM_INVENTORY.md), [OpenCL-Capabilities](hosts/H2_RTX3090/H2_OPENCL_CAPABILITIES.md).

**Auswahl:** NVIDIA OpenCL als primäre H2-Implementierung, clvk als zusätzlicher GPU-Stack und PoCL CPU als Kontrolle. Rusticl/Zink ist eine optionale H1-Brücke. Alternative Laufzeiten sind bisher recherchiert und geplant, nicht installiert. [Entscheidungsvorlage](hosts/H2_RTX3090/H2_IMPLEMENTATION_SELECTION.md).

## Nächste Schritte

1. Offene Semaphore-Verträge und relevante CTS-Querytests statisch abgleichen.
2. Einen kleinen C1-Plan für gültige Kontrollen und B2/B7/B10 zur gesonderten Freigabe vorbereiten.
3. Bei Bedarf clvk und PoCL CPU nach dem dokumentierten Installations-/Rollbackplan isoliert ergänzen.
4. Den tatsächlichen Rechner mit der RTX 5060 Ti inventarisieren und danach die Plattformzuordnung festlegen.

Bisher wurden auf H2 keine GPU-Kernel, Sicherheitsexperimente oder CTS-Läufe ausgeführt. Treiber-/Paketänderungen und neue GPU-Arbeit brauchen weiterhin eine gesonderte Freigabe. Es erfolgte kein GitHub-Push.

## Offline prüfen

```bash
python3 scripts/check_platform_layout.py
python3 hosts/H1_AMD660M/scripts/check_h1_release.py
```

Die erste Prüfung kontrolliert die bytegleiche Verschiebung sämtlicher 729 ursprünglicher H1-Dateien, die H2-Prüfsummen und die aktuellen lokalen Dokumentverweise. Die zweite ist das unveränderte historische H1-Releaseprüfwerkzeug. Beide arbeiten ohne GPU-Zugriff.

Der [Verschiebenachweis](notes/H1_RELOCATION_2026-10-31.json) ordnet jeden bisherigen H1-Pfad seinem neuen Pfad und SHA-256 zu. Historische Logs, eingebettete absolute Pfade und Archivmanifeste wurden nicht umgeschrieben. H1-Plattformskripte enthalten teilweise historische Arbeitsverzeichnisse und sind keine direkt portablen Startanleitungen; vor einer später ausdrücklich freigegebenen Ausführung ist ihr Pfad-/Systembezug zu prüfen.

Die Umordnung und H2-Ergänzungen sind lokal noch ohne Commit. Für die verschobenen H1-Dateien wurden nur Git-„Intent-to-add“-Einträge angelegt, damit zuvor verfolgte Evidenz trotz der unveränderten H1-Ignore-Regeln sichtbar bleibt; es sind keine Inhalte vorgemerkt.
