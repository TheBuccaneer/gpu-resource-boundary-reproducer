# H2-C1: umsetzbarer Arbeits- und Freigabeplan

## Status und feste Grundlage

**Plan und Reproducer vorbereitet, nicht ausgeführt.** Bezug: Übergabe-ZIP SHA-256 `3aacc0512a5af7bf972d65a2dfdbb0184d181053b79af8c487f1c3eacf3935c1`; unverändertes H2-C0-release mit NVIDIA RTX 3090, PCI `0000:01:00.0`, Treiber 580.178.04. Die 729 H1-Dateien bleiben unverändert. Unveröffentlichte interne Übergabematerialien werden nicht hochgeladen.

Vertragspins: OpenCL-Docs `0027b7f1a5e97b1bf5256888757479e012a5cb59`; Header `v2025.07.22`; CTS `406533a0b437f49d2e542bd293789adc7900cbed`. Für die Kernel wird explizit **OpenCL C 1.2** verwendet, obwohl das Gerät zusätzliche C-Versionen meldet. Damit hängt die Uniformitätsfrage nicht von optionalem Nonuniform-Verhalten ab. OpenCL-3.1-only-Regeln werden nicht auf das vorhandene 3.0-Gerät angewendet. Gemeinsame Methodik: H1-Methodology, mit PASS für gültige Kontrollen und F0 für korrekt spezifizierte Fehlerantworten.

## Geplanter Versuchsrahmen für jeden freigegebenen Einzeltest

- **Identität:** vor der einzelnen Kampagne die vorhandenen Loader-/ICD-Dateien gegen die C0-Prüfsummen vergleichen. Im Reproducer nur Zielname, Vendor, Treiberversion und PCI-Zuordnung prüfen; keine automatische Auswahl von Plattform/Gerät 0, kein Softwarefallback. Bei Änderung stoppen und erst dann eine gezielte Provenienzaktualisierung begründen. Kein komplettes `clinfo` oder neues Inventar auf Vorrat.
- **Prozess:** genau ein benannter Fall pro frischem Prozess, seriell, kein paralleler unabhängiger GPU-Client als Versuchsarm. Der aktive Desktop bleibt eine dokumentierte Randbedingung; keine Displayumschaltung oder Treiber-/Watchdogänderung.
- **Wiederholungen:** fünf unabhängige Prozesse je Fall. Der erste ist der Pilot und zählt zu den fünf, wenn er gültig ist. Kein Warm-up verwerfen. Bei jeder unerwarteten Beobachtung alle weiteren Wiederholungen und nachfolgenden Fälle stoppen. Fünf Läufe sind deskriptive Reproduzierbarkeit, keine Aussage über seltene Races oder eine Fehlerwahrscheinlichkeit.
- **Arbeitslast:** höchstens zwei 16-KiB-Puffer gleichzeitig, 64 aktive Work-items (bei einem divisibility-negativen Aufruf G=65, Kernel auf 64 begrenzt), Local Size höchstens 32, keine lokalen Kernelargumente, kein SVM, keine Rohadressen und keine Lebensdauerverletzung. Alle Puffer bleiben bis zum Ende ihrer Verwendung gültig. Kein Test am 48-KiB- oder VRAM-Kapazitätsrand.
- **Zeit/Ressourcen:** äußere Stopfrist 60 s inklusive Initialisierung und Source-Build (dann SIGTERM, nach höchstens 1 s SIGKILL; eine nicht bestätigte Beendigung bleibt ausdrücklich offen), CPU-Zeitlimit 45 s, keine Core-Dumps, Ausgabegrenze 16 MiB, RSS-Stoppschwelle 2 GiB für den gestarteten Prozessbaum. Keine enge virtuelle Adressraumgrenze, die den NVIDIA-Stack unbeabsichtigt einschränken könnte. Das ist eine geplante Hostbegrenzung, keine garantierte GPU-Preemption.
- **Completion:** nach `clFlush` terminalen Eventstatus höchstens 5 s pollen; danach `clWaitForEvents` nur bei bereits beobachtetem COMPLETE. Nur nach erfolgreicher Completion lesen und Objektfreigaben auswerten. Die äußere Frist bleibt auch für blockierende API-Aufrufe erforderlich. Bei einer externen Beendigung sind Cleanup/Completion unbewiesen, kein F0/PASS.
- **User-Events:** jedes absichtlich wartende User-Event auflösen, bevor Nicht-Event-Objekte freigegeben werden. Keine absichtlich hängen gelassenen User-Events. Negative Hoststatus-Tests hängen keine GPU-Kommandos an ein Fehler-User-Event; deren -1 ist eine bewusst gesetzte Hostbedingung, kein GPU-Fehlerbefund.
- **Einzelfehler:** genau eine Vertragsbedingung pro negativer Variante verändern. Keine freigegebenen/erfundenen Handles, keine zusätzlichen ungültigen Speichergrößen, kein „Tolerieren“ beliebiger Fehlercodes. Unerwartetes `CL_SUCCESS` wird vor jeder Folgeaktion protokolliert und beendet den Fall.

## Messdatensatz je Einzelprozess

Unter einem neuen `hosts/H2_RTX3090/results/c1/<campaign>/<case>/r01…r05/` ablegen: genehmigtes Paket/Datum, Case-ID und Wiederholungsnummer, exakte argv und kuratierte Runtimeauswahlvariablen, Start-/Endzeit UTC und monotone Zeit, Quell-/Header-/Hostbinary-/Spezifikations-/Inputhashes, Basiscommit plus Arbeitsstanddiff, tatsächlich aufgelöste und gemappte Loader-/ICD-Pfade samt SHA-256, Queueflags, Kontext-/Objektzuordnung, relevante Kernelmetadaten, sämtliche API-Rückgaben, Eventstatus und verfügbare Profilingtimestamps, vollständige gelesene 16-KiB-Daten einschließlich Guards, stdout/stderr/Buildlog, Prozessstatus/Signal/Timeout und Cleanupstatus.

Kernelquelle ist im gehashten Hostquelltext enthalten. Der feste Datentransform wird für jedes Element gegen die CPU-Erwartung geprüft; außerhalb der ersten 64 Wörter bleibt `0xa55aa55a` erhalten. Die vollständigen Daten werden als `data_u32_hex` protokolliert, nicht nur als PASS. Host- und Gerätetimestamps nicht ungeprüft subtrahieren. Zwischenstände wie QUEUED/SUBMITTED müssen nicht alle beobachtet werden; zulässige schnelle Completion ist kein Fehler.

Der vorbereitete äußere Supervisor `reproducers/run_approved_case.py` setzt die Hostgrenzen pro einzelnem Aufruf um; er wurde nur statisch geprüft und nicht mit einem OpenCL-Prozess gestartet. Seine Anwendung und die vollständige Beschaffung der ergänzenden Provenienz sind Teil des späteren freigegebenen Laufauftrags. Die vorbereiteten C-Dateien ersetzen keinen Supervisor und wurden noch nicht mit dem Treiber funktional geprüft. Kein automatischer Batchlauf gehört zu diesem Auftrag.

## Freigabepakete in Prioritätsreihenfolge

| Paket | Konkreter Umfang / Reproducer | Voraussetzung und Erfolgskriterium | Jetzt freigegeben? |
|---|---|---|---|
| **A — gezielte B12-Queries** | B12-Q; nur Enum-/Infoqueries, keine Objekterzeugung; 5 Prozesse | C0-Mapping bestätigt; Name/Negativkontrollen funktionieren; jede Liste entweder success+auswertbar oder als Vertragsabweichung dokumentiert | Nein, vorbereitet |
| **B — B7 ohne Dispatch** | OWN, INDEX, SIZE, FOREIGN-AB und FOREIGN-BA; je 5 Prozesse | Source-Build, echte getrennte Kontexte und Metadaten; own setter success, Index -49, Größe -51, fremder Kontext -34; keine Queue und kein Enqueue im B7-Zweig | Nein, vorbereitet |
| **C — gültige Baselines** | BL-BUFFER, BL-KERNEL; je 5 Prozesse | kleine erlaubte Objekte/Queue, Hostoracle und Guards; korrekte Daten/Completion/Cleanup | Nein, vorbereitet |
| **D — B2 Geometrie** | drei gültige LWS, DIM0, DIM4, REQ-VALID, REQ-MISMATCH, NONDIVISIBLE; je 5 Prozesse | Paket C bestanden, Kernelmaximum ≥32 und ReqWG-Metadaten stimmen; isolierte erwartete -53/-54 | Nein, vorbereitet |
| **E — B10 Grundkontrollen** | IO, XQUEUE, USER, MARKER-VALID, FOREIGN-EVENT, WAITLIST-NULL, WAITLIST-ZERO; je 5 Prozesse | Paket C bestanden, Producerqueues geflusht, korrekte Daten bzw. -34/-57; keine fremden Datenänderungen | Nein, vorbereitet |
| **F — B10 Ergänzungen** | OOO sowie USER-ERROR, USER-DOUBLE, USER-INVALID; je 5 Prozesse | E sauber; OOO separat weiterhin beworben und tatsächlich eingerichtet; Hoststatusfälle einzeln | Nein, vorbereitet |

B7 kann als eigener Settervertrag ohne vorausgehenden GPU-Kernelpilot geprüft werden: gültige Source/Objekte und OWN sind dort die positiven Kontrollen. Ein späterer tatsächlicher Datenzugriff mit einem Fremdobjekt ist ausdrücklich **nicht** Teil dieser Vorlage. Pakete D–F dürfen nicht aus einem pauschalen „C1 GO“ heraus automatisch auf zusätzliche Fälle oder Stressläufe ausgeweitet werden.

## Verbindliche Stop-/Auswertungsregeln

Unverzüglich stoppen bei falscher Identität/Hash, fehlgeschlagenem Build/positivem Kontrollfall, unerwartetem API-Code, Daten-/Guardabweichung, fehlendem erforderlichem Event, unerwartetem Completionstatus, Timeout, Signal/Prozessabbruch, Ressourcenüberschreitung oder neu beobachteter Desktop-/Treiberstörung. Kein Wiederholen bis „es einmal passt“, kein automatischer Neustart oder Reset. Ein Prozesskill beendet nicht nachweislich bereits eingereichte GPU-Arbeit; kein automatischer Folgetest.

Beim B7-Fremdsetter führt unerwarteter Erfolg direkt zum Prozessstopp; der Code enthält in diesem Zweig überhaupt keinen Enqueue. Bei einer unerwartet akzeptierten B2-Geometrie gibt es ebenfalls keinen Folge-Dispatch und keine zusätzliche Boundary-Eskalation. Testeingaben bleiben klein, aber C1 ist trotzdem eine gesondert zu genehmigende Laufphase.

PASS verlangt gültige Daten/Completion und passende API-Rückgaben; F0 verlangt den vorgesehenen Fehler für genau die geänderte Bedingung. SETUP_ERROR, CAPABILITY_SKIP, CONTRACT_MISMATCH, DATA_MISMATCH und TIMEOUT bleiben getrennte Rohkategorien. Aus diesen entstehen nicht automatisch F2–F8 oder Aussagen über OS-/Prozessisolationsgrenzen. Ressourcenfehler bei einer ansonsten eindeutigen Negativkontrolle werden als SETUP_ERROR untersucht, nicht als erfolgreicher Vertragsnachweis.

Alle Einzelfälle, erwarteten Codes, positiven/negativen Gegenkontrollen und Reproducer sind im [Testkatalog](H2_C1_TEST_CATALOG.md) und in [TEST_CASES.json](TEST_CASES.json) festgelegt. Begrenzungen des H1-Vergleichs stehen [separat](H2_H1_COMPARISON_SCOPE.md).
