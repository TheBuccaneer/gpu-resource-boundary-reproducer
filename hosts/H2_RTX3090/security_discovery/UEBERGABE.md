# OpenCL2 — Security Discovery, 31.10.2026

Zuerst `PROJECT_STATE.md`, `AGENTS.md`, anschließend `hosts/H2_RTX3090/security_discovery/README.md` lesen. Die neue Phase enthält16 Literaturprofile,12 Hypothesen, drei priorisierte Experimentpläne und die drei C1-Auditkorrekturen. Daybreaks Originalreview und begründete Mergeentscheidungen liegen im Unterordner `daybreak/`.

StufeA abgeschlossen für Recherche, Review, Korrekturen und Offline-Nachweise. S01 hat Priorität vor S02; S02 benutzt keinen Datensuffix nach abnormaler Commandterminierung. S03 erfordert erst belegtes Vulkan/OpenCL-Interop und dessen zusätzlich gepinnten Vertrag. Reproducer für S01–S03 sind konkrete Planskizzen, noch keine implementierten Laufprogramme. C1 enthält die weiterhin vorbereiteten26 Einzelfälle und B12-Query. Kein GPU-/Runtime-Lauf und keine neue Vulnerability wurden behauptet.

Nächste Arbeit: vor experimenteller Freigabe gewählten Top3-Reproducer implementieren/offline prüfen, private H1-B4/B9-Doppelungsfrage intern klären, konkrete Binär-/Plan-/Source-Snapshots prüfen. Danach separate B-Freigaben für Queries/Setter/kleine Baselines und je Hypothesenarm. Gefährdende Last, Hangs, Resets und Cross-Client-Störung sind separate StufeC. Keine Installation, Treiberänderung, Veröffentlichung oder Push ist genehmigt.

Das vollständige aktuelle Arbeitsdateipaket enthält alle729 originalen H1-Dateien bytegleich sowie H2-C0-Evidence, C1 und neue Reviewdokumente. Nicht enthalten: `.git`/Historie, alte ZIPs, Cache, vollständige Literatur-PDFs, private H1-B4/B9-Nachträge. Dieses interne Paket darf nicht öffentlich hochgeladen werden.

Nach Entpacken: `python3 scripts/check_platform_layout.py`, `python3 hosts/H2_RTX3090/c1/check_preparation.py`, `python3 hosts/H2_RTX3090/security_discovery/check_preparation.py`; zusätzlich `sha256sum -c handover/SHA256SUMS.txt`. Alle sind offline. Der Securitychecker prüft Dokumente/Hashes; CPU-Regressionen separat mit `python3 hosts/H2_RTX3090/security_discovery/tests/test_security_audit.py`. Kein OpenCL-Aufruf.

Künftige Übergaben automatisch als fertiges kompaktes ZIP, normalerweise unter10MB, mit externer SHA-256. `scripts/create_handover_zip.py` verwendet diesen aktuellen Einstieg. Ein ZIPexport hat keinen nachgewiesenen eigenen Git-HEAD; die frühere Basis und Contenthashes sind dokumentiert.
