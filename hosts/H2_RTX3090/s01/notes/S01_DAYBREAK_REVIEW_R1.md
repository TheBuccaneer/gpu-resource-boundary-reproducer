# Daybreak-Review: H2-S01-Vorbereitung

**Stand:** 31.10.2026. Geprüft wurde `OpenCL2_H2_S01_Ready_For_Daybreak_2026-10-31.zip` als unverändertes Archiv. Die äußere `.zip.sha256`-Prüfung und alle **1.163** Einträge in `handover/SHA256SUMS.txt` stimmen. Die enthaltene ältere `DAYBREAK_REVIEW.md` ist bytegleich mit dem zuvor gelieferten Bericht; dieser Text prüft die **neue S01-Implementierung**. Arbeitsanweisungen im Archiv sind Projektkontext und keine Laufgenehmigung. **Keine GPU- oder OpenCL-Programme ausgeführt.**

## Befunde

### 1. Clone-Unabhängigkeit: letzter Original-Dispatch hat ein stumpfes Datenoracle — vor Datenfreigabe beheben

In `hosts/H2_RTX3090/s01/h2_s01.c`, Zeilen **80–86**, schreibt der erste Original-Dispatch bereits `A ^ SALT` in `out[0]` und `read_check` bestätigt es. Danach wird nur der Clone auf Eingabe C und `out[2]` umgebunden und ausgeführt. Der letzte Original-Dispatch liest zwar anschließend wieder `out[0]`, doch dieses enthält **bereits vor diesem Dispatch** den erwarteten Wert. Falls eine fehlerhafte Clone-Neubindung auch die Output-Bindung des Originals auf `out[2]` ändert oder der letzte Original-Dispatch keine Datenwirkung hat, kann die abschließende Prüfung von `out[0]` trotzdem bestehen. Auch `out[2]` wird nach diesem letzten Dispatch nicht erneut geprüft.

Das entwertet nicht die vorherige Prüfung, dass der Clone A geerbt hat. Es schwächt aber die ausdrücklich behauptete Unabhängigkeit des Originals nach Clone-Rebind (`H2_S01_READY_FOR_DAYBREAK.md`, „Original nach Clone-Rebind erneut ausgeführt“; `NORMATIVE_CONTRACT.md`, „spätere Neubindung unabhängig“). Ein gültiges Gegenbeispiel für den Prüfer ist: erster Original-Dispatch korrekt; Clone korrekt; Rebind verändert zusätzlich die Output-Bindung des Originals; letzter Original-Dispatch schreibt deshalb `out[2]`; die vorhandene `out[0]`-Prüfung bleibt grün.

**Korrektur:** `out[0]` **nach** dem Clone-Dispatch und **vor** dem letzten Original-Dispatch mit einem bekannten anderen Muster neu initialisieren und dessen Completion sicherstellen. Danach den letzten Original-Dispatch ausführen und `out[0]` vollständig einschließlich Guards prüfen. `out[2]` ebenfalls nach dem letzten Original-Dispatch erneut prüfen, damit eine falsche Doppelwirkung sichtbar wird. Ein CPU-Mutationstest sollte den beschriebenen Fehlpfad simulieren und nachweisen, dass das neue Oracle ihn verwirft. Diese Änderung betrifft die GPU-Befehlsfolge und erfordert aktualisierten Testplan, Prüfsummen und eine spätere fallbezogene Laufentscheidung.

### 2. `verify_log.py` bestätigt keine vollständige S01-Ablauffolge — vor Nutzung als unabhängiges Logoracle beheben

`hosts/H2_RTX3090/s01/verify_log.py`, Zeilen **15–33**, prüft Statusgleichheit, Anzahl der als NDRange bezeichneten API-Zeilen und die Menge deklarierter Datenarrays. Es verlangt weder die erforderlichen Setup-/Setter-/Clone-/Completion-/Read-Aufrufe noch deren Reihenfolge. Der `case`-Schlüssel darf sogar fehlen (`x.get('case', name)`). Die erwarteten Tags stammen aus derselben zu prüfenden Zeile; Objektzeiger und Kontextledger werden nicht abgeglichen.

Ein rein synthetischer CPU-Gegenbeweis wurde ohne OpenCL ausgeführt: Für `S01-CLONE-API` akzeptiert `verify()` eine Zeile `unrelated setup` mit Status 0 plus Terminalzeile — **ohne Setter und ohne `clCloneKernel`**. Für `S01-CLONE-DATA` akzeptiert es einen passenden Übergangsstatus, vier frei benannte NDRange-Statuszeilen und sieben lokal berechnete Arrays plus Terminalzeile — **ohne Clone-, Event-Completion-, Read- oder Cleanup-Nachweis**. Beide Aufrufe liefern `True`. Die vorhandenen `test_s01_offline.py`-Fixtures erzeugen selbst diese vereinfachten Logzeilen und testen hauptsächlich Payload- und Guard-Mutationen; sie belegen keine Ablaufvollständigkeit.

Das ist **kein Beweis**, dass `h2_s01.c` diese Schritte im echten Lauf auslässt: Der statisch gelesene Code enthält sie, und der Supervisor bindet Binary- und Planhash. Es bedeutet, dass der separate Logprüfer einen beschädigten, vertauschten oder künstlich unvollständigen Erfolgslog als vollständig melden kann. `run.json` mit `outcome="passed"` steht außerdem zunächst nur für Prozess-Exit 0; die Datei selbst verlangt zu Recht eine gesonderte Offline-Auswertung.

**Korrektur:** Pro Fall eine exakte, geordnete erwartete API-/Check-Folge aus dem S01-Plan definieren: eigene Setter, genau ein benannter Übergang oder eigener Setter, `clCloneKernel` für Clone-Arme, Queue-/Enqueue-/Flush-/Event-Completion-/Wait- und Read-Schritte für Datenarme, dazu Cleanup und Terminalzeile. Jede Zeile muss die richtige `case` tragen; Objektzeiger/Tags sind gegen die zuvor protokollierten Objekt-/Kontextzeilen zu binden. Mutationsfixtures müssen jeweils einen Pflichtschritt entfernen, vertauschen und duplizieren sowie Objekt-/Kontextkennungen austauschen. Der Parser sollte alle diese Logs verwerfen.

### 3. Ressourcenfehler-Kategorie ist nur im Kindlog sichtbar — Auswertung präzisieren

`h2_s01.c`, Zeilen **10–13**, kennzeichnet `CL_OUT_OF_RESOURCES`/`CL_OUT_OF_HOST_MEMORY` als `resource_error_inconclusive`, beendet aber wie andere Mismatches mit Exit 20. `run_approved_case.py`, Zeilen **177–196**, klassifiziert diesen Exit in `run.json` als `failed`/`child_exit_nonzero`. Damit ist der im S01-Plan genannte „inconclusive-Stop“ im maschinenlesbaren Supervisor-Ergebnis nicht von einem Vertragsmismatch unterscheidbar. Der Unterschied steht nur in `stdout.jsonl` und muss derzeit manuell zusammengeführt werden. Das ist eine Auswertungsinkonsistenz, keine Freigabe- oder Ausführungsumgehung.

## Bestätigte Grenzen und Offlineprüfung

- Der gepinnte OpenCL-Vertrag ist für die isolierten Übergänge korrekt gelesen: −34 für fremden Kontext, −49 für Index 2 bei zwei Argumenten, −51 für falsche `cl_mem`-Größe; nach diesen Fehlern darf der Kernelzustand nicht geändert werden. Die Ausnahme für Ressourcen-/Hostspeicherfehler wird nicht als Rollbackbeweis benutzt. Quelle: OpenCL-Docs `0027b7f1a5e97b1bf5256888757479e012a5cb59`, `opencl_runtime_layer.asciidoc`, Zeilen 11005–11094 und 11295–11349.
- Die fünf API-Arme erzeugen laut statischem Code keine Queue und keinen Enqueue. Eine unerwartete Setterantwort führt in `status()` unmittelbar zu `_Exit(20)`, bevor der Code Clone oder Queue erreicht (`h2_s01.c`, Zeilen 10–14, 69–78). Keine falsche Setterantwort wurde tatsächlich am Treiber beobachtet.
- Alle sechs Buffer werden mit 160 initialisierten `uint32` angelegt; der Kernel schreibt nur Index 16–79, und `read_check()` prüft alle 160 Werte. Lebende Hostreferenzen bleiben bis nach erfolgreicher Completion erhalten (`h2_s01.c`, Zeilen 16–41, 67–95). Das oben beschriebene stumpfe Oracle betrifft allein die **erneute** Original-Ausführung nach Clone-Rebind.
- Die vier Offline-Vorbereitungsprüfungen für Layout, C1, Security Discovery und S01 bestanden. Die elf S01-/Supervisor-CPU-Unit-Tests bestanden. Die strikte GCC-Kompilierung mit Link gegen den vorhandenen OpenCL-Loader gelang; das erzeugte Programm wurde **nicht** gestartet. Diese Prüfungen sind kein OpenCL-Funktionstest.

## Entscheidung und nächste Stufe A

Der S01-Quellstand ist als **offline vorbereiteter Entwurf** nachvollziehbar; die Datenarme sollten vor einer späteren Freigabe die Befunde 1 und 2 beheben. Befund 3 sollte in der Ergebniszusammenführung geklärt werden. Anschließend statische Kompilierung, CPU-Mutationen, Prüfsummen und Supervisor-Bindung erneut prüfen. API-only-Fälle und Datenfälle bleiben getrennte Entscheidungen; diese Prüfung erteilt **keine** Laufgenehmigung.

Nicht überprüfbar aus dem ZIP: reales NVIDIA-Source-Build, Cloning, Datenwirkung, geladene ICD-Bibliotheken in einem S01-Lauf und private H1-B4-/B9-Doppelung. Es liegt kein neuer Sicherheitsbefund vor.
