# Daybreak Blue — unabhängiger Vertrags- und Neuheitscheck

**Stand:** 31.10.2026. Dieses Paket ist ein interner, kleiner Arbeitsauszug aus OpenCL2. Lies zuerst diese Datei, dann `PROJECT_STATE.md`, `H2_C0_READINESS.md`, `H2_B1_B12_CAPABILITY_MATRIX.md`, die H1-Abschlüsse und die C1-Dokumente. Das Material enthält bereits eine vorläufige Literature Review. Behandle deren Bewertungen als zu prüfende Aussagen, nicht als unabhängige Bestätigung.

## Abgegrenzter Auftrag

Prüfe *unabhängig* drei Kandidaten für die nächste Sicherheitsstudie. Benutze die beigefügten gepinnten OpenCL-Spezifikationsquellen und Khronos-CTS-Quellen sowie Primärliteratur. Suche gezielt nach Gegenbelegen und bereits publizierten gleichen Mechanismen. Bewerte die Grenze zwischen einem Konformitätsfehler und einer tatsächlich verletzten Schutzdomäne.

1. **S02, Eventfehler und Objektlebenszeit:** Welche rechtmäßigen Zustandsfolgen aus User-Event-Auflösung, Fehlerweitergabe, Queue-Abhängigkeit, Parent-/Subbuffer-Freigabe und Retention können einen vorzeitigen Ressourcen-Reuse sichtbar machen? Lege *keine* noch pending User-Events mit abhängiger Arbeit durch Freigabe anderer Objekte still. Gib für jede Folge den normativen Vertrag und ein Datenoracle an. Ein gleicher Adresswert genügt nicht.
2. **S01, fehlerhafte Setter und gültiger Folgezustand:** Für `clSetKernelArg` mit fremdem Kontext und `clCloneKernel`: Welche Fehlercodes sind normativ, und welche Aussage gilt über den Kernelzustand nach einem Fehler? Unterscheide Rückgabecode, intern gebliebenes Argument, späteres Enqueue und tatsächlichen Speicherzugriff. Ein unerwartet erfolgreicher Setter löst *keinen* Folge-Dispatch aus.
3. **S03, echte private Cross-Process-Grenze:** Skizziere ein zulässiges, kontrolliertes Zwei-Prozess-Experiment mit legitim geteiltem External-Memory-Objekt als Positivkontrolle und separatem nie exportiertem, vollständig initialisiertem Privatobjekt als Sicherheitsoracle. Prüfe Featurevoraussetzungen, Acquire/Release-Verträge und was ein geteilter FD gerade *nicht* beweist. Trenne gleicher-UID-Prozessgrenze, andere-UID-Grenze und MIG-/VM-Grenze. RTX3090 unterstützt hier kein MIG.

## Fragen, die besonders geprüft werden sollen

- Gibt es in Spezifikation oder CTS eine ausdrücklich erlaubte Zustandsänderung nach dem jeweiligen Fehlercode, die unser Oracle entwertet?
- Ist der angenommene Event-/Freigabeablauf vollständig definiert, auch wenn Kommandos mit negativem Eventstatus enden?
- Welche Varianten sind durch H1-B4/B7/SEC1/SEC2 oder durch StepStone, WebGlitch, Guardian, LeftoverLocals und andere Primärliteratur bereits sachlich abgedeckt? Die privaten H1-B4-/B9-Nachträge fehlen; markiere deshalb verbleibende Doppelungsunsicherheit.
- Was wäre jeweils nur API-Konformität, was Kontextintegrität im selben Prozess, und was ein nachweisbarer Prozess- oder Mandantenbruch?
- Welche Capability fehlt H2 bereits sicher? Für S03 ist ein erfolgreiches Query-Ergebnis allein noch kein Interop-Nachweis.

## Erwartete Rückgabe

Liefere **eine Datei `DAYBREAK_REVIEW.md`** mit folgender Struktur:

1. Kurze Tabelle zu S01/S02/S03: Vertrag, genaue Quelle mit Abschnitt/Zeile oder URL, erwartete Rückgabe/Nachzustand, Oracle, Kontrollen, stärkster Confounder, Vorbedingung auf H2, Priorität.
2. Normative Gegenprüfung: jede falsche oder ungesicherte Annahme der mitgelieferten C1-/Literaturtexte als überprüfbarer Befund. Kennzeichne `bestätigt`, `widerlegt` oder `offen`; trenne Quelle und eigene Schlussfolgerung.
3. Prior Art / Neuheit: mindestens die drei stärksten Gegenbelege gegen einen Full-Paper-Claim und die engste noch belastbare Forschungslücke. Verlinke Primärquellen, keine bloßen Suchtreffer.
4. Je Kandidat ein minimaler, *nur geplanter* Reproducer mit gültigem Prefix, gezieltem Zustandsübergang, gültigem Suffix, Positiv-/Negativkontrolle, Protokollfeldern und Stoppregel. Keine ausführbare Angriffskampagne.
5. Offene Fragen und exakte nächste Stufe-A-Arbeit. Gib an, welche Behauptungen durch die vorhandenen Dateien allein nicht geprüft werden konnten.

Schreibe direkt im Bericht: **keine neuen GPU- oder OpenCL-Läufe durchgeführt**. Falls du eine externe Quelle konsultierst, nenne Version/Datum und präzisen Link. Melde keine Vulnerability, solange ein gültiger Vertrag, eine kausale Kontrolle und ein Schutzdomänenbruch nicht gezeigt wurden.

## Grenzen

Nur Lesen, Quellenanalyse, Offline-Dokumentation. **Keine GPU-/Runtime-Experimente**, keine Sicherheitstrigger, Last- oder Fuzzingläufe, Installationen, Treiberänderungen, öffentlichen Meldungen, GitHub-Pushes oder Uploads des internen Gesamtpakets. Ändere die H1-Originaldateien nicht. Daybreak Blue ist Analysehilfe; seine Verfügbarkeit ersetzt keine Stufe-B-/C-Freigabe. Dieses kleine Paket ist ein ausgewählter Auszug und enthält weder die 729 H1-Rohdateien noch die privaten H1-Nachträge, noch vollständige Literatur-PDFs.
