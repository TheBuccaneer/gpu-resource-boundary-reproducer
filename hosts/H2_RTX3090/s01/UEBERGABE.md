# Aktuell: H2 S01 API-only-Pilot vorbereitet

Zuerst `hosts/H2_RTX3090/s01/H2_S01_API_PILOT_READY.md` lesen. Fertiges lokales Binary, Buildprovenienz, ausschließlich lesender echter Ledgercheck und pending Manifest für genau5 API-only-Fälle, je1 Pilot. Keine Runtimeausführung und keine Reservierung. Vor Start weiterhin tatsächliche explizite Freigabe erforderlich. Die Manifestbindung gilt ausschließlich für das ursprüngliche lokale Repository; ein Entpacken dieses ZIPs erfordert eine neue explizite Freeze-/Approvalprüfung. Das Liveledger ist nicht enthalten und darf nicht ersetzt werden.

Read-only-Preflight: `python3 hosts/H2_RTX3090/s01/pilot/check_api_pilot.py` im gebundenen Originalrepository. Einzelfallskript und Reihenfolge im Ready-Dokument. Keine Fallkampagne starten. Nach jedem Lauf einzeln Log/Provenienz prüfen; bei Abweichung stoppen. Die folgenden Dokumente bleiben historische Prüfgrundlage.

# Aktuell: S01 R3 — Wiederholungsbudget korrigiert

Zuerst `hosts/H2_RTX3090/s01/H2_S01_R3_SUPERVISOR_FIX.md` lesen. PASS für die offline geprüfte Budgetdurchsetzung; keine Laufgenehmigung. 24 Supervisor-CPUtests,7 S01-Offlineprüfungen und bestehende Regressionen erfolgreich. S01-Reproducer, Fallplan und Logoracle sind bytegleich zu R2. Neues festes Hostledger und UUIDbindung verhindern Replay derselben Approval-ID/Fall/Nummer auch mit anderem Ergebnisordner und parallelen Prozessen.

Das Liveledger liegt außerhalb des Repositories/ZIPs und wird nicht mitgeliefert. Es darf nicht durch eine Archivkopie ersetzt werden. Auf einem anderen Host sind bewusste Ledgerprovisionierung und neue explizite Approvalbindungen erforderlich. Fehlendes/kaputtes Ledger bricht ab; keine automatische Wiederfreigabe. Noch keine echten API-only-Freigaben ausgestellt oder gestartet. Details und Hashes im R3-Bericht. H1 bleibt unverändert; keine GPU-/OpenCL-Ausführung, Treiberänderung oder Push.

Die folgenden R2-Angaben sind historische Grundlage; R3 ersetzt die betroffene Supervisor-/Budgetbeschreibung.

# OpenCL2 — H2-S01 Implementierungsvorbereitung R2,31.10.2026

Zuerst `AGENTS.md`, `PROJECT_STATE.md` und `hosts/H2_RTX3090/s01/H2_S01_READY_FOR_DAYBREAK.md` lesen. S01 hat11 implementierte Fälle, davon5 ohne Queue/Enqueue und6 Datenfälle für spätere gesonderte Freigabe. Kein OpenCL-Programm wurde ausgeführt. Der Supervisor wurde mit CPUfixtures vollständig durch seinen Startpfad geprüft.

Normen, Kontrollen, Build, Hashes,10 Supervisor-E2E-/7 S01-/8 bestehende Audit-Tests und offene Fragen stehen im S01-Ordner. Der bestehende C1-Supervisor erkennt nun den gesonderten S01-Katalog und bindet dessen Hash. Keine echte Freigabe liegt vor.

Daybreak soll Source/Vertrag/Parser und Ausführungsgrenzen unabhängig prüfen. Danach erst gesonderte fallbezogene Laufentscheidung. Keine H1-Wiederholung, C0-Inventarisierung, GPU-Experimente, Sicherheitstrigger, Installation, Treiberänderung, Veröffentlichung oder Push durch diese Übergabe autorisiert.

Enthalten: vollständige aktuelle Arbeitsdateien,729 H1-Originaldateien bytegleich, H2-C0, C1, Security-Discovery und S01. Ausgelassen: Git-Historie/.git, alte ZIPs, Cache, Literaturvolltexte und private H1-B4/B9-Nachträge. Internes Paket nicht öffentlich hochladen.

Offline nach Entpacken: `python3 scripts/check_platform_layout.py`; `python3 hosts/H2_RTX3090/c1/check_preparation.py`; `python3 hosts/H2_RTX3090/security_discovery/check_preparation.py`; `python3 hosts/H2_RTX3090/s01/check_preparation.py`; `sha256sum -c handover/SHA256SUMS.txt`. Die CPUtests sind separat im S01-Ready-Dokument genannt. Kein GPUzugriff.

R2 behebt die drei Daybreak-Befunde: frischer Originaloutput vor letztem Clone-Kontrolllauf und erneuter Cloneoutputcheck, striktes gebundenes Schema2-Logprotokoll sowie Exit78/inconclusive im Supervisor. Zuerst `s01/REVIEW_R2_CORRECTIONS.md` relativ zum H2-Ordner lesen. Neue Befehlsfolge und Hashes benötigen erneutes Review und spätere separate Freigabe.
