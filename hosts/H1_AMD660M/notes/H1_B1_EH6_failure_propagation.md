# H1 B1 EH6 — Failure Propagation after Compute-Ring Recovery

Date: 2026-08-28
Status: COMPLETE

## Motivation

The instrumented 65537-byte B1 run showed:

- clEnqueueNDRangeKernel = CL_SUCCESS
- clFinish = CL_SUCCESS
- subsequent context-lost message
- process exit 134 / SIGABRT

EH6 investigated this sequence using existing evidence only.
No additional GPU fault-inducing execution was required.

## Harness ordering

The harness performs:

1. clEnqueueNDRangeKernel
2. clFinish
3. blocking clEnqueueReadBuffer(CL_TRUE)

stdout is unbuffered.

Therefore the observed:

clFinish=0
<lost-context message>
SIGABRT

means that clFinish had returned to the application before process
termination.

## Rusticl clFinish semantics

clFinish maps to:

Queue::flush(true)

Queue::flush(true) waits for the last event.

The Rusticl source explicitly states that the event's error state is
ignored by this wait path.

The function subsequently returns Ok(()).

Therefore CL_SUCCESS from clFinish is consistent with the Rusticl
implementation and is not treated here as a separate B1 defect.

## Blocking read

The next application operation is:

clEnqueueReadBuffer(..., CL_TRUE, ...)

The userspace coredump shows the main thread inside:

main
-> clEnqueueReadBuffer
-> create_and_queue
-> Queue::flush
-> Event::wait

A Rusticl worker concurrently executes:

Buffer::read
-> PipeContext::buffer_map
-> si_buffer_transfer_map
-> amdgpu_bo_map
-> si_flush_gfx_cs
-> amdgpu_cs_flush

Thus subsequent driver activity occurs after clFinish returned.

## AMDGPU lost-context handling

AMDGPU command submission reports the hard-recovery case as -ETIME.

The winsys maps this to:

PIPE_GUILTY_CONTEXT_RESET

through:

amdgpu_ctx_set_sw_reset_status()

The userspace coredump identifies the aborting thread as:

__GI_abort
-> amdgpu_ctx_set_sw_reset_status
-> amdgpu_cs_submit_ib
-> util_queue_thread_func

Source inspection shows that amdgpu_ctx_set_sw_reset_status():

1. records the reset status;
2. prints the context-lost diagnostic;
3. if PIPE_CONTEXT_LOSE_CONTEXT_ON_RESET is not set,
   deliberately invokes abort().

The Mesa source describes this as the handling for a non-robust context.

## Interpretation

The SIGABRT is therefore not an unexplained Rusticl panic.

It is the deliberate AMDGPU winsys response when subsequent command
submission encounters the lost non-robust GPU context.

The causal sequence is:

oversized dynamic LDS request
-> over-limit compute resource state
-> GPU compute timeout
-> compute-ring recovery
-> clFinish waits but does not propagate event execution error
-> subsequent blocking read performs further driver activity
-> lost guilty context detected
-> standard non-robust AMDGPU reset handling
-> abort()
-> SIGABRT

## Classification

Primary B1 finding remains:

F5:
compute-ring / GPU-engine recovery

F2:
originating OpenCL process terminates

The F2 termination mechanism is now localized to Mesa's deliberate
non-robust lost-context handling.

The clFinish behavior is recorded as a B13/error-propagation
observation, not claimed as a separate conformance vulnerability.

No claims are made for:

- F4 cross-client impact
- F6 unrelated GPU/display impact
- F7 cross-context corruption/leakage
- F8 memory-safety violation
