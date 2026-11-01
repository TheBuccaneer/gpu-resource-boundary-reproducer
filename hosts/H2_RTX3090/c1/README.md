# H2-C1 — Vertragsprüfung und Experimentvorbereitung

Stand: **31.10.2026**. Grundlage: das unveränderte `OpenCL2_Uebergabe_2026-10-31.zip` auf dem Desktop, SHA-256 `3aacc0512a5af7bf972d65a2dfdbb0184d181053b79af8c487f1c3eacf3935c1`. Zuerst wurden Übergabe, Projektstand, C0-Readiness und B1–B12-Matrix aus genau diesem ZIP gelesen. Keine neue C0-Erhebung, keine Laufzeitinstallation und keine OpenCL-Ausführung in diesem Arbeitsschritt.

| Ergebnis | Datei |
|---|---|
| B12: gültige/ungültige/nicht unterstützte Querypfade, normative Entscheidung | [H2_B12_QUERY_CONTRACT_REVIEW.md](H2_B12_QUERY_CONTRACT_REVIEW.md) |
| Umsetzbarer C1-Plan und Freigabepakete | [H2_C1_WORK_PLAN.md](H2_C1_WORK_PLAN.md) |
| Einzeltests mit Rückgaben, Kontrollen und Reproducer-Zuordnung | [H2_C1_TEST_CATALOG.md](H2_C1_TEST_CATALOG.md), [TEST_CASES.json](TEST_CASES.json) |
| H1↔H2-Vergleich und Capability-Ausschlüsse | [H2_H1_COMPARISON_SCOPE.md](H2_H1_COMPARISON_SCOPE.md) |
| Getrennte Einführungsvorlage für clvk/PoCL CPU | [H2_CLVK_POCL_CPU_DECISION.md](H2_CLVK_POCL_CPU_DECISION.md) |
| Gepinnte Spezifikations-/CTS-Zuordnung und begrenzter Prüfbereich | [H2_CTS_CONTRACT_MAP.md](H2_CTS_CONTRACT_MAP.md) |
| Vorbereitete, nicht ausgeführte Reproducer | [reproducers/README.md](reproducers/README.md) |
| Quellen und Prüfnachweise | `sources/`, `notes/`, [SHA256SUMS.txt](SHA256SUMS.txt) |

**B12-Entscheidung:** Die vier auffälligen Typ-/Exportlisten-Selektoren sind bei den im C0-Log beworbenen Erweiterungen normativ gültig. Nicht unterstützter Export wird durch eine erfolgreiche leere Liste dargestellt. Die vorhandenen `-30`-Rückgaben passen dazu nicht. Das ist eine dokumentierte Query-Vertragsabweichung im archivierten C0-Datensatz, noch keine unabhängig mit dem neuen C-Reproducer bestätigte NVIDIA-Fehlerursache und kein Sicherheitsbefund. Weder „fehlender Export erklärt -30“ noch „falscher Namespace der bisherigen vier Queries“ ist durch den Abgleich gestützt.

**Nächste freigabefähige Schritte:** A) gezielte reine B12-Queries; B) B7-Setterkontrollen ohne Enqueue; C) kleine gültige Buffer-/Kernel-Baselines; D) eng begrenzte B2-Negativkontrollen; E) In-Order-/Cross-Queue-/User-Event-Kontrollen; F) OOO und zusätzliche User-Event-Fehlerfälle. Heute wird nur die Vorlage erstellt. Das Flag eines Reproducers ersetzt keine ausdrückliche Ausführungsfreigabe.

Die Reproducer wurden nur als CPU-Hostcode kompiliert (`-c`, keine Verknüpfung, kein Start, kein OpenCL-Aufruf). Das belegt Syntax-/Compilerverträglichkeit, keinen funktionalen Lauf. Die 729 H1-Dateien und die C0-Rohdaten bleiben unverändert. Das vollständige interne Übergabepaket wurde nirgends hochgeladen; Netzverkehr diente ausschließlich dem lesenden Abruf der unten gepinnten öffentlichen Quellen.

## Auditfortschreibung 31.10.2026

Der aktuelle [Auditbericht](../security_discovery/C1_AUDIT_CORRECTIONS.md) beschreibt vollständige B12-Listenvalidierung, ELF-/Freigabemanifestbindung und Git-/Contentprovenienz. Der Supervisor benötigt jetzt zusätzlich `--approval-manifest`; CLI-Flags sind keine Autorisierung. CPU-Regressionen sind dokumentiert, kein OpenCL-Lauf. Frühere statische Nachweise bleiben datierte Historie.
