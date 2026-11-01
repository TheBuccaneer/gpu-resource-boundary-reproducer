# H2-S01 R3 — Wiederholungsbudget des Supervisors

**Urteil: PASS für die getestete lokale Budgetdurchsetzung.** Stand31.10.2026. Dies bereitet eine anschließende explizite Freigabe der fünf API-only-Fälle vor; keine Laufgenehmigung wurde erzeugt. Kein NVIDIA-/OpenCL-Programm und kein GPU-Kernel wurde gestartet. S01-C-Reproducer, Fallplan, Schema2-Logoracle und H1-Dateien bleiben bytegleich zu R2.

Grundlage: [Daybreaks R2-Finalreview](notes/S01_R2_FINAL_SECURITY_REVIEW.md), [R2-Input und unveränderte Dateien](notes/R3_INPUT_PROVENANCE.json). Die Korrektur betrifft `c1/reproducers/run_approved_case.py`, `provenance.py` und CPU-Supervisortests. Begleitend wurden Nachweise, Manifeste und Übergabeeinstiege fortgeschrieben.

## Dauerhafte Registrierung vor Kindstart

Das Ledger liegt fest unter `/home/rock/.local/state/opencl2/trials-v1/ledger.sqlite3`, außerhalb des Repositories und aller `--out`-Ordner. Der Produktpfad wird aus der OS-Kontodatenbank anhand der UID bestimmt, nicht aus `HOME`, `XDG_STATE_HOME`, einem CLIargument oder dem Checkoutpfad. Ein neuer Ausgabeordner, Löschen von Ergebnissen oder ein weiterer Checkout desselben Kontos wählt kein anderes Ledger.

Nach den vorhandenen Approval-/Binary-/Plan-/Workspace-/Libraryprüfungen, aber **vor Popen**, registriert `reserve_trial()` die Kombination `(approval_id, case_id, repeat_number)` als dauerhaft verbraucht. SQLite `BEGIN IMMEDIATE` serialisiert parallele Schreiber; der zusammengesetzte Primärschlüssel lässt die Kombination nur einmal zu. Der Commit verwendet DELETE-Journal und `synchronous=EXTRA`; Datei und Verzeichnis werden vor Kindstart zusätzlich fsync-gesichert. Locktimeout, I/O-/Syncfehler oder jede andere Ledgerablehnung verhindern den Kindstart. Das Runprotokoll erhält `trial_ledger_rejected`, Fehlertext, null-Kindreturncode und leere Kindausgabe.

Eine reservierte Nummer bleibt verbraucht, auch wenn der Supervisor nach Commit abstürzt, Popen/exec fehlschlägt, der Kindprozess abstürzt oder ein Timeout folgt. Es gibt keine Freigabe-/Lösch-/Retryfunktion. Das ist bewusst konservativ: Auch ein Start, der nach Commit gar nicht mehr zustande kommt, verbraucht die Nummer. Ein weiterer Versuch benötigt eine neue ausdrückliche Entscheidung und eine neue Approval-ID; derselbe Approvalname darf nicht nachträglich mit einem anderen Manifestinhalt erweitert werden.

## Ledgerprüfung und Provenienz

Vor jeder Registrierung: absolute feste Pfadbindung, keine Symlinks, privates UID-eigenes Verzeichnis0700 und reguläre Datei0600 mit einem Link; Größenlimit64MiB. Öffnen ausschließlich im SQLite-Modus `rw`, der fehlende Dateien nicht erzeugt. Geprüft werden SQLiteintegrität, die vollständige benutzerdefinierte Schemaform einschließlich unerwarteter Trigger/Views/Indexes, Ledger-UUID/Version und sämtliche gespeicherten Receipthashes/Schlüssel. Fehlende oder beschädigte Daten führen zum Abbruch. Ein gesperrtes Ledger wartet maximal5s auf den Writerlock und bricht dann ab.

Jedes echte Freigabemanifest muss zusätzlich `ledger_path` und `ledger_id` enthalten. Eine einmalige bewusste Erstprovisionierung wurde für R3 vorgenommen: [Provisionierungsbeleg](notes/R3_LEDGER_PROVISIONING.json). Der Supervisor provisioniert oder ersetzt niemals selbst. Ein ausdrücklich neu erzeugtes Ledger bekommt eine neue UUID; alte Manifeste sind damit unbrauchbar. Das Liveledger wird **nicht** ins Übergabe-ZIP kopiert und darf niemals aus einem ZIP oder früheren Snapshot zurückgespielt werden.

`run.json` enthält Pfad/UUID/Version, Reservierungszeitpunkt, Kombination, Persistenzmechanismus und einen kanonischen SHA-256-Receipt. Dieser bindet das tatsächlich gelesene Approvalmanifest, freigegebenes ELF-Snapshot, Binarypfad, gewählten C1- oder S01-Plan, Workspacecontent sowie Git-HEAD/Branch/Dirty-/Status-/Diffprovenienz. Bestehende Bibliotheks-, Environment- und Vor-/Nachzustandsprüfungen bleiben erhalten. Die Ledgerregistrierung erzeugt keine Selbstreferenz in der Workspace-ID, weil das Ledger außerhalb des Arbeitsbaums liegt. Ein Gesamtdateihash des veränderlichen Liveledgers ist kein dauerhafter Approvalpin; UUID und unveränderliche Trialreceipts übernehmen diese Aufgabe.

Die fünf vorgesehenen API-only-Fälle bleiben `S01-OWN-API`, `S01-INDEX-API`, `S01-SIZE-API`, `S01-FOREIGN-API`, `S01-CLONE-API`. Es wurden keine neuen echten Freigabemanifeste ausgestellt. Vor einem späteren Start muss das endgültig gebaute Binary mit dem dann eingefrorenen Workspace und dem unveränderten S01-Plan ausdrücklich neu gebunden werden; Pilotnummer1 zuerst, spätere Nummern nur im ausdrücklich genehmigten Umfang. Die sechs Datenfälle erhalten dadurch keine Mitfreigabe.

## CPU-Nachweise

Befehl: `python3 hosts/H2_RTX3090/s01/tests/test_supervisor_e2e.py`.

**24 CPU-End-to-End-Tests bestanden**, einschließlich aller zehn bisherigen Regressionen. Zusätzlich geprüft:

- erster freigegebener Start erfolgreich; identische Kombination mit anderem Ausgabeordner und nach Ergebnislöschung abgelehnt;
- vier parallele Supervisorprozesse mit identischer Kombination: genau ein Kindstart, ein Ledgerdatensatz, Exitcodes einmal0 und dreimal20;
- die Ablehnung erfolgt vor Popen: Der isolierte Testwrapper zählt ausschließlich Aufrufe des versiegelten Testbinary-Startpfads; die Zahl bleibt bei Replay unverändert, stdout ist leer und Kindreturncode null;
- Timeout, gewaltsamer Supervisorabbruch und exec-Fehler verbrauchen die Nummer dauerhaft;
- zwei ausdrücklich erlaubte Nummern1/2 funktionieren jeweils einmal, Replay und Nummer3 außerhalb des Budgets scheitern;
- fehlendes, physisch beschädigtes, schreibgeschütztes oder schemafalsches Ledger, falsche UUID und unerwarteter löschender Trigger verhindern jeden Kindstart;
- nach einem injizierten fsync-Fehler **nach** dem Commit bleibt die Reservierung belegt;
- eine Approval-ID kann nicht durch ein nachträglich verändertes Manifest neues Budget erhalten.

Die Testwrapper substituieren nur im temporären CPU-Testrepo Ledgerpfad, harmlose Libraryreferenzen, Kurzfrist und Popen-Beobachter. Produktcode bekommt keine CLIoption zur Ledgerumleitung. CPU-Testapprovals und Testledger werden ausschließlich temporär angelegt; kein Produktionsbudget wurde verbraucht.

Abschlusslauf: `python3 hosts/H2_RTX3090/s01/tests/run_offline_validation.py`. [Vollständige Befehle und Resultate](notes/OFFLINE_VALIDATION.json): 24 Supervisor-E2E,7 unveränderte S01-Offlineprüfungen,8 bestehende Auditregressionen einschließlich22 B12-Hostassertionen sowie C1-Datenoracle bestanden. GCC-Hostkompilierung/-link, Clang-Analyse und Python-AST erfolgreich; das verlinkte OpenCL-Programm wurde nicht ausgeführt. Paket-, H1-/C0- und Fallkonsistenzprüfungen werden erneut am entpackten ZIP ausgeführt.

## Grenzen

Dies ist ein lokaler Schutz vor versehentlichem Replay und konkurrierenden Supervisoraufrufen unter dem bestehenden Hostvertrauen. Lokales ext4 wurde für den Ledgerstandort bestätigt; funktionierendes POSIX-/SQLite-Locking und wahrheitsgemäßes fsync werden vorausgesetzt. Kein Schutz gegen einen böswilligen Kontoeigentümer/Administrator, der Supervisor, Approval oder einen vollständigen früheren Ledgerstand absichtlich manipuliert. Für mehrere OSkonten oder Hosts wäre ein gemeinsamer vertrauenswürdiger Registrierungsdienst erforderlich; diese Ausführungstopologie wird hier nicht behauptet. Zustandsverlust erfordert einen bewussten administrativen Entscheid und neue Approvalbindung, keine automatische Wiederfreigabe.

PASS bezieht sich auf diese Budgetkorrektur und die dokumentierten Offline-Nachweise. Reale NVIDIA-Funktion, GPU-Datenwirkung und mögliche Schutzdomänenbefunde bleiben ungetestet. Keine Treiberänderung, Installation, Veröffentlichung oder Push.

## Hashes

Aktuelle SHA-256-Werte der geänderten Werkzeuge/Tests und des unveränderten S01-Plans stehen in [notes/R3_FILE_HASHES.json](notes/R3_FILE_HASHES.json). Vollständige C1-/S01- und ZIPmanifeste ergänzen diese Auswahl; die finalen Hashes beziehen sich auf den tatsächlich gepackten Stand.
