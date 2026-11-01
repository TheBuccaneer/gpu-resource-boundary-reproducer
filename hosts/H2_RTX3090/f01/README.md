# H2-F01 v1 – rejected foreign setter, own state

New preregistered relation, separate from all S01 files and results. This source tree prepares a future single trial; it grants no runtime authorization. No F02 or F03 implementation is included.

One pinned RTX 3090, two live contexts A/B. Kernel and own arguments are in A. A fully initialized, live B1 buffer belongs to B. A single correct-index/correct-size foreign setter targets arg0. Raw −34 and −38 are distinct status classes sharing only the normative state-preservation relation. No old FOREIGN FAIL is relabelled. Success, resources −5/−6 or any other status stops before queue creation/enqueue.

Exactly one in-bounds dispatch (64 workitems, local 1) after accepted rejection. Arg0 is never rebound. Arg1 is AO. Arg2/arg3 form a fresh valid own control using separate CI/CO buffers in A. Both arms run in the same dispatch; this is not an independent control process or a pretransition execution. This bounded design adds 320 control readback words to the required AO/A1/B1/AS boundary suffix (640 words).

Six buffers, each 160 uint32/640 bytes: A1 tag1, AO tag10, B1 tag2, unused AS tag30, CI tag3, CO tag11. Fresh preregistered nonce 0xf010ca5e; initialized word is u32(0x9e3779b9*(i+1)) XOR u32(tag*0x01020304) XOR nonce. AO active indices16–79 use A1 XOR 0x37a1b5c9; CO uses CI XOR 0x68b2d7e1. All other words unchanged. Read order CO, CI, AO, A1, B1, AS; 960 words, seven completed wait/release sequences. The sentinel is never a kernel argument. B1 is read only through QB in B. All memory objects stay alive until completion/cleanup.

Object generations are application-assigned creation ordinals, not a query of undocumented driver generations. Queried buffer/kernel contexts, live typed handle identities, kernel/program association, event generations, complete data vectors and ordered cleanup are logged and checked. Handles may be reused after event release; distinct live buffers/contexts may not alias.

`oracle_values.py` is a separate pure mathematical module. Neither it nor the protocol parser reads PLAN_V1.json or C source. The CPU fake statically links the real C test against fake API functions; it neither links nor loads libOpenCL/CUDA. Its output origin is CPU_FAKE_RUNTIME, rejected by the default real-evidence parser. No synthetic log is a new measured GPU result.

The new F01-only entrypoint reuses unchanged R3 limits/RSS functions and provenance/atomic-ledger functions. Its monitoring/spawn/kill block is adapted from R3; the old R3 file is untouched. It accepts no alternative case/binary/output/repetition CLI. SHA-bound material preflight and pending GO gate precede sealed execution and permanent reservation. Eight historical receipts must remain byte-identical before a future first F01 run.

A new source tree and binary change the workspace content hash. Historical S01 approvals remain byte-identical evidence and are not silently updated. In particular, any old pending CLONE template must be checked/reprepared against the then-current workspace before a separate future GO. No old approval authorizes F01.

Preflight only (after preparation/freezing):

```
/usr/bin/python3 -I /home/rock/projects/gpu-resource-boundary-reproducer/hosts/H2_RTX3090/f01/run_case.py
```

New independent review and explicit user GO are required before proposing one actual runtime trial. No execution command is authorized by this README. An explicitly activated future F01 manifest would use this same fixed entrypoint with --execute-approved-plan, exactly once; it is pending now.

Post-run default real-evidence checker: `python3 hosts/H2_RTX3090/f01/verify_log.py --log ACTUAL_STDOUT`. CPU logs require --cpu-fixture. API-code interpretation, observed preserved state, a possible foreign-data signature and OS/tenant isolation are separate outputs. A mismatch is a candidate requiring review, not a confirmed vulnerability; no automatic confirmation run.

Limits reused: 60 s wall, 45 s CPU, 2 GiB observed RSS, 16 MiB output, 5 s per event poll. No hard GPU preemption guaranteed. Stop at first unexpected API/control/event/data/resource/provenance failure; no retry. Norm: pinned OpenCLDocs 0027b7f1a5e97b1bf5256888757479e012a5cb59, runtime layer11005–11094 and12405–12440. Complete preparation evidence and handover live under results/preparation/H2_F01_20261101.
