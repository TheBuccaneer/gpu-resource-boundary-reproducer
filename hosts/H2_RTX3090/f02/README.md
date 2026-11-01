# F02-CLONE-SNAPSHOT-REVERSE v1

Nur Offline-Vorbereitung. Neuer getrennter Fall, keine Ausführungsgenehmigung.
Die bindende Fachvorgabe steht in den unveränderten F02_OFFLINE_ASSESSMENT.md
und F02_IMPLEMENTATION_ORDER.md unter results/c1/s01_data_pilot_20261101/
S01-CLONE-CONTROL-DATA_REBIND/review/.

## Eingefrorener Vertrag

Plan PLAN_V1.json; Spezifikationspin 0027b7f1a5e97b1bf5256888757479e012a5cb59,
opencl_runtime_layer.asciidoc 11295–11349. clCloneKernel kopiert Kernelzustand
und zuletzt gesetzte Argumentwerte in eine eigene Kernelinstanz; referenzierter
Bufferspeicher wird nicht kopiert. Ein gültiges vollständig gebundenes Original
erzeugt bei Erfolg einen direkt enqueuebaren Clone. Alle verwendeten Setter,
Queries, Enqueues, Completion-/Cleanup-Aufrufe müssen CL_SUCCESS ergeben;
alle elf Events müssen CL_COMPLETE erreichen. Original-zu-Clone-Unabhängigkeit
folgt aus der eigenen Instanz mit kopierten Argumentwerten. Die Norm erläutert
die Gegenrichtung ausdrücklich; diese Vorbereitung erfindet kein zusätzliches
verbatim Normzitat. Flache Kopie bedeutet legitime gemeinsame Bufferspeicher,
keine legitime Kopplung späterer Kernelargumentbindungen.

Ein Kontext/ein Gerät/eine In-order-Queue; fünf verschiedene lebende eigene
640-Byte-Buffer mit abgefragtem Kontext und Größe. Kernelkontext und -programm,
Queuekontext/-gerät/-properties, Eventkontext/-queue/-commandtype werden geprüft.
Eigene Generationen unterscheiden wiederverwendete Handlewerte von Events.
Alle Buffer bleiben bis nach sämtlichen Readbacks und Kernel-Releases lebend.
Keine Foreign-Setter, ungültigen Argumente, OOB-Zugriffe oder parallelen Setter.

Original A1/O0; Clone erzeugen, **kein einziger Clone-Setter**; Clone1 und
O0:first; O0 vollständig zurücksetzen und Completion sowie O0:reset lesen;
nur Original A3/O1; Clone2 und O0:second; Original und O1:final; A1, A3, O2
vollständig lesen. Exakt drei Dispatches à 64 Workitems/local1, ein Resetwrite,
elf Completion-Sequenzen, sieben Readbacks à160 =1120 Worte. Outputaktive
Indizes16..79, Guards0..15/80..159. A1/A3 und unbenutztes O2 unverändert.
Frische Nonce und Salt wurden einmal mit secrets.randbits erzeugt und für diesen
einen zukünftigen Trial unabhängig in C, Plan und mathematischem Oracle fixiert.
Die Wiederholungsnummer ist ausschließlich1; kein zweiter Trial vorgesehen.

Die unabhängigen Python-Orakel lesen weder C noch Plan noch vom Logger behauptete
Sollwerte. Sie akzeptieren nur den vollständigen geordneten Stream inklusive
Objektidentitäten, genau vier Original-Settern, Reset, Events, allen1120 Werten
und Cleanup/Terminal. Realmodus verlangt OPENCL_RUNTIME und die drei gebundenen
Bibliotheksmappings; CPU-Protokolle dürfen niemals als Runtime-Evidenz bestehen.
Abnormale API/Event-/Protokollfolgen sind INCONCLUSIVE, Datenabweichungen zunächst
UNVERIFIED_DATA_CANDIDATE. Ein erfolgreicher einzelner Lauf trägt nur die beiden
beobachteten Zustandsübergänge in einem Kontext, keine vollständige Konformität
oder Prozess-/Mandantengrenze. Die sieben festgelegten Messpunkte beweisen keine
Abwesenheit beliebiger zwischenzeitlicher oder später überdeckter Writes.

## Offline bauen und prüfen

Aus dem Repository-Wurzelordner, ohne Start des Produktionsbinaries:

```
/usr/bin/python3 hosts/H2_RTX3090/f02/build_offline.py
/usr/bin/python3 hosts/H2_RTX3090/f02/tests/test_f02.py
/usr/bin/python3 hosts/H2_RTX3090/f02/tests/test_ledger.py
/usr/bin/python3 hosts/H2_RTX3090/f02/tests/test_supervisor_f02.py
```

GCC strenge Warnungen/Link, Clang-Analyse, readelf ohne Runtime-Ausführung.
Nur das separat gegen tests/fake_opencl.c gelinkte CPU-Programm wird ausgeführt;
seine NEEDED-Liste enthält ausschließlich libc. Es gibt keine OpenCL-/CUDA-
Abhängigkeit. Mutationen: falsche geerbte Args0/1, gekoppelte Bindungen, zweiter
Dispatch ohne Write, nicht wirksamer Reset, Guards/Inputs/Sentinel, Fehler/Events,
jeder der1120 Werte sowie entfernte/doppelte/vertauschte Protokollzeilen.

Supervisor-Integration verwendet isolierte Git-Workspaces und private Ledgers.
Nur dort werden synthetische historische IDs und CPU-Origin für den integrierten
Oracle-Schritt injiziert. test_ledger prüft separat unveränderte vollständige
Produktions-Auditlogik gegen zehn komplette private Receipt-/Approval-/Raw-
Artefakte. Keine dieser Testinjektionen ist als Produktionsoption verfügbar.

## Vor Start: unabhängiges Audit und separates neues Nutzer-GO

Neue ID H2-F02-CLONE-SNAPSHOT-REVERSE-PILOT-20261101-01, Vorlage
c1/approvals/f02_clone_snapshot_reverse_pilot_20261101.json, zunächst
pending_explicit_approval, Evidence null. Ein technisches PASS aktiviert sie nicht.
Read-only-Preflight (prüft, reserviert nicht):

```
/usr/bin/python3 -I /home/rock/projects/gpu-resource-boundary-reproducer/hosts/H2_RTX3090/f02/run_case.py
```

Erst nach unabhängiger fachlicher Prüfung UND ausdrücklichem neuem Nutzer-GO
nur status/approval_evidence dieser neuen Vorlage dokumentieren und einmal:

```
/usr/bin/bash --noprofile --norc -p /home/rock/projects/gpu-resource-boundary-reproducer/hosts/H2_RTX3090/f02/START_AFTER_EXPLICIT_GO.sh
```

run_case.py übernimmt unverändert die R3-Ressourcenbegrenzung, Prozessgruppen-
überwachung, versiegelte Binarykopie und atomare dauerhafte Ledgerreservierung.
Neuer Falladapter nötig, da der eingefrorene S01-R3-Fallloader F02 nicht kennt;
alte R3-/S01-/F01-Dateien werden nicht erweitert. Preflight bindet genau zehn
historische Receipts samt vollständigen Approval-/Run-/Raw-/Workspace-Nachweisen,
keine beliebigen Ergänzungen. Die neue Vorlage bindet Binary/Plan/Workspace,
Git-Stand, Runtime-SHA, alle Runtime-Auswahlvariablen und Systemtools; nur PATH
wird gemäß bestehendem Stable-Environment-Vertrag auf /usr/bin:/bin gesetzt.
Ausgabeordner fest results/c1/f02_pilot_20261101/F02-CLONE-SNAPSHOT-REVERSE.
Kein Override für Fall/Binary/Output/Repeat. Ein verbrauchter Trial bleibt verbraucht.

Stop bei jeder Bindungs-/Ledger-/API-/Objekt-/Event-/Reset-/Datenabweichung,
Timeout, Signal, Ressourcen- oder Logfehler. Keine Hashübernahme, kein Reset,
keine Ergebnisbereinigung zum Neustart, kein Retry. 60s Wall/45s CPU, 2GiB
beobachtetes Prozessgruppen-RSS, 16MiB Logs, 5s je Event. Prozessabbruch garantiert
keine harte GPU-Präemption. Nach normalem Kindende0 prüft der Adapter das
unabhängige Oracle automatisch; nur PASS_OBSERVED_STATE kann Supervisor-PASS
tragen. Für das anschließende Audit zusätzlich separat im echten Runtime-Modus:

```
/usr/bin/python3 -I /home/rock/projects/gpu-resource-boundary-reproducer/hosts/H2_RTX3090/f02/verify_log.py --log /home/rock/projects/gpu-resource-boundary-reproducer/hosts/H2_RTX3090/results/c1/f02_pilot_20261101/F02-CLONE-SNAPSHOT-REVERSE/stdout.jsonl
```

run.json, stdout/stderr, unabhängiges Oracle, Approval-/Workspace-Snapshots,
Receipt, Hashes und Ressourcen prüfen und nach genau einem Trial beenden.
