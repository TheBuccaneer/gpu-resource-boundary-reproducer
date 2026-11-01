# Neuheit und Publikationsbewertung

Es wurden **keine neuen Vulnerabilities nachgewiesen**. Die abgeschlossene StufeA liefert Hypothesen, Vertragskorrekturen und einen ausführbaren Arbeitsablauf für spätere freizugebende Implementierung/Läufe. Eine Literaturauswahl kann keinen erschöpfenden Ausschluss identischer Prior Art leisten.

## Was bereits vorliegt

StepStone (L01) umfasst OpenCL3.0 und Library-basiertes GPU-Treiberfuzzing mit LLM-Beschreibungen. WebGlitch (L02) umfasst gültige und ungültige stateful API-Aufrufe samt Ownership und Lebenszyklen. CLFuzz (L15) zeigt ältere OpenCL-Kernelfuzzingmethoden, cuFuzz (L12) ganze Host-/Deviceprogramme. „LLM + GPU-API“ ist keine neue Methode.

H1-B7 belegt bereits Kontextzuordnungsprobleme im selben Prozess; H1-B4 belegt SVM-Lifetime-/Reuse-Korruption; SEC1 Availability und SEC2 Remanenz sind geschlossen. Private B4-/B9-Nachträge fehlen und müssen intern gegen nahe Varianten geprüft werden. Eine NVIDIA-Replikation oder anderer Datentyp allein ist kein neuer Mechanismus.

LeftoverLocals (L04) belegt bereits echte fremde Local-Memory-Reste unter seinem Modell. Guardian (L16) zeigt zusätzliche Mandantenschutzmechanismen im gemeinsamen Adressraum. Gewöhnliche OpenCL-Kontexte sind deshalb nicht automatisch OS-Sicherheitsdomänen. CuSafe (L03) und GPU Memory Exploitation (L05) sind relevante Speicherfehlerarbeiten; Anwendungs-OOB ist kein neuer Runtime-Isolationsbefund.

## Engste offene Frage

Können normativ gültige Fehlernachzustände und Referenzübergänge trotz korrekter Fehlerantwort Daten an ein falsches lebendes Objekt binden oder einen privaten Prozessstore zugänglich machen? S01 prüft den engen Fehlernachzustand, S02 ausschließlich gültige Lifetimepfade, S03 einen voraussetzungsreichen Prozessimpact. Diese Kombination ist eine überprüfbare Forschungsfrage, noch kein nachgewiesener Literaturgap oder Fund.

H2 besitzt GDDR6X und kein MIG. GPUHammer/GDDRHammer/GPUThor erfordern gerätespezifische Nachweise; begrenzte GDDR6X-Negativergebnisse sind keine Immunitätsgarantie. Behind Bars/PAGE404 verwenden MIG/GH200-Migration, die H2 fehlen. RFM-Timing ist nicht gleich Rowhammer; gewöhnlicher Lastslowdown belegt keinen Fehlerisolationbruch. Diese Zweige werden nicht zur Verstärkung des Papers künstlich hinzugefügt.

## Entscheidung nach Ergebnissen

| Ergebnis | Realistische Publikationsperspektive |
|---|---|
| Nur B12-Queryabweichung / falscher Code ohne Datenwirkung | Begrenzter Konformitätszusatz, Artefakt oder fokussierter Bericht; kein starker neuer Security-Full-Paper-Claim |
| S01/S02 reproduzierbare neue Root Cause mit kausalen Kontrollen, validem Programm und nachgewiesenem Impact | Full Conference Paper prüfenswert, abhängig von Neuheit, Generalität und Abgrenzung zu privaten H1-Folgen |
| S03 private Prozessdaten über zulässigen APIpfad plus neue Ursache | Starker Security-Kandidat, sofern Herkunft, Autorisierung, Versionsgrenzen und alternative Datenkanäle geklärt sind |
| Nur begrenzte Negativergebnisse | Saubere Differentialstudie möglich; Beitrag muss semantische Abdeckung, Modell und Aussagegrenzen tragen |
| Journal-Erweiterung | Zusätzliche kausale Tiefe, Stack-/Versionsbreite, unabhängige Replikation und neues Modell; bloß mehr Testfälle reicht nicht |

Für starke Claims nötig: minimaler Reproducer, exakte Norm-/Stackpins, positive/negative und kausale Kontrolle, vollständige Daten, Failure-/Nullresults, Root-Cause-Evidenz soweit der offene/geschlossene Stack es erlaubt, unabhängige Replikation auf einer weiteren geeigneten Implementierung oder Version. Closed-source Nutzerraum kann die Ursachenauflösung begrenzen; dann die entsprechende Behauptung begrenzen. Keine Veröffentlichung oder Disclosure ist mit dieser Planung genehmigt.
