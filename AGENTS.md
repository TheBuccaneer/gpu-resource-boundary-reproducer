# OpenCL2 — verbindliche Arbeits- und Übergaberegeln

## Kompakte Übergaben an andere Chats

Der Nutzer möchte Übergaben künftig automatisch als fertige ZIP-Datei erhalten. Diese Präferenz gilt für weitere Arbeitsphasen dieses Projekts; am Ende einer zur Übergabe bestimmten Phase nicht erst nach dem ZIP-Wunsch fragen.

- Das ZIP soll den für den nächsten Chat notwendigen aktuellen Arbeitsstand einschließlich Plan, Ergebnissen, Reproducern, relevanten Rohdaten und Prüfnachweisen enthalten.
- Eine `UEBERGABE.md` im ZIP nennt Einstieg, aktuellen Stand, offene Punkte, nächste Schritte, Ausführungsgrenzen und ausdrücklich nicht enthaltene externe Unterlagen.
- Normalerweise unter 10 MB anstreben. Kein Git-Historienbundle und keine `.git`-Konfiguration standardmäßig mitpacken. Der frühere große Export wurde hauptsächlich durch das Historienbundle aufgebläht.
- Alle 729 ursprünglichen H1-Dateien können in der aktuellen vollständigen Übergabe erhalten bleiben: sie komprimieren gut und ermöglichen den unveränderten Integritätscheck. Keine H1-Datei ändern oder nur zur Größenreduktion löschen.
- Falls notwendige Daten später zu groß werden, ein kompaktes Hauptpaket und klar bezeichnete optionale Evidenzpakete erstellen. Abhängigkeiten und fehlende Hintergrunddaten transparent benennen; keine stillschweigend unvollständige Übergabe als vollständig ausgeben.
- Niemals ZIPs ineinander verschachteln oder alte Übergabepakete erneut ins neue ZIP aufnehmen.
- Datei-/ZIP-Prüfsummen, Entpackbarkeit und die enthaltenen Offlineprüfungen kontrollieren; SHA-256 zusätzlich neben dem ZIP ablegen.
- Im Abschluss direkt auf das ZIP verlinken und seine tatsächliche Größe nennen.

Das wiederverwendbare Werkzeug ist `scripts/create_handover_zip.py`. Ein lokales Übergabe-ZIP ist keine Veröffentlichungsgenehmigung. Das vollständige interne Paket darf nicht öffentlich hochgeladen werden.

## Bestehende Forschungsgrenzen

H1 liegt unverändert unter `hosts/H1_AMD660M/`; Integritätsgrundlage ist `notes/H1_RELOCATION_2026-10-31.json`. H2-C0 nicht ohne konkreten Anlass wiederholen. Vorbereitete C1-Reproducer und Freigabeflags sind keine Ausführungsgenehmigung. Keine GPU-Experimente, Sicherheitstrigger, Runtime-/Paketinstallationen, Treiber-/Kerneländerungen oder GitHub-Pushes ohne passenden gesonderten Auftrag.
