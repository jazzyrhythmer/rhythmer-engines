# FM-1 Rhythmer Break performance source

The unmodified `jungle.c`, `jungle.h` and `six_memory.h` are pinned from the Rhythmer FM-1 baseline identified in PROVENANCE.json. The engine identifies itself as the sound engine of **Amenity**, a port of **Presethead's Jungle Choppa**. Retain those origins together with the FM1 Open/M-VAVE copyright and the Rhythmer adaptation credit to **jazzy rhythmer**.

The source carries GPL-3.0-only with the FM1 SDK linking exception. LICENSE.md preserves the complete upstream grant; UPSTREAM_THIRD_PARTY.md preserves the accompanying component notices. No FM1 proprietary SDK object, recovery firmware, SEA codec implementation, Amen/Think recording or fallback audio is imported here. Those separately licensed components are not required by this MPC port.

`tools/apply_break_acid.py` creates an adapted runtime copy: file-local state becomes independent per-track state; the reader consumes the host's decoded PCM; loop length and device rate follow the host. Original roll, grain, filter, variation and pitch laws remain in the pinned source. Signed output shifts are expressed as multiplication to avoid undefined signed shifts in C++. The original files remain unchanged for comparison. Native sample browsing, event recording, project persistence and the MPC screen are local integration work.
