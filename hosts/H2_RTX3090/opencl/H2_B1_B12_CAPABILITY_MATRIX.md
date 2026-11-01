# H2 B1–B12: erste Capability-Matrix

Stand 31.10.2026, ausschließlich C0 für NVIDIA OpenCL 580.178.04 auf RTX 3090. Basis: H1-`PROJECT_STATE.md`, H1-Evidence-Matrix, Quality Review, `docs/H2_ROADMAP.md`, `docs/METHODOLOGY.md` am Commit `3cd984d…`; alle fünf Dokumente gelesen. Diese unveränderten Dokumente liegen jetzt vollständig unter `hosts/H1_AMD660M/`. Kein Ergebnis und kein Fehlerbefund aus H1 wird auf H2 übertragen.

**U = unterstützt (nur beworben/abfragbar), N = nicht unterstützt im vorhandenen Pfad, ? = noch unklar, I = implementierungsspezifisch, G = erst nach gesonderter Freigabe experimentell prüfen.** U ist kein funktionales PASS. Alternative Runtimes erhalten nach Installation jeweils eine eigene C0-Matrix; heute sind sie nicht gemessen.

| Klasse | C0-Einstufung | Übertragbarer Vertrag / Ausschluss | Nächster Gate, keine Ausführung hier |
|---|---|---|---|
| B1 Kapazität | U / I / G | Dynamische Local-Argumente grundsätzlich übertragbar; Limit 49152 statt H1 65536 Byte; NVIDIA-Belegung/Granularität unbekannt | Kleine gültige Kontrollen, Kernel-Limits und bounded Plan vor Grenzfällen |
| B2 Geometrie | U / G | Device-Limits bekannt, Nonuniform=false; Kernel-spezifisches Maximum noch unbekannt | ReqWG-/Dimension-Verträge pinnen, gültige Kontrollen zuerst |
| B3 kombinierte Ressourcen | U / I / G | Raw-Summe versus effektiver Footprint als Invariante; H1-Alignment und LDS-Blöcke nicht übernehmen | Ressourcenmetadaten/Compilerpfad ermitteln, eigene Grenzen |
| B4 SVM/Lifetime | U (coarse) / I / G | coarse-grain buffer beworben; keine fine-grain/atomics. H1-SVMFree-Ordering-Frage prinzipiell relevant | SVM-Baseline, Map/Unmap-Vertrag und Event-Abhängigkeiten; später separat begrenzter Lifetime-Plan |
| B5 rohe Adressen | N / I | H1 `cl_ext_buffer_device_address` fehlt; kein äquivalenter standardisierter Pfad nachgewiesen | N.A. für den vorhandenen H1-Harness. SVM oder Inline-PTX nicht stillschweigend als Ersatz einsetzen; neues Modell nötig |
| B6 Mutable Revalidation | N | Command-Buffer/Mutable-Dispatch fehlen | Deferred, kein negatives Sicherheitsresultat; eventuell alternatives ICD später |
| B7 Objekt-/Kontextzuordnung | U / G | Gewöhnlicher cl_mem-/Kernel-Kontextvertrag implementierungsübergreifend relevant | Setter-Fehlervertrag zuerst, getrennt von jedem fremden Dispatch; beide brauchen eigenes GO |
| B8 Source/IL | N (H1-SPIR-V-Paar) / ? / G | Source-Compiler beworben; kein Kernel-SPIR-V-IL. Vulkan Shader-SPIR-V ist kein Ersatz für Kernel-IL | Native Binary/PTX wären eigene Repräsentationsstudien; keine behauptete H1-Paar-Replikation |
| B9 Backend-Arithmetik | I / ? / G | ACO/RadeonSI-Root-Cause nicht portierbar; abstrakte Ressourcenarithmetik schon | Zulässige Eingabe/Invariante separat begründen, keine H1-OOM-/Timeout-Trigger übertragen |
| B10 Queue/Event-Zustand | U / G | In-/Out-of-Order und Profiling beworben; Device Enqueue nicht unterstützt | Kleine In-Order-, Cross-Queue-, User-Event- und Fehlerkontrollen; OOO eigener Arm |
| B11 Multi-Device/ICD-Identität | N aktuell / ? später | Ein GPU-Gerät, eine OpenCL-Plattform, keine Partitionierung. CPU-Vulkan-Gerät zählt nicht als zweite OpenCL-Plattform | Mehrere ICDs ermöglichen später ICD-Identitätsarm auf derselben GPU; echter Multi-GPU-Arm benötigt weitere Hardware |
| B12 externe Objekte/Interop | U Import / ? Export / I / G | opaque-FD-Import beworben und abgefragt; Semaphore-Typ-/Exportabfragen -30; kein Sync-FD. GL-Kontextzuordnung offen | Gepinnter Vertragsabgleich und Handle-Kompatibilität zuerst; neue Objekterzeugung und Interop nur nach GO |

Priorität aus dem H1-Qualitätsreview: B2/B7/B10 als knappe Vertragsdifferentiale, B12 zunächst nur Dokument-/Query-Abgleich. B1/B3/B4 und alle Verfügbarkeits-/Cross-Client-Arme erst nach Baselines und eigenem begrenztem Plan. B13 Fehlerweitergabe und B14 Versionsvergleich bleiben Querschnittsachsen; ein Treiberupdate ist jetzt nicht vorgesehen.

H1-Grenzen bleiben verbindlich: B1/B3 nicht als zwei unabhängige Root Causes zählen; B4 ist intra-context temporal integrity, B5 außerhalb definierter Nutzung; B7 ist kein Cross-Process-Nachweis; B8 hat historische Rohdatenlücken; B9-Finding, OOM und Recovery nicht kausal zusammenziehen; B10 ist kein vollständiger CTS-Lauf; B12 ist F1 und keine bewiesene Sicherheitsgrenzverletzung. Bei H2 gibt es bisher weder PASS/F0-Kontrollläufe noch F1–F8-Experimentbefunde.
