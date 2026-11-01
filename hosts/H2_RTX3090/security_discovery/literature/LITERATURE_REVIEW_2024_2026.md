# Literature Review: GPU-Sicherheit und API-Zustände, 2024–2026

**Cutoff31.10.2026.** 15 einschlägige Arbeiten/Preprints aus2024–2026 plus eine ältere OpenCL-Grundlage; keine erschöpfende Vollerhebung. Recherchiert wurden Titel aus dem Auftrag und ihre gezielten Verwandtschaften zu Runtime-Fuzzing, Isolation, Lebenszyklen, Remanenz und Availability. Verwendet werden Autorenfassungen, offizielle Proceedings/Veranstalter und NVIDIA-Dokumentation; News-/Suchsnippets dienten höchstens der Auffindung, nicht als technische Belege.

Gelesen wurden Originalabstracts sowie gezielte Methoden-, Threat-Model-, Ergebnis- und Limitationsabschnitte. Das ist keine unabhängige Reproduktion der Papers oder ihrer CVEs. Fundstellen nennen PDF-Seiten einschließlich etwaiger USENIX-Titelseite. PDF-Hashes/Abrufdaten liegen in `SOURCES*.json`; vollständige Papers und extrahierte Volltexte liegen nur temporär außerhalb des ZIPs. Die Zusammenfassungen sind eigene Paraphrasen, keine Volltextkopien. `READING_LOG.json` dokumentiert die verwendeten Fassungen/Fundstellen.

**Wichtigste Neuheitsgrenze:** Weder OpenCL-Library-Fuzzing noch stateful/ungültige Ownership-API-Fälle sind als solche neu. Die Forschungsfrage muss darüber hinaus einen spezifischen Mechanismus und einen korrekt abgegrenzten Sicherheitsimpact belegen. Ein neuer NVIDIA-Befund wird aus dieser Recherche nicht behauptet.

## L01 — StepStone: LLM-Based GPU Kernel Driver Fuzzing via User-Space Libraries

Zou, Pu et al.; **IEEE S&P 2026**. [Primärquelle](https://www.cs.ucr.edu/~zhiyunq/pub/oakland26_gpu_fuzzing.pdf).

**Forschungsfrage:** Wie erreicht Library-Fuzzing tiefere GPU-Kerneltreiberpfade als IOCTL-Beschreibungen allein?

**Methode:** LLMs extrahieren API-Typen, Abhängigkeiten und Wertebereiche für Syzkaller; Bibliotheken vermitteln Aufrufe zum Kernel. OpenCL 3.0 gehört ausdrücklich zum untersuchten NVIDIA-/AMD-Stack.

**Bedrohungsmodell:** Unprivilegierte GPU-Nutzer; instrumentierte NVIDIA-/AMD-/Mali-Treiber. Die Evaluation nutzt u. a. RTX6000 Ada und NVIDIA560.35.03, nicht unseren580.178.04-Host.

**Berichtetes Ergebnis:** Berichtet11 neue NVIDIA-Treiberbugs und höhere Coverage; darunter UAF/OOB, aber nicht11 unabhängig bewiesene Schutzdomänenbrüche.

**Grenzen:** Geänderte Kernelmodule/KCOV/KASAN und lange Kampagnen sind hier nicht freigegeben. Bibliotheksbeschreibung und LLM-Ausgabe sind fehlerbehaftet.

**Übertragung auf OpenCL2 — eigene Bewertung:** Library-Einstieg und strukturierte Abhängigkeiten übernehmen. Keine „erste OpenCL-API-Fuzzing“-Neuheit beanspruchen; H2 liefert ohne solche Instrumentierung keine gleichwertige Kernelcoverage.

Relevante gelesene PDF-Seiten: 1, 10, 12, 14.

## L02 — WebGlitch: A Randomised Testing Tool for the WebGPU API

Wong und Donaldson; **ECOOP 2025, Experience Paper; DOI10.4230/LIPIcs.ECOOP.2025.39**. [Primärquelle](https://drops.dagstuhl.de/entities/document/10.4230/LIPIcs.ECOOP.2025.39).

**Forschungsfrage:** Wie lässt sich komplexer GPU-API-Zustand wirksam automatisch erzeugen und vergleichen?

**Methode:** Rekursiver Aufbau zulässiger Objektzustände, deklarative Modelle, Crash-/Sanitizer-/Differentialorakel. Ein Modus erzeugt auch ungültige Aufrufe.

**Bedrohungsmodell:** WebGPU-Implementierungen in Browsern/JavaScript-Runtimes und darunterliegende Shadercompiler; robuste Verarbeitung nicht vertrauenswürdiger Programme.

**Berichtetes Ergebnis:** Paper berichtet24 neue Bugs,15 damals behoben. Beispiele betreffen auch Gerätezuordnung, ungültige Zustände und Lebenszyklen.

**Grenzen:** WebGPU-Objekt- und Fehlersemantik ist nicht OpenCL. Weder Zustandsmodellierung noch Invalid-Call-Ownership-Tests sind durch OpenCL2 neu erfunden.

**Übertragung auf OpenCL2 — eigene Bewertung:** Modell und Reduktion adaptieren; als mögliche Erweiterung explizite Fehler-Nachzustände, Ressourcenlebenszeiten und getrennte Sicherheitsdomänen mit messbaren Datenorakeln evaluieren.

Relevante gelesene PDF-Seiten: 1, 8, 9, 13, 14, 24.

## L03 — CuSafe: Capturing Memory Corruption on NVIDIA GPUs

Lu et al.; **USENIX Security 2026**. [Primärquelle](https://www.usenix.org/conference/usenixsecurity26/presentation/lu-hongyi).

**Forschungsfrage:** Wie erkennt man räumliche und zeitliche GPU-Speicherfehler auf verfügbarer NVIDIA-Hardware praktikabel?

**Methode:** Compiler-/Runtime-Sanitizer mit Pointertags, In-band-Bounds, Stack-Epochen und Adressrandomisierung; genaue Allokationsidentität statt Adresse allein.

**Bedrohungsmodell:** Fehlerhafte GPU-Anwendungsprogramme; instrumentierte CUDA-/LLVM-Kette. Die gelesene Vorabfassung evaluiert RTX5090/LLVM21.

**Berichtetes Ergebnis:** Berichtet Abdeckung von33 synthetischen Fehlerfällen und Benchmarks mit44 Programmen; durchschnittlich13% Laufzeitmehrkosten.

**Grenzen:** Ein Sanitizer für Anwendungsmemory ist kein automatisches Oracle für eine OpenCL-ICD-/Prozessgrenzverletzung; die endliche Testsuite ist kein universeller Sicherheitsbeweis.

**Übertragung auf OpenCL2 — eigene Bewertung:** Epochen und Bounds als Denkmodell nutzen. Keine Installation behaupten; H2-Canaries brauchen selbst gültige Programme und eigenständige Lifetime-Verträge.

Relevante gelesene PDF-Seiten: 1, 5, 7, 10, 12.

## L04 — LeftoverLocals: Listening to LLM Responses Through Leaked GPU Local Memory

Sorensen und Khlaaf; **2024 Forschungsbericht / arXiv2401.16603; Herstellerkoordination**. [Primärquelle](https://blog.trailofbits.com/2024/01/16/leftoverlocals-listening-to-llm-responses-through-leaked-gpu-local-memory/).

**Forschungsfrage:** Können lokale GPU-Speicherreste private Daten anderer Prozesse preisgeben?

**Methode:** Writer-/Listener-Paare, Canaries und LLM-Demonstration; Listener liest absichtlich nicht initialisierten lokalen Speicher.

**Bedrohungsmodell:** Ko-residente, getrennte Prozesse/Container mit GPU-Zugang; keine legitime Übergabe der Opferdaten.

**Berichtetes Ergebnis:** Lecks auf getesteten AMD-/Apple-/Qualcomm-Konfigurationen; NVIDIA bestätigte damals keine Betroffenheit. Nicht nur irgendein nichtnulles Wort gilt als Fremdprozessbeleg.

**Grenzen:** OpenCL-C-UB und Compilerverhalten sind Confounder. Historische Herstellerantwort ersetzt keine aktuelle H2-Messung und begründet weder heutige Betroffenheit noch Fehlerfreiheit.

**Übertragung auf OpenCL2 — eigene Bewertung:** Remanenz ist vorbekannt; H1-SEC2 nicht wiederholen. Für H2 nur gezieltes Herkunfts-/Schutzdomänenexperiment mit privater Opfernonce und eigener Evidenz sinnvoll.

Relevante gelesene PDF-Seiten: 1, 4, 9.

## L05 — GPU Memory Exploitation for Fun and Profit

Guo, Zhang und Yang; **USENIX Security 2024**. [Primärquelle](https://www.usenix.org/conference/usenixsecurity24/presentation/guo-yanan).

**Forschungsfrage:** Welche Folgen haben Buffer-Overflows innerhalb moderner GPU-Anwendungen?

**Methode:** Analyse von CUDA-Speicherräumen und Kontrollfluss; zeigt Daten-/Codeangriffe und Auswirkungen auf ML-Anwendungen.

**Bedrohungsmodell:** Angreifer liefert Eingaben an eine verwundbare GPU-Anwendung; ein Anwendungsfehler ist der Ausgangspunkt.

**Berichtetes Ergebnis:** Speicherkorruption kann Modellzustand und GPU-Kontrollfluss beeinflussen; die Untersuchung umfasst mehrere NVIDIA-Generationen.

**Grenzen:** Daraus folgt kein allgemeines Lesen fremder Betriebssystemprozesse durch einen ansonsten zulässigen OpenCL-Client.

**Übertragung auf OpenCL2 — eigene Bewertung:** Impact-Orakel stärker als API-Codes wählen. Eine absichtlich OOB lesende eigene Kerneldatei wäre keine neue Runtime-Isolationslücke.

Relevante gelesene PDF-Seiten: 2, 5, 8, 11, 16.

## L06 — Not so Refreshing: Attacking GPUs using RFM Rowhammer Mitigation

Nazaraliyev et al.; **USENIX Security 2025**. [Primärquelle](https://www.usenix.org/conference/usenixsecurity25/presentation/nazaraliyev).

**Forschungsfrage:** Können Refresh-Schutzmechanismen selbst Timing-Information oder Availability-Störungen erzeugen?

**Methode:** Adress-/Bankanalyse und Timingexperimente mit RFM-Interferenz; covert/side channels sowie Slowdown.

**Bedrohungsmodell:** Unprivilegierter Spy und Opfer teilen GPU-/Speicherressourcen; zusätzlich SoC-Szenarien.

**Berichtetes Ergebnis:** Paper berichtet Fingerprinting, verdeckte Kommunikation und im Mittel mehr als4,8-fachen Slowdown.

**Grenzen:** RFM-Timing ist nicht gleich Bitflip. Gerätespezifische Speicher-/Controllerannahmen und Ko-Lokation sind erforderlich; kein unmittelbarer3090-Nachweis.

**Übertragung auf OpenCL2 — eigene Bewertung:** Normale Konkurrenz nicht als Fehlereindämmungsbug ausgeben. Hohe Aktivierungs-/Störungslast ist StufeC; Timingkanäle haben zunächst geringere Priorität als klarer Daten-/Lifetime-Impact.

Relevante gelesene PDF-Seiten: 2, 4, 5, 9, 12.

## L07 — GPUHammer: Rowhammer Attacks on GPU Memories are Practical

Lin, Qu und Saileshwar; **USENIX Security 2025**. [Primärquelle](https://www.usenix.org/conference/usenixsecurity25/presentation/lin-shaopeng).

**Forschungsfrage:** Ist Rowhammer auf diskretem GPU-Speicher praktisch möglich?

**Methode:** GDDR-Bankcharakterisierung und GPU-spezifische Speicherzugriffe; kontrollierte Modellmanipulation als Effekt.

**Bedrohungsmodell:** Unprivilegierter CUDA-Nutzer in zeitgeteilter Umgebung; RTX A6000 mit GDDR6.

**Berichtetes Ergebnis:** Berichtet Bitflips auf dem getesteten A6000 und daraus folgende ML-Integritätsverluste.

**Grenzen:** Andere Speichertypen, Controller und Maßnahmen brauchen eigenen Nachweis. Die3090 verwendet GDDR6X; keine Gleichsetzung mit A6000.

**Übertragung auf OpenCL2 — eigene Bewertung:** Wichtige Abgrenzung eines bekannten Hardwareangriffs. Kein geeignetes Standardexperiment für unsere normativ begründete OpenCL-Runtime-Studie.

Relevante gelesene PDF-Seiten: 2, 5, 8, 14.

## L08 — GDDRHammer: Greatly Disturbing DRAM Rows — Cross-Component Rowhammer Attacks from Modern GPUs

Hu et al.; **IEEE S&P 2026**. [Primärquelle](https://gddr.fail/files/gddrhammer.pdf).

**Forschungsfrage:** Wie verbreitet und wie folgenschwer sind GPU-Rowhammer-Effekte jenseits eines einzelnen Geräts?

**Methode:** Charakterisierung von25 Karten, stärkere Zugriffsmuster sowie Analyse von Allocator-/Seitentabellenbeziehungen.

**Bedrohungsmodell:** Unprivilegierter GPU-Client mit hinreichend verwundbarem Speicher; Hosteffekt an zusätzliche DMA-/IOMMU-Annahmen gebunden.

**Berichtetes Ergebnis:** Berichtet wesentlich mehr Flips und einen GPU-zu-CPU-Speicherzugriff; nicht automatisch identisch mit jedem behaupteten Root-Shell-Szenario.

**Grenzen:** Keine Übertragung der untersuchten Controller-/Speicherprofile auf jede Ampere-Karte. Quellenbezeichnungen von Ada-Speicher sind nicht unsere3090-Hardwaremessung.

**Übertragung auf OpenCL2 — eigene Bewertung:** Explizite Übersetzung von Speicherprimitive zu Schutzdomäneneffekt als wissenschaftlicher Maßstab. Kein Auftrag für Seitentabellenmanipulation oder Hammering auf H2.

Relevante gelesene PDF-Seiten: 1, 3, 9, 18.

## L09 — GeForge: Hammering GDDR Memory to Forge GPU Page Tables for Fun and Profit

Wan et al.; **IEEE S&P 2026**. [Primärquelle](https://gddr.fail/files/GeForge.pdf).

**Forschungsfrage:** Kann aus GPU-Bitflips eine praktisch verwertbare Übersetzungs- und Hostgrenzverletzung entstehen?

**Methode:** Analyse von Allokations-/Adressübersetzungszuständen und datenabhängige Fehlerausnutzung; End-to-End-Effekt.

**Bedrohungsmodell:** Unprivilegierter CUDA-Nutzer; für den gezeigten direkten Hostzugriff ist ausgeschaltete oder nicht wirksame IOMMU-Isolation relevant.

**Berichtetes Ergebnis:** Berichtet breite GPU-Speicherautorität und systemweiten Impact unter den genannten Voraussetzungen.

**Grenzen:** Ein solcher Hostkonfigurationswechsel ist kein zulässiger Trick, um OpenCL2 stärkere Ergebnisse zu verschaffen. GDDR6-Resultate sind kein3090-Beleg.

**Übertragung auf OpenCL2 — eigene Bewertung:** Nur als verwandte Grenzverletzung und zur Prüfung unserer Bedrohungsmodellannahmen verwenden; keine Reproduktion eingeplant.

Relevante gelesene PDF-Seiten: 1, 3, 4, 13.

## L10 — GPUBreach: Privilege Escalation Attacks on GPUs using Rowhammer

Lin et al.; **IEEE S&P 2026**. [Primärquelle](https://gpubreach.ca/).

**Forschungsfrage:** Ist ein GPU-Hardwarefehler auch bei aktiver IOMMU bis zum Host relevant?

**Methode:** Kombiniert GPU-Speicherkorruption mit Fehlern in vertrauenswürdigem Treiberzustand.

**Bedrohungsmodell:** Unprivilegierter Nutzer; GDDR6-A6000 und gezielt analysierte Host-/Driverpfade, keine bloße GPU-Anwendungs-OOB.

**Berichtetes Ergebnis:** Berichtet GPU-/Prozesswirkungen und Hosteskalation trotz aktiver IOMMU.

**Grenzen:** Der kombinierte Mechanismus und seine Voraussetzungen dürfen nicht als allgemeine Eigenschaft unserer580-Treiberversion interpretiert werden.

**Übertragung auf OpenCL2 — eigene Bewertung:** Impact muss an einer klaren Grenze kausal gezeigt werden. Unser Ansatz priorisiert zulässige Runtime-Zustandsübergänge statt eine schon bekannte Hardwareprimitive nachzustellen.

Relevante gelesene PDF-Seiten: 1, 4, 11, 12.

## L11 — GPUThor: Amplifying Rowhammer Attacks via Non-Uniform Patterns to Exploit ECC-Protected GPUs

Lin, Qu, Rajeev und Saileshwar; **2026 Preprint; CCS2026 für15.–19.11. angekündigt**. [Primärquelle](https://gputhor.com/).

**Forschungsfrage:** Reicht ECC als pauschaler Schutz gegen GPU-Rowhammer?

**Methode:** Untersucht stärkere nichtuniforme Zugriffsmuster und ECC-Fehlerwirkungen auf GDDR6-Workstationkarten.

**Bedrohungsmodell:** Unprivilegierte GPU-Arbeit gegen bestimmte Ampere-A4000/A4500/A5000/A6000; separate Hardwarebedingungen.

**Berichtetes Ergebnis:** Berichtet auch Mehrbitfehler auf ECC-Geräten. Aktuelle Muster erzeugten auf den untersuchten GDDR6X-Konfigurationen keine Flips.

**Grenzen:** Ein begrenztes negatives Hardwareergebnis ist keine Immunitätsaussage. Am31.10. liegt der Konferenztermin noch voraus; Artefaktfreigabe ist erst für15.11. angekündigt.

**Übertragung auf OpenCL2 — eigene Bewertung:** Verhindert den veralteten Schluss „ECC löst alles“. Für3090 weiterhin niedrige Relevanz ohne spezifischen GDDR6X-Nachweis; StufeC und nicht Teil der nächsten Pläne.

Relevante gelesene PDF-Seiten: 1, 3, 12, 16.

## L12 — Hunting CUDA Bugs at Scale with cuFuzz

Ibn Ziad und Kozyrakis; **OOPSLA1/PACMPL2026; DOI10.1145/3798231**. [Primärquelle](https://doi.org/10.1145/3798231).

**Forschungsfrage:** Wie vermeidet man falsche GPU-Fehler, wenn Hostbedingungen und Gerätekernel zusammengehören?

**Methode:** Whole-program fuzzing, kombinierte Host-/Device-Coverage und getrennte Sanitizerläufe.

**Bedrohungsmodell:** CUDA-Anwendungs-/Bibliotheksbugs unter vollständigen Host-Device-Invarianten, nicht primär OpenCL-ICD-Grenzen.

**Berichtetes Ergebnis:** Berichtet43 Bugs in14 Programmen; Kernel-isolierte Testeingaben erzeugten zusätzliche falsche Positive.

**Grenzen:** CUDA-Instrumentierung und Compute-Sanitizer sind keine gleichwertige OpenCL-Sicherheitsprüfung; Werkzeuge und Treiber verändern Beobachtung und Timing.

**Übertragung auf OpenCL2 — eigene Bewertung:** Nur ganze zulässige API-Sequenzen reduzieren und replayen. Keine Orakel, die gültige Host-Präbedingungen nach Reduktion verloren haben. Nicht mit dem anderen „CuFuzz“-Preprint2601.01048 verwechseln.

Relevante gelesene PDF-Seiten: 1, 2, 3, 14.

## L13 — Behind Bars: A Side-Channel Attack on NVIDIA MIG Cache Partitioning Using Memory Barriers

Gu et al.; **USENIX Security 2026**. [Primärquelle](https://www.usenix.org/conference/usenixsecurity26/presentation/gu-cheng).

**Forschungsfrage:** Sind getrennte MIG-Instanzen gegen alle Formen gemeinsamen Cacheverhaltens isoliert?

**Methode:** Analyse von Speicherbarrieren und Timingeffekten über L2-Partitionen; Workload-/Kernelmuster als Seitenkanal.

**Bedrohungsmodell:** Unprivilegierte Clients in verschiedenen MIG-Instanzen auf passenden Rechenzentrums-GPUs.

**Berichtetes Ergebnis:** Berichtet instanzübergreifende Interferenz und Vertraulichkeitswirkungen aus Kernel-Launch-Mustern.

**Grenzen:** MIG ist auf RTX3090 nicht unterstützt; im C0 steht N/A. Zwei OpenCL-Kontexte oder zwei Queues sind keine MIG-Instanzen.

**Übertragung auf OpenCL2 — eigene Bewertung:** Isolation immer an die tatsächlich konfigurierte Domäne binden. Die Methode dient als Abgrenzung, nicht als H2-Experiment oder Ersatz für einen Datenzugriffsbeleg.

Relevante gelesene PDF-Seiten: 2, 4, 11, 12.

## L14 — PAGE404: Breaking GPU Isolation Through Hardware-assisted Page Migration

Nazaraliyev, Zhang und Abu-Ghazaleh; **2026 Preprint v3(21.10.); laut Fassung S&P2027 angenommen**. [Primärquelle](https://arxiv.org/abs/2610.20962v3).

**Forschungsfrage:** Welche Grenzen entstehen durch hardwaregesteuerte Migration und gemeinsam genutzte Zähler?

**Methode:** GH200-spezifische Analyse von Access-Countern, UVM-Migrationspfaden und Timingkanälen.

**Bedrohungsmodell:** Ko-residente Clients einschließlich verschiedener MIG-Instanzen auf Grace-Hopper.

**Berichtetes Ergebnis:** Berichtet verdeckte Kanäle und Workloadinformationen über gemeinsam genutzte Migrationsressourcen.

**Grenzen:** Weder GH200/C2C noch MIG existieren in unserem3090-Setup. 2026er Preprint und2027er Konferenzstatus müssen getrennt bleiben.

**Übertragung auf OpenCL2 — eigene Bewertung:** Neuer als die vorgegebenen Ausgangsarbeiten, aber außerhalb der H2-Mechanik. Lehre: Heterogenitäts-/Migrationsversprechen nie aus gewöhnlichem coarse-grain SVM ableiten.

Relevante gelesene PDF-Seiten: 1, 2, 4, 11.

## L15 — Automated Test Generation for OpenCL Kernels using Fuzzing and Constraint Solving

Peng und Rajan; **GPGPU2020; DOI10.1145/3366428.3380768**. [Primärquelle](https://chao-peng.github.io/publication/gpgpu20/gpgpu20.pdf).

**Forschungsfrage:** Wie erreicht Inputgenerierung Zweige und problematische Interleavings von OpenCL-Kernels?

**Methode:** Mutationsfuzzing, selektives Constraint-Solving und Schedule-Verstärkung auf Kernelseite.

**Bedrohungsmodell:** Gegebene OpenCL-Anwendungskernel und deren Eingaben; Schwerpunkt Code-/Racefehler im Programm.

**Berichtetes Ergebnis:** Zeigt, dass OpenCL-Kernelfuzzing und gezielte Schedulevariation ältere Grundlagen sind.

**Grenzen:** Keine automatische Aussage über Host-Objekt-/ICD-/Prozessisolation. Ein beliebiger gefundener Kernelfehler ist nicht unser gesuchter Runtime-Defekt.

**Übertragung auf OpenCL2 — eigene Bewertung:** Als ältere Basis für Budgetierung und Coveragebegriff aufnehmen; nicht als2024–26-Neuheit zählen. Scheduleverstärkung ist erst in gesondert genehmigten Laufkampagnen zulässig.

Relevante gelesene PDF-Seiten: 1, 3, 5, 8.

## L16 — Guardian: Safe GPU Sharing in Multi-Tenant Environments

Pavlidakis, Vasiliadis, Mavridis, Argyros, Chazapis und Bilas; **2024 Preprint v2**. [Primärquelle](https://arxiv.org/abs/2401.09290v2).

**Forschungsfrage:** Wie können mehrere Mandanten denselben GPU-Adressraum sicher teilen?

**Methode:** Guardian kombiniert PTX-Instrumentierung für Bounds mit API-Interposition und Speicherpartitionierung. Diese Zusatzmechanismen sind Teil des behaupteten Schutzes.

**Bedrohungsmodell:** Verschiedene Mandanten in einer ausdrücklich gemeinsam verwalteten GPU-Adressraumumgebung; die Mandantengrenze entsteht durch Guardians Architektur und Policies.

**Berichtetes Ergebnis:** Die Autoren berichten für ihre Evaluation eine mittlere Laufzeitmehrbelastung von etwa9% bei4–12% Spannweite.

**Grenzen:** Kein Beleg, dass zwei gewöhnliche OpenCL-Kontexte desselben Prozesses von sich aus eine OS-Sicherheitsgrenze bilden. Die eigenen Instrumentierungs- und Policyannahmen dürfen nicht auf NVIDIA-OpenCL übertragen werden.

**Übertragung auf OpenCL2 — eigene Bewertung:** Bei S01/S02 API-Integrität und Kontextzuordnung untersuchen; ein Prozess-/Mandantenbruch braucht eine separat benannte Schutzdomäne und eine kausale Fremddaten- oder Integritätswirkung.

Gelesen: Abstract sowie Architektur-, Bedrohungsmodell- und Evaluationsabschnitte der Autorenfassung.

## Hardware- und Quellenabgleich für H2

Die [NVIDIA-Produktspezifikation](https://www.nvidia.com/en-eu/geforce/graphics-cards/30-series/rtx-3090/) nennt24GB GDDR6X. Die [MIG-Produktliste](https://docs.nvidia.com/datacenter/tesla/mig-user-guide/supported-gpus.html) umfasst spezifizierte Datacenter-/Professionalprodukte, nicht RTX3090. CC≥8 allein ist keine ausreichende Produktfreigabe; der bestehende H2-C0-Befund MIG=N/A passt dazu.

RFM-Timing, Rowhammer-Bitflips und Fehlerisolation sind verschiedene Mechanismen. GDDR6X macht die3090 weder automatisch zum gleichen Ziel wie A6000/GDDR6 noch immun gegen sämtliche Timing-/Speicherfehler. Konkreter Quellenkonflikt: GDDRHammer bezeichnet seine getestete Ada RTX6000 als GDDR6X, während [NVIDIAs RTX6000-Ada-Spezifikation](https://www.nvidia.com/en-us/products/workstations/rtx-6000/) 48GB GDDR6 mit ECC nennt. Ohne Nachweis der untersuchten Einzelkarte übernehmen wir die Paperangabe nicht als verifizierte Produktspezifikation. Die RTX3090-Spezifikation ist davon unabhängig. Für H2 sind Runtime-/Objektzustände zunächst der stärkere und unmittelbarere Forschungsansatz. MIG- und GH200-Arme entfallen; Hardwarehammering und gezielte Cross-Client-Störung bleiben StufeC.

## Such- und Auswahlgrenzen

Ausgangspunkt waren die acht benannten Arbeiten einschließlich beider Hammer-Papers. Ergänzt wurden die direkt relevanten2026er Nachfolgearbeiten, CUDA-Whole-Program-Fuzzing und eine ältere OpenCL-Fuzzingbasis und Guardian zur Mandantengrenze. Suchbegriffe/Datum/IDs stehen in `SEARCH_LOG.json`. Paperzahlen sind damalige Autorenangaben und keine heute erneut validierten Fix-/Vulnerabilitätszahlen. GPUTHORs spätere Artefaktveröffentlichung wurde nicht als bereits verfügbar ausgegeben. Bei PAGE404 ist die2026er Vorabfassung aufgenommen, die2027er Tagung nicht als bereits stattgefunden dargestellt. Kein Exploit-Repository wurde gebaut oder ausgeführt.
