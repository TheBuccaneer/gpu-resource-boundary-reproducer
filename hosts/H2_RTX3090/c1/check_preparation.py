#!/usr/bin/env python3
"""Offline check of prepared artifacts and preservation. Never calls OpenCL."""
import ast
import hashlib
import json
from pathlib import Path
import re

C1 = Path(__file__).resolve().parent
ROOT = C1.parents[2]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    count = 0
    for line in (C1 / 'SHA256SUMS.txt').read_text().splitlines():
        expected, name = line.split('  ', 1)
        path = C1 / name
        assert path.resolve().is_relative_to(C1), name
        assert sha(path) == expected, name
        count += 1
    relocation = json.loads((ROOT / 'notes/H1_RELOCATION_2026-10-31.json').read_text())
    assert len(relocation['files']) == 729
    for row in relocation['files']:
        assert sha(ROOT / row['new_path']) == row['sha256'], row['new_path']
    inp = json.loads((C1 / 'notes/INPUT_PROVENANCE.json').read_text())
    for name, expected in inp['input_c0_file_hashes'].items():
        assert sha(ROOT / name) == expected, name
    plan = json.loads((C1 / 'TEST_CASES.json').read_text())
    ids = {c['id'] for c in plan['cases']}
    text = (C1 / 'reproducers/h2_c1.c').read_text()
    declared = re.search(r'const char \*cases\[\]=\{([^}]+)\}', text).group(1)
    assert ids == set(re.findall(r'"([^"]+)"', declared)) and len(ids) == 26
    for row in plan['cases']:
        assert row['repeat_processes'] == 5 and not row['runtime_executed']
        assert row['positive_control'] and row['negative_control'] and row['measurement'] and row['stop_rule']
    b7 = text.split('if(is_b7) {', 1)[1].split(' }else {', 1)[0]
    assert not re.search(r'\bcl(?:CreateCommandQueue|Enqueue)\w*\s*\(', b7)
    for path in C1.rglob('*.py'):
        if 'sources' not in path.relative_to(C1).parts:
            ast.parse(path.read_text())
    for path in C1.rglob('*.md'):
        if 'sources' in path.relative_to(C1).parts:
            continue
        for link in re.findall(r'\]\(([^)]+)\)', path.read_text()):
            if re.match(r'^[A-Za-z][A-Za-z0-9+.-]*:', link) or link.startswith('#'):
                continue
            assert (path.parent / link.split('#', 1)[0]).resolve().exists(), (path, link)
    print(f'PASS: {count} C1 artifact hashes; 729 H1 originals and {len(inp["input_c0_file_hashes"])} C0 files unchanged; '
          '26 cases mapped; B7 has no queue/enqueue; local links/Python syntax valid. Offline only.')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
