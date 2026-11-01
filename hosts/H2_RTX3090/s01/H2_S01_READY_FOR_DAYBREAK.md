**Fortschreibung S01 R3:** Maßgeblich für Supervisor/Budget und aktuelle Freigabebindung ist [H2_S01_R3_SUPERVISOR_FIX.md](H2_S01_R3_SUPERVISOR_FIX.md). Die S01-Fälle selbst bleiben bytegleich zu R2. Keine Laufgenehmigung. Die nachfolgende Testbeschreibung stammt aus R2; aktuelle Regressionsergebnisse und Hashes stehen im R3-Bericht.

# H2-S01 bereit zur Daybreak-Prüfung

Stand31.10.2026, **Revision R2 nach Daybreak-Review**. [Korrekturen und Grenzen](REVIEW_R2_CORRECTIONS.md). S01 ist implementiert, hostkompiliert und gelinkt; **kein NVIDIA-/OpenCL-Programm wurde ausgeführt, kein Kernel enqueued**. Die Laufgenehmigung ist weiter offen. H1 bleibt unverändert. Grundlage: Security-Discovery-ZIP und gepinnte C1-Spezifikation; kein neues C0-Inventar.

## Implementierte Fälle und Orakel

| Fall | Übergang | Nachweis bei späterem Lauf |
|---|---|---|
| S01-OWN-API | eigener Setter, Erfolg | gültige APIkontrolle, keine Queue |
| S01-FOREIGN-API | fremder lebender Buffer, −34 | Kontextvalidierung, keine Queue |
| S01-INDEX-API | Index2 bei2 Argumenten, −49 | isolierte Indexkontrolle, keine Queue |
| S01-SIZE-API | sizeof(cl_mem)−1, −51 | isolierte Größenkontrolle, keine Queue |
| S01-CLONE-API | gültiger Setter/Clone, Erfolg | flache Cloneerzeugung, keine Queue; interner Wert nicht direkt beobachtbar |
| S01-OWN-DATA | gültiger Setter | vollständige eigene Referenzdaten/Guards |
| S01-ROLLBACK-DATA | fremder Setter, −34 | Original liest weiterhin A |
| S01-INDEX-DATA | Indexfehler, −49 | Original liest weiterhin A |
| S01-SIZE-DATA | Größenfehler, −51 | Original liest weiterhin A |
| S01-CLONE-CONTROL-DATA | gültiger Setter/Clone | geerbtes A; Rebind nur Clone auf C; Original bleibt A |
| S01-CLONE-DATA | fremder Setter, −34, dann Clone | gleiches Datenoracle nach Fehler; gültige Clonekontrolle separat |

[Normative Matrix](NORMATIVE_CONTRACT.md): R11005–11094 und R11295–11364 am OpenCL-Docs-Pin `0027b7f1a5e97b1bf5256888757479e012a5cb59`. OpenCL3.0 Host-API, CL C1.2 Quelle. −34/−49/−51 dürfen Kernelzustand nicht ändern; −5/−6 sind ausgenommen und liefern einen inconclusive-Stop. Clone kopiert Argumentwerte, nicht den Store. Eigene Referenzen aller6 Buffer bleiben bis nach erfolgreicher Completion erhalten.

Jeder APIstatus wird mit Erwartung protokolliert. Jede unerwartete Setterantwort beendet den Prozess vor Clone, Queue oder Enqueue. Bei negativem Eventstatus kein weiteres Datenoracle. Kein raw/ungültiger/freigegebener Handle, keine UB-Sequenz, kein OOB. Ein falscher Return ist zunächst Konformität; ein Datenmismatch nach garantiertem Nachzustand ist ein zu analysierender Kontextintegritätsbefund. Das wäre noch kein OS-Prozessbruch.

## Kontrollen, Daten und Grenzen

Positive Kontrollen: eigener Setter, eigener Clone ohne vorherigen Fehler, identische kleine Datenfälle. Negative APIkontrollen: Fremdkontext, Index, Größe. Der Offlineparser wird mit falschen Payload-, Anfangs-/Endguards, fehlenden Readbacks und fehlendem Abschluss geprüft. Sechs Buffer mit je160 uint32 (640Bytes), zusammen3840Bytes;2 Kontexte; bei Datenfällen2 Queues;64 Workitems und lokale Größe1; höchstens4 Dispatches. Clone-Datenfälle ergänzen einen begrenzten Reset-Write und die dazugehörige Completion-/Readbackbestätigung sowie einen weiteren Cloneausgabe-Readback. Guards außerhalb der64 aktiven Worte bleiben erhalten. Eingaben A/C/B müssen vollständig unverändert bleiben. Beide Cloneausgaben und Originalausgabe werden vollständig geprüft; Originalausgabe nach Clone-Rebind zuerst vollständig zurückgesetzt und bestätigt; danach Original erneut ausgeführt und Original-/Cloneausgabe nochmals geprüft.

Die Muster sind deterministische eigene Testdaten mit getrennten Objektkennungen, keine geheimen fremden Daten. Jeder frische Prozess wird als eigener Trial protokolliert. Erst Pilot je benötigter Kontrolle/Ziel, danach höchstens5 Prozesse pro Fall inklusive Pilot; nach erster Auffälligkeit keine automatische Wiederholung. Supervisor:60s wall,45s CPU,2GiB beobachtetes Prozessgruppen-RSS,16MiB Output; Eventpoll5s. Host-/Treiberallokationen sind damit nicht auf3840Bytes begrenzt. Prozessende garantiert keine GPU-Preemption.

## Build und spätere Auswertung

Vom Repositoryroot, mit vorhandenen Headers/Compiler/Loader. Der Build führt das Programm nicht aus:

```bash
gcc -std=c11 -O2 -Wall -Wextra -Werror -Wpedantic hosts/H2_RTX3090/s01/h2_s01.c -lOpenCL -ldl -o /tmp/h2_s01
```

Keinen direkten Testlauf aus dieser Anleitung ableiten. Zuerst erneutes Daybreakreview von R2, dann ausdrückliche fallbezogene Freigabe, Binary-/Plan-/Workspace-/Environment-Bindung im Manifest unter `hosts/H2_RTX3090/c1/approvals/`. Keine echte Freigabe liegt vor. Supervisor erkennt die S01-Cases aus `s01/TEST_CASES.json`, bindet dessen Hash (nicht den C1-Kataloghash) und startet ein versiegeltes ELF-Snapshot. CLIparameter: `--execute-approved-plan`, `--approval-note`, `--approval-manifest`, `--case`, `--binary`, neuer `--out` unter `hosts/H2_RTX3090/results/c1/`, `--repeat-number`. Das Reproducerflag allein autorisiert nichts.

Nach einem später separat genehmigten erfolgreichen Datenlauf offline: `python3 hosts/H2_RTX3090/s01/verify_log.py --log <stdout.jsonl> --case <S01-CASE>`. Der Schema2-Parser verlangt die geordnete vollständige Pflichtfolge aus dem Plan, gebundene Objekt-/Kontextidentitäten, Setter-/Clone-/Queue-/Completion-/Read-/Cleanup-Schritte, Daten und Abschluss. Supervisorrun.json, stderr, Status und Provenienz müssen zusätzlich geprüft werden. Exit0=erwartete Beobachtungen;20=Mismatch/Fehler;21=fehlgeschlagener Check aus gemeinsamer Infrastruktur;64=CLI;77=Capability-/Setupskip;78=Ressourcenfehler/inconclusive (auch im Supervisorergebnis). Skip wird vom Supervisor getrennt von Erfolg und Fehler protokolliert.

## Bestätigte Offlineprüfung

[Maschinenlesbarer Nachweis](notes/OFFLINE_VALIDATION.json): strikte GCC-Kompilierung, Hostlink, Clang Static Analyzer, Python-AST erfolgreich;10 Supervisor-End-to-End-Tests;7 S01-Offlinetests über alle11 Fälle;8 bestehende Auditregressionen mit22 B12-Hostassertionen; bestehender C1-Datenoracle erfolgreich. C1 und B12 wurden ebenfalls erneut strikt hostkompiliert. Kein kompiliertes OpenCL-Programm wurde gestartet.

Die End-to-End-Tests starten den echten Supervisor-main mit normaler CLI in einem temporären Gitrepository. Nur ein Testwrapper setzt dort harmlose Referenzdateien statt NVIDIA-Libraries und die Frist auf0,25s. Keine Produkt-CLI für einen Testbypass wurde ergänzt. Testfälle: Ressourcen-/inconclusive-Exit78, erlaubtes ELF, falscher Pfad/Hash ohne Kindstart, Exit7, SIGTERM, Timeout mit ignoriertem SIGTERM und SIGKILL, Skip77, kaputtes ELF mit protokolliertem exec-Fehler. Kontrolliert werden stdout/stderr-/Approval-/Workspacehashes, Ergebnisstatus, HEAD, Dirty-/untracked-Status, Plan-/Binaryhash und versiegelte Ausführung. Diese Tests sind kein OpenCL-Funktionstest und keine Sandboxzertifizierung.

## Offene Daybreak-Prüfung

Bitte Norm-/Fall-/Sourcekonsistenz, Wrong-Success-Stop vor Queue, Clone-Control versus Fehlerarm, Pattern-/Guardlayout, Parservollständigkeit und Supervisorintegration unabhängig prüfen. Technisch ungelaufen bleiben Sourcebuild auf NVIDIA, reales Cloning und tatsächliche Datenwirkung. API-Fälle allein beobachten den gespeicherten Argumentwert nicht. Fehlender Clone-Loaderexport ist ein Setupskip, kein Beleg normativer Optionalität auf OpenCL3.0. Eine private H1-B4/B9-Doppelungsprüfung ist weiterhin offen; H1 nicht neu testen.

Nach Review keine automatische Freigabe von Datenfällen: API-only, gültige Datenbaseline und jeder Fehler-/Clone-Datenarm benötigen passenden Umfang. Keine GPU-Experimente in dieser Vorbereitung; keine Treiberänderung, Installation, Veröffentlichung oder Push.

## Dateihashes

SHA-256 der relevanten Dateien dieses Stands; vollständiges S01-Manifest in `SHA256SUMS.txt`, ohne Selbstreferenz:

```text
8450eea4a26768fde3844c0e319e70bd03ec9f77379fcaa4a276caaf9915a873  hosts/H2_RTX3090/s01/h2_s01.c
7e644016244271bc1e5c83b4779c74a61b11f0c4a19c17b345d873a9387806be  hosts/H2_RTX3090/s01/TEST_CASES.json
b7128ef339c6a584feb17e3b48dc4763d50557ee052d8c2fe583eda393116e2a  hosts/H2_RTX3090/s01/NORMATIVE_CONTRACT.md
b41fc5d6786d321f5b65fee78e64fc68f386a8ccd916acb02c4aea77dbbdeef3  hosts/H2_RTX3090/s01/verify_log.py
3c6319f7bf7ac6ab4d867eae4c0cf68f69e6a6fdb76bdf7ab7eb06e9b9e07509  hosts/H2_RTX3090/s01/tests/cpu_fixture.c
6b193779757e44618081cec11f169f3b600b0037290573d0e1c90c6dba4b4d7c  hosts/H2_RTX3090/s01/tests/test_supervisor_e2e.py
ca7b13cc3c07b81461c7f47781e11472cffd3893763c5dba35c946b067c1af9a  hosts/H2_RTX3090/s01/tests/test_s01_offline.py
f7890c395a6c62e5199eaa34291f620315e7d8914197595609b1d05562c855ee  hosts/H2_RTX3090/c1/reproducers/run_approved_case.py
131566bc6ef021e653400671e87c4dd53884e6addb913ca39eda726881decd35  hosts/H2_RTX3090/c1/reproducers/provenance.py
93631c73bd42a6980ed9acd316c62772a8a5d28261e4c1638335f31230d52728  hosts/H2_RTX3090/c1/reproducers/h2_common.h
bee4e676c72d3add899a189a70a8e486c83faf256e9bc7699ea46117a9193549  hosts/H2_RTX3090/s01/notes/OFFLINE_VALIDATION.json
b16bc53ec51429b607835e1df6c31589c4754f6f1fdc43145bacf614ef0c3c77  hosts/H2_RTX3090/s01/tests/protocol_fixture.py
989a52d5cdbec7d2ee364baec56001cf15a42c61b74c1281035c85d2592860a8  hosts/H2_RTX3090/s01/tests/run_offline_validation.py
5603eca0bb3bd7af46cd62314f7b3ed203b47715d85d0f3533866fc06bf27acd  hosts/H2_RTX3090/s01/REVIEW_R2_CORRECTIONS.md
31d9beea3af7210d2e8a6c1c6d05c0fa6ff337417021a721164a60af074d8c27  hosts/H2_RTX3090/s01/notes/S01_DAYBREAK_REVIEW_R1.md
```
