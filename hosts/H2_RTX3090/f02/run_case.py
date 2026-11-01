#!/usr/bin/python3
"""F02-only entry point. Reuses R3 limits/RSS and provenance/atomic ledger.
The monitoring/spawn/termination block below is adapted verbatim from R3.
No old case loader, old plan alteration, alternative case or override CLI.
"""
import sqlite3
import datetime,hashlib,importlib.util,json,os,signal,subprocess,sys,time
from pathlib import Path
from types import SimpleNamespace
sys.path.insert(0,str(Path(__file__).resolve().parent))
import preflight as f
p=f.p
spec=importlib.util.spec_from_file_location('unchanged_r3',f.C1/'reproducers/run_approved_case.py');r3=importlib.util.module_from_spec(spec);spec.loader.exec_module(r3)
limits=r3.limits;rss_group=r3.rss_group;WALL_SECONDS=r3.WALL_SECONDS
sha=p.sha;workspace_provenance=p.workspace_provenance;reserve_trial=p.reserve_trial;TRIAL_LEDGER=p.trial_ledger_path()

def execute(approval,env,workspace_before):
    # Constants are fixed in the reviewed F02 module; there are no path overrides.
    root=f.ROOT;out=f.OUTPUT;binary=f.BINARY;approval_bytes=f.MANIFEST.read_bytes()
    if json.loads(approval_bytes)!=approval:raise ValueError('Approval changed after preflight')
    execution_environment=approval['execution_environment']
    p.validate_approval(approval,binary,f.CASE,1,workspace_before,p.sha(f.PLAN),env)
    execution_fd,execution_snapshot=p.sealed_executable(binary,approval['binary_sha256'])
    try:out.mkdir(parents=True,exist_ok=False)
    except BaseException:os.close(execution_fd);raise
    args=[str(binary),'--approved-run',f.CASE];a=SimpleNamespace(case=f.CASE,repeat_number=1)
    info={'case':f.CASE,'repeat':1,'approval_id':approval['approval_id'],'argv':args,
          'approval_manifest_sha256':hashlib.sha256(approval_bytes).hexdigest(),
          'binary_sha256':execution_snapshot['sha256'],'executable_snapshot':execution_snapshot,
          'test_plan_sha256':p.sha(f.PLAN),'test_plan_path':str(f.PLAN),'plan_version':1,
          'workspace_before_sha256':workspace_before['workspace_sha256'],'git_before':workspace_before['git'],
          'environment':{k:env.get(k) for k in p.ENV_KEYS},'execution_environment':approval['execution_environment'],
          'runtime_file_sha256':approval['runtime_file_sha256'],
          'supervisor_source_sha256':p.sha(Path(__file__)),'unchanged_R3_source_sha256':p.sha(f.C1/'reproducers/run_approved_case.py'),
          'wall_limit_seconds':WALL_SECONDS,'CPU_limit_seconds':45,'observed_RSS_limit_bytes':2*1024**3,'output_limit_bytes':16*1024**2,
          'start_utc':datetime.datetime.now(datetime.timezone.utc).isoformat()}
    (out/'workspace_before.json').write_text(json.dumps(workspace_before,indent=2))
    (out/'approval_used.json').write_bytes(approval_bytes)
    (out/'run.json').write_text(json.dumps(info,indent=2))
    # Stable effective PATH only, after validation; runtime selection unchanged.
    os.environ.clear();os.environ.update(env)
    reason = None
    peak = 0
    child = None
    launch_error = None
    with (out / 'stdout.jsonl').open('wb') as stdout, (out / 'stderr.log').open('wb') as stderr:
        start = time.monotonic()
        try:
            # Durable consumption is independent of --out and precedes every exec.
            try:
                info['trial_ledger'] = reserve_trial(TRIAL_LEDGER, approval, a.case, a.repeat_number,
                    info['approval_manifest_sha256'], {
                        'binary_sha256': info['binary_sha256'], 'binary_path': str(binary),
                        'plan_sha256': info['test_plan_sha256'],
                        'workspace_sha256': info['workspace_before_sha256'],
                        'environment': info['environment'],
                        'execution_environment': execution_environment,
                        'git': info['git_before']})
                (out / 'run.json').write_text(json.dumps(info, indent=2))
            except Exception as exc:
                reason = 'trial_ledger_rejected'
                info['trial_ledger_error'] = str(exc)
                raise OSError('Trial ledger rejected: ' + str(exc)) from exc
            child = subprocess.Popen(args, executable=f'/proc/self/fd/{execution_fd}', pass_fds=(execution_fd,),
                                     stdout=stdout, stderr=stderr, start_new_session=True, preexec_fn=limits)
        except OSError as exc:
            reason = reason or 'process_start_failed'
            launch_error = str(exc)
        finally:
            os.close(execution_fd)
        while child is not None and child.poll() is None:
            current = rss_group(child.pid)
            peak = max(peak, current)
            if time.monotonic() - start > WALL_SECONDS:
                reason = 'wall_timeout'
            elif current > 2 * 1024**3:
                reason = 'rss_limit'
            elif sum((out / name).stat().st_size for name in ['stdout.jsonl', 'stderr.log']) > 16 * 1024**2:
                reason = 'output_limit'
            if reason:
                try:
                    os.killpg(child.pid, signal.SIGTERM)
                except ProcessLookupError:
                    pass
                try:
                    child.wait(timeout=1)
                except subprocess.TimeoutExpired:
                    try:
                        os.killpg(child.pid, signal.SIGKILL)
                    except ProcessLookupError:
                        pass
                    try:
                        child.wait(timeout=2)
                    except subprocess.TimeoutExpired:
                        reason += "_termination_unconfirmed"
                break
            time.sleep(0.05)
    if child is not None and child.returncode is not None and rss_group(child.pid) > 0:
        reason = reason or 'remaining_process_group'
        try:
            os.killpg(child.pid, signal.SIGKILL)
        except ProcessLookupError:
            pass
    code = child.returncode if child is not None else None
    if reason is None and code not in (0, 77, 78):
        reason = 'child_signal' if code is not None and code < 0 else 'child_exit_nonzero'
    workspace_after = workspace_provenance(root)
    (out / 'workspace_after.json').write_text(json.dumps(workspace_after, indent=2))
    if workspace_after['workspace_sha256'] != workspace_before['workspace_sha256']:
        reason = reason or 'workspace_changed_during_run'
    data_oracle=None
    if reason is None and code==0:
        import verify_log
        try:data_oracle=verify_log.assess([json.loads(line) for line in (out/'stdout.jsonl').read_text().splitlines() if line.strip()])
        except (OSError,ValueError) as exc:data_oracle={'outcome':'INCONCLUSIVE_INVALID_LOG','reason':str(exc)}
        if data_oracle['outcome']!='PASS_OBSERVED_STATE':reason='independent_data_oracle_rejected'
    info['independent_data_oracle']=data_oracle
    (out/'independent_oracle.json').write_text(json.dumps(data_oracle,indent=2))
    info.update({'git_after': workspace_after['git'], 'workspace_after_sha256': workspace_after['workspace_sha256'],
                 'workspace_before_record_sha256': sha(out / 'workspace_before.json'),
                 'workspace_after_record_sha256': sha(out / 'workspace_after.json'), 'end_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
                 'elapsed_seconds': time.monotonic() - start, 'returncode': code, 'launch_error': launch_error,
                 'termination_signal': -code if code is not None and code < 0 else None,
                 'outcome': 'failed' if reason else ('capability_skip' if code == 77 else ('inconclusive' if code == 78 else 'passed')),
                 'approval_used_sha256': sha(out / 'approval_used.json'),
                 'stop_reason': reason, 'observed_process_group_peak_rss_bytes': peak,
                 'gpu_completion_guaranteed_by_process_exit': False,
                 'stdout_sha256': sha(out / 'stdout.jsonl'), 'stderr_sha256': sha(out / 'stderr.log')})
    (out / 'run.json').write_text(json.dumps(info, indent=2))
    print('One case completed; no retries or next cases were started. Review', out / 'run.json')
    return 20 if reason else (code if code in (77, 78) else 0)

def main():
    import argparse
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--execute-approved-plan',action='store_true')
    args=parser.parse_args()
    try:
        approval,env,workspace,result=f.check(os.environ)
        print(json.dumps(result,indent=2),flush=True)
        if not args.execute_approved_plan:return 0
        if approval['status']!='explicitly_approved':raise ValueError('New explicit F02 GO missing; no trial started')
        return execute(approval,env,workspace)
    except (OSError,ValueError,KeyError,sqlite3.Error) as exc:
        print('STOP: '+str(exc),file=sys.stderr);return 2
if __name__=='__main__':raise SystemExit(main())
