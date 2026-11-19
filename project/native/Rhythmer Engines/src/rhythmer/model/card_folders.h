// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Kerem Kilic (Ellic Studio)
#pragma once
#include <filesystem>

namespace rhythmer {

// Rhythmer's own folders on the card that hold nothing to open on the MPC: Rhythmer/Cache (samples decoded at the
// device's rate) and Rhythmer/Crashes (crash reports). The MPC's browsers (plug-ins, samples, projects) leave them out;
// the web page's file list still shows them, so crash reports can be downloaded.
inline bool rhythmer_own_folder(const std::filesystem::path& dir) {
    const auto name = dir.filename().string();
    return dir.parent_path().filename() == "Rhythmer" && (name == "Cache" || name == "Crashes");
}

} // namespace rhythmer
