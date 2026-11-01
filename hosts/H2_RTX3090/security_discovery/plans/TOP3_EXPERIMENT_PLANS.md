# Drei konkrete Experimente zur gesonderten Freigabe

Stand 31.10.2026. **Kein Lauf freigegeben oder durchgeführt.** Der normative Ablauf und die minimalen Prefix-/Transition-/Suffix-Sequenzen stehen vollständig in [Daybreak §4](../daybreak/DAYBREAK_REVIEW.md). Diese Planung übernimmt sie mit den nachstehenden verbindlichen Präzisierungen. [Hypothesenmatrix](../HYPOTHESES.md) enthält Impact, Confounder und Literatur. Normpin: OpenCL-Docs `0027b7f1a5e97b1bf5256888757479e012a5cb59`; CTS `406533a0b437f49d2e542bd293789adc7900cbed`.

## Reihenfolge und gemeinsame Messvorgaben

Vorab separat freigeben: gezielte native B12-Queries (keine C0-Wiederholung), C1-Setterkontrollen und kleine gültige Buffer-/Kernel-/Eventbaselines. Nur nach erfolgreichen benötigten Baselines einen der folgenden Arme freigeben. C1 besitzt noch keine Cases für S01–S03: Erst Source, Parser, Fallkatalog, zugelassene Binärdatei und konkrete Freigabemanifeste ergänzen und offline prüfen. Der vorhandene Supervisor darf unbekannte Cases weiterhin ablehnen. Eine Planskizze ist kein fertiger Reproducer und erteilt keine Laufgenehmigung.

Pro Prozess höchstens 2 Kontexte, 2 Queues, 8 Buffer mit je höchstens16KiB; Arbeitsdaten höchstens128KiB, einschließlich Guards. S03 hat höchstens2 Prozesse, dieselben Grenzen je Prozess und höchstens256KiB gesamte Arbeitsdaten. Höchstens64 Workitems je Dispatch, lokale Größe1 oder ein separat gültiges kleines Vielfaches, maximal4 Dispatches je Trial. Keine Delay-Kernel, keine Speicherbelastung, keine Adresssuche. Host-/Treiberinterne Allokationen können größer sein; RSS-Grenze2GiB je Prozess, bei S03 4GiB für das Paar; kein zugesagtes vollständiges GPU-Speicherlimit.

Pilot: je ein frischer Prozess für gültige Kontrolle und Zielarm. Nur wenn Kontrollen, Codes und vollständige Daten stimmen, bis zu5 frische gepaarte Trials pro Variante einschließlich Pilot. Reihenfolge vorab alternierend festlegen, alle Versuche berichten. Erste unerwartete Antwort, Datenabweichung, Kontrollversagen oder Timeout stoppt die ganze Serie; keine automatische Wiederholung. Unzureichende zeitliche Überlappung bei S02 bedeutet „nicht informativ“ und erlaubt keine versteckte Laststeigerung.

Äußere Frist60s pro Prozess, CPU45s, Eventpolling5s, Output höchstens16MiB pro Prozess. S03 benötigt einen Paar-Supervisor mit gemeinsamer60s-Frist und Kill der beiden eigenen Prozessgruppen; der bisherige C1-Einzelfall-Supervisor reicht dafür noch nicht. Prozessende garantiert keine GPU-Preemption. Bei Instabilität keine weiteren GPU-Aufrufe oder Resetversuche; Evidenz sichern und StufeC gesondert planen.

Jeder Buffer enthält eine vollständige hostberechnete uint32-Sequenz mit Objektkennung, Trialkennung und Nonce, plus je16 Guardwörter. Nur in erlaubten Bereichen lesen/schreiben. Vergleich sämtlicher Bytes mit Hostreferenz; Hash alleine ersetzt keinen gespeicherten Vektor. Offlineoracle muss einzelne Payload-, Guard- und Längenänderungen erkennen. Private Nonces bei S03 ausschließlich im jeweiligen privaten Prozessprotokoll speichern; der koordinierende Auswerter liest beide erst nach dem Listenerabschluss. Kontrollnachrichten enthalten keine private Nonce.

Protokoll: APIname/Parameter/Status, Eventstatuswechsel, Hostzeit, Objektkontext/-generation, eigene Referenzledger, Bufferlayout, vollständige Daten, Nonceherkunft, Prozess-/Queuegraph, Loaded-Library-/Deviceidentität, freigegebene ELF-/Plan-/Workspacehashes, Git HEAD/Dirty-Status und Contentmanifest, stdout/stderr, Exit-/Stopgrund. Fehlende Daten erzeugen „inconclusive“, keinen Pass. Positive und negative Kontrollen haben getrennte Prozesse/Trialkennungen.

## P1 / S01 — Fehlernachzustand und Clone

R11005–11094 verlangt bei fremdem gültigem Kontext −34 und unveränderten Kernelzustand; Ressourcenfehler −5/−6 sind ausgeschlossen. R11304–11349: Clone kopiert Argumentzustand; Speicherobjekte müssen durch eigene Referenzen lebend bleiben.

Minimalfolge wie Daybreak §4 S01: eigenes A als Eingabe setzen, eigene Ausgabe setzen, genau einen fremden B-Setter ausführen. Nur nach −34 Clone erzeugen und Original/Clone mit je eigener Ausgabe gültig ausführen. Beide lesen A; B bleibt unverändert. Separate Kontrollvarianten: eigener Setter, falscher Index −49 und falsche Größe −51 bei einem Bufferargument. Zusätzlich Clone vor bzw. nach eigener Neubindung vergleichen: Rebind des Clone darf das Argument des Originals nicht umstellen; gemeinsame tatsächliche Allokation bleibt erlaubtes Sharing.

Erwartung: gültige APIs CL_SUCCESS, Fremdsetter −34, beide Endevents CL_COMPLETE, Daten exakt Hostreferenz. Bei unerwartetem Fremdsetter-Erfolg sofort ohne Dispatch stoppen. Ein solcher Returnbefund ist zunächst Konformität. Datenabweichung nach normativem Fehler kann Kontextintegrität zeigen, noch keinen Prozessbruch. Ressourcenfehler stoppen ohne Rollbackaussage. Reproducer noch zu implementieren: `S01-ROLLBACK`, `S01-CLONE`, jeweils Einzelfall, keine Kampagnenschleife.

## P2 / S02 — gültige Lebenszeit

R5891–5916 schützt queued uses und Parentstore bei lebendem Subbuffer. R12484–12522: User-Event vor jeder Nicht-Event-Freigabe terminal setzen. R12424–12440 erlaubt nach abnormaler Terminierung implementationsdefinierte Kontextfolgen.

Drei getrennte Varianten aus Daybreak §4: U (erfolgreich aufgelöstes User-Event und letzter Hostrelease nach Enqueue), P (Parentrelease bei lebendem Subbuffer), R (explizites Retain und separat letzter Hostrelease nach Enqueue). Jede besitzt einen Arm mit länger gehaltenen Referenzen. Producerqueues flushen, nichtblockierende Hoststorage bis Transfercompletion halten. Nach Release keine API mehr mit freigegebenem Handle. Beobachtungsbuffer O und neu angelegter B bleiben gültig. Nur erfolgreich abgeschlossene O/B-Daten auswerten.

Kein Versuch erzwingt physische Wiederverwendung. Gleichheit von Adressen oder Callbackzeit allein ist kein Befund. Keine Erzeugung eines künstlich langen GPU-Zeitfensters. Das User-Event auf −1 ohne abhängige Kommandos ist eine separate API-Kontrolle (`clWaitForEvents` −14), kein Lifetime-/Reuse-Datenarm. Negative abhängige Arbeit liefert keinen garantierten Datensuffix. Reproducer noch zu implementieren: `S02-U`, `S02-P`, `S02-R`, `S02-E-API`, mit explizitem Referenzledger.

Vor Neuheitsclaim vertraulichen H1-B4-/B9-Index intern vergleichen; diese privaten Nachträge fehlen. H1 bleibt geschlossen.

## P3 / S03 — privater Prozesscanary neben gültigem Sharing

Noch bedingt: C0 belegt beworbenen OpenCL-opaque-FD-Import, keinen erfolgreichen Vulkan/OpenCL-Interop. Vor Code-/Laufabschluss Vulkan-Spezifikation und konkrete Exportfähigkeitsquery zusätzlich pinnen. Geräte-UUID, Buffer/opaque-FD-Kompatibilität, Allocationgröße/Alignment und dedizierte Allokationsanforderungen prüfen. Semaphoren benötigen ihren eigenen funktionalen Gate; ansonsten strikt sequenzielle Hostbarrieren mit nachgewiesener Completion und External-Memory-Acquire/Release. Kein gleichzeitiger Zugriff aus zwei APIs auf S.

Die konkrete Minimalfolge ist Daybreak §4 S03: V exportiert S, L importiert gültig. S-Sharing separat positiv nachweisen. Im Zielarm hält L seinen gültigen Import; V beendet alle Zugriffe, gibt eigene S-Referenzen in Vulkan-Reihenfolge frei und erzeugt erst dann separates privates P. L liest ausschließlich gültige S-Region, V prüft P. Exporter-hält-S und Listener-ohne-FD sind Kontrollen. Neue P-Nonce je Prozesspaar. OpenCL übernimmt den erfolgreich importierten FD; kein anschließender eigener close/use desselben übertragenen FD. FD-close widerruft andere Payloadreferenzen nicht.

S03 untersucht keine uninitialisierten Reads. Ein Sicherheitskandidat braucht private P-Daten in zulässigem L-Readback oder kausale P-Korruption plus ausgeschlossene Hosttransport-/Loggingkanäle. S-Nonce in L ist erwartetes autorisiertes Sharing. Gleiche UID ist die erste benannte Prozessgrenze; andere UID, Container, MIG und VM sind eigene Modelle und hier nicht freigegeben. Reproducer noch zu implementieren: Exporter-/Listenerpaar mit strengem IPC-Nachrichtenschema, Einzelfall-Supervisor und Offlineparser. Fehlendes Feature oder fehlender gültiger Lifecycle bedeutet Capability-Skip.
