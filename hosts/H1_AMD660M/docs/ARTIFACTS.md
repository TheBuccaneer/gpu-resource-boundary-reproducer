# Artifact organization and retention

Inventory cutoff: 2026-10-30, local `main` based on `be0e6bf37e61d78ba7d537e90ddb471262b40cc5`. The GitHub `main` head was independently read through the GitHub connector and matches this commit. The local `PROJECT_STATE.md` had a newer uncommitted B10 update; that content was preserved in the consolidation backup before editing.

| Area | Local files / approximate logical size | Disposition |
|---|---|---|
| `notes/` before consolidation | 53 / 277 kB | Canonical gates indexed; new safe reports selected; sensitive follow-up details held |
| `src/` | 83 / 639 kB | Preserve all; select small B8/B10/B12 additions; exclude unreviewed B9/FUP additions |
| `scripts/`, `patches/` | 6 + 4 / 37 kB | Preserve; original published fixes remain; new FUP patch/evaluator held |
| `artifacts/` | 124 / 7.0 MB | Select useful small logs/input; hold B9 details; exclude new dumps and transient outputs |
| `results/` | 551 / 204.5 MB | 530 historically tracked files remain. Untracked large core and diagnostic residue stay local. |
| `vendor-src/` | 138,919 / 10.42 GB | Mesa/CTS/dependencies/builds/installs; ignored, not an upload source |
| `vendor-prefix/` | 32 / 2.3 MB | Local installed dependencies; ignored |
| `probes/` | 85 / 1.5 MB | Mostly generated executables; preserve locally, exclude from new upload |
| FUP integration | Two compressed archives / 34.9 MB plus index | Local disclosure hold; 14-file integration manifest verified with zero mismatches |
| B9 30 October package | 56 manifest entries | Copied byte-for-byte into local-only evidence; verified with zero mismatches |
| Review bundle and expanded copy | ZIP 11.3 MB; expanded 188.5 MB | Redundant local review material, excluded; retained |

Sizes are logical file sizes, not deduplicated disk use. Symlink/build contents and local state may change later. The complete pre-edit file/size inventory, Git snapshots, duplicate groups and excluded-file list are retained locally outside the upload selection.

## Canonical versus historical records

The matrix selects a closure document for each class. Phase reports and raw outputs remain historical observations: a later result can supersede a gate without invalidating an earlier correctly recorded run. B12 C0/C1 snapshots now point to R1 as the current disposition. B8 has source/IL inputs but no located durable paired stdout; it remains a report-backed negative record with that explicit limitation.

The B10 root ZIPs are overlapping cumulative packages (14, 19, 39 and 43 members). The selected upload uses unpacked small reports, sources and logs; the ZIPs and their original checksum files remain local. B9's historical report ZIP predates later amendments and must not be treated as the latest state. No archive was deleted.

Content hashing found 135 duplicate groups among eligible small local research files, including repeated/empty logs and archive-extracted copies. Duplicates were inventoried, not removed. The original B4 integration and 30 October B9 manifests both verify completely.

## Public and local-only material

The public upload is an explicit allowlist, not the set of everything shown by `git status`. `local-only/` contains internal audit detail, original document backups, inventory, the unpublished B9 packet, disclosure holds and a reviewable patch. `experiments/`, FUP report/source/patch paths and unreviewed B9 paths are explicitly ignored for new additions. Ignore rules are not access control; the allowlist/checker is the publication gate.

Original evidence logs are never rewritten. Three B10 CTS build/configuration logs have separately named public derivatives in `artifacts/public/`; only local path identifiers are replaced with `<REPO>`. The [redaction manifest](../manifests/H1_REDACTIONS_2026-10-30.tsv) records original and derivative hashes and the exact transformation. Core B10 result logs and both B12 logs are selected byte-for-byte.

Existing large devcoredumps and historical private paths already present in Git history were not removed or rewritten. Six tracked dump files exceed 1 MiB. New archives, core dumps, build/install trees and executables are excluded. Removal of historical artifacts or history rewriting would require a separate preservation/review decision.
