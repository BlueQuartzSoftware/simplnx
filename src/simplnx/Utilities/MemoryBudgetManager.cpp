#include "simplnx/Utilities/MemoryBudgetManager.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>

#ifdef __APPLE__
#include <sys/sysctl.h>
#include <sys/types.h>
#elif defined(__linux__)
#include <fstream>
#include <string>
#elif defined(_WIN32)
#define NOMINMAX
#include <windows.h>
#endif

namespace nx::core
{

MemoryBudgetManager::MemoryBudgetManager()
: m_BudgetBytes(defaultBudgetBytes())
{
}

MemoryBudgetManager& MemoryBudgetManager::instance()
{
  static MemoryBudgetManager s_Instance;
  return s_Instance;
}

uint64 MemoryBudgetManager::defaultBudgetBytes()
{
  static constexpr uint64 k_MinBudget = uint64{1} * 1024 * 1024 * 1024; // 1 GB

  uint64 totalRam = 0;

#ifdef __APPLE__
  int mib[2] = {CTL_HW, HW_MEMSIZE};
  uint64 memsize = 0;
  size_t len = sizeof(memsize);
  if(sysctl(mib, 2, &memsize, &len, nullptr, 0) == 0)
  {
    totalRam = memsize;
  }
#elif defined(__linux__)
  std::ifstream meminfo("/proc/meminfo");
  std::string line;
  while(std::getline(meminfo, line))
  {
    if(line.find("MemTotal:") == 0)
    {
      // Format: "MemTotal:       12345678 kB"
      uint64 kb = 0;
      // Skip "MemTotal:" prefix and parse the number
      auto pos = line.find_first_of("0123456789");
      if(pos != std::string::npos)
      {
        try
        {
          kb = std::stoull(line.substr(pos));
        } catch(const std::exception&)
        {
          return k_MinBudget;
        }
      }
      totalRam = kb * 1024;
      break;
    }
  }
#elif defined(_WIN32)
  MEMORYSTATUSEX memStatus;
  memStatus.dwLength = sizeof(memStatus);
  if(GlobalMemoryStatusEx(&memStatus))
  {
    totalRam = memStatus.ullTotalPhys;
  }
#endif

  if(totalRam == 0)
  {
    return k_MinBudget;
  }

  uint64 halfRam = totalRam / 2;
  return std::max(halfRam, k_MinBudget);
}

std::pair<MemoryBudgetManager::AllocationHandle, std::vector<MemoryBudgetManager::AllocationHandle>> MemoryBudgetManager::allocate(const std::string& subsystem, const std::string& key,
                                                                                                                                   uint64 sizeBytes, EvictionCallback onEvict)
{
  std::lock_guard<std::mutex> lock(m_Mutex);

  std::vector<AllocationHandle> evicted = makeRoom(sizeBytes);

  AllocationHandle handle = m_NextHandle++;
  Entry entry;
  entry.subsystem = subsystem;
  entry.key = key;
  entry.sizeBytes = sizeBytes;
  entry.lastAccessed = std::chrono::steady_clock::now();
  entry.onEvict = std::move(onEvict);

  m_Entries.emplace(handle, std::move(entry));
  m_UsedBytes += sizeBytes;

  return {handle, std::move(evicted)};
}

void MemoryBudgetManager::touch(AllocationHandle handle)
{
  std::lock_guard<std::mutex> lock(m_Mutex);
  auto it = m_Entries.find(handle);
  if(it != m_Entries.end())
  {
    it->second.lastAccessed = std::chrono::steady_clock::now();
  }
}

void MemoryBudgetManager::release(AllocationHandle handle)
{
  std::lock_guard<std::mutex> lock(m_Mutex);
  auto it = m_Entries.find(handle);
  if(it != m_Entries.end())
  {
    m_UsedBytes -= it->second.sizeBytes;
    m_Entries.erase(it);
  }
}

void MemoryBudgetManager::setBudgetBytes(uint64 bytes)
{
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_BudgetBytes = bytes;
}

uint64 MemoryBudgetManager::budgetBytes() const
{
  std::lock_guard<std::mutex> lock(m_Mutex);
  return m_BudgetBytes;
}

uint64 MemoryBudgetManager::usedBytes() const
{
  std::lock_guard<std::mutex> lock(m_Mutex);
  return m_UsedBytes;
}

void MemoryBudgetManager::clear()
{
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_Entries.clear();
  m_UsedBytes = 0;
}

std::vector<MemoryBudgetManager::AllocationHandle> MemoryBudgetManager::makeRoom(uint64 needed)
{
  std::vector<AllocationHandle> evicted;

  while(!m_Entries.empty() && m_UsedBytes + needed > m_BudgetBytes)
  {
    // Find entry with oldest lastAccessed using a direct iterator
    auto oldest = m_Entries.end();
    for(auto it = m_Entries.begin(); it != m_Entries.end(); ++it)
    {
      if(oldest == m_Entries.end() || it->second.lastAccessed < oldest->second.lastAccessed)
      {
        oldest = it;
      }
    }

    if(oldest == m_Entries.end())
    {
      break;
    }

    // Invoke eviction callback under mutex (must be non-blocking)
    if(oldest->second.onEvict)
    {
      oldest->second.onEvict();
    }

    m_UsedBytes -= oldest->second.sizeBytes;
    evicted.push_back(oldest->first);
    m_Entries.erase(oldest);
  }

  return evicted;
}

} // namespace nx::core
