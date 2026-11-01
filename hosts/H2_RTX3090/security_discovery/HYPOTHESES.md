# Security-Hypothesenmatrix

Stand 31.10.2026. Zwölf Hypothesen in fünf Forschungsrichtungen, keine zwölf gefundenen Fehler. S02/S10 und S03/S06 sind verwandte Familien. R bezeichnet die gepinnte Runtimequelle unter `../c1/sources/`; Normpin `0027b7f1a5e97b1bf5256888757479e012a5cb59`, CTS `406533a0b437f49d2e542bd293789adc7900cbed`. L-IDs verweisen auf die Literature Review.

S01 hat nach Daybreaks Gegenprüfung Priorität vor S02: Der Fehlernachzustand ist enger spezifiziert. S02 verwendet ausschließlich erfolgreiche Completion als Datenoracle. Die negativen Eventarme bleiben separate API-Kontrollen.

| ID | Priorität | Kandidat | Richtung |
|---|---|---|---|
| S01 | P1 | Setter-Rollback und Clone-Nachzustand | A/E |
| S02 | P2 | Gültige Release-/Parent-/Subbuffer-Lebenszeit | B/E |
| S03 | P3 | Privates Prozessobjekt neben gültigem FD-Import | C/B |
| S04 | P4 | Hostpointer- und Map-Lebenszeiten | B/C |
| S05 | P4 | Subbuffer-Intervalle bei Copy/Fill | A/C |
| S06 | P5 | FD-Duplikat und Objektgeneration | C/B |
| S07 | P4 | Eigener Program-Binary-Roundtrip | D/E |
| S08 | P4 | Completion vor sichtbaren Effekten | B/E |
| S09 | deferred C | Ressourcenvertrag und Fehlerisolation | D |
| S10 | P5 | Destructorcallback und Lebenszeitordnung | B |
| S11 | deferred | Cross-ICD-Objektzuordnung | A/E |
| S12 | deferred | Remanenz mit Herkunftsbeweis | C |

## S01 — Setter-Rollback und Clone-Nachzustand

**boundary_threat_model:** Kontextintegrität innerhalb eines Prozesses; kein OS-Prozessclaim

**normative_contract:** clSetKernelArg: Kontextgleichheit und unveränderter Zustand nach -34/-49/-51; clCloneKernel kopiert Argumente, hält deren Objekte nicht am Leben (R11005–11094,11304–11349).

**mechanism:** Fehlercode korrekt, intern dennoch fremdes oder unset Argument; Clone übernimmt beschädigten Zustand.

**oracle:** Nach genau -34 gültiger eigener Dispatch von Original/Clone: vollständige eigene Nonce und Guards; fremdes B bleibt unverändert. Bei unerwartetem Erfolg kein Enqueue.

**controls:** Eigener Setter/Clone; separate Ausgaben; falscher Index und falsche Größe; Offline-Datenmutation.

**impact:** Fehlgeleitete Datenzugriffe bei korrekter Fehlerantwort.

**confounders:** Objektlebenszeit, Clone teilt tatsächliche Allokation, Ressourcenfehler dürfen Zustand ändern.

**reproduction:** Alle Argumentobjekte lebend halten; nur einen Fehler einfügen; gültigen Suffix replayen.

**stop:** Unerwarteter Setterstatus, Ressourcenfehler, Kontrollfehler: sofort ohne Dispatch stoppen.

**literature:** L01 L02 L16; H1-B7 vorbekannt

**platforms:** H2 zuerst; später clvk/PoCL nach Installationfreigabe. H1 nur vorhandene Daten.

**novelty:** Enger Nachzustandsvergleich; Neuheit nur bei neuem Mechanismus/Impact.

**Wiederholungen:** Für neue B-Arme: Pilot 1 je Kontrolle/Ziel, danach maximal 5 frische gepaarte Trials nur bei erfolgreichen Kontrollen; kein Retry nach Auffälligkeit. Deferred-Arme benötigen erst eigenen konkreten Vertrag.

**Messdaten:** API-/Eventstatus, vollständige Daten/Guards, Objekt-/Referenzgraph, Zeit/RSS/Output, Binary/Plan/Workspace/ICD-Provenienz.

## S02 — Gültige Release-/Parent-/Subbuffer-Lebenszeit

**boundary_threat_model:** Objekt- und Speicherintegrität innerhalb eines Kontextes

**normative_contract:** R5891–5916: Store bleibt bis Ende nutzender Kommandos und lebender Subbuffer; R12484–12522: pending User-Events vor Nicht-Event-Release terminal setzen.

**mechanism:** Vorzeitige Storefreigabe trotz gültigem queued use oder lebendem Child.

**oracle:** Nur erfolgreiche Completion: A→O unverändert; neue B-Nonce und Guards korrekt; keine Adressgleichheit als Beweis.

**controls:** Referenzen bis Completion halten; Parent länger halten; Retain-Kontrolle; Offline-Guardmutation.

**impact:** Korrumpierte zulässige Arbeit; stärkerer Domainimpact erst gesondert zu belegen.

**confounders:** Scheduling kann Fenster schließen; Refcount/Destructorcallback beweist keine physische Wiederverwendung; negativer Commandstatus macht Folgezustand implementationsdefiniert.

**reproduction:** U/P/R-Arme aus Daybreak §4; User-Event erfolgreich auflösen bevor Nicht-Event-Release; kein Datenoracle nach abnormaler Terminierung.

**stop:** Negativer Commandstatus, unbekannte Referenzbuchhaltung, fehlende Überlappung: stoppen bzw. nicht informativ werten.

**literature:** L02 L03 L12; H1-B4 und privater Nachtrag abgleichen

**platforms:** H2 mit Buffern; H1-SVM ist kein gleiches Feature; spätere Implementierungen separat.

**novelty:** Nur neue zulässige Zustandsvariante; private H1-Doppelung offen.

**Wiederholungen:** Für neue B-Arme: Pilot 1 je Kontrolle/Ziel, danach maximal 5 frische gepaarte Trials nur bei erfolgreichen Kontrollen; kein Retry nach Auffälligkeit. Deferred-Arme benötigen erst eigenen konkreten Vertrag.

**Messdaten:** API-/Eventstatus, vollständige Daten/Guards, Objekt-/Referenzgraph, Zeit/RSS/Output, Binary/Plan/Workspace/ICD-Provenienz.

## S03 — Privates Prozessobjekt neben gültigem FD-Import

**boundary_threat_model:** Zwei eigene Prozesse gleicher UID, getrennte private GPU-Allokationen

**normative_contract:** R6049–6073/6147–6169: Acquire/Release; R6234–6285: opaque-FD-Transport und Ownership. Vulkan-Exportvertrag separat pinnen.

**mechanism:** Exporter gibt eigene S-Referenz frei; gültiger Import hält S, Backend verwechselt S mit neuem privaten P.

**oracle:** Nie exportierte P-Nonce erscheint in zulässigem L-Readback oder P wird kausal korrumpiert; S-Sharing allein ist Positivkontrolle.

**controls:** Exporter behält S; Listener ohne FD; neue private Nonce; vollständige initialisierte Bytes/Guards; keine P-Hostkanäle.

**impact:** Private Daten-/Integritätsverletzung über Prozessgrenze.

**confounders:** Legitimes Sharing, FD-dup/Ownership, Hosttransport, kein FD-close-Revocationvertrag; Interop nicht bewiesen.

**reproduction:** Daybreak §4 S03 nach einzelnem Feature-Gate; keine rohen cl_mem-Pointer per IPC.

**stop:** Fehlende kompatible Exportfähigkeit/UUID/Synchronisation: Capability-Skip; erster Mismatch stoppt.

**literature:** L04 L16; External Memory Spezifikation

**platforms:** H2 bedingt auf Vulkan/OpenCL-Interop; MIG/VM/andere UID nicht Teil dieses Arms.

**novelty:** Stärkster potentieller Impact, aber bislang kein beobachteter Mechanismus.

**Wiederholungen:** Für neue B-Arme: Pilot 1 je Kontrolle/Ziel, danach maximal 5 frische gepaarte Trials nur bei erfolgreichen Kontrollen; kein Retry nach Auffälligkeit. Deferred-Arme benötigen erst eigenen konkreten Vertrag.

**Messdaten:** API-/Eventstatus, vollständige Daten/Guards, Objekt-/Referenzgraph, Zeit/RSS/Output, Binary/Plan/Workspace/ICD-Provenienz.

## S04 — Hostpointer- und Map-Lebenszeiten

**boundary_threat_model:** Host/GPU-Datenintegrität im eigenen Prozess

**normative_contract:** clEnqueueMapBuffer/Unmap und nichtblockierende Transfers: Hoststorage und Synchronisation bis erforderlicher Completion gültig halten (R, jeweilige API-Abschnitte).

**mechanism:** Runtime verwendet trotz abgeschlossener Übergabe falsche Mappinggeneration.

**oracle:** Vollständig initialisierte Muster nach legalem Map/Unmap/Copy; Guards und unverwandter Buffer intakt.

**controls:** Blocking-Transfer; getrennte Hoststorage; Offline-Payloadmutation.

**impact:** Stale Copy oder unberechtigte Hostdatenänderung; zunächst Prozessintern.

**confounders:** CL_MEM_READ_ONLY betrifft Kernelzugriff und ist keine Sicherheits-ACL; Host-UAF wäre Testfehler.

**reproduction:** Ein legaler Mappingwechsel, keine Hostfreigabe während Transfer; relevante Flags/Abschnitte vor Lauf in Vertragsmatrix präzisieren.

**stop:** Jede ungeklärte Pointerlebenszeit oder negative Completion stoppt.

**literature:** L03 L05 L12

**platforms:** H2/clvk/PoCL bedingt auf Flags; keine neue H1-Kampagne.

**novelty:** Mechanismus noch zu konkretisieren; hinter Top3.

**Wiederholungen:** Für neue B-Arme: Pilot 1 je Kontrolle/Ziel, danach maximal 5 frische gepaarte Trials nur bei erfolgreichen Kontrollen; kein Retry nach Auffälligkeit. Deferred-Arme benötigen erst eigenen konkreten Vertrag.

**Messdaten:** API-/Eventstatus, vollständige Daten/Guards, Objekt-/Referenzgraph, Zeit/RSS/Output, Binary/Plan/Workspace/ICD-Provenienz.

## S05 — Subbuffer-Intervalle bei Copy/Fill

**boundary_threat_model:** Integrität getrennter Bufferintervalle

**normative_contract:** clCreateSubBuffer: Alignment/Größe; clEnqueueCopyBuffer: Bereiche und Überlappung; R jeweilige API-Verträge.

**mechanism:** Validierung prüft Parentgröße statt Subbufferbereich oder lässt nach abgewiesenem Copy falschen Nachzustand.

**oracle:** Gültige Operation ändert nur erlaubtes Intervall; isolierter ungültiger API-Bereich wird abgewiesen; Siblingguards unverändert.

**controls:** Gültige ausgerichtete Intervalle; genau ein API-Grenzfehler; Offline-Guardmutation.

**impact:** Unbeabsichtigte Datenänderung im Parent; keine automatische OS-Grenze.

**confounders:** Gemeinsamer Parent ist legitimes Sharing; niemals GPU-OOB-Kernel als Oracle.

**reproduction:** Grenzen mit checked host arithmetic modellieren; exakten Fehlercode nach Funktionsvertrag vor Lauf fixieren.

**stop:** Mehrere ungültige Parameter oder unklarer Alignmentvertrag: Fall verwerfen.

**literature:** L02 L05

**platforms:** H2 und spätere ICDs nach Capabilityschnittmenge.

**novelty:** F1 allein schwach; Datenwirkung und neue Ursache nötig.

**Wiederholungen:** Für neue B-Arme: Pilot 1 je Kontrolle/Ziel, danach maximal 5 frische gepaarte Trials nur bei erfolgreichen Kontrollen; kein Retry nach Auffälligkeit. Deferred-Arme benötigen erst eigenen konkreten Vertrag.

**Messdaten:** API-/Eventstatus, vollständige Daten/Guards, Objekt-/Referenzgraph, Zeit/RSS/Output, Binary/Plan/Workspace/ICD-Provenienz.

## S06 — FD-Duplikat und Objektgeneration

**boundary_threat_model:** Autorisierte externe Payloadidentität

**normative_contract:** R6234–6285: importierter FD wird bei Erfolg an OpenCL übertragen; Export/dup referenziert Payload.

**mechanism:** Falsche Payloadzuordnung nach legalem Descriptor-/Objektwechsel.

**oracle:** Jeder gültige Import liefert exakt seine eigene freigegebene Nonce; kein Schluss aus gleicher FD-Nummer.

**controls:** Separat exportierte S/T; dup vor Import; Exporter hält Referenz.

**impact:** Falsche Payload kann private Daten betreffen, erst nach Herkunftsbeleg.

**confounders:** FD-Nummern werden legal wiederverwendet; close widerruft andere Referenzen nicht.

**reproduction:** Kleine zwei-Payload-Variante von S03 nach Interop-Gate.

**stop:** Ownershipzweifel oder fehlender gültiger Exportpfad stoppt.

**literature:** L16; External Memory

**platforms:** H2 bedingt; gleiche Familie wie S03, kein unabhängiger Fundzählpunkt.

**novelty:** Nur bei anderem kausalem Mechanismus eigenständiger Beitrag.

**Wiederholungen:** Für neue B-Arme: Pilot 1 je Kontrolle/Ziel, danach maximal 5 frische gepaarte Trials nur bei erfolgreichen Kontrollen; kein Retry nach Auffälligkeit. Deferred-Arme benötigen erst eigenen konkreten Vertrag.

**Messdaten:** API-/Eventstatus, vollständige Daten/Guards, Objekt-/Referenzgraph, Zeit/RSS/Output, Binary/Plan/Workspace/ICD-Provenienz.

## S07 — Eigener Program-Binary-Roundtrip

**boundary_threat_model:** Semantik und Ressourcenmetadaten des eigenen Programms

**normative_contract:** clGetProgramInfo/CL_PROGRAM_BINARIES und clCreateProgramWithBinary: eigenes Binary für passendes Gerät; gültiger Build.

**mechanism:** Native-Binary-Pfad verliert Geometrie-/Ressourceninvarianten des Sourcepfads.

**oracle:** Eigener Source-/Binary-Pfad liefert gleiche vollständige Integerdaten und konsistente verpflichtende Metadaten.

**controls:** Erneuter Sourcebuild; identische Optionen; triviales Kernel; Offline-Datenmutation.

**impact:** Accountingabweichung als Ausgangspunkt, Integritätswirkung separat.

**confounders:** Compileroptionen, Gerätebindung, optionale Metadaten, unzulässige Binarymutation.

**reproduction:** Nur selbst erzeugte Binary; kein willkürlich manipuliertes Binary; kleine Geometrie.

**stop:** Build/Device-Mismatch oder ungeklärte Metadatenpflicht: Skip.

**literature:** L01 L12; H1-B8

**platforms:** H2 native Binary möglich; Kernel-SPIRV-IL fehlt.

**novelty:** Kein Source-vs-IL-Claim auf H2; neue Invariante nötig.

**Wiederholungen:** Für neue B-Arme: Pilot 1 je Kontrolle/Ziel, danach maximal 5 frische gepaarte Trials nur bei erfolgreichen Kontrollen; kein Retry nach Auffälligkeit. Deferred-Arme benötigen erst eigenen konkreten Vertrag.

**Messdaten:** API-/Eventstatus, vollständige Daten/Guards, Objekt-/Referenzgraph, Zeit/RSS/Output, Binary/Plan/Workspace/ICD-Provenienz.

## S08 — Completion vor sichtbaren Effekten

**boundary_threat_model:** Erfolgreiche Queue-/Event-Synchronisation im eigenen Kontext

**normative_contract:** R12405–12440 und clWaitForEvents: CL_COMPLETE bedeutet erfolgreicher Abschluss; Abhängigkeiten müssen erfüllt sein.

**mechanism:** Erfolg wird vor Abschluss abhängiger Copy-/Kernelwirkungen gemeldet.

**oracle:** Nach erfolgreichem Endevent vollständige Kette A→B→O mit Nonce/Guards; keine Datenannahme nach abnormaler Terminierung.

**controls:** In-Order; explizites Cross-Queue-Event+Flush; Hostreferenzrechner.

**impact:** Stale Daten oder Verwendungsrennen bei gültiger Synchronisation.

**confounders:** Fehlendes Flush, Hostbuffer zu früh verwendet, Readback selbst nicht abgeschlossen.

**reproduction:** Ein Producer und Consumer; gültige Ereignisgraphen mit kleiner Länge.

**stop:** Negativer Eventstatus/Timeout stoppt ohne Datensuffix.

**literature:** L02 L12; H1-B10

**platforms:** H2; spätere OOO nur wenn beworben.

**novelty:** Grundform vorbekannt; neue Completionzustandsvariante nötig.

**Wiederholungen:** Für neue B-Arme: Pilot 1 je Kontrolle/Ziel, danach maximal 5 frische gepaarte Trials nur bei erfolgreichen Kontrollen; kein Retry nach Auffälligkeit. Deferred-Arme benötigen erst eigenen konkreten Vertrag.

**Messdaten:** API-/Eventstatus, vollständige Daten/Guards, Objekt-/Referenzgraph, Zeit/RSS/Output, Binary/Plan/Workspace/ICD-Provenienz.

## S09 — Ressourcenvertrag und Fehlerisolation

**boundary_threat_model:** Geräteressourcen/andere eigene Clients nur mit separater C-Freigabe

**normative_contract:** B1/B3/B9-Verträge und jeweilige Device-/Kernelgrenzen; kein allgemeines Fairnessversprechen.

**mechanism:** Accounting berücksichtigt dynamische/static Ressourcen oder Binarymetadaten unvollständig.

**oracle:** A: nur statischer Vergleich; B: gültige kleine Fälle; C: reproduzierbare kausale Störung getrennter Canary.

**controls:** Gleiche gültige Arbeit; Compiler-/Metadatendifferenz; unbeeinflusster Canary.

**impact:** Availability, nur bei Effekt jenseits normaler geteilter Auslastung.

**confounders:** Gemeinsame GPU mit Display; gewöhnlicher Slowdown; keine harte SLA.

**reproduction:** Erst normativen Grenzvertrag prüfen; Grenzüberschreitung/Reset nur eigener C-Plan.

**stop:** Kein Belastungsarm in A/B; erste Instabilität beendet Kampagne.

**literature:** L01 L06; H1-B1/B3/B9/SEC1 geschlossen

**platforms:** H2 C nur geeignete Testumgebung; keine H1-Wiedereröffnung.

**novelty:** Bekannte Familie; anderer Mechanismus und kausale Isolation nötig.

**Wiederholungen:** Für neue B-Arme: Pilot 1 je Kontrolle/Ziel, danach maximal 5 frische gepaarte Trials nur bei erfolgreichen Kontrollen; kein Retry nach Auffälligkeit. Deferred-Arme benötigen erst eigenen konkreten Vertrag.

**Messdaten:** API-/Eventstatus, vollständige Daten/Guards, Objekt-/Referenzgraph, Zeit/RSS/Output, Binary/Plan/Workspace/ICD-Provenienz.

## S10 — Destructorcallback und Lebenszeitordnung

**boundary_threat_model:** Ressourcenlebenszeit im eigenen Kontext

**normative_contract:** R clSetMemObjectDestructorCallback: memobj im Callback ungültig, keine blockierenden teuren OpenCL-Aufrufe.

**mechanism:** Callback-/Storeordnung inkonsistent mit verbleibendem nutzenden Kommando oder Child.

**oracle:** Callback schreibt nur atomaren Hostmarker; erfolgreicher O-Datenpfad und Referenzbuchhaltung.

**controls:** Callback ohne queued use; Parent/Child länger gehalten; Hostmarker-Selbstkontrolle.

**impact:** Hinweis auf vorzeitige Freigabe; Callback allein beweist kein tatsächliches Backendfree.

**confounders:** Callbacktiming/Threading und ungültige memobj-Nutzung; keine APIs auf Callbackhandle.

**reproduction:** Nur Zusatzbeobachtung zu S02; Callbackstatus nie alleiniger Reusebeweis.

**stop:** Callback ruft keine OpenCL-API; negative Completion stoppt.

**literature:** L02 L03; H1-B4

**platforms:** H2; gleiche Familie S02, kein eigener Vulnerabilitycount.

**novelty:** Nur bei neuem Kausalbeleg.

**Wiederholungen:** Für neue B-Arme: Pilot 1 je Kontrolle/Ziel, danach maximal 5 frische gepaarte Trials nur bei erfolgreichen Kontrollen; kein Retry nach Auffälligkeit. Deferred-Arme benötigen erst eigenen konkreten Vertrag.

**Messdaten:** API-/Eventstatus, vollständige Daten/Guards, Objekt-/Referenzgraph, Zeit/RSS/Output, Binary/Plan/Workspace/ICD-Provenienz.

## S11 — Cross-ICD-Objektzuordnung

**boundary_threat_model:** Loader-/ICD-Zuordnung innerhalb desselben Prozesses

**normative_contract:** Loaderdispatch und jeweils gültige Objekt-/Kontextverträge vor Lauf gesondert pinnen.

**mechanism:** Opaque-Handle wird falscher Implementierung zugeordnet.

**oracle:** Setter-only-Status und korrekte Objektprovenienz; keine Dispatchausweitung nach Wrong-Success.

**controls:** Jede ICD allein; eigenes Objekt; kontrollierter falscher Kontext.

**impact:** Prozessintegrität; keine automatische OS-Eskalation.

**confounders:** Numerische Handle-/Adressgleichheit ist kein Identitätsbeweis; Loader kann invalides Handle nicht sicher validieren.

**reproduction:** Erst zwei freigegebene ICDs und konkreten Vertrag etablieren; derzeit nur Quellenanalyse.

**stop:** Fehlende zweite ICD oder undefinierter fremder Handlevertrag: kein Lauf.

**literature:** L01 L02; ICD-Spezifikation noch zu pinnen

**platforms:** H2 derzeit NVIDIA; clvk/PoCL nicht installiert.

**novelty:** Zurückgestellt, bis ein definierter Testvertrag besteht.

**Wiederholungen:** Für neue B-Arme: Pilot 1 je Kontrolle/Ziel, danach maximal 5 frische gepaarte Trials nur bei erfolgreichen Kontrollen; kein Retry nach Auffälligkeit. Deferred-Arme benötigen erst eigenen konkreten Vertrag.

**Messdaten:** API-/Eventstatus, vollständige Daten/Guards, Objekt-/Referenzgraph, Zeit/RSS/Output, Binary/Plan/Workspace/ICD-Provenienz.

## S12 — Remanenz mit Herkunftsbeweis

**boundary_threat_model:** Private Daten getrennter autorisierter Prozesse

**normative_contract:** OpenCL uninitialisierte lokale Reads sind UB; separater Produkt-/OS-Isolationsvertrag nötig.

**mechanism:** Nicht gelöschter Speicher enthält authentische private Victimnonce.

**oracle:** Nur exakte geheime Nonce mit Herkunftsbeleg; nichtnull allein wertlos; vollständig initialisierte Readerkontrolle.

**controls:** Kein Victim; neue Nonce; initialisierter Reader; Compilervergleich.

**impact:** Vertraulichkeit unter separat belegter Isolationspolicy.

**confounders:** UB, Compileroptimierung, Hostnonce-Transport, H1-SEC2 und LeftoverLocals vorbekannt.

**reproduction:** Keine H1-Wiederholung; erst klarer zusätzlicher Mechanismus/Policy, dann separater Plan.

**stop:** Ohne Policy/Herkunft kein Sicherheitsclaim; keine Ausführung hier.

**literature:** L04; H1-SEC2 geschlossen

**platforms:** H2 keine automatische Betroffenheit; MIG entfällt.

**novelty:** Niedrige Priorität; reine Replikation kein neuer Mechanismus.

**Wiederholungen:** Für neue B-Arme: Pilot 1 je Kontrolle/Ziel, danach maximal 5 frische gepaarte Trials nur bei erfolgreichen Kontrollen; kein Retry nach Auffälligkeit. Deferred-Arme benötigen erst eigenen konkreten Vertrag.

**Messdaten:** API-/Eventstatus, vollständige Daten/Guards, Objekt-/Referenzgraph, Zeit/RSS/Output, Binary/Plan/Workspace/ICD-Provenienz.
