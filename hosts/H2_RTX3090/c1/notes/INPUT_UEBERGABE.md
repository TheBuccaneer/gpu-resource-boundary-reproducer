# OpenCL2 — fertige Übergabe, 31. Oktober 2026

## Einstieg

1. ZIP vollständig entpacken.
2. `README.md` für die aktuelle Übersicht öffnen; `PROJECT_STATE.md` beschreibt den Gesamtstand.
3. H1 liegt vollständig unter `hosts/H1_AMD660M/`, H2 unter `hosts/H2_RTX3090/`.

Enthalten sind alle Dateien des aktuellen Arbeitsstands: H1-Berichte, Rohdaten, Inventare, Quellcode und Fixes; H2-C0-Rohdaten, Recherchequellen, fünf Berichte, B1–B12-Matrix und Installations-/Rollbackplan; außerdem die Verzeichniszuordnung und Prüfwerkzeuge. Die vollständige vorhandene Git-Historie ist separat in `handover/BASE_HISTORY.bundle` enthalten. Eine lokale `.git`-Konfiguration wurde nicht kopiert. Der ursprüngliche H2-Arbeitsauftrag liegt unter `handover/WORK_ORDER_H2_C0.txt`.

Die 729 ursprünglichen H1-Dateien bleiben bytegleich. H2 ist vorbereitet; keine GPU-Experimente, zusätzlichen Laufzeitinstallationen oder Treiberänderungen wurden durchgeführt. Die RTX 5060 Ti wurde auf H2 nicht gefunden. Nächste Schritte und gesonderte Ausführungsfreigaben stehen im H2-Readiness-Bericht.

## Offline-Integrität prüfen

Im entpackten Hauptordner:

```bash
sha256sum -c handover/SHA256SUMS.txt
python3 scripts/check_platform_layout.py
```

Der erste Befehl prüft sämtliche Paketdateien außer dem Prüfsummenmanifest selbst. Dessen Integrität ist über die separat bereitgestellte SHA-256 des gesamten ZIPs abgesichert. Die zweite Prüfung bestätigt die H1-Inhalte, H2-Rohdaten und aktuellen Dokumentverweise. Beide Schritte benötigen keine GPU und keine Git-Initialisierung. Zusätzlich kann im Ordner `hosts/H1_AMD660M/` die ursprüngliche H1-Auswahl mit `sha256sum -c manifests/H1_UPLOAD_SHA256_2026-10-30.txt` geprüft werden.

## Optional: Git-Arbeit fortsetzen

Der entpackte Projektstand ist direkt lesbar. Für die spätere Entwicklung kann im entpackten Hauptordner die Historie wieder angebunden werden, sofern dort noch kein `.git` existiert:

```bash
git init --initial-branch=main
git fetch handover/BASE_HISTORY.bundle main
git reset --mixed FETCH_HEAD
git add --intent-to-add --force -- hosts/H1_AMD660M/
```

Das stellt die Basis-Historie her und zeigt die bereits enthaltene Verzeichnisneuordnung sowie H2 als lokale Änderungen. Es überschreibt keine Arbeitsdateien. `git add --intent-to-add` merkt keine Dateiinhalte für einen Commit vor; es hält die verschobene historische Evidenz trotz der H1-Ignore-Regeln sichtbar. Erst danach ist auch das unveränderte historische `hosts/H1_AMD660M/scripts/check_h1_release.py` vollständig mit seiner Git-basierten Linkprüfung nutzbar. Ein GitHub-Remote kann bei Bedarf später eingerichtet werden; es wurde nichts gepusht.

## Umfang und bekannte Grenzen

Dieses Paket ist eine lokale Forschungsübergabe, keine neue öffentliche Upload-Auswahl. Historisch bereits enthaltene Dumps, Programme und lokale Pfadangaben bleiben erhalten. Die damalige H1-Publikationsauswahl und ihre Einschränkungen liegen unverändert unter `hosts/H1_AMD660M/docs/` und `manifests/`.

Die H1-Dokumentation erwähnt zusätzlich vertraulich zurückgehaltene B4-/B9-Detailpakete und frühere externe Build-/Quellbäume. Diese waren im hier verfügbaren Repository nicht enthalten und sind deshalb nicht Bestandteil dieses ZIPs; ihre Vollständigkeit wird nicht behauptet. Auch historische B8-Rohdatenlücken werden durch diese Übergabe nicht geschlossen. Der vollständige hier vorhandene Repositoryinhalt und die vorhandene Git-Historie sind enthalten. Historische absolute Pfade bleiben Belege, keine portable Ausführungsanleitung.

Kein enthaltenes GPU-/Plattformskript automatisch starten. Für die nächste Arbeitsphase gelten die dokumentierten C0/C1-Gates und Freigabegrenzen.
