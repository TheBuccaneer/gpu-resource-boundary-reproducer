#!/usr/bin/python3
"""Portable offline material/plan/CPU evidence check. Never access live ledger."""
from pathlib import Path
import ast,hashlib,json,sys
B=Path(__file__).resolve().parent;R=B.parents[2];D=R/'hosts/H2_RTX3090/results/preparation/H2_F02_20261101'
sys.path.insert(0,str(B));import receipt_audit as audit;import verify_log as oracle
sha=audit.p.sha

def main():
 plan=json.loads((B/'PLAN_V1.json').read_text());case=plan['cases'][0]
 assert plan['plan_id']==case['id']=='F02-CLONE-SNAPSHOT-REVERSE' and len(plan['cases'])==1
 assert (case['repeat_processes'],case['kernel_dispatches'],case['reset_writes'],case['workitems'],case['local_size'],case['readbacks'],case['total_words'],case['completion_sequences'])==(1,3,1,64,1,7,1120,11)
 proof=json.loads((D/'LEDGER_AUDIT.json').read_text());verified=audit.verify_artifacts(R,proof['rows'],proof['metadata'],proof['schema']);assert len(verified)==10
 baseline=json.loads((D/'PRESERVATION_BEFORE.json').read_text());assert proof['rows']==baseline['ledger_rows'] and proof['ledger_sha256_after']==baseline['ledger_sha256']
 build=json.loads((D/'BUILD_PROVENANCE.json').read_text());assert build['production_binary_executed'] is False and build['OpenCL_runtime_invocations']==build['GPU_dispatches']==0
 assert sha(B/'build/h2_f02')==build['production_binary_sha256']
 for name,digest in build['inputs_sha256'].items():assert sha(R/name)==digest,name
 assert all(x['returncode']==0 for x in build['checks'])
 assert plan['canaries']['nonce_u32']==oracle.NONCE and plan['canaries']['salt_u32']==oracle.SALT
 for py in B.rglob('*.py'):ast.parse(py.read_text())
 for name,digest in json.loads((B/'SHA256SUMS.json').read_text()).items():assert sha(R/name)==digest,name
 valid=[json.loads(line) for line in (D/'CPU_FIXTURES/valid.jsonl').read_text().splitlines()]
 result=oracle.verify(valid,'CPU_FAKE_RUNTIME');assert result['words']==1120
 assert oracle.assess(valid)['outcome']=='INCONCLUSIVE_PROTOCOL_OR_API'
 a=json.loads((R/'hosts/H2_RTX3090/c1/approvals/f02_clone_snapshot_reverse_pilot_20261101.json').read_text())
 assert a['status']=='pending_explicit_approval' and a['approval_evidence'] is None and a['max_repeats']==1 and a['allowed_cases']==[plan['plan_id']]
 w=json.loads((R/a['workspace_snapshot_path']).read_text());assert hashlib.sha256(json.dumps(w['files'],sort_keys=True,separators=(',',':')).encode()).hexdigest()==w['workspace_sha256']==a['workspace_sha256']
 for name,digest in a['prepared_artifacts_sha256'].items():assert sha(R/name)==digest,name
 print(json.dumps({'outcome':'PASS_PREPARATION_ONLY','portable_ten_receipt_audit':'PASS','CPU_fixture':result,'live_ledger_access':False,'production_binary_executed':False},indent=2))
if __name__=='__main__':main()
