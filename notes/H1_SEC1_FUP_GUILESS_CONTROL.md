# H1 SEC-1 FUP2 — GUI-less Control

**Status:** COMPLETED — CONTROL FINDING

## Question

Does the established B3 failure and its cross-client impact require an active
KDE/Wayland graphical desktop?

## Condition

System booted to:

    multi-user.target

Verified for each test condition:

- display manager inactive
- no SDDM
- no KWin
- no Plasma shell
- XDG_SESSION_TYPE=tty
- Rusticl OpenCL device remained available

This condition is described as GUI-less / no graphical desktop.

It is not claimed to be a fully headless compute-only GPU because DRM/KMS and
display hardware may remain active.

## Method

Victim:

    sec1_victim_heartbeat_v2

Trigger:

    b3_multi_local_probe 32767 32769

Five independent boots were tested with exactly one B3 trigger per boot.

## Results

| Trial | Recovery | Victim | Max heartbeat |
|------:|----------|--------|--------------:|
| 1 | RING_RESET_ONLY | 73/73 correct, exit 0 | 3.45 ms |
| 2 | RING_RESET_ONLY | 73/73 correct, exit 0 | 7.38 ms |
| 3 | RING_RESET_ONLY | 73/73 correct, exit 0 | 7.98 ms |
| 4 | RING_RESET_ONLY | 73/73 correct, exit 0 | 8.06 ms |
| 5 | RING_RESET_ONLY | 73/73 correct, exit 0 | 7.26 ms |

Aggregate:

- 5/5 B3 compute-ring timeouts
- 5/5 successful ring resets
- 0/5 MODE2 full-GPU resets
- 365/365 victim heartbeats correct
- 0 WRONG_RESULT
- 0 victim aborts
- 0 >=100 ms latency excursions

## Comparison with GUI V2 campaign

GUI / KDE / Wayland cold-boot campaign:

- N=10
- 2 full-GPU resets
- 3 >=100 ms victim latency excursions
- 1 V2 WRONG_RESULT
- 1 victim abort

GUI-less campaign:

- N=5
- 0 full-GPU resets
- 0 >=100 ms excursions
- 0 WRONG_RESULT
- 0 victim aborts

## Supported conclusion

The B3 compute failure itself does not require KDE/Wayland or graphical
desktop activity.

Under the GUI-less control condition, all five independent B3 failures were
contained by successful compute-ring recovery and no substantial victim impact
was observed.

## Interpretation boundary

The difference between GUI and GUI-less campaigns suggests that graphical /
display workload or broader GPU system state may influence recovery escalation
or cross-client impact.

N=5 is insufficient to establish a causal GUI effect.

No claim is made that KDE, KWin, Wayland, SDDM, or display activity causes
MODE2 escalation.

## Decision

Control complete.

No additional identical GUI-less H1 boots are currently justified.
