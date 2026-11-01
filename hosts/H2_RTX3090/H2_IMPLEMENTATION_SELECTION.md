# H2 Entscheidungsvorlage

**Empfehlung:** Zunächst den vorhandenen NVIDIA-OpenCL-Stack als primäre H2-Implementierung verwenden. Als zusätzliche GPU-Implementierung **clvk** isoliert vorbereiten; **PoCL CPU** als Kontrollarm. Rusticl/Zink nur für eine gezielte Schichtvergleichsfrage ergänzen. Alternative Runtimes werden in diesem Auftrag nicht installiert.

Begründung und Primärbelege: [Recherche](H2_IMPLEMENTATION_RESEARCH.md); tatsächliche lokale Merkmale: [Capabilities](H2_OPENCL_CAPABILITIES.md). Alle Bewertungen alternativer Stacks sind Auswahlentscheidungen unter offenen C0/C1-Gates, keine experimentell bestätigten Erfolgsquoten.

## Kriterien 1–5: Plattform und Implementierung

| Kandidat | (1) RTX 3090 | (2) RTX 5060 Ti | (3) OpenCL/Features | (4) Eigenständigkeit | (5) Offenheit / Root Cause |
|---|---|---|---|---|---|
| NVIDIA OpenCL | Enumeriert, C1 offen | Plausibel, Host/ICD ungemessen | 3.0, konkrete lokale Feature-Matrix | Eigener herstellerspezifischer Stack | Userspace geschlossen; API-/Versionsdifferentiale |
| clvk | Vulkan-Voraussetzungen teilweise bestätigt, C0 offen | Plausibel, C0 offen | Projekt 3.0, Shader-SPIR-V; kein OOO/Partitionieren | Eigenes Frontend/Runtime/Compiler, NVIDIA Vulkan geteilt | Hoch bis Vulkan-Grenze |
| Rusticl/Zink | Plausibel, Build/Profil offen | Plausibel, zusätzliche Treiberprüfung | Version/profilabhängig; lokaler Rusticl-Pfad fehlt | H1-Frontend geteilt, anderer Backendpfad | Hoch oberhalb NVIDIA Vulkan |
| PoCL CUDA | Plausibel, experimentell | LLVM/libdevice/Architektur offen | Backendabhängiger 3.0-Pfad, Optionalfeatures offen | PoCL/NVPTX eigenständig, CUDA-Unterbau geteilt | Hoch bis CUDA-Grenze |
| Rusticl/Nouveau direkt; NVK-Variante | Direktpfad zurückstellen; NVK-Route separat prüfen | Direktpfad ungeeignet als Annahme; NVK dokumentiert Blackwell | OpenCL-Profil nicht lokal nachgewiesen | Mehr untere Schichten verschieden, Rusticl geteilt | Hoch; Firmwaregrenze bleibt |
| PoCL CPU | Threadripper-Kontrollarm, nicht GPU | Unabhängig vom GPU-Modell | Projekt 3.0, Features nach Build abfragen | Unabhängiger CPU-Ausführungspfad | Hoch, Sanitizer/LLVM-Audit möglich |
| PoCL Vulkan | Technisch plausibel, geringe Reife | Ungeprüft | Unvollständiger Backendpfad | PoCL und clspv mit anderen Armen geteilt | Offen, aber hoher Pflegebedarf |

## Kriterien 6–8: wissenschaftliche Vergleichbarkeit

| Kandidat | (6) B1–B12-Eignung | (7) CTS | (8) Reproduzierbarkeit |
|---|---|---|---|
| NVIDIA | B2/B7/B10 zuerst, B12 Query-Abgleich; B1/B3/B4 später; B5/B6/B8-IL/B11 derzeit ausgeschlossen | Offizieller CTS nutzbar; kein lokaler Pass | Hoch bei Treiber-/ICD-/Kernel-Pinning; geschlossene internals |
| clvk | Gute API-/Ressourcen-/Ownership-Vergleiche; B10 ohne OOO; B4/B5/B6/B8/B12 erst eigene C0, B11 nur ICD-/Kontextvarianten | Projektintegration vorhanden, Konfiguration erforderlich | Gut bei vollständigen Submodul-/clspv-/Vulkan-Pins |
| Rusticl/Zink | Wertvoller H1-Frontend-/Backendvergleich; alle Optionalpfade erneut abfragen, B9 nicht als ACO-Replikation | CTS nach Geräteprofil; Zertifizierung anderer Hardware nicht übertragen | Gut, wenn komplettes Prefix/Compiler/Profil archiviert |
| PoCL CUDA | API/Queue/Ressourcen plus PoCL CPU↔GPU; Featurelücken können B4/B5/B6/B8/B12 begrenzen | Kein moderner Vollpass dieses Backends belegt | Mittel, Compiler-/Toolkitkopplung und geringe Reife |
| Nouveau/NVK-Route | Stärkere Treiber-/Compiler-Differentiale denkbar, H1-Frontend bleibt | Vulkan-Konformität genügt nicht für OpenCL; eigene Gates | Separate Boot-/Firmware-/Kernelprovenienz nötig |
| PoCL CPU | B2/B7/B10 und Compiler-/Lifetime-Kontrollen; B1/B3 CPU-Local-Memory nicht als Hardware-LDS gleichsetzen, keine GPU-Containmentaussage | Reifer CPU-Pfad laut Projekt; hier ungetestet | Hoch bei gepinntem Build/CPU-Features |
| PoCL Vulkan | Teilmenge mit vielen Ausschlüssen; geringe zusätzliche unabhängige Abdeckung | Kein belegter Backend-Vollpass | Niedriger im Verhältnis zum Aufwand |

## Kriterien 9–11: Aufwand, Bestandsschutz und Erkenntniswert

| Kandidat | (9) Aufwand | (10) Risiko für Bestand | (11) Novelty / Impact / Generalisierung |
|---|---|---|---|
| NVIDIA | Gering, bereits vorhanden | Kein Installationsrisiko; spätere Experimente separat | Wichtigster Cross-Vendor-Kontrast zu H1 |
| clvk | Mittel/hoch, clspv/LLVM-Build | Niedrig bei privatem Prefix; GPU-Arbeit später separat | Zweites OpenCL-Frontend auf gleicher GPU trennt Hardware- von Runtimeeffekten |
| Rusticl/Zink | Hoch, fehlende Rust-Buildkette | Niedrig ohne systemweite Mesa-Ersetzung | Gezielte Schichtkontrolle statt weiterer vollständig unabhängiger Runtime |
| PoCL CUDA | Mittel/hoch | Niedrig im Prefix; kein Toolkit-/Treiberersatz | NVPTX/CUDA-Differential, aber nicht drei Wege als unabhängige Vendoren zählen |
| Nouveau/NVK-Route | Sehr hoch, separates Bootmedium | Hoch bei Wechsel am laufenden Desktop; separate Umgebung nötig | Größere Root-Cause-Sichtbarkeit, vorerst ungünstiges Aufwand/Nutzen-Verhältnis |
| PoCL CPU | Mittel, vorhandene LLVM-Kette hilfreich | Niedrig im privaten Prefix | Kontrolliert Host-/API-/Compilerursachen; keine GPU-Isolationsgeneralisierung |
| PoCL Vulkan | Hoch durch unvollständigen Backendpfad | Niedrig isoliert, hoher Forschungs-/Pflegeaufwand | Geringe zusätzliche Unabhängigkeit neben clvk/PoCL |

## Priorisierte Rollen

1. **Primäre Forschungsimplementierung:** NVIDIA OpenCL, exakter vorhandener Treiberstand. Sofort als C0-Basis nutzbar; C1/GPU-Arbeit benötigt eigene Freigabe.
2. **Zusätzliche GPU-Implementierung:** clvk auf NVIDIA Vulkan. Erst privates Build-/Installationspaket mit allen Pins und erneutem C0; sein GPU-Pfad ist nicht vollständig unabhängig vom NVIDIA-Treiber.
3. **CPU-Kontrollimplementierung:** PoCL CPU auf dem vorhandenen Threadripper, getrennte ICD-Auswahl und eigene Feature-Matrix.
4. **Optionale H1-Brücke:** Rusticl/Zink nur bei einer benannten Frage, die vom gemeinsamen Rusticl-Frontend profitiert. Kein pauschales Installieren aller Varianten.
5. **Reserve:** PoCL CUDA, falls ein konkret fehlender NVPTX-/CUDA-Differential einen Mehraufwand rechtfertigt.
6. **Derzeit zurückstellen:** direkte Rusticl/Nouveau-Route und PoCL Vulkan; NVK-Route nur in gesonderter Bootumgebung nach neuer Entscheidung.

## RTX 5060 Ti / H3

**Als zusätzliche Architektur sinnvoll**, sobald tatsächlich zugänglich: Blackwell gegenüber Ampere ermöglicht einen innerhalb desselben Herstellers kontrollierten Architekturdifferential. Das ergänzt die Cross-Vendor-Frage H1↔H2; es ersetzt keinen unabhängigen Runtimevergleich. Der erwartete 16-GB-Ausbau ist Nutzerangabe, hier nicht gemessen.

Auf dem untersuchten Host wurde sie nicht gefunden. Daher vorläufig **H3-Kandidat**, keine Behauptung eines bekannten separaten Rechners. Falls später auf anderem Rechner: eigene H3-Plattformakte. Falls in denselben Host eingebaut: eigene Geräte-/Konfigurationsidentität innerhalb H2 mit PCI-/ICD-Zuordnung; ein anderer physischer Host ist dann nicht gegeben. Bei einer Vergleichsstudie OS, Treiber und Teststand möglichst angleichen, VRAM-/Local-/WG-Grenzen pro Gerät skalieren und Displayrolle separat erfassen. CUDA 12.4 allein ist keine nachgewiesene Blackwell-Buildkette; für NVIDIA OpenCL und Vulkan folgt daraus nicht automatisch ein Toolkit-Updatebedarf.

Installations-/Rollback-Voraussetzungen: [Plan](notes/INSTALLATION_ROLLBACK_PLAN.md). Nächste Schritte und GO-Grenzen: [Readiness](H2_C0_READINESS.md).
