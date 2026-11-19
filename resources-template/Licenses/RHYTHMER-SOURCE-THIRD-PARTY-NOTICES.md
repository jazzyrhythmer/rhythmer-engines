# Rhythmer OS development build: sources and attribution

Rhythmer OS is based on **Melodee 0.3.0 by Kerem Kiliç (Ellic Studio)** and sample-base, AGPL-3.0-only. Copyright and SPDX headers remain intact. The two reserved upstream boot images are replaced with original geometric CC0 artwork. The product code is mechanically renamed to Rhythmer while serialized identity and original instrument IDs remain. The upstream sequencer, sampler voice policy, mixer, effects, drivers, recovery and project format are retained.

| Component | Origin / licence | Local treatment |
|---|---|---|
| Melodee / sample-base | Kerem Kilic (Ellic Studio), AGPL-3.0-only | Pinned submodules; own boot/About, monochrome pixel skin, drum-map fallback and native GEN/Break UI extensions; mechanical product naming |
| JUCE and bundled dependencies | JUCE 9.0.5, AGPL route; see bundled JUCE-LICENSE.md and SPDX inventory | Upstream pinned build; no framework rewrite |
| Rubik | Rubik Project Authors, OFL-1.1 | Upstream fetched font/notice |
| MPC Sample device wordmark | inMusic Brands, Inc.; official User Guide cover vector geometry | White-on-black hardware identifier; rights and source in assets/device/NOTICE.md. Separate from original CC0 Rhythmer artwork and software licensing |
| VST3 SDK / plugin support | See runtime licences and Debian runtime/codec copyright files | Upstream build and packaging retained |
| Open MPC wrapper / Schwung adapter | open-mpc/mpc-vst-plugins contributors, MIT | Pinned wrapper core with native-selector/chunk host extensions; DSP calls are serialised by its existing mutex |
| Sophie | Matt Estela and digisophie contributors, MIT | Original Move DSP unchanged; original 16 pads and 12 voices, runtime parameter metadata |
| TR-MPC and standalone 6W6/8W8/CW-78/9W9 | sd88me; original Schwung 6W6/8W8/CW-78/9W9 by athousanddetails, GPL-3.0 | Existing aarch64 portable release unchanged; optional slot note-name sidecar added |
| TR-MPC 909 cymbal data | Upstream identifies ER-99, GPL-3.0 | Already in original release; no break-loop library added |
| Amenity / Jungle Break DSP | Presethead Jungle Choppa; Amenity/FM1 Open contributors, M-VAVE; Rhythmer adaptation by jazzy rhythmer. GPL-3.0-only with the FM1 SDK linking exception | Pinned original FM-1 source and notices in third_party/fm1-break. Per-track state, host PCM reader, device-rate bridge and native event/UI integration are local changes. No FM1 SDK binaries or original break recordings included |
| JIANT GEN / Rhythmer expansion | Juan Ignacio Antonucci (Juan Jiant), GPL-3.0-only; jazzy rhythmer genre/break additions | Original musical tables/progressions and Break GEN/sequence rules adapted to native patterns. Source snapshots, hashes and GPL text in third_party/jiant-gen; no reduced JAZZY DSP or break audio |

Exact runtime/source commits are in SOURCE_LOCK.json; prebuilt asset URLs, resolved source commits, sizes and SHA-256 are in RELEASES_LOCK.json. Local port changes are recorded in ports/*/VENDORED.md. Additional GPL/AGPL/MIT texts are included with the SD files.

This is an unofficial beta for owner device testing. Individual component and asset terms remain separate; the source inventory does not claim blanket redistribution clearance. No proprietary firmware, ROMs, OP-1 images or factory break sample library are included. Device compatibility, physical controls and listening require MPC Sample validation.

Rhythmer OS is not affiliated with or endorsed by inMusic, Akai Professional, Roland, Elektron or the upstream authors. Their names identify origins and compatibility; upstream manufacturer fields and credits are retained.


## Native instruments in 0.3 beta

The selected release has 18 native FM-1/Move/DRUM-X ports plus the original Sophie adapter and five unchanged MPC drum packages. Five tagged Move source trees and populated submodules accompany the separate MOVE-SOURCE archive. The main SOURCE archive contains original selected FM-1 units, adapters, runtime and build tools. Per-port SOURCE.json and VENDORED.md preserve pinned provenance and adaptations. Elixir is the canonical LXR-family port.

Vendor SysEx dumps/converted presets without a clear redistribution basis are omitted from the relevant ports. Dexed uses the 33 bank files from its pinned original source, with original names and source notices; replacement generated banks are not supplied. CS-1 keeps manual-chart mappings and original non-converted recipes. Elixir external Erica/Hrtl factory packs are omitted. Hortator/VCSL percussion recordings by Versilian Studios / Sam Gossner are CC0, with attribution alongside the WAVs. Original reSID tables identify GPL-2.0-or-later provenance and are used under GPL-3.0. Original MPL-2.0, MIT, BSD and Apache notices remain with the corresponding sources.

The official MPC Sample vector device wordmark is a trademark of inMusic Brands, Inc. It is separate from the original CC0 Rhythmer artwork; see assets/device/NOTICE.md. Hardware identification does not imply manufacturer endorsement.

Rhythmer Kit and the single-sound host view are original AGPL-3.0-only host integration by jazzy rhythmer. They instantiate the credited original VST engines and retain their individual licences and notices. The original band-limited sample-base rate bridge provides percussion tape-pitch conversion. Named preset metadata is derived from the same pinned, licensed engine data; no additional proprietary preset banks are supplied.

The main SOURCE archive also includes the actual JUCE and Rubik dependency trees, including nested license texts and JUCE SPDX inventory. See CREDITS_AUDIT.md for the checked scope and remaining redistribution gates; attribution preservation does not certify every license combination or trademark right.

FM-1 predecessor authors and technical-reference distinctions are detailed in FM1_LINEAGE_KO.md. Per-port UPSTREAM_NOTICES preserves pinned CREDITS, LICENSING, NOTICE, licence texts and upstream README context, with original hashes and commit links.

## Direct FM-1 generator reuse

`third_party/fm1-gen` preserves the exact required Rhythmer/Felucca/JIANT host sources and generated headers with SHA-256 receipts and upstream license texts. The original `firmware/src/gen.c` and genre tables execute through an event-only native bridge; FM-1 DSP is not used for output. Acid GEN preserves `acid_generate.c`, `tb3po.c` and their schwung-tb3po / Phazerville Hemisphere Suite / djphazer lineage. Original author headers remain. `bridge.c` and `bridge.h` are the local integration.

The 0.2.0 host includes the MIT-licensed CLAP 1.2.9 headers, the LV2/lilv/serd/sord/sratom sources bundled with JUCE, and upstream's extended Debian support pack (libsndfile/codecs, libsamplerate and FFTW single/double/thread libraries). Their original COPYING/licence texts and package-version notices are included in the runtime release; actual CLAP and JUCE/LV2 sources accompany the source archive. The original Melodee web wordmark is replaced with Rhythmer text and the web pages retain explicit upstream credit.

The SUPPORT-SOURCE archive supplies exact Debian source packages and cross-build rules for the embedded runtime/codec libraries, plus the underlying glibc and GCC sources. SOURCE_PACKAGES.json records package versions, filenames, hashes and original source URLs. Rhythmer original Prophet/Elixir patches are GPL-3.0-only parameter edits by jazzy rhythmer, separate from the excluded external banks.

## Source-built 0.2.0 additions

- **Frodhes** — Leonardo Mirabal and the credited Felucca/SLOOP/Dinghy predecessors, GPL-3.0-only. Original v0.9.12 MPC VST2 synthesis and 57 factory programs; the local wrapper only rejects unavailable instances cleanly. The vendored MPC SDK headers are MIT; the bundled font and additional component notices remain in the original source and staged legal texts. See `ports/fm1-frodhes/LICENSING.md`, `SOURCE.json` and `UPSTREAM_NOTICES`.
- **Tenebrae** — legsmechanical, MIT. Original Schwung DSP, eight oscillator models and Init + 163 bank entries, built from pinned source with the existing numeric-key wrapper. Original DSP and factory records are retained. The Schwung API headers and Open MPC wrapper retain their MIT notices. See `ports/move-tenebrae/SOURCE.json` and `original/LICENSE`.

FMini-1 uses the original 2.6.0 source and bank, with all 200 original patch names and original sound-related factory extras. Its original GPL notices and complete lineage documents remain bundled.

## Auxiliary separator and screen fonts

The separately executed Stems separator and converted model are reused byte-for-byte from the pinned Melodee Sample v0.3.0 release. Original notices accompany the installation. SpleeterRT (GPL-3.0) is pinned to `d818fb9ca826d31cb7dda60409f3f14e6e789597`; OpenBLAS (BSD-3-Clause) to `5ef8b1964658f9cb6a6324a06f6a1a022609b0c5`. Deezer’s Spleeter four-stem model is MIT licensed. Their corresponding code, notices and pinned model-fetch instructions accompany the source distribution. Additional Google Fonts faces retain their SIL OFL notices. See the runtime’s `THIRD_PARTY.md`, dependency pins and the installed `licenses/stems/` folder.
