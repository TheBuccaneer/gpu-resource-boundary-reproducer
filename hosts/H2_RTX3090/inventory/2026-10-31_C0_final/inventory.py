#!/usr/bin/env python3
"""Read-only host collection; writes evidence only to a NEW output directory.
No installation, sudo, contexts, GPU dispatches, allocations or configuration changes.
Run: python3 inventory.py --out /absolute/new/directory
"""
import argparse, ctypes, datetime, glob, hashlib, json, os, pathlib, re, shutil, subprocess, sys
P=pathlib.Path
ap=argparse.ArgumentParser(description=__doc__); ap.add_argument('--out',required=True); a=ap.parse_args()
out=P(a.out).resolve(); out.mkdir(parents=True,exist_ok=False)
log=[]
def run(label,argv,timeout=35):
    start=datetime.datetime.now(datetime.timezone.utc).isoformat()
    try:
        r=subprocess.run(argv,capture_output=True,text=True,timeout=timeout)
        stdout,stderr,rc=r.stdout,r.stderr,r.returncode
    except (OSError,subprocess.TimeoutExpired) as e: stdout,stderr,rc='',str(e),None
    (out/(label+'.stdout')).write_text(stdout); (out/(label+'.stderr')).write_text(stderr)
    log.append(dict(label=label,argv=argv,start_utc=start,returncode=rc))
    return stdout
commands={
 'uname':['uname','-a'],'os':['cat','/etc/os-release'],'cpu':['lscpu'],'cpu_topology':['lscpu','-e=CPU,NODE,SOCKET,CORE,ONLINE'],
 'memory':['free','-b'],'virtualization':['systemd-detect-virt'],'pci':['lspci','-Dnnk'],'pcie_gpu':['lspci','-Dvv','-s','01:00.0'],
 'modules':['lsmod'],'nvidia_version':['cat','/proc/driver/nvidia/version'],
 'nvidia_gpus':['nvidia-smi','--query-gpu=index,name,pci.bus_id,driver_version,memory.total,compute_cap','--format=csv'],
 'nvidia_detail':['nvidia-smi','-q'],'nvidia_topology':['nvidia-smi','topo','-m'],
 'ldconfig':['ldconfig','-p'],'clinfo_version':['clinfo','--version'],'clinfo_list':['clinfo','--list'],
 'opencl_query':[sys.executable,str(P(__file__).with_name('query_opencl.py'))],
 'packages':['dpkg-query','-W','-f=${binary:Package}\t${Version}\t${db:Status-Abbrev}\n'],
 'pkgconfig':['pkg-config','--modversion','OpenCL','vulkan','libdrm'],
 'gcc':['gcc','--version'],'clang':['clang','--version'],'llvm':['llvm-config','--version'],'llvm20':['llvm-config-20','--version'],'llvm21':['llvm-config-21','--version'],'llvm_targets':['llvm-config-21','--targets-built'],
 'python':[sys.executable,'--version'],'rust':['rustc','--version'],'cargo':['cargo','--version'],
 'cmake':['cmake','--version'],'meson':['meson','--version'],'ninja':['ninja','--version'],'make':['make','--version'],
 'nvcc':['nvcc','--version'],'ninja_version':['ninja','--version'],'nvcc_alt':['/usr/local/cuda/bin/nvcc','--version'],
 'nvidia_modinfo':['modinfo','nvidia'],'nouveau_modinfo':['modinfo','nouveau']}
for name,argv in commands.items(): run(name,argv)
# Build only a CPU-side query utility into a temporary directory.
import tempfile
with tempfile.TemporaryDirectory(prefix='h2-c0-') as tmp:
    binary=str(P(tmp)/'query_vulkan')
    run('vulkan_query_build',['gcc','-Wall','-Wextra',str(P(__file__).with_name('query_vulkan.c')),'-lvulkan','-o',binary])
    if P(binary).exists(): run('vulkan_query',[binary])
try:
    cuda=ctypes.CDLL('libcuda.so.1'); version=ctypes.c_int()
    rc=cuda.cuDriverGetVersion(ctypes.byref(version))
    (out/'cuda_driver_version.json').write_text(json.dumps({'api':'cuDriverGetVersion (no cuInit)','status':rc,'version':version.value}))
except OSError as e: (out/'cuda_driver_version.json').write_text(json.dumps({'error':str(e)}))
for name in ['inventory.py','query_opencl.py','query_vulkan.c']:
    shutil.copyfile(P(__file__).with_name(name),out/name)
files={}
patterns=['/boot/config-*','/proc/config.gz','/proc/cmdline','/proc/meminfo','/etc/OpenCL/vendors/*','/usr/share/vulkan/icd.d/*','/etc/vulkan/icd.d/*','/usr/local/cuda/version*','/usr/include/CL/*.h','/usr/include/vulkan/vulkan_core.h','/sys/devices/system/node/online','/sys/class/dmi/id/board_name','/sys/class/dmi/id/board_vendor']
for pat in patterns:
 for name in glob.glob(pat):
  try:
   raw=P(name).read_bytes(); files[name]={'resolved':str(P(name).resolve()),'sha256':hashlib.sha256(raw).hexdigest(),'bytes':len(raw)}
   if raw and b'\x00' not in raw and len(raw)<300000: files[name]['text']=raw.decode('utf-8','replace')
  except OSError as e: files[name]={'error':str(e)}
pci=[]
for p in sorted(P('/sys/bus/pci/devices').iterdir()):
 try:
  cls=(p/'class').read_text().strip(); vendor=(p/'vendor').read_text().strip()
  if vendor!='0x10de' and not cls.startswith('0x03'): continue
  row={'bdf':p.name,'driver':str((p/'driver').resolve()) if (p/'driver').exists() else None}
  for f in ['vendor','device','class','numa_node','local_cpulist','current_link_speed','current_link_width','max_link_speed','max_link_width']:
   try: row[f]=(p/f).read_text().strip()
   except OSError as e: row[f]={'error':str(e)}
  pci.append(row)
 except OSError: pass
(out/'files.json').write_text(json.dumps(files,indent=2)); (out/'pci_sysfs.json').write_text(json.dumps(pci,indent=2))
# Only environment variables capable of changing runtime selection, no arbitrary environment dump.
keys=['OCL_ICD_VENDORS','OCL_ICD_FILENAMES','OPENCL_VENDOR_PATH','LD_LIBRARY_PATH','LD_PRELOAD','RUSTICL_ENABLE','MESA_LOADER_DRIVER_OVERRIDE','VK_ICD_FILENAMES','VK_DRIVER_FILES','CUDA_VISIBLE_DEVICES','POCL_DEVICES','PATH']
(out/'runtime_environment.json').write_text(json.dumps({k:os.environ.get(k) for k in keys},indent=2))
# Hash actual loader/ICD/compiler dependencies and their resolution; no library binaries copied.
paths=set()
for line in (out/'ldconfig.stdout').read_text().splitlines():
 if re.search(r'OpenCL|nvidia|libcuda|libvulkan|Rusticl|rusticl|libpocl|libLLVM|libLLVMSPIRV',line) and '=>' in line: paths.add(line.split('=>')[-1].strip())
try:
 q=json.loads((out/'opencl_query.stdout').read_text())
 for p in q.get('platforms',[]): pass
 for line in q.get('loaded_library_maps',[]):
  if '/' in line: paths.add('/'+line.split('/',1)[1])
except (ValueError,OSError): pass
hashes=[]
for s in sorted(paths):
 try:
  p=P(s); h=hashlib.sha256()
  with p.open('rb') as f:
   for b in iter(lambda:f.read(1024*1024),b''): h.update(b)
  hashes.append({'path':s,'resolved':str(p.resolve()),'sha256':h.hexdigest(),'bytes':p.stat().st_size})
 except OSError as e: hashes.append({'path':s,'error':str(e)})
(out/'library_hashes.json').write_text(json.dumps(hashes,indent=2))
(out/'commands.json').write_text(json.dumps(log,indent=2))
(out/'collection.json').write_text(json.dumps({'started_by':str(P(__file__).resolve()),'finished_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'script_sha256':hashlib.sha256(P(__file__).read_bytes()).hexdigest(),'query_sha256':hashlib.sha256(P(__file__).with_name('query_opencl.py').read_bytes()).hexdigest(),'scope':'enumeration/info only; no context/program/queue/device allocation/dispatch'},indent=2))
manifest=''.join(f'{hashlib.sha256(p.read_bytes()).hexdigest()}  {p.relative_to(out)}\n' for p in sorted(out.rglob('*')) if p.is_file())
(out/'SHA256SUMS').write_text(manifest)
print(out)
