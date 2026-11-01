#!/usr/bin/python3
"""Offline F01 checksum/protocol consistency; optional read-only host preflight."""
from pathlib import Path
import argparse,ast,hashlib,json,sys,os
B=Path(__file__).resolve().parent;R=B.parents[2];D=R/'hosts/H2_RTX3090/results/preparation/H2_F01_20261101'
sys.path.insert(0,str(B));import verify_log as oracle
sha=lambda file:hashlib.sha256(Path(file).read_bytes()).hexdigest()
def main():
 parser=argparse.ArgumentParser();parser.add_argument('--host-preflight',action='store_true');args=parser.parse_args()
 count=0
 for line in (B/'SHA256SUMS.txt').read_text().splitlines():
  digest,name=line.split('  ',1);file=B/name;assert file.resolve().is_relative_to(B) and sha(file)==digest,name;count+=1
 plan=json.loads((B/'PLAN_V1.json').read_text());assert plan['plan_id']=='H2-F01' and plan['plan_version']==1 and len(plan['cases'])==1 and plan['cases'][0]['kernel_dispatches']==1 and plan['cases'][0]['repeat_processes']==1
 build=json.loads((D/'BUILD_PROVENANCE.json').read_text());assert build['outcome']=='PASS' and build['production_binary_executed'] is False and build['OpenCL_runtime_invocations']==build['GPU_dispatches']==0 and sha(B/'build/h2_f01')==build['production_binary_sha256']
 for file,digest in build['inputs_sha256'].items():assert sha(R/file)==digest,file
 for mode,status in [('reject38',-38),('reject34',-34)]:
  rows=[json.loads(line) for line in (D/'CPU_FIXTURES'/(mode+'.jsonl')).read_text().splitlines()];result=oracle.verify(rows,'CPU_FAKE_RUNTIME');assert result['actual_foreign_status']==status and result['words']==plan['cases'][0]['total_words']
  try:oracle.verify(rows)
  except oracle.ProtocolError:pass
  else:raise AssertionError('Fixture accepted as real measurement')
 for file in B.rglob('*.py'):ast.parse(file.read_text())
 if args.host_preflight:
  import preflight
  _,_,_,proof=preflight.check(os.environ);assert proof['status']=='pending_explicit_approval' and not proof['execution_authorized'];print(json.dumps(proof,indent=2))
 print(f'PASS: {count} F01 source/binary hashes; independent CPU fixtures and parser; no native/OpenCL execution.')
if __name__=='__main__':main()
