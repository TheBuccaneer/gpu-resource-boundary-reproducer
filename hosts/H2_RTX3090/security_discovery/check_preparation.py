#!/usr/bin/env python3
"""Offline handover integrity only. Never loads OpenCL."""
import hashlib,json,ast
from pathlib import Path
B=Path(__file__).resolve().parent
R=B.parents[2]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 count=0
 for line in (B/'SHA256SUMS.txt').read_text().splitlines():
  expected,name=line.split('  ',1);p=B/name
  assert p.resolve().is_relative_to(B) and sha(p)==expected,name
  count+=1
 h=json.loads((B/'HYPOTHESES.json').read_text())
 assert len(h)==12 and {x['id'] for x in h}=={f'S{i:02}' for i in range(1,13)}
 for x in h:
  assert not x['runtime_executed']
  for k in ['normative_contract','boundary_threat_model','mechanism','oracle','controls','impact','confounders','reproduction','stop','literature','platforms','novelty','measurements','repetitions']:assert x[k],(x['id'],k)
 assert len(json.loads((B/'literature/READING_LOG.json').read_text()))==16
 for p in (B/'literature').rglob('*'):
  assert p.suffix.lower() not in ['.pdf','.txt'],p
 for p in B.rglob('*.py'):ast.parse(p.read_text())
 v=json.loads((B/'notes/OFFLINE_VALIDATION.json').read_text())
 assert v['audit_unittests_returncode']==0 and v['host_compilation_passed']==2 and v['OpenCL_calls_executed']==0
 original=json.loads((R/'notes/H1_RELOCATION_2026-10-31.json').read_text())
 assert len(original['files'])==729
 for x in original['files']:assert sha(R/x['new_path'])==x['sha256']
 print(f'PASS: {count} Security Discovery hashes, 12 hypotheses, 16 literature profiles, offline evidence and 729 unchanged H1 originals. No OpenCL.')
if __name__=='__main__':main()
