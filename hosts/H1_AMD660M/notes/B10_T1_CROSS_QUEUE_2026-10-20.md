# B10 – Phase B, T1 Cross-Queue-Ordering

**Datum:** 20.10.2026  
**Gate:** T1 `PASS`; B10-Klassengate offen.  
**Umfang:** Genau ein T1-Lauf nach C1-PASS. T2 und T3 wurden nicht ausgeführt.

## Frage und Oracle

Eine Produzenten-Queue schreibt vier `cl_uint`-Werte in einen Buffer. Ein Kernel in einer zweiten Queue desselben Kontextes hängt über seine Event-Waitlist vom Produzenten-Event ab und addiert `1` zu Wort 1. Beide Queues sind In-Order. Die Produzenten-Queue wird vor der Verwendung ihres Events in der zweiten Queue explizit geflusht. Dies erfüllt die Khronos-Voraussetzung für Cross-Queue-Events. Nach erfolgreichem `clWaitForEvents` auf das Read-Event müssen alle beteiligten Commands abgeschlossen und die Werte `[0xa5a5a5a5, 43, 0x5a5a5a5a, 0x12345678]` lesbar sein.

Um die Abhängigkeit tatsächlich auszuüben, wartet der Produzenten-Write auf ein zunächst offenes User Event. Beide Queue-Befehle werden vor Freigabe dieser Schranke enqueued und geflusht. Vor Freigabe dürfen Produzent und Konsument nicht `CL_RUNNING` oder `CL_COMPLETE` erreichen. Das User Event ist hier nur ein deterministisches Gate; T2-Fehler- und Zustandsfälle wurden nicht untersucht.

Normative Basis: OpenCL API §§ 5.1, 5.11, 5.14 und 5.16, insbesondere die Waitlist-Synchronisierung, User-Event-Garantie und `clFlush`-Pflicht für Cross-Queue-Events: https://registry.khronos.org/OpenCL/specs/unified/html/OpenCL_API.html.

## Laufzeit und Durchführung

- H1: AMD Radeon 660M (radeonsi, rembrandt, ACO, DRM 3.64, Kernel 7.0.0-38-generic).
- Plattform/Gerät: Rusticl, OpenCL 3.0; `CL_DRIVER_VERSION=26.0.8-1ubuntu0.3`.
- Im T1-Prozess geladenes ICD: `/usr/lib/x86_64-linux-gnu/libRusticlOpenCL.so.1.0.0`.
- Queue-Properties: Produzent `0x0`, Konsument `0x0`.
- Probe `src/b10_t1_cross_queue.c`, kompiliert mit `cc -std=c11 -O2 -Wall -Wextra -Werror ... -lOpenCL` zu `probes/b10_t1_cross_queue`.
- Ein GPU-Lauf mit `RUSTICL_ENABLE=radeonsi` und äußerem 30-Sekunden-Timeout. Kein Sleep, keine Schleife und kein Speicherstress.

## Unveränderte Beobachtung

| Messpunkt | Ergebnis |
|---|---|
| Create/Build/Args/Enqueue/Flush | jeweils `CL_SUCCESS` (`0`) |
| Vor Freigabe | User Event `CL_SUBMITTED` (`2`); Write- und Kernel-Event `CL_QUEUED` (`3`) |
| Freigabe/Wait/Finish | jeweils `CL_SUCCESS` (`0`) |
| Finalstatus | User-, Write-, Kernel- und Read-Event jeweils `CL_COMPLETE` (`0`) |
| Nutzwert und Canarys | `42 → 43`; alle drei Canarys unverändert |
| Probe/Timeout | Exit `0`; Timeout nicht erreicht |
| Kerneljournal nach dem Lauf | keine Einträge im abgefragten Zeitfenster ab 08:19 CEST |

Rohlog: `artifacts/b10_t1_cross_queue_2026-10-20.log`.

**Interpretation:** T1 `PASS` / F0 auf dieser H1-Laufzeit. Der kontrolliert offene Produzent verhindert, dass ein bereits abgeschlossener Write die Abhängigkeitsprüfung trivialisieren würde. Ein einzelner erfolgreicher Lauf ist ein begrenzter Konformitätsnachweis, kein Beleg für allgemeine Fehlerfreiheit und kein neuer wissenschaftlicher Bugbefund. T2, T3 und weiterführende Wiederholungen bedürfen einer neuen Entscheidung.

SHA256: Quelle `b0ef538488f52b15c824a91b54f865e3d73e7e033fe4f63c85f79280f3c21b4b`; Binärdatei `f76d789c0cdf9b8c95c27253ed8ebc115c2c77143fa56ef8f7cbd6019c76b107`; Rohlog `07412d7c0a8359a5fc3bd456a24994b743e61daf5f6d7214b5dc388b31f078b5`.
