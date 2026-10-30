# Reproduction prerequisites

This is a documentation handoff. No experiment, OpenCL command, compilation, driver installation or system control change is performed by the consolidation tools.

## Offline review

`python3 scripts/check_h1_release.py` checks the proposed file selection, hashes, local Markdown links and publication exclusions using the Python standard library. It does not import OpenCL or run any reproducer. Historical raw logs and reports can be inspected without GPU access. The public allowlist records changed/new files; existing published evidence remains in the base history.

## Requirements for a separately approved future run

Select a specific named test and its oracle first. Check platform/device identity, supported API/extension versions, needed function pointers and actual loaded libraries in the process. Preserve source/input hashes, compile flags, exit/timeout status and cleanup observations. Do not infer active ICD provenance from the package manager alone. A configuration that lists zero devices in an isolated environment is not a host hardware finding.

H1's configurations include system Rusticl 26.0.3 and 26.0.8, isolated Mesa 26.2.0, and distinct local B4/B7/B9 patch builds. `RUSTICL_ENABLE=radeonsi` may be needed; per-process ICD/library selection and process maps must identify which build was actually used. Do not select a patched B9 build as a regular baseline. Existing Meson/Ninja build trees contain historical absolute paths and are not portable merely by copying them.

Small sources generally require a C compiler, OpenCL headers and an ICD loader. Source/IL comparisons additionally require an explicitly recorded OpenCL-to-SPIR-V toolchain and input validation. The recorded CTS work uses commit `9feccbb8fcb8eefeebf86a48036173a8155f9d21`; optional dependencies and the 300/310 header fallback are documented in the B10 report. A successful build is not a successful GPU test.

For original B4/B7 local fix reproduction, retain the base revision plus exact patch, options and dependency hashes; create a fresh isolated source/build/install location only under an approved plan. The local follow-up/B9 detail packages are not public run instructions. No system Mesa/ICD, kernel parameter, cleaner control or display state should be changed as part of an ordinary documentation review.

Some historical sources deliberately trigger device timeouts or global OOM. B1/B3/B9 runtime triggers and platform-control scripts are not smoke tests. Any future approved GPU work needs small resources, an outer deadline, controlled cleanup, stop on fault/timeout, and an explicit decision about shared display/client load. The H2 roadmap begins with capabilities and valid positive controls.

## Portability and missing records

Repository links are relative; historical raw logs keep original paths/addresses as evidence. External vendor source trees and local evidence archives are indexed but not uploaded. A complete system rebuild is not supplied, and no vendor-wide result is implied. B8 paired stdout and several historical per-run library/journal records are missing; see the [quality review](H1_QUALITY_REVIEW_2026-10-30.md). Do not manufacture them or replace them with current runs presented as historical evidence.
