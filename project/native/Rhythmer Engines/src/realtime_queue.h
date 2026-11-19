// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <atomic>
#include <array>
#include <cstdint>
namespace re {
struct Edit {uint64_t binding=0;float value=0;unsigned sequence=0;};
// Bounded MPSC queue. Producers never wait for audio; a full queue drops the
// edit and reports overflow. No locks, allocation or reference counting.
class Edits {
 static constexpr size_t capacity=1024;
 struct Slot{std::atomic<size_t>sequence{0};Edit edit;};
 std::array<Slot,capacity>slots_{};std::atomic<size_t>write_{0};size_t read_=0;
 public:
 std::atomic<unsigned>overflows{0};
 Edits(){for(size_t i=0;i<capacity;i++)slots_[i].sequence.store(i);}
 bool push(Edit e){size_t at=write_.load(std::memory_order_relaxed);for(unsigned tries=0;tries<16;tries++){
  auto&s=slots_[at%capacity];size_t seq=s.sequence.load(std::memory_order_acquire);intptr_t d=intptr_t(seq)-intptr_t(at);
  if(d<0){overflows++;return false;}if(d>0){at=write_.load(std::memory_order_relaxed);continue;}
  if(write_.compare_exchange_weak(at,at+1,std::memory_order_relaxed)){s.edit=e;s.sequence.store(at+1,std::memory_order_release);return true;}
 }overflows++;return false;}
 bool pop(Edit&e){auto&s=slots_[read_%capacity];if(s.sequence.load(std::memory_order_acquire)!=read_+1)return false;e=s.edit;s.sequence.store(read_+capacity,std::memory_order_release);read_++;return true;}
};
}
