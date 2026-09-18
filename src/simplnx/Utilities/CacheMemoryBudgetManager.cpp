#include "simplnx/Utilities/CacheMemoryBudgetManager.hpp"

#include <algorithm>
#include <limits>
#include <utility>

#include "simplnx/Utilities/MemoryUtilities.hpp"

namespace nx::core
{

namespace
{
constexpr uint64 k_MinBudget = uint64{1} * 1024 * 1024 * 1024;          // 1 GiB
constexpr uint64 k_BudgetReserveBytes = uint64{6} * 1024 * 1024 * 1024; // 6 GiB OS/app headroom

uint64 SaturatingAdd(uint64 lhs, uint64 rhs)
{
  if(rhs > std::numeric_limits<uint64>::max() - lhs)
  {
    return std::numeric_limits<uint64>::max();
  }
  return lhs + rhs;
}
} // namespace

CacheMemoryBudgetManager::WorkingMemoryReservation::WorkingMemoryReservation(WorkingMemoryReservationKey, CacheMemoryBudgetManager* manager, uint64 sizeBytes) noexcept
: m_Manager(manager)
, m_SizeBytes(sizeBytes)
{
}

CacheMemoryBudgetManager::WorkingMemoryReservation::~WorkingMemoryReservation() noexcept
{
  release();
}

CacheMemoryBudgetManager::WorkingMemoryReservation::WorkingMemoryReservation(WorkingMemoryReservation&& other) noexcept
: m_Manager(std::exchange(other.m_Manager, nullptr))
, m_SizeBytes(std::exchange(other.m_SizeBytes, 0))
{
}

CacheMemoryBudgetManager::WorkingMemoryReservation& CacheMemoryBudgetManager::WorkingMemoryReservation::operator=(WorkingMemoryReservation&& other) noexcept
{
  if(this != &other)
  {
    release();
    m_Manager = std::exchange(other.m_Manager, nullptr);
    m_SizeBytes = std::exchange(other.m_SizeBytes, 0);
  }
  return *this;
}

uint64 CacheMemoryBudgetManager::WorkingMemoryReservation::sizeBytes() const noexcept
{
  return m_SizeBytes;
}

void CacheMemoryBudgetManager::WorkingMemoryReservation::shrinkTo(uint64 sizeBytes) noexcept
{
  if(sizeBytes >= m_SizeBytes)
  {
    return;
  }
  const uint64 releasedBytes = m_SizeBytes - sizeBytes;
  m_SizeBytes = sizeBytes;
  if(m_Manager != nullptr)
  {
    m_Manager->releaseWorkingMemory(releasedBytes);
  }
}

void CacheMemoryBudgetManager::WorkingMemoryReservation::release() noexcept
{
  if(m_Manager != nullptr)
  {
    m_Manager->releaseWorkingMemory(m_SizeBytes);
    m_Manager = nullptr;
    m_SizeBytes = 0;
  }
}

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

CacheMemoryBudgetManager::WorkingMemoryReservation CacheMemoryBudgetManager::reserveWorkingMemory(uint64 requestedBytes)
{
  std::lock_guard<std::mutex> lock(m_Mutex);
  const uint64 maximumWorkingBytes = m_BudgetBytes / 4;
  const uint64 remainingWorkingBytes = maximumWorkingBytes > m_ReservedWorkingMemoryBytes ? maximumWorkingBytes - m_ReservedWorkingMemoryBytes : 0;
  const uint64 grantedBytes = std::min(requestedBytes, remainingWorkingBytes);
  if(grantedBytes == 0)
  {
    return {};
  }

  makeRoom(grantedBytes);
  m_ReservedWorkingMemoryBytes += grantedBytes;
  return WorkingMemoryReservation(WorkingMemoryReservationKey{}, this, grantedBytes);
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

uint64 CacheMemoryBudgetManager::maximumWorkingMemoryBytes() const
{
  std::lock_guard<std::mutex> lock(m_Mutex);
  return m_BudgetBytes / 4;
}

uint64 CacheMemoryBudgetManager::reservedWorkingMemoryBytes() const
{
  std::lock_guard<std::mutex> lock(m_Mutex);
  return m_ReservedWorkingMemoryBytes;
}

uint64 CacheMemoryBudgetManager::effectiveCacheBudgetBytes() const
{
  std::lock_guard<std::mutex> lock(m_Mutex);
  return m_BudgetBytes > m_ReservedWorkingMemoryBytes ? m_BudgetBytes - m_ReservedWorkingMemoryBytes : 0;
}

void CacheMemoryBudgetManager::clear()
{
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_Entries.clear();
  m_UsedBytes = 0;
  m_PinnedBytes = 0;
}

/**
 * @brief Makes room for an allocation under the shared cache budget.
 * @param needed Requested allocation size in bytes.
 * @return Handles removed through direct entry eviction.
 *
 * A delegated handler receives the absolute byte deficit at the time of the call.
 */
std::vector<CacheMemoryBudgetManager::AllocationHandle> CacheMemoryBudgetManager::makeRoom(uint64 needed)
{
  std::vector<AllocationHandle> evicted;

  const uint64 effectiveCacheBudget = m_BudgetBytes > m_ReservedWorkingMemoryBytes ? m_BudgetBytes - m_ReservedWorkingMemoryBytes : 0;
  auto exceedsEffectiveBudget = [this, effectiveCacheBudget, needed]() { return m_UsedBytes > effectiveCacheBudget || needed > effectiveCacheBudget - m_UsedBytes; };

  while(!m_Entries.empty() && exceedsEffectiveBudget())
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
      const uint64 combinedBytes = SaturatingAdd(m_UsedBytes, needed);
      const uint64 deficit = combinedBytes > effectiveCacheBudget ? combinedBytes - effectiveCacheBudget : 0;
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

void CacheMemoryBudgetManager::releaseWorkingMemory(uint64 sizeBytes) noexcept
{
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_ReservedWorkingMemoryBytes = sizeBytes <= m_ReservedWorkingMemoryBytes ? m_ReservedWorkingMemoryBytes - sizeBytes : 0;
}

} // namespace nx::core
