# FUP-B4-2A / T12.2 — completion-entry oracle

This variant preserves the T1 workload and controls while adding a
synchronized positive ordering oracle. The original T1 source remains
unchanged.

The `CL_COMPLETE` event callback stores its callback status and, only for
normal completion, publishes `normal_completion_published = 1` with release
semantics. The destructor callback loads that marker with acquire semantics as
its first relevant C instrumentation action and records the value for the host.

A C1 trial is classified as:

- `FAIL` when the existing direct oracle observes the destructor and then
  successfully reads a nonterminal event status;
- `PASS` when cleanup completed normally, both callbacks were observed, the
  completion callback status is `CL_COMPLETE`, and the destructor's first
  instrumented load acquired the published normal-completion marker;
- `INCONCLUSIVE` otherwise. A missing marker is never a failure by itself.

The release/acquire pair establishes a C11 happens-before relationship from
the normal completion callback to the destructor's first instrumented load.
It cannot observe the implementation's internal callback-dispatch boundary or
the function prologue before the first C atomic operation. Claims must therefore
refer to the first instrumented destructor action, not an unobservable internal
dispatch instant.

The binary has an offline self-test mode that does not initialize OpenCL:

```sh
./fup_b4_2a_completion_entry --oracle-self-test
```

The self-test covers direct `FAIL`, synchronized `PASS`, missing-marker
`INCONCLUSIVE`, negative-completion `INCONCLUSIVE`, and missing-callback
`INCONCLUSIVE` cases.

For the isolated H1 build, embed the source hash just as in T1:

```sh
source_sha256=$(sha256sum fup_b4_2a_completion_entry.c | awk '{print $1}')
cc -std=c11 -O2 -Wall -Wextra -Wpedantic \
  -DFUP_SOURCE_SHA256="\"$source_sha256\"" \
  fup_b4_2a_completion_entry.c -lOpenCL \
  -o fup_b4_2a_completion_entry
```

Building and running `--oracle-self-test` are CPU-only. Any OpenCL execution
requires a separate experiment gate.
