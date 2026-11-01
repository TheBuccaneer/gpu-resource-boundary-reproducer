# Noch nicht ausgeführter Installations- und Rückkehrplan

Dieser Auftrag installiert nichts. Die folgenden Pläne machen einen späteren Erweiterungsauftrag konkret. Systempakete, Treiber, Module, Kernelparameter und Neustarts bleiben bis zu einer gesonderten Freigabe ausgeschlossen. Ein künftiger Buildauftrag autorisiert nicht automatisch GPU-Tests.

## Gemeinsamer Rahmen für clvk, PoCL und Rusticl

1. Vorher den release-C0-Datensatz, Paketstatus und Hashes abgleichen. Quelle und transitive Abhängigkeiten auf exakte Commits/Versionen pinnen; Build-Rezept, Compiler, Flags und Abhängigkeitsmanifest archivieren. Die Recherche-Heads sind Anker, keine bereits getesteten kompatiblen Lockfiles.
2. Getrennte private Präfixe außerhalb des Systempfads verwenden, etwa `/home/rock/projects/opencl-h2-runtimes/<name>-<commit>/`. Jeder Build erhält eigene Source-/Build-/Prefix-Unterordner. Keine Installation nach `/usr`, kein Ersetzen von Mesa/`libOpenCL.so`, kein `ldconfig`, keine Änderungen in `/etc/OpenCL/vendors`.
3. Zuerst fehlende Buildpakete exakt ermitteln. Deren Installation wäre eine gesonderte Systemänderung; bevor sie beantragt wird, konkrete Paketliste und simulierte Pakettransaktion mit Dependency-/Entfernungsdiff vorlegen. In diesem Auftrag wurde auch keine Pakettransaktion simuliert oder Paketquelle geändert. Alternativ eine gepinnte Buildumgebung nutzen; GPU-Gerätefreigabe dort nicht implizieren.
4. Pro Runtime ein privates ICD-Verzeichnis mit absolutem Pfad der Herstellerbibliothek erstellen. Für den vorhandenen ocl-icd-Loader nur pro Prozess `OCL_ICD_VENDORS=<privates-Verzeichnis>` setzen; keine dauerhafte Shell-/Desktopkonfiguration. Bei kombiniertem ICD-Arm NVIDIA explizit ebenfalls eintragen, Plattform/Gerät über Vendor+PCI-Identität statt Index auswählen.
5. Laufzeitabhängigkeiten über kontrollierte Wrapper/RPATH lösen. Insbesondere clvks als `libOpenCL.so` benannte Buildbibliothek darf den Systemloader nicht durch globale `LD_LIBRARY_PATH`-Änderungen verdecken. Abfrageprozess-Mappings und Hashes müssen Loader und Implementierung auseinanderhalten.
6. Vulkan nur pro Prozess auf die im C0 archivierte NVIDIA-ICD-JSON pinnen (`VK_DRIVER_FILES`); tatsächliche Geräteidentität erneut abfragen. Kein unbemerkter lavapipe-Fallback. Bereits für eine neue Runtime-C0 vorher klären, ob die Enumeration intern GPU-Arbeit/Initialisierung ausführt; eigenes Queryprogramm garantiert nur die von ihm gerufenen APIs.
7. Nach bewilligter Installation ausschließlich C0 erfassen. GPU-Baselines, Build-/Kernelprüfungen und CTS bleiben für den dafür separat bewilligten Schritt. Akzeptanz: unveränderte Systempakete/Module, identischer ursprünglicher NVIDIA-C0-Pfad und ein eindeutig identifizierter zusätzlicher ICD.

**Rollback:** Wrapper verlassen und prozesslokale Variablen aufheben; neuen Prefix nur nach Pfad-/Manifestprüfung aus der Auswahl nehmen und zunächst erhalten/umbenennen. Keine Systembibliothek muss zurückkopiert werden, weil keine ersetzt wurde. NVIDIA-C0 danach erneut abfragen und mit dem vorherigen Stand vergleichen. Abweichungen stoppen die Einführung.

## Kandidatenspezifisch

| Kandidat | Konkrete spätere Vorbereitung | Offene Bedingungen |
|---|---|---|
| clvk | Gepinnten clvk-Commit und exakt dessen Submodule/clspv/LLVM verwenden; CMake-Releasebuild mit `CLVK_VULKAN_IMPLEMENTATION=system`, zunächst Tests deaktiviert; separater Compilerpfad im Wrapper | Vollständige clspv-Buildkette, Installlayout/ICD-Eignung, Vulkan-Features und Ressourcenbedarf; Build darf keinen enthaltenen Smoke-Test automatisch starten |
| PoCL CPU | Gepinnte PoCL-Version, CPU-Backend und ICD an, CUDA/Vulkan/Remote und Tests aus; versioniertes LLVM explizit auswählen | Zu diesem PoCL-Stand passende LLVM-Version und Bibliotheken; SPIR-V-Translator nur bei tatsächlich benötigtem IL-Arm |
| PoCL CUDA | Eigenes Präfix, `ENABLE_CUDA=ON`, NVPTX-LLVM; Toolkit-/libdevice-Pfad explizit; später `POCL_DEVICES=CUDA` im Wrapper | 3090 sm_86 abgleichen; 5060 Ti sm_120 erst nach passendem LLVM-/Toolkitnachweis. Kein Toolkitwechsel ohne zusätzliche Freigabe |
| Rusticl/Zink | Gepinnter 26.2.x-Stand, nur benötigte Gallium-Zink-/Rusticl-Bauteile, `gallium-rusticl=true`, `gallium-drivers=zink`, LLVM und private Rust-Buildkette; später `RUSTICL_ENABLE=zink` | Mesa-Tag und Zink-Profil/Featureprüfung, kompatible LLVM/libclc/Translator-Versionen; keine System-Mesa-Ersetzung |

CMake-/Meson-Optionsnamen vor einem tatsächlichen Build am ausgewählten Commit prüfen. Diese Tabelle ist kein bereits validiertes Installationsrezept. Bauen umfangreicher Compiler kann CPU/RAM belegen; bei einem späteren Auftrag Ressourcenlimits explizit festlegen und keine ungeprüften Standardtargets mit eingebetteten Ausführungstests verwenden.

## Nouveau / NVK: ausschließlich separate Bootumgebung

Keine In-place-Umstellung des derzeitigen NVIDIA-Desktops. Zunächst eigenes Bootmedium/Systemabbild mit festem Kernel-, Firmware-, Mesa-/NVK-/Rusticl-Stand erstellen und Kompatibilität zu GA102 bzw. Consumer-Blackwell prüfen. Bootmanager-/Secure-Boot-/Datenträgerauswahl, Zugänglichkeit bei schwarzem Bildschirm und Backup des bestehenden Systems vorab konkretisieren. Ein Container allein genügt nicht, da er den Kernel und die Gerätebindung teilt.

Erst nach ausdrücklicher Freigabe dieser Änderungen im alternativen System booten und prüfen, welcher Treiber an welcher PCI-Funktion hängt. Kein Versuch, denselben GPU-PCI-Endpunkt gleichzeitig mit NVIDIA und nouveau zu binden. Zuerst C0; Experimente gesondert. **Rückkehr:** zum unveränderten ursprünglichen System booten, NVIDIA-Modul-/Paket-/ICD-Hashes und C0 vergleichen. Keine Modulparameter oder Initramfs der bestehenden Installation ändern. Ist eine solche Trennung nicht sicher umsetzbar, Kandidat zurückstellen.
