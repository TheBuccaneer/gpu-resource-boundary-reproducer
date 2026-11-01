#!/usr/bin/env python3
"""Create a compact complete working-tree handover, with no Git-history bundle.
Copies source files and runs only included offline integrity checks after extraction.
Never executes an OpenCL reproducer, installs anything, commits or pushes.
"""
import argparse
import datetime
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[1]


def sha(path):
    h = hashlib.sha256()
    with path.open('rb') as f:
        for block in iter(lambda: f.read(1024 * 1024), b''):
            h.update(block)
    return h.hexdigest()


def run(argv, cwd):
    return subprocess.check_output(argv, cwd=cwd, stderr=subprocess.STDOUT, text=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--max-mb', type=float, default=10,
                        help='Maximum compressed output MB; split evidence rather than silently exceeding.')
    a = parser.parse_args()
    output = a.output.resolve()
    if output.is_relative_to(ROOT):
        parser.error('Place the ZIP outside the source repository to avoid recursive inclusion.')
    partial = output.with_suffix('.zip.partial')
    checksum = output.with_suffix('.zip.sha256')
    if any(p.exists() for p in [output, partial, checksum]):
        parser.error('Output exists; refusing overwrite.')
    if output.suffix.lower() != '.zip' or a.max_mb <= 0:
        parser.error('Use a .zip path and a positive size limit.')
    output.parent.mkdir(parents=True, exist_ok=True)
    files = [p for p in sorted(ROOT.rglob('*')) if p.is_file()
             and '.git' not in p.relative_to(ROOT).parts
             and '__pycache__' not in p.relative_to(ROOT).parts
             and p.suffix not in ['.pyc', '.zip', '.bundle']]
    if any(p.is_symlink() for p in files):
        parser.error('Review symlink inclusion explicitly before packaging.')
    with tempfile.TemporaryDirectory(prefix='opencl2-compact-handover-') as tmp:
        workspace = Path(tmp)
        stage = workspace / output.stem
        stage.mkdir()
        hashes = {}
        for p in files:
            rel = p.relative_to(ROOT)
            dest = stage / rel
            dest.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(p, dest)
            hashes[rel.as_posix()] = sha(p)
            if sha(dest) != hashes[rel.as_posix()]:
                raise RuntimeError('Copy mismatch: ' + str(rel))
        extra = stage / 'handover'
        extra.mkdir(exist_ok=True)
        (stage / 'UEBERGABE.md').write_text((ROOT / 'hosts/H2_RTX3090/s01/UEBERGABE.md').read_text())
        info = {'created_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
                'scope': 'complete current working files, excluding Git internals, history bundles, ZIP archives and caches',
                'working_files': len(files), 'git_history_included': False,
                'source_copy_sha256': 'all match', 'opencl_executions': 0,
                'base_commit': '3cd984d3e7cd8b71a2670c8b599c31d65872a078'}
        (extra / 'PACKAGE_INFO.json').write_text(json.dumps(info, indent=2) + '\n')
        manifest = extra / 'SHA256SUMS.txt'
        manifest.write_text(''.join(f'{sha(p)}  {p.relative_to(stage)}\n'
                                  for p in sorted(stage.rglob('*')) if p.is_file() and p != manifest))
        with zipfile.ZipFile(partial, 'x', compression=zipfile.ZIP_DEFLATED,
                             compresslevel=9, allowZip64=True) as z:
            for p in sorted(stage.rglob('*')):
                if p.is_file():
                    z.write(p, p.relative_to(workspace).as_posix())
        if partial.stat().st_size > a.max_mb * 1000000:
            size = partial.stat().st_size
            partial.unlink()
            raise SystemExit(f'Compressed size {size} exceeds requested limit; split optional evidence.')
        extract = workspace / 'extracted'
        with zipfile.ZipFile(partial) as z:
            bad = z.testzip()
            if bad:
                raise RuntimeError('ZIP CRC mismatch: ' + bad)
            z.extractall(extract)
        unpacked = extract / stage.name
        run(['sha256sum', '-c', 'handover/SHA256SUMS.txt'], unpacked)
        platform_check = run(['python3', 'scripts/check_platform_layout.py'], unpacked)
        c1_check = run(['python3', 'hosts/H2_RTX3090/c1/check_preparation.py'], unpacked)
        discovery_check = run(['python3', 'hosts/H2_RTX3090/security_discovery/check_preparation.py'], unpacked)
        s01_check = run(['python3', 'hosts/H2_RTX3090/s01/check_preparation.py'], unpacked)
        run(['sha256sum', '-c', 'manifests/H1_UPLOAD_SHA256_2026-10-30.txt'], unpacked / 'hosts/H1_AMD660M')
        for name, value in hashes.items():
            if sha(ROOT / name) != value or sha(unpacked / name) != value:
                raise RuntimeError('Source/extracted snapshot mismatch: ' + name)
        partial.rename(output)
        checksum.write_text(sha(output) + '  ' + output.name + '\n')
        print(json.dumps({'zip': str(output), 'bytes': output.stat().st_size,
                          'decimal_MB': round(output.stat().st_size / 1000000, 2),
                          'files': len(manifest.read_text().splitlines()) + 1,
                          'sha256': sha(output), 'platform_check': platform_check.strip(),
                          'c1_check': c1_check.strip(), 'discovery_check': discovery_check.strip(), 's01_check': s01_check.strip(), 'historical_H1_selected_hashes': 'all match'}, indent=2))


if __name__ == '__main__':
    main()
