#!/usr/bin/python3
"""Compile/link/analyze only production ELF; execute ONLY static CPU fake."""
from pathlib import Path
import hashlib,json,os,subprocess,tempfile,datetime,sys
B=Path(__file__).resolve().parent;R=B.parents[2];D=R/'hosts/H2_RTX3090/results/preparation/H2_F01_20261101'
sys.path.insert(0,str(B));import verify_log as oracle
sha=lambda file:hashlib.sha256(Path(file).read_bytes()).hexdigest()
def main():
 records=[]
 def run(argv):
  x=subprocess.run(argv,cwd=R,capture_output=True,text=True,timeout=60);records.append({'argv':[str(a) for a in argv],'returncode':x.returncode,'stdout':x.stdout,'stderr':x.stderr})
  if x.returncode:raise RuntimeError(x.stderr or x.stdout)
  return x
 flags=['-std=c11','-Wall','-Wextra','-Werror','-Wpedantic','-O2']
 binary=B/'build/h2_f01';binary.parent.mkdir(exist_ok=True)
 if binary.exists():binary.chmod(0o700)
 with tempfile.TemporaryDirectory(prefix='f01-build-cpu-') as temp:
  temp=Path(temp)
  run(['/usr/bin/gcc',*flags,'-c',str(B/'h2_f01.c'),'-o',str(temp/'f01.o')])
  run(['/usr/bin/gcc',*flags,str(B/'h2_f01.c'),'-L/usr/lib/x86_64-linux-gnu','-lOpenCL','-Wl,-z,relro,-z,now','-o',str(binary)])
  run(['/usr/bin/clang','--analyze',*flags,'-Xanalyzer','-analyzer-output=text',str(B/'h2_f01.c')])
  fake=temp/'f01_CPU_FAKE';run(['/usr/bin/gcc',*flags,'-DF01_CPU_FIXTURE',str(B/'h2_f01.c'),str(B/'tests/fake_opencl.c'),'-o',str(fake)])
  cpu_dynamic=run(['/usr/bin/readelf','-d',str(fake)]).stdout
  assert all(name not in cpu_dynamic for name in ['libOpenCL','libcuda','nvidia'])
  fixtures=D/'CPU_FIXTURES';fixtures.mkdir(exist_ok=True);fixture_results=[]
  for mode in ['reject38','reject34','unexpected_success','resource5','resource6','other49','foreign_bind','bad_completion']:
   x=subprocess.run([str(fake),'--approved-run','H2-F01'],env={**os.environ,'F01_CPU_SCENARIO':mode},capture_output=True,text=True,timeout=10)
   (fixtures/(mode+'.jsonl')).write_text(x.stdout);(fixtures/(mode+'.stderr')).write_text(x.stderr)
   result=oracle.assess([json.loads(line) for line in x.stdout.splitlines()],'CPU_FAKE_RUNTIME');fixture_results.append({'mode':mode,'exit_code':x.returncode,'oracle':result})
   if mode in ['reject38','reject34']:assert x.returncode==0 and result['outcome']=='PASS_CPU_FIXTURE'
   else:assert x.returncode!=0 and not result['outcome'].startswith('PASS')
  # Replace the earlier development sample with the current complete fixture.
  (D/'CPU_reject38.jsonl').write_bytes((fixtures/'reject38.jsonl').read_bytes());(D/'CPU_reject38.stderr').write_bytes((fixtures/'reject38.stderr').read_bytes())
  cpu_hash=sha(fake)
 binary.chmod(0o500);dynamic=run(['/usr/bin/readelf','-d',str(binary)]).stdout;assert 'libOpenCL.so.1' in dynamic
 for tool in ['/usr/bin/gcc','/usr/bin/clang','/usr/bin/python3']:run([tool,'--version'])
 (D/'ELF_DYNAMIC.txt').write_text(dynamic)
 inputs=[B/'h2_f01.c',B/'tests/fake_opencl.c',B/'oracle_values.py',B/'verify_log.py',R/'hosts/H2_RTX3090/c1/reproducers/h2_common.h']
 result={'outcome':'PASS','built_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'production_binary_path':str(binary),'production_binary_sha256':sha(binary),'production_binary_executed':False,'production_linked_OpenCL_executed':False,'OpenCL_runtime_invocations':0,'GPU_dispatches':0,'CPU_fixture_binary_sha256':cpu_hash,'CPU_fixture_has_no_OpenCL_CUDA_dependency':True,'inputs_sha256':{str(p.relative_to(R)):sha(p) for p in inputs},'checks':records,'CPU_fixture_results':fixture_results}
 (D/'BUILD_PROVENANCE.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k not in ['checks','inputs_sha256','CPU_fixture_results']},indent=2))
if __name__=='__main__':main()
