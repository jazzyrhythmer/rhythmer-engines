# Build Rhythmer Engines for Mac

Apple Silicon Mac, macOS 13 or later, Xcode Command Line Tools, Python 3 and
CMake 3.22 or later. The system Apple SDK/developer tools are not bundled.
Keep the original directories and filenames with spaces when extracting.

```sh
python3 BUILD.py --build-root /path/to/rhythmer-build --cmake cmake --jobs 2
```

All generated files go to `--build-root`; the source tree stays read-only.
`formats/AU` and `formats/VST3` contain complete, ad-hoc-signed bundles.
`--native-only` builds the native backend/source-engine modules without JUCE.
`--stage-only` restages an already completed build, including nested DSP modules.

`native-build-recipes.json` records 191 compile units and 45 link outputs using
portable `${SOURCE}` paths. It retains the original Frodhes build receipts and
its runtime module alias. `project/desktop/CMakeLists.txt` builds the native UI.
`SOURCE-MANIFEST.json` lists SHA-256 hashes for the delivered source and data.
The source snapshot was rebuilt and its AU/VST3 output was checked at three
sample rates. Original component licences and author notices remain in place.
