# OpenCL2 – H2 Security Discovery und Literaturrecherche

## Ziel

Entwickle unsere bisherige OpenCL-Konformitäts- und Sicherheitsstudie gezielt zu einer systematischen Suche nach bislang unbekannten GPU-Sicherheitslücken weiter.

Wir verfügen über Daybreak Blue als unterstützende Entwicklungs- und Analyseumgebung. Die Experimente beziehen sich ausschließlich auf eigene beziehungsweise ausdrücklich autorisierte Systeme.

**Primäres wissenschaftliches Ziel:** Nachweis neuer, reproduzierbarer Verletzungen von OpenCL-Sicherheitsgrenzen mit möglichst hoher Novelty, wissenschaftlicher Aussagekraft und Publikationsrelevanz.

Das Auffinden gewöhnlicher Konformitätsfehler bleibt wichtig, ist aber nicht das alleinige Forschungsziel.

## 1. Ausgangspunkt

Projekt: https://github.com/TheBuccaneer/gpu-resource-boundary-reproducer

Berücksichtige die H1-Abschlussdaten und das neueste H2-C1-Übergabepaket vom 31. Oktober 2026.

H1: AMD Radeon 660M, Mesa Rusticl.

H2: NVIDIA RTX 3090, proprietäres NVIDIA OpenCL 3.0.

Weitere mögliche Implementierungen: clvk, PoCL, Rusticl/Zink.

Nutze die bestehenden B1–B14-Klassen, H1-Findings und H2-Testorakel.

Eröffne nicht unnötig bereits abgeschlossene H1-Untersuchungen.

## 2. Aktuelle wissenschaftliche Literatur untersuchen

Recherchiere einschlägige Arbeiten aus 2024–2026, gegebenenfalls ergänzt durch ältere Grundlagenliteratur.

Priorisiere:

- GPU-Treiber- und Runtime-Fuzzing
- OpenCL-/CUDA-/Vulkan-API-Validierung
- GPU-Speichersicherheit
- Cross-Context- und Cross-Process-Isolation
- Use-after-free und GPU-Objektlebenszyklen
- Race Conditions und Completion-Order-Fehler
- GPU-Speicherremanenz
- Resource-Accounting-Fehler
- Cross-Client-Availability und Failure-Containment
- Differential Testing und spezifikationsgestützte Testgenerierung

Prüfe insbesondere:

- StepStone (IEEE S&P 2026)
- WebGlitch (ECOOP 2025)
- CuSafe (USENIX Security 2026)
- LeftoverLocals (2024)
- GPU Memory Exploitation for Fun and Profit (USENIX Security 2024)
- Not so Refreshing (USENIX Security 2025)
- GPUHammer und GDDRHammer (2025/2026)

Für jede relevante Arbeit dokumentiere Forschungsfrage, Methode, Bedrohungsmodell, Ergebnisse, Grenzen und konkrete Übertragbarkeit auf OpenCL2.

Führe außerdem eine Neuheitsprüfung durch: Was wurde bereits gezeigt, und welche wissenschaftlich belastbare Forschungslücke bleibt offen?

Bewerte Rowhammer, MIG und Hardware-Seitenkanäle kritisch hinsichtlich ihrer tatsächlichen Relevanz für unsere RTX 3090. Nicht jede interessante GPU-Angriffstechnik gehört automatisch in unser Paper.

## 3. Gezielte Security-Hypothesen entwickeln

Untersuche insbesondere fünf Forschungsrichtungen.

### A – Object Ownership und Context Isolation

Ausgehend von H1-B7:

Können OpenCL-Objekte fremder Kontexte oder Prozesse durch unzureichende Validierung akzeptiert werden?

Untersuche mögliche Unterschiede zwischen:

- Setter-Validierung
- Command-Enqueue
- tatsächlicher GPU-Ausführung
- Objektfreigabe und Wiederverwendung

Unterscheide API-Fehler, Kontextverletzung und tatsächlichen Schutzdomänenbruch.

### B – Temporal Memory Safety

Ausgehend von H1-B4:

Gibt es Zustandsübergänge, bei denen Speicher oder Ressourcen vor Abschluss abhängiger GPU-Arbeit wiederverwendet werden?

Untersuche Event-Abhängigkeiten, Objektlebenszyklen, Freigaben und Queue-Grenzen.

Suche nach bislang ungetesteten, aber durch Spezifikation und Implementierungsanalyse begründeten Varianten.

### C – Memory Isolation

Prüfe mögliche Verletzungen zwischen getrennten Kontexten und kontrollierten Prozessen.

Untersuche insbesondere:

- Speicherfreigabe und Wiederverwendung
- fehlende Initialisierung
- Speicherremanenz
- fehlerhafte Ownership-Prüfung
- Import-/Export-Objektzustände

Eine Beobachtung undefinierten Verhaltens ist noch kein nachgewiesener Sicherheitsbruch. Definiere daher geeignete Bedrohungsmodelle und Kontrollgruppen.

### D – Resource Accounting und Availability

Ausgehend von H1-B1/B3/B9 und SEC-1:

Untersuche die Möglichkeit fehlerhafter Ressourcenvalidierung sowie mangelhafter Fehlerisolation.

Berücksichtige unterschiedliche Ressourcenklassen, Compiler-Metadaten und Runtime-Zustände.

Potentiell instabile Tests dürfen ausschließlich nach gesonderter Freigabe in einer geeigneten Testumgebung ausgeführt werden.

### E – Stateful API Validation und Differential Fuzzing

Entwickle eine Forschungsstrategie für zustandsabhängiges OpenCL-API-Testing.

Berücksichtige:

- gültige API-Sequenzen
- gezielte Verletzungen von Objektzustandsbedingungen
- Kontext- und Gerätezugehörigkeit
- Lebenszyklusübergänge
- Sequenz- und Reihenfolgevariationen
- Vergleich mehrerer OpenCL-Implementierungen
- automatische Reduktion auffälliger Testfälle

Inspiriere dich methodisch an StepStone und WebGlitch, ohne deren Ansätze lediglich nachzubauen.

## 4. H2-C1-Audit-Korrekturen erledigen

Behebe die drei bereits identifizierten Punkte:

1. Vollständige B12-Semaphore-Listenvalidierung einschließlich Plattform- und Gerätelisten.
2. Supervisor-Härtung durch Bindung an freigegebene Binärdateien und deren Hashes.
3. Git-Commit- und Working-Tree-Provenienz in den Testprotokollen.

Führe anschließend alle geeigneten Offline-Selbsttests erneut aus.

## 5. Sicherheitsorientierte Testplanung

Erstelle für jede aussichtsreiche Hypothese:

- Sicherheitsgrenze und Bedrohungsmodell
- normative OpenCL-Anforderung
- angenommener Fehlermechanismus
- Testorakel
- notwendige positive und negative Kontrollen
- möglicher sicherheitsrelevanter Impact
- Alternativerklärungen und Confounder
- Reproduktionsstrategie
- Abbruchkriterien
- relevante Literatur
- erwarteten wissenschaftlichen Erkenntnisgewinn

Priorisiere explizite Sicherheitsverletzungen gegenüber trivialen Fehlerrückgaben.

Berücksichtige aber auch, dass ein begrenztes Negativergebnis bei guter Differentialabdeckung wissenschaftlich wertvoll sein kann.

## 6. Experimentfreigaben

Unterscheide drei Stufen:

**Stufe A – rein lesende/offline Untersuchungen:** Codeanalyse, Spezifikationsprüfung, Recherche, Kompilierung, statische Tests und Dokumentation. Selbstständig durchführen.

**Stufe B – begrenzte GPU-/Runtime-Experimente:** Kleine kontrollierte Tests mit ausschließlich eigenen Testdaten und klaren Ressourcen-/Zeitgrenzen. Zur Freigabe vorbereiten.

**Stufe C – potentiell systemgefährdende Experimente:** GPU-Hangs, Resets, Cross-Client-Störungen, intensive Fuzzing- oder Speicherbelastungskampagnen sowie vergleichbare Risikofälle. Separat planen und nur nach ausdrücklicher Zustimmung ausführen.

Keine Treiberänderungen, öffentlichen Schwachstellenmeldungen oder GitHub-Pushes ohne Freigabe.

## 7. Erwartetes Ergebnis

Liefere:

1. Eine substanzielle Literature Review 2024–2026.
2. Eine priorisierte Security-Hypothesenmatrix mit mindestens 8–12 konkreten Hypothesen.
3. Eine Neuheitsanalyse gegenüber der aktuellen Literatur.
4. Eine Bewertung, welche Hypothesen H1-, H2- oder implementierungsübergreifend sind.
5. Die drei korrigierten H2-C1-Werkzeuge mit Offline-Nachweisen.
6. Einen konkreten Experimentplan für die drei aussichtsreichsten Security-Hypothesen.
7. Eine Publikationsbewertung: Full Conference Paper, Journal Paper oder nur begrenzter Zusatzbefund.

**Arbeite nicht nur einen bestehenden Testplan ab. Suche aktiv nach übersehenen Zustandsübergängen, schwachen Sicherheitsannahmen und noch nicht untersuchten Verletzungsmechanismen.**

Bleibe wissenschaftlich kritisch. Erfinde keine Vulnerabilities, aber verwerfe aussichtsreiche Hypothesen auch nicht allein deshalb, weil bisher nur ein Konformitätsfehler nachgewiesen wurde.

Ziel ist eine neue Security-Discovery-Phase mit konkreten, prüfbaren Hypothesen und hoher wissenschaftlicher Qualität.
