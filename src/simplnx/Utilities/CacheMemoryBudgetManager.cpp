#include "simplnx/Utilities/CacheMemoryBudgetManager.hpp"

#include <algorithm>

#include "simplnx/Utilities/MemoryUtilities.hpp"

namespace nx::core
{

namespace
{
constexpr uint64 k_MinBudget = uint64{1} * 1024 * 1024 * 1024;          // 1 GiB
constexpr uint64 k_BudgetReserveBytes = uint64{6} * 1024 * 1024 * 1024; // 6 GiB OS/app headroom
} // namespace

uint64 CacheMemoryBudgetManager::totalSystemRamBytes()
{
  return nx::core::Memory::GetTotalMemory();
}

CacheMemoryBudgetManager::CacheMemoryBudgetManager()
: m_BudgetBytes(defaultBudgetBytes())
{
}

CacheMemoryBudgetManager& CacheMemoryBudgetManager::instance()
{
  static CacheMemoryBudgetManager s_Instance;
  return s_Instance;
}

uint64 CacheMemoryBudgetManager::defaultBudgetBytes()
{
  const uint64 totalRam = totalSystemRamBytes();
  if(totalRam == 0)
  {
    return k_MinBudget;
  }
  // Start with half of RAM and apply the 1-GiB floor.
  // The maximum budget then preserves the required operating-system and application reserve.
  return std::min(std::max(totalRam / 2, k_MinBudget), maxBudgetBytes());
}

uint64 CacheMemoryBudgetManager::maxBudgetBytes()
{
  const uint64 totalRam = totalSystemRamBytes();
  if(totalRam == 0)
  {
    return k_MinBudget;
  }
  // Reserve 6 GiB, but do not permit more than 95 percent of RAM.
  // Subtraction by one twentieth keeps the percentage calculation in integer arithmetic and avoids multiplication overflow.
  const uint64 reserved = (totalRam > k_BudgetReserveBytes) ? (totalRam - k_BudgetReserveBytes) : 0;
  const uint64 fraction95 = totalRam - totalRam / 20;
  const uint64 cap = std::min(reserved, fraction95);
  return std::max(cap, k_MinBudget);
}

std::pair<CacheMemoryBudgetManager::AllocationHandle, std::vector<CacheMemoryBudgetManager::AllocationHandle>> CacheMemoryBudgetManager::allocate(const std::string& subsystem, const std::string& key,
                                                                                                                                                  uint64 sizeBytes, EvictionCallback onEvict)
{
  return allocate(subsystem, key, sizeBytes, std::move(onEvict), AllocationOptions{});
}

std::pair<CacheMemoryBudgetManager::AllocationHandle, std::vector<CacheMemoryBudgetManager::AllocationHandle>> CacheMemoryBudgetManager::allocate(const std::string& subsystem, const std::string& key,
                                                                                                                                                  uint64 sizeBytes, EvictionCallback onEvict,
                                                                                                                                                  const AllocationOptions& options)
{
  std::lock_guard<std::mutex> lock(m_Mutex);

  std::vector<AllocationHandle> evicted = makeRoom(sizeBytes);

  AllocationHandle handle = m_NextHandle++;
  Entry entry;
  entry.subsystem = subsystem;
  entry.key = key;
  entry.sizeBytes = sizeBytes;
  entry.pinCount = options.initiallyPinned ? 1 : 0;
  entry.lastAccessed = std::chrono::steady_clock::now();
  entry.onEvict = std::move(onEvict);

  m_Entries.emplace(handle, std::move(entry));
  m_UsedBytes += sizeBytes;
  if(options.initiallyPinned)
  {
    m_PinnedBytes += sizeBytes;
  }

  return {handle, std::move(evicted)};
}

std::optional<CacheMemoryBudgetManager::AllocationHandle> CacheMemoryBudgetManager::reservePinned(const std::string& subsystem, const std::string& key, uint64 sizeBytes)
{
  std::lock_guard<std::mutex> lock(m_Mutex);
  if(m_PinnedBytes > m_BudgetBytes || sizeBytes > m_BudgetBytes - m_PinnedBytes)
  {
    return std::nullopt;
  }

  (void)makeRoom(sizeBytes);

  const AllocationHandle handle = m_NextHandle++;
  Entry entry;
  entry.subsystem = subsystem;
  entry.key = key;
  entry.sizeBytes = sizeBytes;
  entry.pinCount = 1;
  entry.lastAccessed = std::chrono::steady_clock::now();
  m_Entries.emplace(handle, std::move(entry));
  m_UsedBytes += sizeBytes;
  m_PinnedBytes += sizeBytes;
  return handle;
}

bool CacheMemoryBudgetManager::pin(AllocationHandle handle)
{
  std::lock_guard<std::mutex> lock(m_Mutex);
  auto iter = m_Entries.find(handle);
  if(iter == m_Entries.end())
  {
    return false;
  }

  if(iter->second.pinCount == 0)
  {
    m_PinnedBytes += iter->second.sizeBytes;
  }
  iter->second.pinCount++;
  iter->second.lastAccessed = std::chrono::steady_clock::now();
  return true;
}

CacheMemoryBudgetManager::PinResult CacheMemoryBudgetManager::pinWithinBudget(AllocationHandle handle)
{
  std::lock_guard<std::mutex> lock(m_Mutex);
  auto iter = m_Entries.find(handle);
  if(iter == m_Entries.end())
  {
    return PinResult::UnknownHandle;
  }

  if(iter->second.pinCount == 0)
  {
    if(m_PinnedBytes > m_BudgetBytes || iter->second.sizeBytes > m_BudgetBytes - m_PinnedBytes)
    {
      return PinResult::BudgetExceeded;
    }
    m_PinnedBytes += iter->second.sizeBytes;
  }
  iter->second.pinCount++;
  iter->second.lastAccessed = std::chrono::steady_clock::now();
  return PinResult::Success;
}

bool CacheMemoryBudgetManager::unpin(AllocationHandle handle)
{
  std::lock_guard<std::mutex> lock(m_Mutex);
  auto iter = m_Entries.find(handle);
  if(iter == m_Entries.end())
  {
    return false;
  }

  if(iter->second.pinCount == 0)
  {
    return true;
  }

  iter->second.pinCount--;
  if(iter->second.pinCount == 0)
  {
    m_PinnedBytes -= iter->second.sizeBytes;
    iter->second.lastAccessed = std::chrono::steady_clock::now();
  }
  return true;
}

void CacheMemoryBudgetManager::touch(AllocationHandle handle)
{
  std::lock_guard<std::mutex> lock(m_Mutex);
  auto it = m_Entries.find(handle);
  if(it != m_Entries.end())
  {
    it->second.lastAccessed = std::chrono::steady_clock::now();
  }
}

void CacheMemoryBudgetManager::registerSubsystem(const std::string& subsystem, SubsystemEvictionHandler handler)
{
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_SubsystemHandlers[subsystem] = std::move(handler);
}

void CacheMemoryBudgetManager::release(AllocationHandle handle)
{
  std::lock_guard<std::mutex> lock(m_Mutex);
  auto it = m_Entries.find(handle);
  if(it != m_Entries.end())
  {
    if(it->second.pinCount > 0)
    {
      m_PinnedBytes -= it->second.sizeBytes;
    }
    m_UsedBytes -= it->second.sizeBytes;
    m_Entries.erase(it);
  }
}

bool CacheMemoryBudgetManager::setBudgetBytes(uint64 bytes)
{
  // Clamp only the upper bound. Calculate the machine limit before acquiring m_Mutex.
  const uint64 maxAllowed = maxBudgetBytes();
  bool clamped = false;
  if(bytes > maxAllowed)
  {
    bytes = maxAllowed;
    clamped = true;
  }

  std::lock_guard<std::mutex> lock(m_Mutex);
  m_BudgetBytes = bytes;
  return clamped;
}

uint64 CacheMemoryBudgetManager::budgetBytes() const
{
  std::lock_guard<std::mutex> lock(m_Mutex);
  return m_BudgetBytes;
}

uint64 CacheMemoryBudgetManager::usedBytes() const
{
  std::lock_guard<std::mutex> lock(m_Mutex);
  return m_UsedBytes;
}

uint64 CacheMemoryBudgetManager::pinnedBytes() const
{
  std::lock_guard<std::mutex> lock(m_Mutex);
  return m_PinnedBytes;
}

void CacheMemoryBudgetManager::clear()
{
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_Entries.clear();
  m_UsedBytes = 0;
  m_PinnedBytes = 0;
}

std::vector<CacheMemoryBudgetManager::AllocationHandle> CacheMemoryBudgetManager::makeRoom(uint64 needed)
{
  std::vector<AllocationHandle> evicted;

  const auto lacksRoom = [this, needed]() { return needed > m_BudgetBytes || m_UsedBytes > m_BudgetBytes - needed; };
  while(!m_Entries.empty() && lacksRoom())
  {
    auto oldest = m_Entries.end();
    for(auto it = m_Entries.begin(); it != m_Entries.end(); ++it)
    {
      if(it->second.pinCount != 0)
      {
        continue;
      }
      if(oldest == m_Entries.end() || it->second.lastAccessed < oldest->second.lastAccessed)
      {
        oldest = it;
      }
    }

    if(oldest == m_Entries.end())
    {
      break;
    }

    const auto handlerIter = m_SubsystemHandlers.find(oldest->second.subsystem);
    if(handlerIter != m_SubsystemHandlers.end())
    {
      // A delegated subsystem releases entries later. Avoid duplicate requests while its accounting is pending.
      const uint64 targetExistingBytes = needed >= m_BudgetBytes ? 0 : m_BudgetBytes - needed;
      const uint64 deficit = m_UsedBytes - targetExistingBytes;
      handlerIter->second(deficit);
      break;
    }

    // The callback runs under m_Mutex and can only mark its entry for later removal.
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
