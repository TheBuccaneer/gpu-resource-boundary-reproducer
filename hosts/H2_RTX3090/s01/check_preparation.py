#!/usr/bin/env python3
"""Offline S01 integrity/consistency; no OpenCL loading or execution."""
import ast,hashlib,json,re
from pathlib import Path
B=Path(__file__).resolve().parent
R=B.parents[2]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 for line in (B/'SHA256SUMS.txt').read_text().splitlines():
  h,n=line.split('  ',1);p=B/n;assert p.resolve().is_relative_to(B) and sha(p)==h,n
 plan=json.loads((B/'TEST_CASES.json').read_text());src=(B/'h2_s01.c').read_text()
 declared=src.split('static const char *cases[]={',1)[1].split('};',1)[0]
 assert set(re.findall(r'"([^"]+)"',declared))=={x['id'] for x in plan['cases']}
 assert len(plan['cases'])==11 and plan['log_schema_version']==2
 for c in plan['cases']:
  trace=c['ordered_protocol'];assert trace and c['log_schema_version']==2
  assert sum(x.get('operation')=='dispatch' for x in trace)==c['kernel_dispatches']
  assert sum(x.get('operation')=='reset' for x in trace)==c['reset_writes']
  if c['reset_writes']:
   at=next(i for i,x in enumerate(trace) if x.get('operation')=='reset')
   assert any(x.get('storage_tag')==10 and x.get('expected_input_tag')==0 for x in trace[at:])
   assert sum(x.get('storage_tag')==12 for x in trace)==2
 assert sum(x['kernel_dispatches']==0 for x in plan['cases'])==5
 assert all(not x['runtime_executed'] and x['kernel_dispatches']<=4 for x in plan['cases'])
 v=json.loads((B/'notes/OFFLINE_VALIDATION.json').read_text());assert v['all_passed'] and v['OpenCL_runtime_invocations']==v['GPU_kernel_enqueues']==0
 for name,h in json.loads((B/'notes/R3_INPUT_PROVENANCE.json').read_text())['unchanged_R2_files'].items():assert sha(R/name)==h,name
 assert v['revision']=='S01-R3' and v['supervisor_e2e_methods']==24
 for p in B.rglob('*.py'):ast.parse(p.read_text())
 for x in json.loads((R/'notes/H1_RELOCATION_2026-10-31.json').read_text())['files']:assert sha(R/x['new_path'])==x['sha256']
 for p in B.glob('*.md'):
  for link in re.findall(r'\]\(([^)]+)\)',p.read_text()):
   if not re.match(r'^[A-Za-z][A-Za-z0-9+.-]*:',link):assert (p.parent/link.split('#')[0]).exists(),(p,link)
 print('PASS: S01 hashes, 11 cases (5 API-only, 6 data), offline results and unchanged H1 originals. No OpenCL.')
if __name__=='__main__':main()
