# Drei C1-Auditkorrekturen

## Vollständige B12-Listenprüfung

`h2_b12_query.c` fragt Plattform und alle Plattformgeräte (CL_DEVICE_TYPE_ALL, maximal16) nach Semaphoretypen sowie Import-/Exportlisten ab. Pure Hostvalidierung in `b12_validation.h` prüft Erfolg beider Queryphasen, exakte Größen, Elementbreite, 4096-Byte-Grenze, erlaubte Enums/advertisierte Untertypen und Duplikate. Plattformlisten werden mit der mengenmäßigen Schnittmenge **aller** Gerätelisten verglichen, reihenfolgeunabhängig. Import-only ist erlaubt; eine fehlgeschlagene Query wird nie zu „leerer Liste“ umgedeutet. Wrong-Scope- und undersized-name-Kontrollen bleiben erhalten. Keine Context-/Queue-/Semaphoreerzeugung.

C0s vier −30 bei beworbenen Erweiterungen bleiben eine archivierte Query-Vertragsabweichung; native Bestätigung und NVIDIA-Ursache sind offen. Fehlender Export erlaubt erfolgreiche leere Liste. Kein Sicherheitsfund. Gezielter späterer Querylauf braucht eigene StufeB-Freigabe.

## Bindung des Supervisors

`run_approved_case.py` benötigt zusätzlich ein konkret geprüftes Manifest unter `c1/approvals/` mit Freigabebeleg, absolutem Binarypfad, ELF-SHA, Fall-/Repeatumfang, Plan-/Workspacehash, Gitmodus/HEAD und Runtime-Auswahlvariablen. Keine echte Freigabe wurde erzeugt. `provenance.py` liest die geprüften nativen ELF-Bytes (höchstens64MiB) in ein gegen Schreiben/Vergrößern/Verkleinern versiegeltes Linux-memfd; dieses Snapshot wird ausgeführt. Ein späterer Austausch des Dateipfads ersetzt die geprüften Bytes nicht. Scripts/Hashmismatch werden abgewiesen.

Dies ist Schutz vor versehentlicher Falschausführung, keine Sicherheitsgrenze gegen den Hosteigentümer. Loader/laufende Shared Libraries sind kein durch das memfd isolierter Container. Bekannte Runtimebibliotheken und ausgewählte Environmentvariablen werden zusätzlich geprüft. Für S03 muss erst ein Paar-Supervisor entwickelt werden; bestehender C1-Supervisor erlaubt nur bekannte C1-Cases.

## Commit und Working Tree

HEAD, Branch, NUL-getrennter Gitstatus, SHA der staged/working Diffs und ein Contentmanifest sämtlicher Quelldateien einschließlich untracked Dateien werden vor/nach dem Lauf protokolliert. Gitdateien/Cache sowie Approvalordner und generierte H2-Ergebnisse sind bewusst aus der Content-ID ausgeschlossen; Approval wird separat gehasht/kopiert. In einem ZIP ohne Git gilt ehrlich `exported_snapshot`, HEAD=null. Inhaltsänderungen trotz identischem Gitstatus werden erkannt. Ein Sourcewechsel während des Laufs setzt den Stopgrund.

## Nachweise und Grenzen

CPU-Regressionen in `tests/test_security_audit.py` prüfen Approvalablehnung, ELFhash/Versiegelung, Austausch des Diskbinary nach Snapshot, Git-/untracked-Provenienz und Exportmodus. `test_b12_validation.c` enthält22 reine Hostassertionen für Listenfehler und mehrgerätige Schnittmengen. `notes/OFFLINE_VALIDATION.json` und Testlogs enthalten die tatsächlichen Ergebnisse. Die beiden OpenCL-Hostquellen werden nur mit `-c` kompiliert; kein OpenCL gelinkt/geladen/ausgeführt. Synthetischer Datenoracle und Python-Syntax werden geprüft. Originalzustände der geänderten C1-Dateien/Manifestnachweise liegen unter `notes/c1_before_audit/`.

Es fand kein funktionaler OpenCL-Lauf statt. Tests belegen diese Hostlogik und die Erhaltung der Evidence, keine Fehlerfreiheit des Treibers oder vollständige Supervisorisolation. H1 bleibt unverändert; H2-C0 wurde nicht neu erhoben.
