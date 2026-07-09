#include "DelayVfd.hpp"

#include <H5FDdevelop.h>
#include <H5Fpublic.h>
#include <H5Ppublic.h>

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <thread>

using namespace nx::core;

namespace
{
// Per-operation sleep, in microseconds, applied on every open and read. Atomic
// so the benchmark can arm and disarm the driver without touching HDF5 state.
std::atomic<uint64> s_DelayMicroseconds{0};

// The registered driver id, cached so Register() is idempotent across calls.
hid_t s_DriverId = H5I_INVALID_HID;

// Sleeps for the currently configured per-operation delay. A zero delay is a
// no-op, so the registered driver behaves as a plain pass-through when disarmed.
void Sleep()
{
  const uint64 usec = s_DelayMicroseconds.load();
  if(usec > 0)
  {
    std::this_thread::sleep_for(std::chrono::microseconds(usec));
  }
}

// The driver's per-file state. Its first member is the public H5FD_t part that
// HDF5 fills in and hands back to every callback; keeping it first lets the
// callbacks reinterpret_cast the H5FD_t* they receive back to a DelayFile*.
struct DelayFile
{
  H5FD_t pub{};
  std::FILE* fp = nullptr;
  haddr_t eoa = 0;
  haddr_t eof = 0;
};

int64 FileSize(std::FILE* fp)
{
#ifdef _WIN32
  _fseeki64(fp, 0, SEEK_END);
  return _ftelli64(fp);
#else
  fseeko(fp, 0, SEEK_END);
  return ftello(fp);
#endif
}

void SeekTo(std::FILE* fp, haddr_t addr)
{
#ifdef _WIN32
  _fseeki64(fp, static_cast<int64_t>(addr), SEEK_SET);
#else
  fseeko(fp, static_cast<off_t>(addr), SEEK_SET);
#endif
}

H5FD_t* DelayOpen(const char* name, unsigned flags, hid_t /*fapl*/, haddr_t /*maxaddr*/)
{
  if((flags & H5F_ACC_RDWR) != 0)
  {
    return nullptr; // read-only driver: benchmarks only ever read
  }
  Sleep();
  std::FILE* fp = std::fopen(name, "rb");
  if(fp == nullptr)
  {
    return nullptr;
  }
  auto* file = new DelayFile();
  file->fp = fp;
  file->eof = static_cast<haddr_t>(FileSize(fp));
  return reinterpret_cast<H5FD_t*>(file);
}

herr_t DelayClose(H5FD_t* fdt)
{
  auto* file = reinterpret_cast<DelayFile*>(fdt);
  std::fclose(file->fp);
  delete file;
  return 0;
}

haddr_t DelayGetEoa(const H5FD_t* fdt, H5FD_mem_t /*type*/)
{
  return reinterpret_cast<const DelayFile*>(fdt)->eoa;
}

herr_t DelaySetEoa(H5FD_t* fdt, H5FD_mem_t /*type*/, haddr_t addr)
{
  reinterpret_cast<DelayFile*>(fdt)->eoa = addr;
  return 0;
}

haddr_t DelayGetEof(const H5FD_t* fdt, H5FD_mem_t /*type*/)
{
  return reinterpret_cast<const DelayFile*>(fdt)->eof;
}

herr_t DelayRead(H5FD_t* fdt, H5FD_mem_t /*type*/, hid_t /*dxpl*/, haddr_t addr, size_t size, void* buf)
{
  Sleep();
  auto* file = reinterpret_cast<DelayFile*>(fdt);
  SeekTo(file->fp, addr);
  const size_t got = std::fread(buf, 1, size, file->fp);
  if(got < size)
  {
    std::memset(static_cast<char*>(buf) + got, 0, size - got);
  }
  return 0;
}

herr_t DelayWrite(H5FD_t* /*fdt*/, H5FD_mem_t /*type*/, hid_t /*dxpl*/, haddr_t /*addr*/, size_t /*size*/, const void* /*buf*/)
{
  return -1; // read-only
}

// Builds the driver class descriptor. The struct is zero-initialized so every
// optional callback stays null; only the members H5FDregister requires (open,
// close, get_eoa, set_eoa, get_eof, read, write) plus identity fields are set.
// This driver's unique H5FD class value, identifying it among registered drivers.
constexpr int k_DriverClassValue = 4242;

H5FD_class_t MakeDriverClass()
{
  H5FD_class_t cls{};
  cls.version = H5FD_CLASS_VERSION;
  cls.value = static_cast<H5FD_class_value_t>(k_DriverClassValue);
  cls.name = "simplnx_delay";
  cls.maxaddr = HADDR_MAX - 1;
  cls.fc_degree = H5F_CLOSE_WEAK;
  cls.fapl_size = 0;
  cls.open = DelayOpen;
  cls.close = DelayClose;
  cls.get_eoa = DelayGetEoa;
  cls.set_eoa = DelaySetEoa;
  cls.get_eof = DelayGetEof;
  cls.read = DelayRead;
  cls.write = DelayWrite;
  return cls;
}
} // namespace

namespace DelayVfd
{
hid_t Register()
{
  if(s_DriverId == H5I_INVALID_HID)
  {
    static H5FD_class_t cls = MakeDriverClass();
    s_DriverId = H5FDregister(&cls);
  }
  return s_DriverId;
}

void SetDelayMicroseconds(uint64 microseconds)
{
  s_DelayMicroseconds.store(microseconds);
}
} // namespace DelayVfd
