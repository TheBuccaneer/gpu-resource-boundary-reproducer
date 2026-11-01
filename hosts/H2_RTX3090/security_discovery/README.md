# H2 Security Discovery — Vorbereitung, keine neue Vulnerabilitätsbehauptung

Stand **31.10.2026**, StufeA abgeschlossen. Die jüngste kompakte H2-C1-Übergabe und H1-Abschlüsse wurden berücksichtigt. Kein erneutes H1-Experiment, keine C0-Neuinventarisierung, kein GPU-/OpenCL-Lauf, keine Installation, Treiberänderung, öffentliche Meldung oder Push.

| Ergebnis | Einstieg |
|---|---|
| 16 Literaturprofile mit Forschungsfrage, Methode, Modell, Ergebnissen, Grenzen und Übertragung | [Literature Review](literature/LITERATURE_REVIEW_2024_2026.md) |
| 12 konkrete Hypothesen, Prioritäten und Plattformzuordnung | [Hypothesenmatrix](HYPOTHESES.md), [JSON](HYPOTHESES.json) |
| Neuheit, Gegenbelege und verbleibende Forschungslücke | [Neuheitsanalyse](NOVELTY_AND_PUBLICATION.md) |
| Drei konkrete, getrennt freizugebende Security-Experimente | [TOP3_EXPERIMENT_PLANS.md](plans/TOP3_EXPERIMENT_PLANS.md) |
| Zustandsmodell, generative Strategie und Reduktion | [STATEFUL_API_STRATEGY.md](plans/STATEFUL_API_STRATEGY.md) |
| B12-/Supervisor-/Git-Provenienzkorrekturen und Offline-Nachweise | [C1_AUDIT_CORRECTIONS.md](C1_AUDIT_CORRECTIONS.md) |
| Integrität und aktuelle Prüfungen | [SHA256SUMS.txt](SHA256SUMS.txt), `notes/` und `tests/` |

**Entscheidung:** Zuerst korrekt abgewiesene Setter mit gültigem Folgezustand/Kernelclones vergleichen (S01), dann normkonforme Ressourcenlebenszeiten mit erfolgreicher Completion untersuchen (S02). Als stärkeren, voraussetzungsreicheren Sicherheitsarm einen kontrollierten privaten Cross-Process-Canary neben legitimer External-Memory-Nutzung vorbereiten (S03). Alle drei sind Hypothesen, keine gefundenen Lücken.

**Neuheit kritisch:** StepStone verwendet bereits OpenCL; WebGlitch enthält bereits ungültige Zustände und Ownership-Fälle. „LLM + GPU-API-Fuzzing“ oder „ein fremder Kontext wird akzeptiert“ ist allein kein tragfähiger neuer Methodenanspruch. Neuheitswert muss aus einem bislang nicht belegten Mechanismus, zulässigen Gegenkontrollen und sauberer Schutzdomänen-/Impact-Evidenz kommen.

H1s private B4-Follow-up-/B9-Pakete fehlen im verfügbaren Archiv. Vor einer Neuheitsbehauptung zu einer nahen Lifetime-Variante muss deren vertraulicher Index intern abgeglichen werden. Diese Einschränkung blockiert die H2-Planung nicht und öffnet H1 nicht neu.

[Daybreak-Review](daybreak/DAYBREAK_REVIEW.md) und [Mergeentscheidungen](daybreak/MERGE_DECISIONS.md) sind berücksichtigt. S01–S03 sind noch zu implementierende Reproducerpläne.

Fortschreibung: [S01 ist nun implementiert und offline geprüft](../s01/H2_S01_READY_FOR_DAYBREAK.md); die frühere Aussage „noch zu implementieren“ bleibt für S02/S03 gültig.
