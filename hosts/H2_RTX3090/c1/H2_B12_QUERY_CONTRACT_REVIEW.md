# B12: Bewertung der NVIDIA-Queryrückgaben

## Evidenz und normative Basis

Untersucht wird **ausschließlich** `inventory/2026-10-31_C0_release/opencl_query.stdout` aus dem Übergabe-ZIP sowie der dort mitarchivierte `query_opencl.py`. Kein neuer Querylauf. NVIDIA 580.178.04 meldet auf Plattform und Gerät `cl_khr_semaphore` und `cl_khr_external_semaphore`, jeweils Version 1.0.0; opaque-FD-Import ist beworben. Der Plattform-/Gerätezustand ist der C0-Snapshot, keine aktuelle Neumessung.

Normative Basis: OpenCL-Docs **`0027b7f1a5e97b1bf5256888757479e012a5cb59`**, insbesondere die vollständigen Plattform- und Runtime-Kapitel einschließlich Extensiontabellen. Die früher archivierten Extensionübersichten allein enthielten diese Query-Verträge nicht vollständig. Numerische Werte/Typbreiten wurden mit OpenCL-Headers **`v2025.07.22`** und den im C0 archivierten Headern abgeglichen. Die Erweiterungsnummern im Treiber sind keine Zusage eines lokal absolvierten CTS-Laufs.

Primärquellen: [Plattform-/Device-Tabellen und Fehlerbedingungen](https://github.com/KhronosGroup/OpenCL-Docs/blob/0027b7f1a5e97b1bf5256888757479e012a5cb59/api/opencl_platform_layer.asciidoc), [Semaphore-Typen und Objektqueries](https://github.com/KhronosGroup/OpenCL-Docs/blob/0027b7f1a5e97b1bf5256888757479e012a5cb59/api/opencl_runtime_layer.asciidoc), [Header](https://github.com/KhronosGroup/OpenCL-Headers/blob/v2025.07.22/CL/cl_ext.h). Die lokal gespeicherten Kopien und Abrufhashes liegen in `sources/`.

## Drei verschiedene Namespaces

| Abfrage | Selektor / Rückgabetyp | Gültiger Empfänger |
|---|---|---|
| Verfügbare Typen der Plattform | `CL_PLATFORM_SEMAPHORE_TYPES_KHR` = `0x2036`, `cl_semaphore_type_khr[]` | `clGetPlatformInfo(platform, …)` |
| Verfügbare Typen des Geräts | `CL_DEVICE_SEMAPHORE_TYPES_KHR` = `0x204c`, gleicher Arraytyp | `clGetDeviceInfo(device, …)` |
| Exportfähigkeiten der Plattform | `CL_PLATFORM_SEMAPHORE_EXPORT_HANDLE_TYPES_KHR` = `0x2038`, `cl_external_semaphore_handle_type_khr[]` | `clGetPlatformInfo` |
| Exportfähigkeiten des Geräts | `CL_DEVICE_SEMAPHORE_EXPORT_HANDLE_TYPES_KHR` = `0x204e`, gleicher Arraytyp | `clGetDeviceInfo` |
| Tatsächlicher Typ eines Objekts | `CL_SEMAPHORE_TYPE_KHR` = `0x203d`, `cl_semaphore_type_khr` | `clGetSemaphoreInfoKHR(semaphore, …)` |
| Exporttypen eines Objekts | `CL_SEMAPHORE_EXPORT_HANDLE_TYPES_KHR` = `0x203f`, Handletyp-Array | `clGetSemaphoreInfoKHR` |

Die beiden Arrayelementtypen sind `cl_uint`, im archivierten ABI vier Byte. Die bisher verwendeten vier Plattform-/Geräteselektoren sind **keine versehentlich an die falsche API gegebenen Objektselektoren**. Letztere dienen erst als explizite negative Kontrollen im vorbereiteten Reproducer.

## Entscheidung pro protokollierter Abfrage

| C0-Abfrage | C0-Rückgabe | Normative Bewertung bei den beworbenen Extensions |
|---|---|---|
| Plattformtypen `0x2036` | -30 | Gültiger Selektor; in dieser Ein-Gerät-Konfiguration ist die gemeinsame Typmenge zu berichten. Binärsemaphore-Unterstützung ist vorgeschrieben |
| Gerätetypen `0x204c` | -30 | Gültiger Selektor; die Geräteliste muss in der Ein-Gerät-Plattform die Mindestanforderung erfüllen. Eine Fehlerantwort ist keine leere Liste |
| Plattformexport `0x2038` | -30 | Gültiger Selektor; ohne gemeinsame Exporttypen darf die erfolgreich erfragte Liste Größe 0 haben |
| Geräteexport `0x204e` | -30 | Gültiger Selektor; auch wenn Export nicht implementiert ist, sieht die Tabelle eine erfolgreiche leere Liste vor |
| Plattform-/Geräteimport `0x2037` / `0x204d` | Erfolg, `[0x2055]` | Gültiger Selektor; opaque-FD als Importtyp ist konsistent mit der Werbung, aber Importfunktionalität nicht erprobt |

Die Spezifikation verlangt bei `cl_khr_external_semaphore` mindestens eine nichtleere Import- oder Exportliste, nicht zwingend beide. **Import-only ist zulässig.** Es legitimiert keinen Fehler beim Abfragen der anderen, gegebenenfalls leeren Liste. Eine Plattformliste ist allgemein die gemeinsame Unterstützung ihrer Geräte; eine leere Plattformliste darf nicht mit „kein einziges Gerät unterstützt diese Fähigkeit“ gleichgesetzt werden. H2 hat im maßgeblichen Snapshot nur ein Gerät.

## Prüfung des bisherigen Aufrufs

Der Python-Reproducer deklariert die Getter mit `cl_void_p`-Handle, `cl_uint`-Selektor, `c_size_t`-Größe und `POINTER(c_size_t)` für die Rückgabe. Der erste Aufruf ist jeweils `(handle, selector, 0, NULL, &size)` und erhält bereits -30. Laut Getter-Vertrag wird die Puffergröße bei `param_value=NULL` ignoriert. Ein zu kleiner Ausgabepuffer oder die spätere Interpretation der Bytes erklärt diesen **ersten** Fehler daher nicht. Die Zahlen stimmen mit den archivierten offiziellen Headern überein; Handleprovenienz und erfolgreiche Name-/Importqueries im selben Prozess sind positive Gegenbelege gegen einen generellen ungültigen Handle.

**Wichtig:** `size=0` bei `status=-30` ist in den Rohdaten keine bestätigte leere Liste. Der lokale Größenwert wurde vorher auf 0 initialisiert; nach einem Fehler ist er kein Capability-Nachweis. Der neue C-Reproducer kennzeichnet deshalb Größenwerte nur nach Erfolg als auswertbar.

Damit sind ein naheliegender Namespace-, Zahlenwert- oder reiner Puffergrößenfehler als Erklärung nicht gestützt. Ein unabhängiger nativer C-Lauf wurde noch nicht durchgeführt. Interne Treiberpfade, ältere Implementierungsannahmen oder fehlende Handler sind mögliche Ursachen, aber durch den geschlossenen Userspace und die vorliegenden Abfragen **nicht kausal bewiesen**.

## Gültig, ungültig und nicht unterstützt

| Situation | Erwarteter Umgang |
|---|---|
| Richtiger Scope, beworbene einschlägige Erweiterung, NULL-Ausgabepuffer zur Größenabfrage | `CL_SUCCESS` und definierte Größeninformation; danach gegebenenfalls zweiter Aufruf |
| Keine Exporttypen unterstützt, aber externe Semaphore-Erweiterung beworben | `CL_SUCCESS`, Größe 0 ist zulässig; Exportrichtung N.A. für funktionale Tests |
| Extension überhaupt nicht beworben | Extensionpfad capabilitybedingt überspringen. Vorhandene Header oder Funktionszeiger aktivieren ihn nicht; -30 wäre dann kein selbständiger Defektbeleg |
| Objektselektor `0x203d` an `clGetPlatformInfo`/`clGetDeviceInfo` | Ungültiger Selektor in diesem Namespace; -30 ist die erwartete negative Kontrolle |
| Nicht-NULL-Puffer kleiner als erforderliche Rückgabe | -30 ist eine gültige Fehlerantwort; für den archivierten Größenaufruf trifft diese Bedingung nicht zu |
| Kein Semaphoreobjekt vorhanden | Objektquery nicht durchführen. Keine umgedeutete Geräteabfrage und keine erfundenen Objektmetadaten |
| Sync-FD-Interop auf H2 | Nicht beworben; H1s Sync-FD-Objektfall entfällt. opaque-FD ersetzt nicht automatisch dessen Vertrag |

Bei einem unerwarteten Getterstatus wird kein exportierbares Objekt erzeugt und keine Interopfunktion auf Verdacht aufgerufen. Ein erfolgreich auflösbarer Exportfunktionszeiger ist kein Export-Capability-Nachweis.

## CTS-Abgleich und Schlussfolgerung

Am gepinnten CTS-Commit **`406533a0b437f49d2e542bd293789adc7900cbed`** behandelt der Helper `get_device_semaphore_handle_types` Fehler beim Größenaufruf als Testfehler und erlaubt ausdrücklich eine Größe 0. `doTestImportExport` verwendet diesen Helper für Import und Export. Das stützt die Unterscheidung zwischen „leere Exportliste“ und „ungültiger Query“. Der mit `external_semaphores_queries` benannte Test fragt dagegen Eigenschaften **importierter Objekte** ab und ist kein eigenständiger Plattformtypen-/Exportlisten-Querytest. Details und begrenzter Inspektionsumfang: [CTS-Mapping](H2_CTS_CONTRACT_MAP.md).

**Abgeschlossene Vertragsbewertung:** Die vier fraglichen Abfragen sind normativ zulässig; Exportunterstützung darf fehlen, die entsprechenden gültigen Listenqueries bleiben dabei gültig. Die archivierten -30 passen nicht zum geklärten Vertrag. Die zutreffende Arbeitsklassifikation lautet **„Query-Vertragsabweichung im vorhandenen C0-Log, unabhängige native Bestätigung und Root Cause offen“**. Es wird weder ein bestätigter interner NVIDIA-Bug noch eine Sicherheitslücke behauptet und noch keine neue F1-Kampagne als ausgeführt gezählt.

Nächster separat freigabefähiger Schritt: [B12-Q-Reproducer](reproducers/h2_b12_query.c), fünf frische Prozesse mit gültigen Namensqueries, den sechs einschlägigen Extensionqueries, zwei absichtlich falschen Namespacequeries und zwei zu kleinen Namenspuffern. Kein Kontext, Semaphoreobjekt, Vulkan-Device, Programm, Queue oder Dispatch. Ein C-Pass würde die bisherige ctypes-Beobachtung relativieren; erneut -30 bei bestandenen Kontrollen würde die Query-Vertragsabweichung unabhängig bestätigen. Keine allgemeine C0-Wiederholung erforderlich.
