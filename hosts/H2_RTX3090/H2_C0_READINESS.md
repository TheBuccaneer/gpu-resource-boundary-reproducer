# H2 C0 Readiness und nächste Schritte

Stand **31.10.2026**. **Vorbereitungsauftrag abgeschlossen; vorhandener NVIDIA-Stack inventarisiert. Experimente bleiben NO-GO ohne gesonderte Freigabe.** Dies bedeutet keine Vollständigkeit aller Feature-Verträge und keine validierte Installation alternativer Runtimes.

| Gate | Stand |
|---|---|
| Repository lokalisiert/synchronisiert | Maßgeblicher Pfad `/home/rock/projects/gpu-resource-boundary-reproducer`, sauberes `main` vor Änderungen, Fast-Forward-Pull erfolgreich; Basis `3cd984d3e7cd8b71a2670c8b599c31d65872a078` |
| H1 geschützt | Ursprüngliche H2-Vorbereitung nur durch neue H2-Dateien; anschließend H1 bytegleich nach `hosts/H1_AMD660M/` verschoben. Inhalte und historische Prüfsummen unverändert |
| Host-/Treiber-/Bibliotheksprovenienz | Rohdaten, Paketstatus, vollständige aktive Kernelkonfiguration, Mappings und Hashes erfasst |
| NVIDIA C0 | Enumeration/Query abgeschlossen; B1–B12-Gates abgegrenzt |
| Zweite physische GPU | Nicht nachgewiesen; 5060 Ti ist ungemessener H3-Kandidat |
| Alternativen und Auswahl | Recherche samt Primärquellenständen, Auswahl und vorbereitenden Rollbackplänen vorhanden; keine Alternative installiert |
| Semaphorenexport/Typabfragen | Offen: -30 bei den dokumentierten Selektoren, Vertrags-/Versionsabgleich erforderlich |
| Zink/clvk vollständige Vulkan-Voraussetzungen | Basisfeatures abgefragt; vollständiger tag-/format-/profilspezifischer Nachweis fehlt |
| C1/Baselines/CTS | Nicht ausgeführt, nicht durch C0 freigegeben |
| GitHub | Kein Push, kein öffentlicher Kommentar/Issue/PR |

## Reihenfolge der nächsten notwendigen Schritte

1. **Ohne GPU-Arbeit:** Semaphore-Query-Verträge an gepinnter OpenCL-/Extensionrevision und NVIDIA-spezifischer Dokumentation abgleichen; relevante aktuelle CTS-Querytests am erfassten Commit statisch zuordnen. Keine allgemeine CTS-Lücke behaupten. Die archivierten Extensionübersichten ersetzen noch nicht alle normativen API-Querytabellen.
2. **Konkreter C1-Plan zur Freigabe:** kleine gültige Source-/Buffer-, B2-Geometrie-, B7-Setter- und B10-Eventkontrollen ausarbeiten; API-Status, Completion, Daten und Prozess-/Library-Provenienz getrennt erfassen. B12 zunächst auf passende opaque-FD-Verträge begrenzen. Auch neue Objektanlage ist außerhalb dieses rein abfragenden Auftrags.
3. **Optionaler Installationsauftrag:** clvk und PoCL CPU nach dem [Installations-/Rollbackplan](notes/INSTALLATION_ROLLBACK_PLAN.md) priorisieren; Quellen und Abhängigkeiten pinnen, fehlende Pakete konkret benennen (u. a. fehlen Rust/Meson und `libdrm.pc` in der abgefragten Umgebung), Systemänderungen vorab freigeben lassen. Erst ihre eigenen C0-Daten entscheiden über zusätzliche B-Klassen.
4. **Hardwarefrage klären:** das Inventarisierungsskript auf dem tatsächlichen Rechner mit der 5060 Ti ausführen; Speicherbestückung, PCI-/Hostidentität und Treiber erfassen. Kein zweiter Host ist hier zugänglich oder geprüft worden.
5. **Vor Grenz-/Containmentversuchen:** gesonderten bounded Plan mit Memory-/Zeit-/Prozessgrenzen, Displayabhängigkeit, Recovery-/Stopbedingungen und klarer Schutzdomäne genehmigen. B1/B3/B4/B9-Trigger aus H1 nicht mechanisch übernehmen. Verfügbarkeit eines unabhängigen Beobachtungspfads prüfen; in diesem Auftrag keine Watchdogs oder Kernelparameter verändern.

Normative Revision, CTS-Commit/Testauswahl, kleinste zulässige Ausgangskonfiguration und konkrete Oracles müssen pro Klasse vor C1 feststehen. C0 sagt nichts über Fehlerfreiheit oder Vorhandensein einer NVIDIA-Sicherheitslücke aus.

## Reproduktion der rein abfragenden Inventarisierung

Ohne sudo und ohne neue Systempakete, aus dem Repository:

```bash
python3 hosts/H2_RTX3090/scripts/inventory.py   --out /tmp/H2_C0_eindeutig_neues_verzeichnis
```

Der Zielordner muss neu sein; das Skript verweigert vorhandene Ziele. Es benötigt Python 3; optionale Abfragen protokollieren fehlende Tools statt sie zu installieren. Der Vulkan-Helfer wird nur bei vorhandener GCC-/Vulkan-Entwicklungskette in einem temporären Ordner kompiliert und ruft keine Device-/Submit-APIs. Für einen anderen Rechner werden alle GPUs per PCI/sysfs/NVML erfasst; fehlender NVIDIA-Zugriff wird als fehlgeschlagene Abfrage protokolliert, nicht als bewiesene Hardwareabwesenheit.

Prüfsummen des maßgeblichen Datensatzes:

```bash
cd hosts/H2_RTX3090/inventory/2026-10-31_C0_release
sha256sum -c SHA256SUMS
```

Die beiden älteren Datensätze behalten ihre ursprünglichen Befehls-/Skriptstände. `2026-10-31_C0_release` ist die abschließende Referenz, nicht ein zusammengepooltes Experiment. [Abschlussmanifest](notes/H2_PREPARATION_SHA256SUMS.txt) umfasst auch die Berichte und Quellen. Für wissenschaftliche Weitergabe vorab die lokalen Rohdaten auf Host-/Prozessidentifikatoren prüfen; Veröffentlichung ist in diesem Auftrag nicht erfolgt.
