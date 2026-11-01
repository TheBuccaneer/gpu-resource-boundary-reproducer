#!/usr/bin/env python3
"""Offline integrity checks for platform separation; never executes GPU work."""
from pathlib import Path
import hashlib
import json
import re

ROOT = Path(__file__).resolve().parents[1]
H2 = ROOT / 'hosts/H2_RTX3090'


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    errors = []
    relocation = json.loads((ROOT / 'notes/H1_RELOCATION_2026-10-31.json').read_text())
    for item in relocation['files']:
        path = ROOT / item['new_path']
        if not path.is_file() or digest(path) != item['sha256']:
            errors.append(f"H1 content differs or is missing: {item['new_path']}")
    raw_count = 0
    for manifest in sorted(H2.glob('inventory/*/SHA256SUMS')):
        for line in manifest.read_text().splitlines():
            expected, name = line.split('  ', 1)
            path = manifest.parent / name
            if not path.is_file() or digest(path) != expected:
                errors.append(f'H2 raw evidence differs or is missing: {path.relative_to(ROOT)}')
            raw_count += 1
    manifest = H2 / 'notes/H2_PREPARATION_SHA256SUMS.txt'
    for line in manifest.read_text().splitlines():
        expected, name = line.split('  ', 1)
        path = H2 / name
        if not path.is_file() or digest(path) != expected:
            errors.append(f'H2 preparation file differs or is missing: {name}')
    documents = [ROOT / 'README.md', ROOT / 'PROJECT_STATE.md']
    documents += [p for p in H2.rglob('*.md') if 'research/sources' not in p.as_posix()]
    for path in documents:
        for target in re.findall(r'\]\(([^)]+)\)', path.read_text()):
            if re.match(r'^[A-Za-z][A-Za-z0-9+.-]*:', target) or target.startswith('#'):
                continue
            destination = (path.parent / target.split('#', 1)[0]).resolve()
            if not destination.is_relative_to(ROOT) or not destination.exists():
                errors.append(f'Broken local link: {path.relative_to(ROOT)} -> {target}')
    if errors:
        for error in errors:
            print('FAIL:', error)
        return 1
    print(f"PASS: {relocation['file_count']} H1 files byte-identical after relocation; "
          f'{raw_count} H2 raw checksum entries and current preparation manifest verified; '
          'current overview/H2 document links resolve. Offline only.')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
