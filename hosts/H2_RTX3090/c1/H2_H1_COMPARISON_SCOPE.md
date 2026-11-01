# Wissenschaftlicher H1↔H2-Vergleich für C1

Grundlage: unveränderte H1-Evidenzmatrix, Qualitätsreview, Methodik und PROJECT_STATE am übergebenen Basiscommit. H1 ist AMD660M/Rusticl/RadeonSI/ACO; H2 ist NVIDIA3090 mit eigenem OpenCL-Stack. Das ist ein **kombinierter Hardware-/Softwarekontrast**, kein isolierter Effekt nur der GPU-Architektur. Ampere↔Blackwell wäre eine andere, noch ungemessene Vergleichsachse.

| Klasse | Sinnvoll vergleichbar? | Behandlung in dieser C1-Vorlage |
|---|---|---|
| B1 Local-Kapazität | Abstrakte Ressourceninvariante ja, konkrete H1-65536-Byte-Trigger nein | Außerhalb C1. H2 meldet 49152 Byte; keine Grenz-/Exzessversuche |
| B2 Geometrie | Ja: derselbe Kernel-/Dimension-/Uniformitätsvertrag, jeweils gültige gerätebezogene Parameter | G64 mit L1/16/32, DIM0/4, ReqWG32 gegen16 und G65/L32. H1s Kernelmaximum512 nicht auf H2 übernehmen |
| B3 Ressourcenkomposition | Prinzipiell invariantenseitig; AMD-LDS-Layout/Alignment nicht unverändert portierbar | Außerhalb C1; kein reiner Zahlenersatz als Reproduktion |
| B4 Lifetime/SVM | coarse-grain Buffer-SVM ist auf H2 beworben; Originalfrage prinzipiell später möglich | Hier ausgeschlossen. Keine Free-vor-Completion-Arme und keine übernommenen Kausalpatches |
| B5 rohe Geräteadressen | Nein für den vorhandenen H1-Extensionpfad | `cl_ext_buffer_device_address` fehlt. SVM, PTX oder Vulkan-BDA sind kein stillschweigender Ersatz. N.A., kein PASS/negativer Sicherheitsbefund |
| B6 mutable Dispatches | Nein aktuell | erforderliche Command-Buffer-/Mutable-Dispatch-Erweiterungen fehlen, N.A. |
| B7 ordinary cl_mem/Kontext | Ja für Setter-/Objektvertrag | Kontextmetadaten, gültiger setter, AB/BA fremder setter -34; **kein** fremder Dispatch. Ein Setter-Ergebnis misst weder Korruption noch Cross-Process-Zugriff |
| B8 Source/IL | Nein für das bisherige Kernel-SPIR-V-Paar | Source-Buildkontrolle ja; kein Kernel-SPIR-V-IL-Pfad. clvk-Shader-SPIR-V später als eigener Arm, nicht als identischer H1-Input |
| B9 Backend-Arithmetik | Nicht als ACO-Root-Cause-Replikation | Außerhalb C1; NVIDIA-Backend geschlossen/anders. Keine historischen OOM-/Timeouttrigger übertragen |
| B10 Queue/Events | Ja für die benannten einfachen Verträge | IO/Cross-Queue/User-/Markerfehler; OOO eigener zusätzlicher H2-Arm, kein bereits ausgeführter H1-OOO-Vergleich. `clFlush` jeder Producerqueue explizit |
| B11 Identität mehrerer Geräte/ICDs | Nicht in der gegenwärtigen Konfiguration | eine OpenCL-Plattform/ein Gerät, keine Partitionierung. Zwei Kontexte derselben GPU ersetzen keine zweite Plattform oder GPU |
| B12 Metadaten | Query-Vertragsmethodik ja, konkreter H1-Objektfall nein | H1: Sync-FD-Objektexportliste bei success; H2: Plattform-/Device-Queries geben -30. Unterschiedliche Ebene/Handlearten und Voraussetzungen, daher keine Gleichsetzung desselben Fehlers |

Vergleichseinheit ist das Tupel aus Spezifikationspredicate, zulässigem Ausgangszustand, einzelner geänderter Bedingung und Oracle. PASS und F0 auseinanderhalten. Neue H2-Zahlen stets an exakte Laufzeit-/Quell-/Library-Pins und eigene Counts binden; H1-N nicht mit H2-N poolen. Fehlende Capabilities sind N.A., fehlende Messungen „nicht geprüft“.

Ein künftiger Setter-Wrong-Success wäre ein API-/Metadatenbefund, keine automatisch bewiesene Schutzgrenzverletzung. Verfügbarkeit, Kernelreset, Fremdprozesswirkung und Datenintegrität brauchen eigene freigegebene Versuche. Die Originalberichte werden weder umgeschrieben noch durch neue Ergebnisse ersetzt; insbesondere bleiben die dort dokumentierten B8-Rohdatenlücken und B9-Kausalitätsgrenzen sichtbar.
