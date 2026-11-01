# Gepinnte Vertrags- und CTS-Zuordnung

Spezifikation: **OpenCL-Docs `0027b7f1a5e97b1bf5256888757479e012a5cb59`**. CTS: **`406533a0b437f49d2e542bd293789adc7900cbed`**. Es wurden gezielt öffentliche Quelltexte abgerufen und statisch gelesen. Kein CTS-Checkout konfiguriert, gebaut oder ausgeführt. Die vollständigen Rohquellen und URL/SHA-256-Metadaten liegen in `sources/`; die JSON-Tree-Dateien sind Pfadindizes, keine vollständigen CTS-Dateikopien.

## Normative Fundstellen im lokalen Snapshot

| Vertrag | Datei / Stellen |
|---|---|
| Gültige Plattformtypen-/Import-/Exportlisten, leere Exportliste, Getterfehler | `sources/opencl_platform_layer.asciidoc`, Zeilen 319–347 und 367–383 |
| Gerätetypen/-export, leere Liste und Mindestanforderung Import oder Export | gleiche Datei, Zeilen 2242–2273 und 2355–2359; Getterfehler 2381–2401 |
| Pflichtunterstützung Binärsemaphore | `sources/opencl_runtime_layer.asciidoc`, 13553–13563 |
| Objektquery-Typ und Exportliste | gleiche Datei, 14310–14333; dies ist kein Plattform-/Device-Namespace |
| Argument-/Kontextvertrag und -34/-49/-51 | gleiche Datei, Abschnitt `clSetKernelArg`, 10922–11095 |
| Geometrie und -53/-54, mehrere gleichzeitig verletzte Bedingungen vermeiden | gleiche Datei, Abschnitt `clEnqueueNDRangeKernel`, 11997–12223 |
| Event-/Waitlist-/Statusverträge | gleiche Datei: `clSetUserEventStatus` ab12477, `clWaitForEvents` ab12541, `clEnqueueMarkerWithWaitList` ab13316; Queue-Erzeugung am Kapitelanfang |

[Plattformkapitel am Commit](https://github.com/KhronosGroup/OpenCL-Docs/blob/0027b7f1a5e97b1bf5256888757479e012a5cb59/api/opencl_platform_layer.asciidoc), [Runtime-Kapitel am Commit](https://github.com/KhronosGroup/OpenCL-Docs/blob/0027b7f1a5e97b1bf5256888757479e012a5cb59/api/opencl_runtime_layer.asciidoc). Zeilennummern beziehen sich auf diese gespeicherten Dateien, nicht auf späteres `main`. Neuere 3.1-Abschnitte bedeuten nicht, dass 3.1-Features auf H2 verfügbar sind.

## CTS: tatsächlich inspizierter Bereich

`CTS_SOURCES.json` enthält 30 Dateien: vollständige Quelltextverzeichnisse `test_conformance/extensions/cl_khr_semaphore/`, `…/cl_khr_external_semaphore/`, `test_conformance/events/` sowie fünf gezielte API-Dateien. Zusätzlich wurde `test_conformance/api/test_platform.cpp` separat gelesen. `cts_tree.json` hat `truncated=false`. Außerhalb dieser Dateimenge wird **keine vollständige CTS-Abdeckungs-/Lückenbehauptung** aufgestellt; DX-Fence-, Vulkan-Helper- und andere Backends wurden nicht vollständig auditiert.

| B-Klasse / eigener Fall | Benannter CTS-Pfad und Test | Bedeutung / Grenze |
|---|---|---|
| B12 Geräte-Import/Exportlisten | `extensions/cl_khr_external_semaphore/test_external_semaphore.cpp`, Helper `get_device_semaphore_handle_types` Zeilen88–119; `doTestImportExport` 197ff., Aufrufe216–224 | Helper verlangt Erfolg beim Größenaufruf und akzeptiert Größe0. Der Code unterscheidet damit fehlenden Export von einem Queryfehler |
| B12 Objektqueries | gleiche Datei, `external_semaphores_queries` ab122 | Fragt nach Vulkan-Initialisierung und Import Eigenschaften realer Objekte ab, nicht die vier fraglichen Plattform-/Device-Typ-/Exportqueries. **Nicht als rein lesenden Querytest starten** |
| B12 funktionale Interoptests | gleiche Datei, `external_semaphores_cross_context` ab313 und `external_semaphores_import_export` ab334 verwenden `doTestImportExport` | Exportlistenabfrage ist also nicht generell ungetestet; die Tests enthalten aber Kontext-/Queue-/Signal-/Wait-/Interopaktionen und sind nicht für PaketA freigegeben |
| B12 Semaphoreobjektbasis | `extensions/cl_khr_semaphore/test_semaphores_queries.cpp`, `semaphores_device_list_queries`, `semaphores_no_device_list_queries` | Prüfen Typ/Kontext/Eigenschaften echter Objekte; Multi-Device-Variante versucht Partitionierung und ist auf H2 capabilitybedingt ungeeignet |
| B12 eng begrenzter offener Abdeckungsbereich | Beide genannten Extensionverzeichnisse sowie `api/test_platform.cpp` und negative Plattform-/Gerätetests | Im gelesenen Bereich wurden keine direkten Verwendungen von `CL_PLATFORM_SEMAPHORE_TYPES_KHR`, `CL_DEVICE_SEMAPHORE_TYPES_KHR` oder `CL_PLATFORM_SEMAPHORE_EXPORT_HANDLE_TYPES_KHR` gefunden. Das ist ein datei-/commitgebundener Suchbefund, keine allgemeine CTS-Lücke |
| B2 Dimensionen | `api/negative_enqueue_ndrange.cpp`, `negative_enqueue_invalid_work_dim` | DIM0 und Überschreitung der unterstützten Dimensionenzahl mit ausreichend großen Hostarrays; passt zum isolierten C1-Vertrag |
| B2 lokale Größen | gleiche Datei, `negative_enqueue_work_group_size_exceeded`, `negative_enqueue_zero_work_group` | Erkennt mehrere gleichzeitig verletzte Größenregeln und toleriert dann die passenden unterschiedlichen Fehler. Unser C1 vermeidet solche Mehrdeutigkeiten; keine Maximal-/Exzesskampagne |
| B7 Setter | `api/test_kernel_arg_changes.cpp` plus normativer Settervertrag | Der gelesene Test behandelt gültige Argumentwechsel; er ist kein Nachweis einer identischen Fremdkontext-Negativprüfung. Keine repo-weite B7-CTS-Lücke behauptet; eigener kleiner Reproducer ist direkt an der Norm orientiert |
| B10 negative Marker | `api/negative_enqueue_marker.cpp`, `negative_enqueue_marker_with_wait_list` | Fremder Kontext, widersprüchliche Waitlistlängen und ungültige Eventeinträge. Eigene Varianten benutzen gültige, bereits aufgelöste User-Events für klaren Scope und begrenztes Warterisiko |
| B10 gültige Marker/Events | `events/test_events.cpp`, `event_enqueue_marker`, `event_enqueue_marker_with_event_list`; `events/test_userevents.cpp`, `userevents` | Benannte Vergleichspunkte, keine Aussage über gesamten CTS oder alle Rennen |
| B10 OOO | `events/test_event_dependencies.cpp`, `out_of_order_event_waitlist_single_queue` und `…multi_queue` | Beschreibt explizite Dependency-Arme. Multi-Device-Tests gehören nicht in die aktuelle H2-Konfiguration |

[CTS-Exporthelper und Interoptests am Commit](https://github.com/KhronosGroup/OpenCL-CTS/blob/406533a0b437f49d2e542bd293789adc7900cbed/test_conformance/extensions/cl_khr_external_semaphore/test_external_semaphore.cpp), [B2-Tests](https://github.com/KhronosGroup/OpenCL-CTS/blob/406533a0b437f49d2e542bd293789adc7900cbed/test_conformance/api/negative_enqueue_ndrange.cpp), [negative Marker](https://github.com/KhronosGroup/OpenCL-CTS/blob/406533a0b437f49d2e542bd293789adc7900cbed/test_conformance/api/negative_enqueue_marker.cpp).

Die H1-CTS-Ergebnisse am historischen Commit `9feccbb8…` bleiben unangetastet. Aktuelle Source-Abdeckung am neuen Pin darf weder als damals vorhandener Test noch als heutiger H2-Pass ausgegeben werden. Für einen späteren CTS-Lauf wären Header-/Harness-Abhängigkeiten separat vollständig zu pinnen und nur ausdrücklich genehmigte Testnamen zu wählen.
