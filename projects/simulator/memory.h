#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include <signal.h>
#include <map>
#include <vector>
#include <asm/ldt.h>
#include <boost/array.hpp>
#include <stdint.h>
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>

const int PAGE_SIZE = 4096;

enum Use {USE_READ, USE_WRITE, USE_EXECUTE};

struct Page {
  bool allow_read, allow_write, allow_execute;
  uint8_t* real_base;
  bool in_use;

  Page(bool r, bool w, bool x, uint8_t* real_base)
    : allow_read(r), allow_write(w), allow_execute(x), real_base(real_base), in_use(true) {}
  Page(): allow_read(false), allow_write(false), allow_execute(false), real_base(0), in_use(false) {}

  bool checkUse(Use use) const {
    switch (use) {
      case USE_READ: return allow_read;
      case USE_WRITE: return allow_write;
      case USE_EXECUTE: return allow_execute;
    }
  }

  void clear() {
    in_use = false;
  }

  ~Page() {
    clear();
  }
};

inline std::ostream& operator<<(std::ostream& o, const Page& p) {
  o << "Page " << (p.allow_read ? 'r' : '-') << (p.allow_write ? 'w' : '-') << (p.allow_execute ? 'x' : '-') << " at " << (void*)p.real_base;
  return o;
}

struct Memory {
  std::vector<Page> pages;

  Memory(): pages((1ULL << 32) / PAGE_SIZE) {}

  void mapPage(uint32_t vaddr, uint8_t* data, bool readable, bool writable, bool executable) {
    ROSE_ASSERT (vaddr % PAGE_SIZE == 0);
    Page& p = findPage(vaddr);
    p = Page(readable, writable, executable, data);
  }

  void mapZeroPageIfNeeded(uint32_t vaddr) {
    Page p = findPage(vaddr);
    if (!p.in_use) {
      mapPage(vaddr - (vaddr % PAGE_SIZE), (uint8_t*)calloc(PAGE_SIZE, 1), true, true, false);
    }
  }

  Page& findPage(uint32_t vaddr) {
    return pages[vaddr / PAGE_SIZE];
  }

  const Page& findPage(uint32_t vaddr) const {
    return pages[vaddr / PAGE_SIZE];
  }

  template <size_t SizeInBytes>
  uint64_t read(uint32_t vaddr) const {
    Page p1 = findPage(vaddr);
    Page p2 = findPage(vaddr + SizeInBytes - 1);
    if (!p1.checkUse(USE_READ) || !p2.checkUse(USE_READ)) {
      fprintf(stderr, "Bad read access to 0x%08"PRIX32" length %u\n", vaddr, SizeInBytes);
      abort();
    }
    size_t partInFirstPage = PAGE_SIZE - (vaddr % PAGE_SIZE);
    if (partInFirstPage > SizeInBytes) partInFirstPage = SizeInBytes;
    uint64_t result = 0;
    for (size_t i = 0; i < partInFirstPage; ++i) {
      result |= ((uint64_t)p1.real_base[(vaddr + i) % PAGE_SIZE] << (8 * i));
    }
    for (size_t i = partInFirstPage; i < SizeInBytes; ++i) {
      result |= ((uint64_t)p2.real_base[(vaddr + i - PAGE_SIZE) % PAGE_SIZE] << (8 * i));
    }
    return result;
  }

  uint8_t readForExec(uint32_t vaddr) const {
    Page p1 = findPage(vaddr);
    if (!p1.checkUse(USE_EXECUTE)) {
      fprintf(stderr, "Bad execute access to 0x%08"PRIX32"\n", vaddr);
      abort();
    }
    return p1.real_base[vaddr % PAGE_SIZE];
  }

  void readMultiple(uint8_t* data, uint32_t size, uint32_t addr) const {
    for (size_t i = 0; i < size; ++i) {
      data[i] = read<1>(addr + i);
    }
  }

  void readMultipleForExec(uint8_t* data, uint32_t size, uint32_t addr) const {
    for (size_t i = 0; i < size; ++i) {
      data[i] = readForExec(addr + i);
    }
  }

  std::string readString(uint32_t addr) const {
    std::string str;
    while (true) {
      uint8_t val = read<1>(addr);
      if (val == 0) break;
      str += (char)val;
      ++addr;
    }
    return str;
  }

  std::vector<std::string> readStringVector(uint32_t addr) const {
    std::vector<std::string> vec;
    for (; read<4>(addr) != 0; addr += 4) {
      vec.push_back(readString(read<4>(addr)));
    }
    return vec;
  }

  std::vector<uint8_t> readAsFarAsPossibleForExec(uint32_t addr, uint32_t maxLen) const {
    // Read either maxLen bytes or until reading is no longer possible
    std::vector<uint8_t> v;
    while (maxLen > 0) {
      if (!findPage(addr).checkUse(USE_EXECUTE)) break;
      uint32_t thisLen = PAGE_SIZE - (addr % PAGE_SIZE);
      if (thisLen > maxLen) thisLen = maxLen;
      size_t oldVLen = v.size();
      v.resize(oldVLen + thisLen);
      readMultipleForExec(&v[oldVLen], thisLen, addr);
      addr += thisLen;
      maxLen -= thisLen;
    }
    return v;
  }

  template <size_t SizeInBytes>
  void write(uint32_t vaddr, uint64_t val) {
    Page p1 = findPage(vaddr);
    Page p2 = findPage(vaddr + SizeInBytes - 1);
    if (!p1.checkUse(USE_WRITE) || !p2.checkUse(USE_WRITE)) {
      fprintf(stderr, "Bad write access to 0x%08"PRIX32" length %u\n", vaddr, SizeInBytes);
      abort();
    }
    size_t partInFirstPage = PAGE_SIZE - (vaddr % PAGE_SIZE);
    if (partInFirstPage > SizeInBytes) partInFirstPage = SizeInBytes;
    for (size_t i = 0; i < partInFirstPage; ++i) {
      p1.real_base[(vaddr + i) % PAGE_SIZE] = (val >> (8 * i)) & 0xFF;
    }
    for (size_t i = partInFirstPage; i < SizeInBytes; ++i) {
      p2.real_base[(vaddr + i - PAGE_SIZE) % PAGE_SIZE] = (val >> (8 * i)) & 0xFF;
    }
  }

  void writeMultiple(const uint8_t* data, uint32_t size, uint32_t addr) {
    for (size_t i = 0; i < size; ++i) {
      write<1>(addr + i, data[i]);
    }
  }

  void writeString(const char* data, uint32_t addr) {
    while (true) {
      uint8_t val = *data;
      write<1>(addr, val);
      ++addr;
      ++data;
      if (val == 0) break;
    }
  }

};

#if 0

extern volatile struct SignalQueue {
  static const int MAX_PENDING_SIGNALS = 32;
  int pendingSignals[MAX_PENDING_SIGNALS];
  int pendingSignalCount;
  int startEntry;

  SignalQueue(): pendingSignalCount(0), startEntry(0) {}
  void addSignal(int sig) volatile {
    assert (pendingSignalCount != MAX_PENDING_SIGNALS);
    pendingSignals[(startEntry + pendingSignalCount++) % MAX_PENDING_SIGNALS] = sig;
  }
  bool anySignalsWaiting() const volatile {
    return pendingSignalCount != 0;
  }
  int popSignal() volatile {
    assert (pendingSignalCount != 0);
    int sig = pendingSignals[startEntry++];
    --pendingSignalCount;
    if (startEntry == MAX_PENDING_SIGNALS) startEntry = 0;
    return sig;
  }
} signalQueue;

inline bool simulate_signals_pending() {
  return signalQueue.anySignalsWaiting();
}

struct MachineState {
  uint32_t gpr[8];
  uint32_t ip;
  bool flags[32];
  uint16_t cs, ds, es, ss, fs, gs; // Should not actually be used
  uint32_t gsOffset;
  void (*eipShadow)();
  uint32_t signalHandlers[_NSIG + 1];
  uint32_t signalStack;
  uint32_t brk;

  std::vector<user_desc> thread_areas;

  Memory memory;

  MachineState(): ip(0), gs(0), gsOffset(0), eipShadow(0), signalStack(0) {
    for (size_t i = 0; i < 32; ++i) flags[i] = false;
    flags[1] = true;
    gpr[0] = 0;
    gpr[1] = 0;
    gpr[2] = 0;
    gpr[3] = 0;
    gpr[4] = 0xc0000000UL;
    gpr[5] = 0;
    gpr[6] = 0;
    gpr[7] = 0;
  }
};

void dumpRegs(const MachineState& ms);

void simulate_load_gs(uint32_t new_gs);

void simulate_int_0x80();

void simulate_setup(int argc, char** argv);

void simulate_signal_check(uint32_t returnAddr);
void simulate_sigreturn();
void dispatch(uint32_t addr);

void dumpRegs(const MachineState& st);

#endif

#endif // MEMORY_H