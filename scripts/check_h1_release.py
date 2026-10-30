#!/usr/bin/env python3
"""Offline review of the explicit H1 upload. Never executes OpenCL tests."""
from pathlib import Path
import fnmatch
import hashlib
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
SELECTION = 'manifests/H1_UPLOAD_SELECTION_2026-10-30.txt'
SUMS = 'manifests/H1_UPLOAD_SHA256_2026-10-30.txt'
BLOCKED = (
    'local-only/*', 'vendor-src/*', 'vendor-prefix/*', 'experiments/*',
    'probes/*', 'notes/FUP_B4_2A_*', 'src/fup-b4-2a-*/*',
    'scripts/fup_b4_2a_completion_order_oracle.py',
    'patches/b4/fup_b4_2a_clmem_terminal_retention_pof.patch',
    'B9_*', 'src/b9_*', 'src/o1_victim_heartbeat.c',
    'artifacts/b9_*', 'artifacts/o1_b9_*', '*.zip', '*.tar.xz', '*.core', '*.bin',
)
PATTERNS = {
    'private home path': re.compile(r'/home/[A-Za-z0-9_.-]+/'),
    'private key': re.compile(r'-----BEGIN (?:RSA |OPENSSH |EC )?PRIVATE KEY-----'),
    'GitHub token': re.compile(r'\bgh[pousr]_[A-Za-z0-9]{25,}\b|\bgithub_pat_[A-Za-z0-9_]{30,}\b'),
    'AWS access key': re.compile(r'\b(?:AKIA|ASIA)[A-Z0-9]{16}\b'),
    'credential assignment': re.compile(r'(?i)(?:password|access_token|api_key)\s*[:=]\s*[\"\'][A-Za-z0-9_./+=-]{20,}[\"\']'),
}

def main():
    errors = []
    names = [s for s in (ROOT / SELECTION).read_text().splitlines()
             if s and not s.startswith('#')]
    if len(names) != len(set(names)):
        errors.append('duplicate selection entries')
    selected = set(names)
    checksums = {}
    for line in (ROOT / SUMS).read_text().splitlines():
        if not line or line.startswith('#'):
            continue
        digest, name = line.split(maxsplit=1)
        checksums[name.lstrip('*')] = digest
    if set(checksums) != selected - {SUMS}:
        errors.append('checksum coverage differs from selected paths (excluding self)')
    try:
        tracked = set(subprocess.check_output(
            ['git', '-C', str(ROOT), 'ls-files'], text=True).splitlines())
    except (OSError, subprocess.CalledProcessError):
        tracked = set()
        errors.append('could not read tracked-file base for link checks')
    available = tracked | selected
    for name in sorted(selected):
        rel = Path(name)
        if rel.is_absolute() or '..' in rel.parts:
            errors.append(f'unsafe path: {name}')
            continue
        if any(fnmatch.fnmatchcase(name, rule) for rule in BLOCKED):
            errors.append(f'withheld/excluded path selected: {name}')
        p = ROOT / name
        if not p.is_file() or p.is_symlink():
            errors.append(f'missing/nonregular file: {name}')
            continue
        raw = p.read_bytes()
        if len(raw) > 1024 * 1024:
            errors.append(f'file exceeds 1 MiB selection bound: {name}')
        if raw.startswith(b'\x7fELF'):
            errors.append(f'executable selected: {name}')
        if name in checksums and hashlib.sha256(raw).hexdigest() != checksums[name]:
            errors.append(f'checksum mismatch: {name}')
        try:
            content = raw.decode('utf-8')
        except UnicodeDecodeError:
            if name != 'artifacts/b8_c2_dynamic_local.spv':
                errors.append(f'unreviewed binary: {name}')
            continue
        for label, pattern in PATTERNS.items():
            for match in pattern.finditer(content):
                line = content.count('\n', 0, match.start()) + 1
                errors.append(f'{name}:{line}: {label}')
        if p.suffix == '.md':
            for target in re.findall(r'\]\(([^)]+)\)', content):
                if re.match(r'^[A-Za-z][A-Za-z0-9+.-]*:', target) or target.startswith('#'):
                    continue
                target = target.split('#', 1)[0]
                resolved = (p.parent / target).resolve()
                try:
                    dest = resolved.relative_to(ROOT).as_posix()
                except ValueError:
                    errors.append(f'nonportable local link in {name}')
                    continue
                if dest not in available and not any(s.startswith(dest.rstrip('/') + '/') for s in available):
                    errors.append(f'link outside public base/selection: {name} -> {target}')
    if errors:
        for error in errors:
            print('FAIL', error)
        return 1
    total = sum((ROOT / n).stat().st_size for n in selected)
    print(f'PASS: {len(selected)} selected files, {total} bytes; hashes, exclusions, basic credential/path scan and local links verified.')
    print('Offline checks only. Historical base files are not rescanned or removed. No OpenCL execution, commit or push.')
    return 0

if __name__ == '__main__':
    sys.exit(main())
