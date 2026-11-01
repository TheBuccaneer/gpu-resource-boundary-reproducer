# H2 RTX 3090 — C0 Vorbereitung, 31. Oktober 2026

Diese Plattformakte liegt getrennt von H1 unter `hosts/H2_RTX3090/`. H1 befindet sich seit der anschließenden Verzeichnisneuordnung vollständig unter [`../H1_AMD660M/`](../H1_AMD660M/README.md).
Keine GPU-Experimente, keine Installation, kein Treiberwechsel, kein Push.

| Datei/Verzeichnis | Zweck |
|---|---|
| [H2_SYSTEM_INVENTORY.md](H2_SYSTEM_INVENTORY.md) | Hardware, OS, Treiber, Toolchain und Git-Provenienz |
| [H2_OPENCL_CAPABILITIES.md](H2_OPENCL_CAPABILITIES.md) | Abgefragte Fähigkeiten, Grenzen und offene Query-Verträge |
| [H2_IMPLEMENTATION_RESEARCH.md](H2_IMPLEMENTATION_RESEARCH.md) | Primärquellen und technische Kandidatenbewertung |
| [H2_IMPLEMENTATION_SELECTION.md](H2_IMPLEMENTATION_SELECTION.md) | Priorisierte Entscheidung anhand der elf Kriterien |
| [H2_C0_READINESS.md](H2_C0_READINESS.md) | Freigabegrenzen und nächste Schritte |
| `inventory/2026-10-31_C0_release/` | Maßgebliche Rohdaten; Befehle, Status, stderr, Quellkopien und SHA256SUMS |
| `inventory/2026-10-31_C0/`, `inventory/2026-10-31_C0_final/` | Frühere C0-Abfragefassungen, historisch erhalten; keine funktionalen Tests |
| [opencl/H2_B1_B12_CAPABILITY_MATRIX.md](opencl/H2_B1_B12_CAPABILITY_MATRIX.md) | Übertragbarkeit von H1 nach Klassen |
| `research/sources/` | Abgerufene Primärdokumente, Quellenstände und Prüfsummen |
| [notes/INSTALLATION_ROLLBACK_PLAN.md](notes/INSTALLATION_ROLLBACK_PLAN.md) | Konkrete, noch nicht ausgeführte Erweiterungspläne |
| `scripts/` | Rein abfragende Inventarisierung und Rechercheabruf |
| `results/` | Für separat freigegebene spätere Experimente reserviert |

Das bisherige H1-Inventar liegt jetzt unter `../H1_AMD660M/inventory/`. Es werden weder H1-Rohdaten kopiert noch bestehende Projektberichte umgedeutet.
