# S01: eingefrorener Vertrag

Geprüft am31.10.2026 anhand der lokal gepinnten OpenCL-Docs `0027b7f1a5e97b1bf5256888757479e012a5cb59`, Datei `../c1/sources/opencl_runtime_layer.asciidoc`. OpenCL3.0 ist H2s Host-API; Kernelquelle benutzt CL C1.2. Die vereinigte Quelle enthält auch neuere Versionen; deren gesonderte3.1-local-Argument-Regel wird hier nicht verwendet. CTS ist ergänzende Implementierungsevidenz, keine normative Autorität.

| Prüfung | Vertrag / Fundstelle | Erwartung |
|---|---|---|
| Gültiger Buffer-Setter | R10939–11006 | CL_SUCCESS; argument data copied |
| Lebender fremder Kontext | R11005–11006,11033–11035 | CL_INVALID_CONTEXT −34 |
| Index2 bei genau2 Argumenten | R11030–11032 | CL_INVALID_ARG_INDEX −49 |
| Bufferargumentgröße sizeof(cl_mem)−1 | R11056–11060 | CL_INVALID_ARG_SIZE −51; gültiger Objektpointer, sonstige Parameter gültig |
| Fehlernachzustand | R11086–11094 | Zustand nur bei −5/−6 veränderbar; nach −34/−49/−51 alter gültiger Zustand |
| Argumentlebenszeit | R11010–11021 | Kernel/Clone behalten Buffer nicht für Anwendung lebend; eigene Referenzen bis nach Completion |
| Clone | R11295–11349 | Seit API2.1; flache Argumentkopie; Source unverändert; spätere Neubindung unabhängig; referenzierter Store bleibt geteilt |
| Clone-Erfolg | R11351–11364 | CL_SUCCESS plus neuer nichtnull Kernel; Ressourcenfehler liefern keinen Zustandsbeweis |
| Completion | R12405–12440 | Nur CL_COMPLETE liefert normales Datenoracle; abnormaler Status beendet Fall |

Kernelkontext und Bufferkontexte werden vor dem Übergang abgefragt und mit dem Ledger verglichen. Alle6 Buffer bleiben lebend. Kein ungültiger/raw/freigegebener Handle wird benutzt. Es gibt keine konkurrierenden Setter, kein OOB, keine SVM-Lifetime, keine uninitialisierten Reads. API-Fälle erzeugen keine Queues und führen keine Enqueues aus. Sie können den internen gespeicherten Argumentwert nicht direkt abfragen; ein erwarteter Return allein ist kein Rollbackbeweis.

Datenfälle sind fertig implementiert, aber ungestartet: gültige eigene Eingabe A, Ausgaben mit Guards und vollständige Hostreferenz. Fremdsetter-Wrong-Success beendet den Prozess vor Clone und Queue/Dispatch. Ressourcenfehler werden als inconclusive protokolliert, nicht als Rollbackverletzung. Sonstige Mismatches sind zunächst Vertrag-/Setupkandidaten für Daybreak; kein automatischer Vulnerabilityclaim.

Clone-Gate: Platform-APIversion mindestens2.1 und Loaderexport `clCloneKernel`; fehlender Export ist ein Setup-/Capability-Skip, kein Beweis, dass die API auf OpenCL3.0 normativ optional sei. Kompilerverfügbarkeit separat geprüft. Auf dem gepinnten H2-OpenCL3.0-Pfad ist die Clone-API zu erwarten; die reale Funktionsfähigkeit bleibt ungeprüft. Kein stiller Fallback auf einen anderen Host.

R2: Der Clone-Unabhängigkeitstest setzt die Originalausgabe nach Clone-Dispatch vollständig zurück und bestätigt das Reset vor dem letzten Originaldispatch; danach Original- und Cloneausgabe erneut prüfen. Ressourcenfehler −5/−6 stoppen mit78/inconclusive. Schema2-Pflichtfolgen stehen im Fallkatalog; [Reviewkorrekturen](REVIEW_R2_CORRECTIONS.md).
