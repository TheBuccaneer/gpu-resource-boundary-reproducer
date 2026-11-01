#!/usr/bin/env python3
"""Download primary documentation/source snapshots only; no builds or installation."""
import datetime, hashlib, json, pathlib, urllib.request, concurrent.futures
root=pathlib.Path(__file__).resolve().parents[1]/'research'/'sources'; root.mkdir(exist_ok=False)
urls={
 'mesa_zink.html':'https://docs.mesa3d.org/drivers/zink.html',
 'mesa_rusticl.html':'https://docs.mesa3d.org/rusticl.html',
 'mesa_nvk.html':'https://docs.mesa3d.org/drivers/nvk.html',
 'mesa_26_2.html':'https://docs.mesa3d.org/relnotes/26.2.0.html',
 'pocl_cuda.html':'https://portablecl.org/docs/html/cuda.html',
 'pocl_vulkan.html':'https://portablecl.org/docs/html/vulkan.html',
 'pocl_readme.md':'https://raw.githubusercontent.com/pocl/pocl/main/README.md',
 'clvk_readme.md':'https://raw.githubusercontent.com/kpet/clvk/main/README.md',
 'clvk_device.cpp':'https://raw.githubusercontent.com/kpet/clvk/main/src/device.cpp',
 'clvk_properties.cpp':'https://raw.githubusercontent.com/kpet/clvk/main/src/device_properties.cpp',
 'clvk_gitmodules.txt':'https://raw.githubusercontent.com/kpet/clvk/main/.gitmodules',
 'clspv_readme.md':'https://raw.githubusercontent.com/google/clspv/main/README.md',
 'clspv_language.md':'https://raw.githubusercontent.com/google/clspv/main/docs/OpenCLCOnVulkan.md',
 'nvidia_opencl.html':'https://developer.nvidia.com/opencl',
 'nvidia_gpus.html':'https://developer.nvidia.com/cuda/gpus',
 'khronos_conformance.html':'https://www.khronos.org/conformance/adopters/conformant-products/opencl',
 'cl_semaphore.asciidoc':'https://raw.githubusercontent.com/KhronosGroup/OpenCL-Docs/main/extensions/cl_khr_semaphore.asciidoc',
 'cl_external_semaphore.asciidoc':'https://raw.githubusercontent.com/KhronosGroup/OpenCL-Docs/main/extensions/cl_khr_external_semaphore.asciidoc',
 'cts_readme.md':'https://raw.githubusercontent.com/KhronosGroup/OpenCL-CTS/main/README.md',
 'mesa_zink_profile_26_0_8.json':'https://gitlab.freedesktop.org/mesa/mesa/-/raw/mesa-26.0.8/src/gallium/drivers/zink/VP_ZINK_requirements.json',
 'mesa_zink_profile_26_2_0.json':'https://gitlab.freedesktop.org/mesa/mesa/-/raw/mesa-26.2.0/src/gallium/drivers/zink/VP_ZINK_requirements.json',
}
for name,repo in [('clvk','kpet/clvk'),('clspv','google/clspv'),('pocl','pocl/pocl'),('cts','KhronosGroup/OpenCL-CTS'),('opencl_docs','KhronosGroup/OpenCL-Docs')]: urls[name+'_head.json']='https://api.github.com/repos/'+repo+'/commits?per_page=1'
def get(item):
 name,url=item; row={'file':name,'url':url,'retrieved_utc':datetime.datetime.now(datetime.timezone.utc).isoformat()}
 try:
  req=urllib.request.Request(url,headers={'User-Agent':'OpenCL-H2-C0-research/1.0'})
  with urllib.request.urlopen(req,timeout=25) as response: data=response.read();row['final_url']=response.url;row['status']=response.status
  (root/name).write_bytes(data);row['sha256']=hashlib.sha256(data).hexdigest();row['bytes']=len(data)
 except Exception as e: row['error']=str(e)
 return row
with concurrent.futures.ThreadPoolExecutor(max_workers=6) as pool: rows=list(pool.map(get,urls.items()))
(root/'SOURCES.json').write_text(json.dumps(rows,indent=2)); print(json.dumps([{k:v for k,v in r.items() if k in ['file','error','status']} for r in rows],indent=2))
