# OpenCL2: GPU resource and API contract boundaries

This repository records a specification-led OpenCL study on **H1: AMD Radeon 660M / Rembrandt, Mesa Rusticl / RadeonSI / ACO, Linux AMDGPU**. The consolidated research cutoff is **30 October 2026**. H1 discovery is substantially complete; the next planned platform is **H2: NVIDIA RTX 3090**. No H2 results are claimed.

Start with [PROJECT_STATE.md](PROJECT_STATE.md), the [H1 evidence matrix](docs/H1_EVIDENCE_MATRIX_2026-10-30.md), and the [quality review and open questions](docs/H1_QUALITY_REVIEW_2026-10-30.md). These distinguish confirmed implementation findings, misuse characterization, bounded negative results, and unavailable capabilities. An OpenCL context inside one host process is not assumed to be an OS security boundary.

## Repository layout

| Path | Contents |
|---|---|
| `notes/` | Class gates, historical phase reports, and current closure reports |
| `src/` | Small host-side test sources; some sources can dispatch GPU work |
| `probes/` | Historically tracked kernel inputs; generated executables are ignored |
| `scripts/` | Evaluation and historical platform-control scripts; none run automatically |
| `patches/` | Research fixes with scope/provenance; local fixes are not upstream acceptance |
| `inventory/` | Recorded platform and toolchain inventories |
| `results/` | Historical, already tracked raw evidence; new generated results are ignored by default |
| `artifacts/` | Selected newer logs and small representation inputs |
| `docs/` | Consolidated methodology, artifact policy, review, and H2 roadmap |
| `manifests/` | Explicit proposed upload selection and content checksums |

See [artifact organization](docs/ARTIFACTS.md) and [reproduction prerequisites](docs/REPRODUCTION.md). Archives, Mesa source/build/install trees, devcoredumps, compiled executables, and disclosure-sensitive follow-up material are retained locally but excluded from the new public upload selection. Some large dumps and executable artifacts already exist in Git history; this consolidation does not delete or rewrite that history.

## Method and evidence

The study follows classes B1–B14 and a gated C0–C9 progression. Each experiment needs a verified capability, an admissible starting state, a specific changed condition, and an oracle. API return values, event completion, data correctness, process failure, engine recovery, and effects on independent clients are separate observations. Valid successful controls are **PASS**; **F0** denotes a correctly contained OpenCL error. See [methodology](docs/METHODOLOGY.md).

The strongest established H1 cases are dynamic/combined local-memory accounting and recovery (B1/B3), SVM lifetime ordering with causal controls (B4), and ordinary object/context ownership (B7). B12 adds a reproducible **F1 metadata query error**, with no GPU dispatch or demonstrated security-boundary violation. B8 and B10 preserve useful bounded negative results. B5 characterizes out-of-contract raw-pointer behavior and is not a conformance finding.

This is a research record, not an automatically runnable test suite. Some historical reproducers caused compute-ring timeouts, process aborts, global host OOM, or wider recovery effects. No GPU experiment, driver change, upstream disclosure, commit, or public push is part of the 30 October documentation consolidation. New execution requires a separately approved, bounded plan.

## Review the proposed upload

The allowlist is [H1_UPLOAD_SELECTION_2026-10-30.txt](manifests/H1_UPLOAD_SELECTION_2026-10-30.txt). Run `python3 scripts/check_h1_release.py` for offline integrity and publication checks; it does not invoke OpenCL. The checksums cover the proposed changed/new files, not every historical file. The [publication policy](docs/PUBLICATION_SELECTION_2026-10-30.md) explains withheld material and residual limitations. Public push remains pending explicit approval.

No repository-wide software license is currently present. Existing upstream-derived material retains its own notices; this consolidation does not invent a licensing grant.
