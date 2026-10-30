# B10 – kleiner Sicherheits-Nachtest: fremdkontextiges User Event

**Datum:** 20.10.2026  
**Ergebnis:** `PASS` / F0; keine Sicherheitslücke in diesem Lauf nachgewiesen.  
**Umfang:** Ein Prozess, ein gültiger Kontrollpfad und ein ungültiger Waitlist-Aufruf. Keine Stress- oder Wiederholungsläufe.

## Hypothese und Oracle

Hypothese: Ein User Event aus Kontext B könnte eine `clEnqueueFillBuffer`-Operation auf Queue/Buffer aus Kontext A unerlaubt als Abhängigkeit beeinflussen. Die [Khronos-OpenCL-Spezifikation für `clEnqueueFillBuffer`](https://registry.khronos.org/OpenCL/specs/unified/html/OpenCL_API.html#_filling_buffer_objects) verlangt denselben Kontext für Queue und Waitlist-Events und nennt für einen fremden Event `CL_INVALID_CONTEXT`. Bei einem erfolglosen Enqueue darf kein Ausgabe-Event entstehen. Eine Annahme des fremden Events wäre zunächst eine Validierungsabweichung, allein noch kein Beweis für Cross-Client-Impact oder eine ausnutzbare Sicherheitslücke.

Der offizielle CTS-Test `negative_enqueue_marker_with_wait_list` hatte die fremde Kontextzugehörigkeit bereits für `clEnqueueMarkerWithWaitList` geprüft. Dieser Nachtest betrifft die andere API `clEnqueueFillBuffer`; er wiederholt keinen B4- oder B9-GPU-Test.

## Durchführung und Rohbeobachtung

- H1: AMD Radeon 660M, Rusticl/OpenCL 3.0. Der Prozess protokollierte das tatsächlich geladene ICD `/usr/lib/x86_64-linux-gnu/libRusticlOpenCL.so.1.0.0`.
- Quelle `src/b10_final_foreign_event_probe.c`; Binärdatei `probes/b10_final_foreign_event_probe`. Build: `cc -std=c11 -O2 -Wall -Wextra -Werror ... -lOpenCL`.
- Zwei Kontexte auf demselben Gerät, eine In-Order-Queue und ein 16-Byte-Buffer in Kontext A. Der gültige Pfad verwendet ein abgeschlossenes User Event aus A. Danach wird genau ein Fill-Aufruf mit einem noch offenen User Event aus B versucht.
- Einziger H1-Lauf mit `RUSTICL_ENABLE=radeonsi` und äußerem `timeout 30s`. Rohlog: `artifacts/b10_final_foreign_event_2026-10-20.log`.

| Messpunkt | Ergebnis |
|---|---|
| Gültiger Fill mit Event aus A | Enqueue und Wait `CL_SUCCESS`; Event `CL_COMPLETE` |
| Datenkontrolle | Vier Wörter jeweils `0x4b10a55a` |
| Fill mit Event aus B | `CL_INVALID_CONTEXT (-34)` wie erwartet |
| Ausgabe-Event beim abgewiesenen Aufruf | `NULL` |
| Prozess und Timeout | `result=PASS_CONTEXT_BOUNDARY`, `wrapper_exit=0`; 30-Sekunden-Grenze nicht erreicht |

**Interpretation:** Die spezifizierte Kontextgrenze hielt in diesem einen Fill-Buffer-Aufruf. Es wurde keine Sicherheitslücke provoziert und kein neuer B10-Befund gewonnen. Die B10-H1-Entscheidung `CLOSE / F0` bleibt bestehen. Der Lauf beweist weder allgemeine Synchronisationssicherheit noch Cross-Client-Isolation; ein Kerneljournal-Rohbeleg wurde für diesen Nachtest nicht erhoben. Es folgen keine weiteren Versuche ohne neue konkrete Hypothese.

SHA256: Quelle `ce8db2f89edabc8191fdebc8ba7055b8a692e1e83d30568aa25d51dbfc8e8a7f`; Binärdatei `b39845f5bafec3477e72c26893268e0d20cd7ff3c66f267d5801ee0c7186c87c`; Rohlog `70d21ac7a76f30e432d67199049872cd46c5d07ab66f7e55c952d28abd12b117`.
