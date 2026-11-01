#!/usr/bin/env python3
"""Offline repository provenance and approved immutable executable snapshots.
This is an accidental-misexecution guard, not an isolation boundary against the owner.
"""
from contextlib import closing
import fcntl
import hashlib
import json
import os
from pathlib import Path
import shutil
import stat
import subprocess
import sys

ENV_KEYS = ('OCL_ICD_VENDORS', 'OCL_ICD_FILENAMES', 'LD_LIBRARY_PATH', 'LD_PRELOAD',
            'LD_AUDIT', 'CUDA_VISIBLE_DEVICES', 'VK_DRIVER_FILES', 'VK_ICD_FILENAMES',
            'RUSTICL_ENABLE', 'MESA_LOADER_DRIVER_OVERRIDE', 'POCL_DEVICES', 'PATH')

STABLE_PATH = '/usr/bin:/bin'
STABLE_TOOLS = {'python3': '/usr/bin/python3', 'git': '/usr/bin/git', 'bash': '/usr/bin/bash'}
GIT_BINDING_KEYS = ('mode', 'head', 'branch', 'dirty', 'working_diff_sha256', 'staged_diff_sha256')


def sha(path):
    h = hashlib.sha256()
    with Path(path).open('rb') as f:
        for block in iter(lambda: f.read(1024 * 1024), b''):
            h.update(block)
    return h.hexdigest()


def stable_tool_snapshot():
    """Explicit preparation only: snapshot system programs, never run OpenCL."""
    result = {}
    for name, command in STABLE_TOOLS.items():
        resolved = Path(command).resolve(strict=True)
        for path in (resolved, *resolved.parents):
            mode = path.stat()
            if mode.st_uid != 0 or mode.st_mode & 0o022:
                raise ValueError('Stable program/parent is not root-owned and non-writable: ' + str(path))
        if not resolved.is_file() or not os.access(resolved, os.X_OK):
            raise ValueError('Stable program unavailable: ' + command)
        result[name] = {'command_path': command, 'resolved_path': str(resolved), 'sha256': sha(resolved)}
    return result


def validate_execution_environment(approval, env):
    """Opt-in fixed-path contract. Legacy approvals keep exact PATH semantics."""
    if 'execution_environment' not in approval:
        return None
    policy = approval['execution_environment']
    if (not isinstance(policy, dict) or set(policy) != {'version', 'path', 'tools'}
            or type(policy['version']) is not int or policy['version'] != 1
            or policy['path'] != STABLE_PATH or env.get('PATH') != STABLE_PATH):
        raise ValueError('Invalid stable execution environment/PATH.')
    actual = stable_tool_snapshot()
    if policy['tools'] != actual:
        raise ValueError('Stable executable binding changed (path, target or SHA-256).')
    for name, command in STABLE_TOOLS.items():
        if shutil.which(name, path=env['PATH']) != command:
            raise ValueError('Stable executable resolution changed: ' + name)
    if str(Path(sys.executable).resolve()) != actual['python3']['resolved_path']:
        raise ValueError('Supervisor/launcher uses an unbound Python interpreter.')
    return {'version': 1, 'path': STABLE_PATH, 'tools': actual}


def stable_child_environment(approval, ambient):
    """Only the newly reviewed fixed-PATH contract permits this explicit change.
    Every runtime selection variable is checked BEFORE any child starts.
    """
    if 'execution_environment' not in approval:
        raise ValueError('Stable launcher requires a new explicit environment contract.')
    if set(approval.get('environment', {})) != set(ENV_KEYS):
        raise ValueError('Incomplete runtime selection binding.')
    if approval['environment']['PATH'] != STABLE_PATH:
        raise ValueError('Approval PATH is not the stable PATH.')
    for key in ENV_KEYS:
        if key != 'PATH' and approval['environment'][key] != ambient.get(key):
            raise ValueError('Runtime environment differs: ' + key)
    child = dict(ambient)
    child['PATH'] = STABLE_PATH
    validate_execution_environment(approval, child)
    return child


def git_provenance(root):
    def git(*args):
        return subprocess.check_output(
            ['/usr/bin/git', '--no-optional-locks', '-c', 'core.fsmonitor=false', '-C', str(root), *args],
            stderr=subprocess.DEVNULL, timeout=15)
    try:
        top = Path(git('rev-parse', '--show-toplevel').decode().strip()).resolve()
        if top != Path(root).resolve():
            raise ValueError('The enclosing Git repository is not this workspace.')
        status = git('status', '--porcelain=v1', '-z', '--untracked-files=all')
        unstaged = git('diff', '--no-ext-diff', '--no-textconv', '--binary', 'HEAD', '--')
        staged = git('diff', '--no-ext-diff', '--no-textconv', '--cached', '--binary', 'HEAD', '--')
        return {'mode': 'repository', 'head': git('rev-parse', 'HEAD').decode().strip(),
                'branch': git('rev-parse', '--abbrev-ref', 'HEAD').decode().strip(),
                'status_porcelain_v1_z': status.decode('utf-8', 'replace'),
                'status_sha256': hashlib.sha256(status).hexdigest(), 'dirty': bool(status),
                'working_diff_sha256': hashlib.sha256(unstaged).hexdigest(),
                'staged_diff_sha256': hashlib.sha256(staged).hexdigest()}
    except (OSError, ValueError, subprocess.SubprocessError) as exc:
        return {'mode': 'exported_snapshot', 'head': None, 'reason': str(exc),
                'note': 'No verified Git HEAD; content snapshot below is authoritative.'}


def workspace_provenance(root):
    root = Path(root).resolve()
    files = []
    # Approval documents and generated future run outputs must not self-reference the hash.
    excluded = ('hosts/H2_RTX3090/results/', 'hosts/H2_RTX3090/c1/approvals/')
    for p in sorted(root.rglob('*')):
        rel = p.relative_to(root)
        if any(part in ('.git', '__pycache__') for part in rel.parts) or p.suffix == '.pyc':
            continue
        if any(rel.as_posix().startswith(prefix) for prefix in excluded):
            continue
        if p.is_symlink():
            raise ValueError('Workspace symlinks require explicit review: ' + str(rel))
        if p.is_file():
            files.append({'path': rel.as_posix(), 'bytes': p.stat().st_size, 'sha256': sha(p)})
    canonical = json.dumps(files, sort_keys=True, separators=(',', ':')).encode()
    return {'workspace_root': str(root), 'workspace_sha256': hashlib.sha256(canonical).hexdigest(),
            'files': files, 'excluded_generated_prefixes': list(excluded),
            'git': git_provenance(root)}


def validate_approval(approval, binary, case, repeat, workspace, plan_sha, env):
    if approval.get('schema_version') != 1 or approval.get('status') != 'explicitly_approved':
        raise ValueError('No explicitly approved manifest. A template or CLI flag grants no approval.')
    if not approval.get('approval_id') or not approval.get('approval_evidence'):
        raise ValueError('Missing reviewable approval identity/evidence.')
    binary = Path(binary).resolve(strict=True)
    if str(binary) != approval.get('binary_path'):
        raise ValueError('Binary path is not approved.')
    if case not in approval.get('allowed_cases', []) or not 1 <= repeat <= approval.get('max_repeats', 0):
        raise ValueError('Case/repetition outside approved scope.')
    if approval.get('plan_sha256') != plan_sha or approval.get('workspace_sha256') != workspace['workspace_sha256']:
        raise ValueError('Plan or workspace content changed after review.')
    git = workspace['git']
    if approval.get('git_mode') != git['mode'] or approval.get('git_head') != git.get('head'):
        raise ValueError('Git provenance differs from approved mode/commit.')
    if set(approval.get('environment', {})) != set(ENV_KEYS):
        raise ValueError('Approval must bind every runtime-selection environment key.')
    for key in ENV_KEYS:
        if approval['environment'][key] != env.get(key):
            raise ValueError('Runtime environment differs: ' + key)
    if validate_execution_environment(approval, env) is not None:
        if (set(approval.get('git_binding', {})) != set(GIT_BINDING_KEYS)
                or any(approval['git_binding'][k] != git.get(k) for k in GIT_BINDING_KEYS)):
            raise ValueError('Stable approval Git state differs from reviewed snapshot.')
    return binary


def sealed_executable(binary, approved_sha256):
    """Copy checked ELF bytes to a sealed memfd and execute that snapshot, not the path."""
    if not isinstance(approved_sha256, str) or len(approved_sha256) != 64:
        raise ValueError('Invalid approved binary SHA-256.')
    path = Path(binary).resolve(strict=True)
    with path.open('rb') as f:
        mode = os.fstat(f.fileno()).st_mode
        if not stat.S_ISREG(mode) or not (mode & 0o111):
            raise ValueError('Approved binary must be a regular executable file.')
        data = f.read(64 * 1024 * 1024 + 1)
    if len(data) > 64 * 1024 * 1024 or not data.startswith(b'\x7fELF'):
        raise ValueError('Only bounded native ELF binaries are accepted; no interpreter scripts.')
    actual = hashlib.sha256(data).hexdigest()
    if actual != approved_sha256:
        raise ValueError('Binary SHA-256 differs from approval; no child started.')
    fd = os.memfd_create('h2-approved-executable', os.MFD_CLOEXEC | os.MFD_ALLOW_SEALING)
    try:
        offset = 0
        while offset < len(data):
            offset += os.write(fd, data[offset:])
        os.fchmod(fd, 0o500)
        seals = fcntl.F_SEAL_WRITE | fcntl.F_SEAL_GROW | fcntl.F_SEAL_SHRINK | fcntl.F_SEAL_SEAL
        fcntl.fcntl(fd, fcntl.F_ADD_SEALS, seals)
        if fcntl.fcntl(fd, fcntl.F_GET_SEALS) != seals:
            raise ValueError('Executable snapshot sealing failed.')
        return fd, {'source_path': str(path), 'sha256': actual, 'bytes': len(data),
                    'execution': 'sealed_memfd', 'seals': seals}
    except BaseException:
        os.close(fd)
        raise

# Fixed per OS account, deliberately independent of HOME/XDG, checkout and --out.
# An exported workspace must use this same host ledger, never a bundled copy.
def trial_ledger_path():
    import pwd
    return Path(pwd.getpwuid(os.getuid()).pw_dir) / '.local/state/opencl2/trials-v1/ledger.sqlite3'


LEDGER_SCHEMA = {
    'metadata': 'CREATE TABLE metadata (ledger_id TEXT PRIMARY KEY, schema_version INTEGER NOT NULL)',
    'trials': 'CREATE TABLE trials (approval_id TEXT NOT NULL, case_id TEXT NOT NULL, repeat_number INTEGER NOT NULL, record_json TEXT NOT NULL, record_sha256 TEXT NOT NULL, PRIMARY KEY (approval_id, case_id, repeat_number))',
}


def initialize_trial_ledger(path):
    """Explicit one-time provisioning only; never called by the supervisor.
    Refuse existing state. A replacement gets a new UUID, invalidating old approvals.
    """
    import sqlite3
    import uuid
    path = Path(path)
    path.parent.mkdir(parents=True, mode=0o700, exist_ok=False)
    fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL | os.O_NOFOLLOW, 0o600)
    os.close(fd)
    identity = str(uuid.uuid4())
    with closing(sqlite3.connect(str(path))) as db:
        db.execute('PRAGMA journal_mode=DELETE')
        db.execute('PRAGMA synchronous=EXTRA')
        for sql in LEDGER_SCHEMA.values():
            db.execute(sql)
        db.execute('INSERT INTO metadata VALUES (?, 1)', (identity,))
        db.commit()
    _sync_ledger(path)
    return {'path': str(path), 'ledger_id': identity, 'schema_version': 1}


def _sync_ledger(path):
    for name, flags in ((path, os.O_RDONLY), (path.parent, os.O_RDONLY | os.O_DIRECTORY)):
        fd = os.open(name, flags | os.O_NOFOLLOW)
        try:
            os.fsync(fd)
        finally:
            os.close(fd)


def _check_ledger_file(path):
    # Missing, symlinked, non-private or non-regular state fails closed.
    if not path.is_absolute():
        raise ValueError('Ledger path must be absolute.')
    if any(p.is_symlink() for p in (path, *path.parents)):
        raise ValueError('Ledger symlinks are not allowed.')
    directory, file = path.parent.stat(), path.stat()
    if (not stat.S_ISDIR(directory.st_mode) or directory.st_uid != os.getuid()
            or stat.S_IMODE(directory.st_mode) != 0o700
            or not stat.S_ISREG(file.st_mode) or file.st_uid != os.getuid()
            or stat.S_IMODE(file.st_mode) != 0o600 or file.st_nlink != 1
            or not 0 < file.st_size <= 64 * 1024 * 1024):
        raise ValueError('Ledger unavailable or invalid owner/mode/type/size.')


def reserve_trial(path, approval, case, repeat, approval_sha256, binding):
    """Commit a unique, permanent reservation BEFORE Popen (including failed exec).
    SQLite's IMMEDIATE writer transaction serializes concurrent supervisors.
    No deletion/release/retry API exists. Errors never grant permission to start.
    """
    import datetime
    import sqlite3
    import uuid
    path = Path(path)
    _check_ledger_file(path)
    if approval.get('ledger_path') != str(path):
        raise ValueError('Approval is not bound to the fixed ledger path.')
    identity = approval.get('ledger_id')
    if not isinstance(identity, str) or str(uuid.UUID(identity)) != identity:
        raise ValueError('Approval has no valid ledger identity.')
    if not isinstance(approval.get('approval_id'), str) or not approval['approval_id'].strip():
        raise ValueError('Invalid approval identity.')
    if type(repeat) is not int or not 1 <= repeat <= approval.get('max_repeats', 0):
        raise ValueError('Trial outside approved repetition budget.')
    if case not in approval.get('allowed_cases', []):
        raise ValueError('Trial case is not approved.')
    record = {'approval_id': approval['approval_id'], 'case_id': case, 'repeat_number': repeat,
              'approval_manifest_sha256': approval_sha256, 'ledger_id': identity,
              'reserved_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
              'reservation_state': 'permanently_consumed_before_spawn', 'binding': binding}
    encoded = json.dumps(record, sort_keys=True, separators=(',', ':'))
    digest = hashlib.sha256(encoded.encode()).hexdigest()
    db = sqlite3.connect(path.as_uri() + '?mode=rw', uri=True, timeout=5)
    try:
        if db.execute('PRAGMA journal_mode').fetchone() != ('delete',):
            raise ValueError('Unexpected ledger journal mode.')
        db.execute('PRAGMA synchronous=EXTRA')
        if db.execute('PRAGMA synchronous').fetchone() != (3,):
            raise ValueError('Ledger durability setting unavailable.')
        db.execute('BEGIN IMMEDIATE')
        if db.execute('PRAGMA integrity_check').fetchall() != [('ok',)]:
            raise ValueError('Corrupt trial ledger.')
        schema = dict(db.execute("SELECT name, sql FROM sqlite_master WHERE sql IS NOT NULL"))
        if schema != LEDGER_SCHEMA or db.execute('SELECT * FROM metadata').fetchall() != [(identity, 1)]:
            raise ValueError('Ledger schema or identity mismatch.')
        for aid, cid, number, old, old_hash in db.execute('SELECT * FROM trials'):
            previous = json.loads(old)
            if (hashlib.sha256(old.encode()).hexdigest() != old_hash
                    or (previous['approval_id'], previous['case_id'], previous['repeat_number']) != (aid, cid, number)
                    or previous['ledger_id'] != identity):
                raise ValueError('Damaged trial receipt.')
            if aid == approval['approval_id'] and previous['approval_manifest_sha256'] != approval_sha256:
                raise ValueError('Approval-ID cannot be reused for another manifest.')
        try:
            db.execute('INSERT INTO trials VALUES (?, ?, ?, ?, ?)',
                       (approval['approval_id'], case, repeat, encoded, digest))
        except sqlite3.IntegrityError as exc:
            raise ValueError('Trial already consumed: no child process may start.') from exc
        db.commit()
        _sync_ledger(path)
    finally:
        db.close()  # Rolls back incomplete transaction; no child has been started.
    return {'path': str(path), 'ledger_id': identity, 'schema_version': 1,
            'reservation_sha256': digest, 'reservation': record,
            'persistence': 'SQLite BEGIN IMMEDIATE; DELETE journal; synchronous EXTRA; file/directory fsync'}
