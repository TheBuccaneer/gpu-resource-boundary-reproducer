# Proposed public upload — 30 October 2026

**Preparation only. No commit or push has been performed.** The exact changed/new path allowlist is [H1_UPLOAD_SELECTION_2026-10-30.txt](../manifests/H1_UPLOAD_SELECTION_2026-10-30.txt); checksums are [H1_UPLOAD_SHA256_2026-10-30.txt](../manifests/H1_UPLOAD_SHA256_2026-10-30.txt). Existing public files remain in the base commit. The Git index is not used as a broad selection mechanism.

## Included

- Updated project status, a new README, complete B1–B12 public evidence matrix, methodology, artifact inventory, quality/open-question review and short H2 roadmap.
- Small B8 Source/IL probes and the tested IL input; closure explicitly notes the missing paired stdout.
- B10 canonical closure/phase reports, four small test sources, selected original result/provenance logs, and three separately identified path-redacted CTS build/configuration logs.
- B12 C0/C1/R1 reports, the F1 issue draft, original small source and both raw result logs. Documentation paths are made repository-relative; raw logs and source remain byte-identical.
- Coarse B4-follow-up/B9 status notes, without their unpublished technical packages.
- Offline selection/integrity checker and content/redaction manifests.

## Withheld or excluded

| Material | Reason |
|---|---|
| New ordinary-object B4 follow-up reports, source, PoF, evaluator and archives | Local record explicitly requests confidential coordination; publication review required |
| B9 detailed historical report, new reproducer/IR/patch/diagnostic logs and 30 October packet | Unreported availability/backend finding and mitigation need disclosure review |
| Local Mesa/CTS source, build and installation trees; vendor prefixes | Generated/upstream dependency trees, roughly 10 GB; reproduce from version/patch provenance instead |
| Root ZIP packages and expanded review bundles | Overlapping or redundant; preserve locally, select small canonical content |
| New devcoredumps/userspace cores and compiled executables | Unnecessary upload size and possible memory content; binary hashes remain evidence |
| Existing modified B7 executable | Unrelated pre-existing working-tree change; omitted |
| `local-only/` | Backups, complete internal evidence index, disclosure holds, inventory and prepared diff |

Ignore rules exclude these untracked paths by default. The local holds list and full inventory are available for the owner's review; ignored material still exists on disk. Existing tracked files are unaffected by ignore rules. Nothing previously published is deleted to hide a contradiction or reduce repository size.

## Review before any public push

The offline checker verifies selected file hashes, bounded sizes, known disclosure exclusions, private-home-path/credential patterns and relative Markdown links. During consolidation a broader scan also examined 1,205 local text records, including small text members of the root ZIPs: no credential-pattern match was found; 61 records contain private local paths and remain accounted for in the internal inventory. This automated scan cannot prove the absence of every secret or undisclosed claim. Historical public paths/dumps remain a documented residual condition; new private local paths are removed from selected documentation and separately redacted derivatives.

The prepared patch applies only the allowlisted paths against the recorded base. It is generated with a temporary Git index; the user's real index, unrelated modified binary and pre-existing research files remain intact. Public push requires a fresh explicit approval based on the final diff, disclosure holds and known gaps. Upstream issue filing is a separate action.
