# Build Rhythmer Engines — desktop sources

The archive includes the native instrument, source-engine inputs and data, custom
JUCE frontend, common Windows shims and original licence notices. Apple and Microsoft
SDKs / compilers are not redistributed.

## Mac
Apple Silicon Mac, macOS 13+, Xcode Command Line Tools, Python 3 and CMake 3.22+.

```sh
python3 BUILD.py --build-root /path/to/build/mac --cmake cmake --jobs 2
```

Keep directory names with spaces. All output is under --build-root; source stays
read-only. The result is formats/AU, formats/VST2 and formats/VST3, ad-hoc signed.
--native-only builds only the native backend / components; --stage-only restages
an existing build. No Developer ID or notarization is required for a local build.

## Windows x64 cross-build on Mac
The shipped Windows beta uses Zig 0.15.2 for native components, Apple Clang 17 in
MSVC mode for JUCE, LLVM resource/link/archive tools and JUCE 9.0.5. The builder must
supply and accept the licences for Microsoft Windows SDK and MSVC headers/libraries.
The release build used SDK 10.0.26100.0 (NuGet 10.0.26100.9457) and MSVC 14.44.17.14.

First build Mac above to obtain its native juceaide host tool. Supply an LLVM bin
folder containing lld-link, llvm-ar, llvm-rc and llvm-mt. The SDK root must contain
VC/Tools/MSVC/<version> and Windows Kits/10/{Include,Lib}/<version>.

```sh
python3 BUILD-WINDOWS.py --build-root /path/to/build/windows \
  --zig /path/to/zig --clang /path/to/clang --llvm-bin /path/to/llvm/bin \
  --sdk-root /path/to/windows-sdk --cmake cmake --jobs 2 \
  --host-juceaide /path/to/build/mac/desktop/juce/tools/extras/Build/juceaide/juceaide_artefacts/Custom/juceaide
```

The result is desktop/RhythmerDesktop_artefacts/Release/VST3/Rhythmer Engines.vst3.
The whole folder is the plug-in bundle. --native-only builds component DLLs;
--stage-only stages an existing build. This is a cross-build recipe; a Windows DAW
run is a separate acceptance step. The SDK and toolchains must be installed by the
builder, not copied from the binary ZIP.

native-build-recipes.json records the pinned source compile/link inputs.
SOURCE-MANIFEST.json records delivered file hashes. Corresponding original notices
are in resources-template/Licenses and upstream. Windows platform glue uses actual
Win32 loading/thread primitives; Frodhes uses its upstream Fibers and SRW-lock shim.
Tenebrae uses its C-minor fallback without a Move host-information reader.
