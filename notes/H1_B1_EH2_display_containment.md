# H1 B1 Evidence Hardening EH2 — Display / Compositor Containment

Date: 2026-08-26

## Event examined

Dynamic-local +1-byte test:

- requested local memory: 65537 B
- advertised limit: 65536 B
- event time: 2026-08-26 12:49:03 +02:00

Kernel evidence:

- compute ring comp_1.1.0 timeout;
- AMDGPU device coredump created;
- originating process identified;
- compute-ring reset started;
- compute-ring reset succeeded;
- device recovered through reset;
- originating OpenCL process terminated with SIGABRT.

## Display / desktop logs

The system and user journals around the event were searched for:

- kwin
- plasmashell
- Wayland
- Xwayland
- DRM/display errors
- EGL/Vulkan errors
- GPU faults
- compositor crashes
- session failures

No compositor, Wayland, Xwayland, display, EGL/Vulkan, or desktop-session
failure was observed in the examined time window.

KDE PowerDevil received udev events at the reset time and reported
permission messages for /dev/i2c-* devices. These messages do not establish
a display or compositor failure.

## Classification

Confirmed:
- F2 originating process failure
- F5 compute-ring / engine reset

Not observed:
- F6 unrelated GPU/display failure

F6 is not established and is not claimed to be excluded.
