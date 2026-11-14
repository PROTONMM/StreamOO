# Development Entry Point

[English index](00_INDEX.md)

For the current implementation, start with [STATUS.md](STATUS.md), then the
[build guide](BUILD_GUIDE.md), [architecture](03_PLATFORM.md) and
[known limitations](13_DECISIONS_OPEN.md). Use the [test map](14_TRACEABILITY.md)
to locate existing checks before changing a component.

The English documentation covers current operation and technical behavior.
It does not translate the full development history. Exact earlier measurements,
hardware reports and decisions remain in the [Polish handover](../RESUME.md),
[status log](../STATUS.md) and [ADR directory](../adr/).

Preserve local user configuration, secrets and the private playlist. Do not erase
NVS/LittleFS or flash a device as part of a documentation check. Keep pending local
changes until the user asks to commit/publish them, and verify repository state
rather than assuming an old handover's branch or test result is still current.
