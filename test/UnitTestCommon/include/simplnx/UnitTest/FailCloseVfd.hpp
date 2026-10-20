#pragma once

#include "simplnx/Utilities/Parsing/HDF5/H5Support.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/FileIO.hpp"

#include <hdf5.h>

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>

/**
 * @file FailCloseVfd.hpp
 * @brief Test-only HDF5 virtual file driver whose close callback reports a failure while armed.
 *
 * Writers must report a file whose final close failed (for example, a last flush that hits a full disk). Real disks cannot be made to
 * fail on demand, so tests install this driver through HDF5::FileIO::SetFaplConfigurator(). It reads and writes through stdio like the
 * default driver; while armed, its close callback still releases the file but returns a failure and counts it.
 */
namespace nx::core::UnitTest::FailCloseVfd
{
/**
 * @struct FailCloseFile
 * @brief Per-file driver state. HDF5 requires H5FD_t as the first member so it can treat the struct as its public file record.
 */
struct FailCloseFile
{
  H5FD_t pub{};
  std::FILE* fp = nullptr;
  haddr_t eoa = 0;
  haddr_t eof = 0;
};

/**
 * @brief True while the close callback must report a failure. Only FailCloseArmGuard sets it.
 */
inline std::atomic<bool> s_Armed{false};

/**
 * @brief Number of close callbacks that reported a failure in this process. Tests compare it before and after a write.
 */
inline std::atomic<int32_t> s_CloseFailureCount{0};

/**
 * @brief Moves the stdio position to an HDF5 address with a 64-bit offset on every platform.
 * @param fp Open stdio stream.
 * @param addr Byte offset from the start of the file.
 */
inline void Seek(std::FILE* fp, haddr_t addr)
{
#ifdef _WIN32
  _fseeki64(fp, static_cast<int64_t>(addr), SEEK_SET);
#else
  fseeko(fp, static_cast<off_t>(addr), SEEK_SET);
#endif
}

/**
 * @brief Driver open callback: opens the file with the stdio mode that matches the HDF5 access flags.
 * @param name File path.
 * @param flags HDF5 access flags (H5F_ACC_TRUNC creates or truncates, H5F_ACC_RDWR opens for update, otherwise read-only).
 * @return Driver file record, or null when the file cannot be opened.
 */
inline H5FD_t* Open(const char* name, unsigned flags, hid_t /*fapl*/, haddr_t /*maxaddr*/)
{
  const char* mode = (flags & H5F_ACC_TRUNC) != 0 ? "w+b" : ((flags & H5F_ACC_RDWR) != 0 ? "r+b" : "rb");
  std::FILE* fp = std::fopen(name, mode);
  if(fp == nullptr)
  {
    return nullptr;
  }
  auto* file = new FailCloseFile();
  file->fp = fp;
#ifdef _WIN32
  _fseeki64(fp, 0, SEEK_END);
  file->eof = static_cast<haddr_t>(_ftelli64(fp));
#else
  fseeko(fp, 0, SEEK_END);
  file->eof = static_cast<haddr_t>(ftello(fp));
#endif
  return reinterpret_cast<H5FD_t*>(file);
}

/**
 * @brief Driver close callback: always releases the stream and the record; while armed, reports a failure and counts it.
 * @param fdt Driver file record.
 * @return 0, or -1 while armed.
 */
inline herr_t Close(H5FD_t* fdt)
{
  auto* file = reinterpret_cast<FailCloseFile*>(fdt);
  std::fclose(file->fp);
  delete file;
  if(s_Armed.load())
  {
    s_CloseFailureCount.fetch_add(1);
    return -1;
  }
  return 0;
}

/**
 * @brief Driver callback: returns the end of the address space HDF5 has allocated.
 */
inline haddr_t GetEoa(const H5FD_t* fdt, H5FD_mem_t /*type*/)
{
  return reinterpret_cast<const FailCloseFile*>(fdt)->eoa;
}

/**
 * @brief Driver callback: records the end of the address space HDF5 has allocated.
 */
inline herr_t SetEoa(H5FD_t* fdt, H5FD_mem_t /*type*/, haddr_t addr)
{
  reinterpret_cast<FailCloseFile*>(fdt)->eoa = addr;
  return 0;
}

/**
 * @brief Driver callback: returns the physical end of the file.
 */
inline haddr_t GetEof(const H5FD_t* fdt, H5FD_mem_t /*type*/)
{
  return reinterpret_cast<const FailCloseFile*>(fdt)->eof;
}

/**
 * @brief Driver read callback: reads bytes at an address and zero-fills past the physical end, as HDF5 drivers must.
 */
inline herr_t Read(H5FD_t* fdt, H5FD_mem_t /*type*/, hid_t /*dxpl*/, haddr_t addr, size_t size, void* buf)
{
  auto* file = reinterpret_cast<FailCloseFile*>(fdt);
  Seek(file->fp, addr);
  const size_t got = std::fread(buf, 1, size, file->fp);
  if(got < size)
  {
    std::memset(static_cast<char*>(buf) + got, 0, size - got);
  }
  return 0;
}

/**
 * @brief Driver write callback: writes bytes at an address and extends the physical end.
 * @return 0, or -1 after a short write.
 */
inline herr_t Write(H5FD_t* fdt, H5FD_mem_t /*type*/, hid_t /*dxpl*/, haddr_t addr, size_t size, const void* buf)
{
  auto* file = reinterpret_cast<FailCloseFile*>(fdt);
  Seek(file->fp, addr);
  if(std::fwrite(buf, 1, size, file->fp) != size)
  {
    return -1;
  }
  file->eof = std::max(file->eof, static_cast<haddr_t>(addr + size));
  return 0;
}

/**
 * @brief Registers the driver with HDF5 once per process.
 * @return The driver identifier, or a negative value when registration failed.
 *
 * The identifier stays registered for the life of the process, so every test in an executable shares it.
 */
inline hid_t RegisterFailCloseVfd()
{
  static const hid_t driverId = []() {
    static H5FD_class_t driverClass{};
    driverClass.version = H5FD_CLASS_VERSION;
    driverClass.value = static_cast<H5FD_class_value_t>(4243);
    driverClass.name = "simplnx_fail_close";
    driverClass.maxaddr = HADDR_MAX - 1;
    driverClass.fc_degree = H5F_CLOSE_WEAK;
    driverClass.open = Open;
    driverClass.close = Close;
    driverClass.get_eoa = GetEoa;
    driverClass.set_eoa = SetEoa;
    driverClass.get_eof = GetEof;
    driverClass.read = Read;
    driverClass.write = Write;
    std::lock_guard<std::mutex> hdf5Lock(HDF5::Support::ApiLock());
    return H5FDregister(&driverClass);
  }();
  return driverId;
}

/**
 * @brief Returns how many close callbacks have reported a failure in this process.
 */
inline int32_t CloseFailureCount()
{
  return s_CloseFailureCount.load();
}

/**
 * @class FailCloseArmGuard
 * @brief Routes every file that FileIO creates or opens through the driver and arms its close failure for the guard's lifetime.
 *
 * Construct it only around the call under test: files opened while it lives use the driver, and every driver close fails until it is
 * destroyed. The destructor disarms the driver and removes the configurator, even when the call under test throws.
 */
class FailCloseArmGuard
{
public:
  /**
   * @brief Installs the driver through FileIO::SetFaplConfigurator() and arms the close failure.
   * @param driverId Identifier from RegisterFailCloseVfd(); the caller checks that it is valid.
   */
  explicit FailCloseArmGuard(hid_t driverId)
  {
    HDF5::FileIO::SetFaplConfigurator([driverId](hid_t faplId) { H5Pset_driver(faplId, driverId, nullptr); });
    s_Armed.store(true);
  }

  /**
   * @brief Disarms the close failure and restores the default file-access property list.
   */
  ~FailCloseArmGuard() noexcept
  {
    s_Armed.store(false);
    HDF5::FileIO::SetFaplConfigurator({});
  }

  FailCloseArmGuard(const FailCloseArmGuard&) = delete;
  FailCloseArmGuard(FailCloseArmGuard&&) = delete;
  FailCloseArmGuard& operator=(const FailCloseArmGuard&) = delete;
  FailCloseArmGuard& operator=(FailCloseArmGuard&&) = delete;
};
} // namespace nx::core::UnitTest::FailCloseVfd
