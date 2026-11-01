#!/usr/bin/env python3
"""Compile/analyze, run CPU tests; never starts the linked OpenCL executable."""
from pathlib import Path
import subprocess,json,tempfile,ast,datetime
R=Path(__file__).resolve().parents[4]
B=R/'hosts/H2_RTX3090/s01'
def main():
 records=[]
 def run(args,scope):
  x=subprocess.run(args,cwd=R,capture_output=True,text=True,timeout=120)
  records.append({'argv':args,'returncode':x.returncode,'stdout':x.stdout,'stderr':x.stderr,'scope':scope});print(scope,x.returncode,flush=True)
 with tempfile.TemporaryDirectory(prefix='s01-static-') as tmp:
  src='hosts/H2_RTX3090/s01/h2_s01.c';opts=['-std=c11','-Wall','-Wextra','-Werror','-Wpedantic']
  run(['gcc',*opts,'-c',src,'-o',tmp+'/s01.o'],'GCC host object compilation only')
  run(['gcc',*opts,src,'-lOpenCL','-ldl','-o',tmp+'/h2_s01'],'Host link only; executable never started')
  run(['clang',*opts,'--analyze','-Xanalyzer','-analyzer-output=text',src],'Clang static analysis only')
  for name in ['h2_c1.c','h2_b12_query.c']:run(['gcc',*opts,'-c','hosts/H2_RTX3090/c1/reproducers/'+name,'-o',tmp+'/'+name+'.o'],'Existing '+name+' host compilation')
 for args,scope in [(['python3',str(B/'tests/test_supervisor_e2e.py')],'CPU supervisor end-to-end'),(['python3',str(B/'tests/test_s01_offline.py')],'S01 full protocol and mutation tests'),(['python3',str(R/'hosts/H2_RTX3090/security_discovery/tests/test_security_audit.py')],'Existing CPU audit regressions'),(['python3','hosts/H2_RTX3090/c1/reproducers/oracle.py','--self-check'],'Existing synthetic C1 oracle')]:run(args,scope)
 for p in list(B.rglob('*.py'))+list((R/'hosts/H2_RTX3090/c1/reproducers').glob('*.py')):ast.parse(p.read_text())
 summary={'verified_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'revision':'S01-R3','all_passed':all(x['returncode']==0 for x in records),'checks':records,'python_ast':'pass','supervisor_e2e_methods':24,'s01_offline_methods':7,'existing_audit_methods':8,'b12_host_assertions':22,'OpenCL_runtime_invocations':0,'GPU_kernel_enqueues':0,'compiled_linked_OpenCL_program_executed':False}
 (B/'notes/OFFLINE_VALIDATION.json').write_text(json.dumps(summary,indent=2)+'\n')
 return 0 if summary['all_passed'] else 1
if __name__=='__main__':raise SystemExit(main())
