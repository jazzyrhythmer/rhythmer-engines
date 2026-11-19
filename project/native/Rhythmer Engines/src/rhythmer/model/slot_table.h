// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Kerem Kilic (Ellic Studio)
#pragma once
#include "rhythmer/model/sampler.h"
#include <array>
#include <cstddef>
#include <iterator>
#include <memory>

// One value for each of a project's sample slots (its file, its chop). The table has sample_slots places and keeps
// only the ones written, sixty-four to a block: a project of a few samples holds and copies a few, and one of a
// thousand holds a thousand. A slot never written reads as an empty value.
//
// operator[] reads and never makes a block; edit() is the slot to write. A reference from either stays good while
// the table lives, whatever else is written (a table copied over is another matter).
namespace rhythmer {

template <typename T>
class SlotTable {
public:
    SlotTable() = default;
    SlotTable(const SlotTable& other) { *this = other; }
    SlotTable(SlotTable&&) noexcept = default;
    SlotTable& operator=(SlotTable&&) noexcept = default;
    SlotTable& operator=(const SlotTable& other) {
        if (this == &other) return *this;
        for (size_t b = 0; b < blocks.size(); ++b) {
            if (!other.blocks[b]) blocks[b].reset();
            else if (blocks[b]) *blocks[b] = *other.blocks[b]; // a copy made again (the screen's, each frame) allocates nothing
            else blocks[b] = std::make_unique<Block>(*other.blocks[b]);
        }
        return *this;
    }
    static constexpr size_t size() { return sample_slots; }
    const T& operator[](size_t n) const {
        if (n >= sample_slots) return none();
        const auto& block = blocks[n / per];
        return block ? (*block)[n % per] : none();
    }
    T& edit(size_t n) {
        n %= sample_slots;
        auto& block = blocks[n / per];
        if (!block) block = std::make_unique<Block>();
        return (*block)[n % per];
    }
    // Every slot in turn, the empty ones too: for (const auto& value : table).
    class Walk {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = const T*;
        using reference = const T&;
        Walk() = default;
        Walk(const SlotTable& table, size_t at) : table(&table), at(at) {}
        const T& operator*() const { return (*table)[at]; }
        const T* operator->() const { return &(*table)[at]; }
        Walk& operator++() { ++at; return *this; }
        Walk operator++(int) { Walk was = *this; ++at; return was; }
        bool operator==(const Walk& other) const { return at == other.at; }
        bool operator!=(const Walk& other) const { return at != other.at; }
    private:
        const SlotTable* table = nullptr;
        size_t at = 0;
    };
    Walk begin() const { return {*this, 0}; }
    Walk end() const { return {*this, sample_slots}; }
    // The slots to look through: none at or past it was ever written. For what runs every tick or frame.
    size_t span() const {
        size_t b = blocks.size();
        while (b && !blocks[b - 1]) --b;
        return b * per;
    }
    // The blocks it keeps (the tests: what a copy costs).
    size_t blocks_held() const { size_t n = 0; for (const auto& b : blocks) n += b != nullptr; return n; }
    static constexpr size_t block_slots = 64;

private:
    static constexpr size_t per = block_slots;
    using Block = std::array<T, per>;
    static const T& none() { static const T empty {}; return empty; }
    std::array<std::unique_ptr<Block>, (sample_slots + per - 1) / per> blocks;
};

} // namespace rhythmer
