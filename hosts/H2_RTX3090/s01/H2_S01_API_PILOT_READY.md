# H2-S01 API-only-Pilot — unmittelbar vorbereitet, noch nicht freigegeben

Stand31.10.2026. Das echte Binary ist gebaut und alle lokalen Bindungen sind eingefroren. Das Manifest bleibt **pending_explicit_approval**. Kein Supervisor-/Reproducerstart, kein OpenCL-Aufruf, keine GPU-Enqueues und kein Produktionsledger-Verbrauch in dieser Vorbereitung.

## Fertiger lokaler Stand

- Binary: `/home/rock/projects/gpu-resource-boundary-reproducer/hosts/H2_RTX3090/s01/pilot/h2_s01_api_pilot`
- SHA-256: `4453fda89513a800112dddbecbf5b78d0e26b90eebfd6e4adaa88942c8fd430f`
- Toolchain: GCC15.2.0, Ubuntu15.2.0-16ubuntu1, x86_64-linux-gnu; strikter C11-Build mit `-O2 -Wall -Wextra -Werror -Wpedantic -lOpenCL -ldl`, Exit0, keine Warnung. [Buildlog](pilot/BUILD.log), [Buildprovenienz](pilot/BUILD_PROVENANCE.json), [gelesene ELF-Abhängigkeiten](pilot/ELF_DYNAMIC.txt).
- Unveränderter Fallplan-SHA: `7e644016244271bc1e5c83b4779c74a61b11f0c4a19c17b345d873a9387806be`. Der Reproducer enthält weiterhin alle11 geprüften Fälle; das separate Manifest lässt ausschließlich die folgenden5 zu, jeweils Wiederholungsnummer1. Kein Datenfall ist freigegeben.
- Ledger: `/home/rock/.local/state/opencl2/trials-v1/ledger.sqlite3`, UUID `10859f6a-779a-4dbe-99ca-73a355ade12b`. Ausschließlich `mode=ro`/`query_only` geöffnet; Schema und UUID gegen R3 bestätigt, SQLiteintegrität ok, DELETE-Journal, null Trials und gleicher ursprünglicher Datei-SHA. [Lesenachweis](pilot/LEDGER_READONLY_CHECK.json). Keine Reservierungsfunktion aufgerufen.
- Vorhandene Librarydateien `libOpenCL.so.1.0.0`, `libnvidia-opencl.so.580.178.04`, `libcuda.so.580.178.04` gezielt per Hash gegen C0 geprüft:3/3 gleich. Das bestätigt Dateiintegrität, noch keine tatsächlich geladene ICD oder Geräteidentität. Kein `ldd`, keine Runtimeausführung und keine neue Geräteinventarisierung.

Manifest: `hosts/H2_RTX3090/c1/approvals/s01_api_pilot_20261031.json`. Daneben liegen der vollständige eingefrorene `s01_api_pilot_workspace.json` und `s01_api_pilot_binding.json`. Diese Dateien sind bewusst aus der Workspace-Content-ID ausgeschlossen, werden jedoch separat gehasht beziehungsweise beim späteren Start ins Runprotokoll kopiert. Die Approval-ID bezeichnet eine neue vorbereitete Scope-ID; sie ist selbst keine Freigabe. Status und Freigabebeleg dürfen ausschließlich nach tatsächlichem menschlichem Laufauftrag geändert werden. Binary-/Plan-/Workspace-/Environment-/Ledger- und Fallbindungen dann nicht stillschweigend ändern.

Git bei Build und finalem Freeze: echte Repositoryprovenienz, Basis-HEAD und Dirty-/untracked-Status sind vollständig aufgezeichnet. Der Arbeitsbaum ist bewusst dirty (vorherige H1-Verlagerung und H2-Arbeit); daraus wird kein Clean-Commit behauptet. Finales HEAD/Branch/Status-/Diffhashes stehen im Workspace-Snapshot. Ein nur ähnlicher Arbeitsstand oder ein anderes Environment ist kein zulässiger Ersatz für den eingefrorenen Stand.

## Startreihenfolge und Erwartung

| Reihenfolge | Fall | Erwarteter Übergang | Zusätzliche Pflicht |
|---|---|---|---|
|1|S01-OWN-API|eigener wiederholter Setter:0|alle Setup-/Cleanup-APIs0|
|2|S01-INDEX-API|Index2 bei2 Argumenten:−49|gültiger eigener Setter vor dem Fehler|
|3|S01-SIZE-API|7 statt8 Byte für cl_mem:−51|sonstige Parameter gültig|
|4|S01-FOREIGN-API|gültiges fremdes Kontextobjekt:−34|bei Wrong-Success sofortiger Stop vor Clone/Queue|
|5|S01-CLONE-API|gültiger Setter und Clone:0|neuer nichtnull Clone, Original bleibt gültig|

Pro Fall genau ein frischer Prozess, Nummer1. Zwei Kontexte, sechs640-Byte-Buffer, Programm-/Kernelbuild; keine Queue und kein Enqueue. API-only bedeutet Runtime-/Compileraufrufe bei einem späteren Pilot, nicht reine CPU-Offlineprüfung. Der nachfolgende Start ist deshalb weiterhin von einem ausdrücklichen Laufauftrag abhängig. Ressourcenfehler−5/−6 sind78/inconclusive, kein Rollbackbeweis.77 ist Skip. Beide stoppen die Pilotfolge. Ein erwarteter APIcode beweist allein keinen gespeicherten Argumentwert oder Datenimpact.

## Ausführbare Einzelbefehle — jetzt noch nicht gestartet

Zuerst den konkreten Stand und den Scope tatsächlich freigeben lassen. Nur danach Status/Beleg des vorbereiteten Manifests auf die reale Freigabe fortschreiben; keine neue Approval-ID oder Hashwerte ohne erneute Prüfung. Danach die folgenden Befehle **einzeln**, nicht als Schleife oder zusammenhängenden Block ausführen. Nach jedem Befehl vollständig prüfen und erst dann über den nächsten Fall entscheiden.

```bash
cd /home/rock/projects/gpu-resource-boundary-reproducer
bash hosts/H2_RTX3090/s01/pilot/run_one_api_pilot.sh S01-OWN-API
```

Nach bestandener Einzelprüfung und passendem Auftrag anschließend jeweils einzeln:

```bash
bash hosts/H2_RTX3090/s01/pilot/run_one_api_pilot.sh S01-INDEX-API
```

```bash
bash hosts/H2_RTX3090/s01/pilot/run_one_api_pilot.sh S01-SIZE-API
```

```bash
bash hosts/H2_RTX3090/s01/pilot/run_one_api_pilot.sh S01-FOREIGN-API
```

```bash
bash hosts/H2_RTX3090/s01/pilot/run_one_api_pilot.sh S01-CLONE-API
```

Das ausführbare [Einzelfallskript](pilot/run_one_api_pilot.sh) enthält den vollständigen Supervisorbefehl mit `--execute-approved-plan`, Manifestpfad, eingefrorenem Binarypfad, Case, `--repeat-number 1` und festem neuen Ergebnisordner. Es ändert keine Freigabe und hat keine Fallschleife. Solange das Manifest pending ist, lehnt der Supervisor vor Reservierung/Kindstart ab. Das Skript wurde hier nur mit `bash -n` geprüft, nie gestartet.

Ergebniswurzel: `/home/rock/projects/gpu-resource-boundary-reproducer/hosts/H2_RTX3090/results/c1/s01_api_pilot_20261031/<CASE>`.

## Prüfung nach jedem einzelnen Lauf

1. Supervisor-Exit muss0 sein. `run.json` muss den richtigen Fall, repeat1, die tatsächliche Approval-ID und Manifest-SHA, Binary-/Plan-/Workspacehashes, Git- und Environmentbindung, unveränderte Runtimefilehashes sowie das passende dauerhafte Ledgerreceipt enthalten. `outcome=passed`, `stop_reason=null`, `returncode=0`, kein Signal. Exit0 allein genügt nicht.
2. stdout-/stderr-/Approval-/Workspace-SHA gegen Dateien prüfen. stderr auf Buildfehler, Treiber-/Kontextfehler und andere Auffälligkeiten lesen. Librarymaps/Identitätslogs müssen den gepinnten NVIDIA3090-/580.178.04-/PCI0000:01:00.0-Zielpfad zeigen. Fehlende Librarymaps nicht durch die vorherigen reinen Filehashchecks ersetzen.
3. Vollständigen Schema2-Log offline prüfen (CASE und Pfad jeweils ersetzen):

```bash
python3 hosts/H2_RTX3090/s01/verify_log.py --log hosts/H2_RTX3090/results/c1/s01_api_pilot_20261031/S01-OWN-API/stdout.jsonl --case S01-OWN-API
```

4. Der Parser verlangt geordnete eigene Setter, genau den erwarteten Übergang, bei Clone dessen Erzeugung, Objekt-/Kontextzuordnung, Cleanup und Abschluss; keine Queue-/Enqueue-/Datenzeile im APIarm. Abgelehntes oder unvollständiges Erfolgslog bedeutet Stop.
5. Lesender Preflight für den nächsten gewählten Fall darf dessen Nummer1 nicht als verbraucht zeigen. Bereits gestarteter Fall bleibt verbraucht, auch bei Fehlstart/Timeout. Bei einem nicht erwarteten Ergebnis keine Wiederholung mit anderem Ausgabeordner, keine neue Approval-ID als automatische Recovery und kein Zurücksetzen des Ledgers.

Read-only-Preflight ohne Laufberechtigung und ohne Verbrauch:

```bash
python3 hosts/H2_RTX3090/s01/pilot/check_api_pilot.py
```

Er prüft Hashes, Git, Environment, Scope und lesend das Ledger. Er zeigt den Freigabestatus gesondert an; PASS bei pending bestätigt ausschließlich die Vorbereitung. Die Atomarität beim tatsächlichen Start erzwingt weiterhin der R3-Supervisor.

## Stopkriterien und Bindungsgrenzen

Bei unerwartetem APIcode, falscher Positivkontrolle, Skip/Inconclusive, Timeout, Signal, Ressourcenlimit, Parserfehler, unbekannter Identität, Source-/Plan-/Binary-/Environmentänderung, Ledgerproblem oder Treiber-/Desktopstörung sofort stoppen. Keine Folgefälle oder automatische Reparatur. Grenzen60s wall,45s CPU,2GiB beobachtetes Prozessgruppen-RSS,16MiB Output; Prozessende garantiert keine GPU-Preemption.

Die Workspace-ID wird **nach** Build, Dokumentation und Manifestaktualisierung eingefroren. Danach keine weiteren Projektdateien ändern. Freigabeänderungen im ausgeschlossenen Approvalordner und spätere Ergebnisdateien ändern die Content-ID nicht; HEAD, Branch, Dirtyzustand und Index-/Working-Diffs bleiben gebunden. Zusätzliche untracked Ergebnisdateien dürfen den vollständigen Porcelain-Status ändern; Quellen-/Binaryinhalte bleiben durch die Content-ID gebunden. Inhalte einer bereits verwendeten Approval-ID bleiben unveränderlich. Das alte Source-ZIP oder ein frisch entpacktes neues ZIP ist kein automatisch gleich gebundener Workspace: absoluter Pfad, .git-Zustand und zusätzliche Archivdateien unterscheiden sich. Der vorbereitete Scope gilt für das ursprüngliche lokale Repository; bei Umzug/Entpacken ausdrücklich neu prüfen und binden. Liveledger niemals aus einem Paket kopieren/ersetzen.

Für Handover sind der fertige native Build, die Logs und das pending Manifest enthalten. Das Paket ist intern. H1, S01-Reproducer, Fallplan und Orakel bleiben unverändert. Keine Treiberänderung, Installation oder GitHub-Push.
