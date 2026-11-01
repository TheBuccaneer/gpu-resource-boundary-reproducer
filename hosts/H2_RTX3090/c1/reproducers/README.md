# Vorbereitete Reproducer — noch nicht ausgeführt

`h2_b12_query.c` fragt nur Plattform-/Geräteinformationen ab; `h2_c1.c` enthält exakt die 26 benannten C1-Fälle. Der B7-Zweig erzeugt keine Queue und enthält keinen Enqueue. `h2_common.h` erzwingt die bekannte NVIDIA3090-/Treiber-/PCI-Identität, protokolliert Librarymaps und begrenzt Eventpolling. Kein automatischer Testlauf und kein Zugriff auf H1-Dateien.

Die beiden C-Dateien wurden als CPU-Hostcode mit GCC nur zu temporären Objektdateien kompiliert, nicht gelinkt oder gestartet. [Prüfnachweis](../notes/STATIC_COMPILE.json). Die CPU-Selbstprüfung des Offline-Datenoracles greift ebenfalls nicht auf OpenCL zu. **Hostkompilierung ist kein erfolgreicher OpenCL-Build oder funktionaler Test.**

## Späterer Build nach gesondertem Laufauftrag

Aus dem Repository, in ein neu gewähltes lokales Buildverzeichnis; kein GPU-Aufruf durch diese beiden Host-Buildkommandos:

```bash
gcc -std=c11 -O2 -Wall -Wextra hosts/H2_RTX3090/c1/reproducers/h2_b12_query.c -lOpenCL -o /tmp/h2_b12_query
gcc -std=c11 -O2 -Wall -Wextra hosts/H2_RTX3090/c1/reproducers/h2_c1.c -lOpenCL -o /tmp/h2_c1
```

Eine Ausführung braucht eine ausdrückliche Freigabe des Pakets und einen Supervisor nach dem Arbeitsplan. Nur dann etwa `/tmp/h2_c1 --approved-run BL-KERNEL`, einzeln und mit stdout/stderr/Exitcode-Aufzeichnung. Das Wort `approved` im Flag erteilt keine Erlaubnis. Kein automatisches Durchlaufen der Case-Liste und keine Übernahme einer in dieser Dokumentation gezeigten Befehlszeile als Laufauftrag.

Die Codefrist von 5 s begrenzt nur das Eventpolling. Einzelne Treiberaufrufe, Source-Build und Freigaben benötigen zusätzlich die äußere 60-s-Prozessfrist; Prozessbeendigung garantiert keine GPU-Preemption. Kein absichtlich offenes User-Event auf einem Fehlerpfad freigeben. Bei unerwartetem Status wird der Prozess unmittelbar beendet, ohne einen gefährlichen Folge-Dispatch oder unbeschränktes `clFinish`.

Der B12-Reproducer sammelt voneinander unabhängige Infoquery-Rückgaben innerhalb eines Prozesses, auch wenn eine davon den Normerwartungen widerspricht. Sein Exitcode20 kennzeichnet solche Mismatches; fehlende Geräte-/Setupvoraussetzungen stoppen ebenfalls. Er startet keine Semaphore-Objekttests.

`oracle.py --self-check` prüft nur synthetische Hostvektoren. `oracle.py --log <stdout.jsonl> --mode kernel` bzw. `--mode buffer` prüft später die vollständigen gespeicherten Daten. Für setter-/query-only-Fälle gibt es keine erfundenen Gerätedaten. Autoritative Laufinterpretation benötigt zusätzlich die API-/Event-/Prozess- und Provenienzdaten aus dem Arbeitsplan.

## Vorbereiteter Einzelfall-Supervisor

`run_approved_case.py` startet ausschließlich mit `--execute-approved-plan`, einer Freigabenotiz, genau einer Case-ID, Wiederholungsnummer1–5, Hostbinary und neuem Ergebnisordner. Es gibt keine Schleife über Fälle oder Wiederholungen. Er prüft gezielt die ursprünglichen NVIDIA-/Loaderdateihashes, protokolliert Quell-/Binary-/Plan-Pins, stdout/stderr und Exitstatus und setzt die geplanten Prozessgrenzen um. Das ist ein statisch geprüfter Entwurf; er wurde hier nicht zum Start eines OpenCL-Prozesses verwendet.

## Auditfortschreibung 31.10.2026

Der aktuelle [Auditbericht](../../security_discovery/C1_AUDIT_CORRECTIONS.md) beschreibt vollständige B12-Listenvalidierung, ELF-/Freigabemanifestbindung und Git-/Contentprovenienz. Der Supervisor benötigt jetzt zusätzlich `--approval-manifest`; CLI-Flags sind keine Autorisierung. CPU-Regressionen sind dokumentiert, kein OpenCL-Lauf. Frühere statische Nachweise bleiben datierte Historie.
