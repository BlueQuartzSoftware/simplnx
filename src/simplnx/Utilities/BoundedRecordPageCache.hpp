#pragma once

#include "simplnx/DataStructure/IO/Generic/ITemporaryRecordStore.hpp"

#include <fmt/format.h>

#include <algorithm>
#include <cstring>
#include <iterator>
#include <limits>
#include <list>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

/**
 * @namespace nx::core
 * @brief Contains simplnx core types and functions.
 */
namespace nx::core
{

/**
 * @class BoundedRecordPageCache
 * @brief Non-concurrent least-recently-used (LRU) page cache for typed temporary records.
 * @tparam T Specifies a trivially copyable record type.
 *
 * Logical state can grow with cells, features, or generated mesh size. A fixed
 * page count bounds resident memory. Dirty pages are written before eviction.
 * Callers must provide synchronization or confine each instance to one thread.
 *
 * The cache borrows its record store. The store must outlive the cache, and the
 * caller must call flush() before another object consumes pending writes.
 */
template <typename T>
class BoundedRecordPageCache
{
private:
  struct Page;

public:
  static_assert(std::is_trivially_copyable_v<T>);
  /**
   * @brief Creates an empty cache with fixed page and page-count bounds.
   * @param store Mutable fixed-record store whose record width must equal sizeof(T).
   * @param recordsPerPage Number of records loaded and evicted as one page.
   * @param maximumPages Hard upper bound on simultaneously resident pages.
   */
  BoundedRecordPageCache(ITemporaryRecordStore& store, uint64 recordsPerPage, usize maximumPages)
  : m_Store(store)
  , m_RecordsPerPage(recordsPerPage)
  , m_MaximumPages(maximumPages)
  {
  }

  /**
   * @brief Reads one typed record, loading and promoting its page as needed.
   * @param index Zero-based record index.
   * @param shouldCancel Cancellation flag.
   * @return A copy of the record or a cancellation, configuration, range, allocation, or store-I/O error.
   */
  Result<T> read(uint64 index, const std::atomic_bool& shouldCancel)
  {
    if(m_RecordsPerPage == 0)
    {
      return MakeErrorResult<T>(-6040, "Temporary record page-cache has zero records per page");
    }
    if(index >= m_Store.recordCount())
    {
      return MakeErrorResult<T>(-6041, "Temporary record page-cache request is outside the store");
    }
    auto pageResult = loadPage(index / m_RecordsPerPage, shouldCancel);
    if(pageResult.invalid())
    {
      return ConvertInvalidResult<T>(std::move(pageResult));
    }
    const auto& page = pageResult.value().get();
    return {page.records[static_cast<usize>(index % m_RecordsPerPage)]};
  }

  /**
   * @brief Reads requested records in order while reusing one page lookup for consecutive indexes on that page.
   * @return A valid result or a cancellation, configuration, range, allocation, or store-I/O error.
   */
  Result<> readMany(nonstd::span<const uint64> indices, nonstd::span<T> values, const std::atomic_bool& shouldCancel)
  {
    if(indices.size() != values.size())
    {
      return MakeErrorResult(-6040, fmt::format("Temporary record page-cache bulk read requires equal index and output counts. Index count: {}; output count: {}.", indices.size(), values.size()));
    }
    if(indices.empty())
    {
      return {};
    }
    if(m_RecordsPerPage == 0)
    {
      return MakeErrorResult(-6040, "Temporary record page-cache bulk read has zero records per page.");
    }

    uint64 currentPageIndex = std::numeric_limits<uint64>::max();
    Page* currentPage = nullptr;
    for(usize index = 0; index < indices.size(); ++index)
    {
      const uint64 recordIndex = indices[index];
      const uint64 pageIndex = recordIndex / m_RecordsPerPage;
      if(currentPage == nullptr || pageIndex != currentPageIndex)
      {
        auto pageResult = loadPage(pageIndex, shouldCancel);
        if(pageResult.invalid())
        {
          return ConvertResult(std::move(pageResult));
        }
        currentPage = &pageResult.value().get();
        currentPageIndex = pageIndex;
      }
      if(recordIndex < currentPage->firstRecord || recordIndex - currentPage->firstRecord >= currentPage->recordCount)
      {
        return MakeErrorResult(-6041, fmt::format("Temporary record page-cache bulk read index {} is outside its loaded page range [{}, {}).", recordIndex, currentPage->firstRecord,
                                                  currentPage->firstRecord + currentPage->recordCount));
      }
      values[index] = currentPage->records[static_cast<usize>(recordIndex - currentPage->firstRecord)];
    }
    return {};
  }

  /** @brief Invokes a non-throwing visitor with a cached record without copying the record. */
  template <typename VisitorT>
  Result<> inspect(uint64 index, VisitorT&& visitor, const std::atomic_bool& shouldCancel)
  {
    static_assert(std::is_nothrow_invocable_v<VisitorT&&, const T&>);
    if(m_RecordsPerPage == 0)
    {
      return MakeErrorResult(-6040, "Temporary record page-cache inspection has zero records per page.");
    }
    if(index >= m_Store.recordCount())
    {
      return MakeErrorResult(-6041, fmt::format("Temporary record page-cache inspection index {} is outside the {}-record store.", index, m_Store.recordCount()));
    }
    auto pageResult = loadPage(index / m_RecordsPerPage, shouldCancel);
    if(pageResult.invalid())
    {
      return ConvertResult(std::move(pageResult));
    }
    const auto& page = pageResult.value().get();
    std::forward<VisitorT>(visitor)(page.records[static_cast<usize>(index % m_RecordsPerPage)]);
    return {};
  }

  /** @brief Invokes a non-throwing modifier on one cached record and marks its page dirty. */
  template <typename ModifierT>
  Result<> modify(uint64 index, ModifierT&& modifier, const std::atomic_bool& shouldCancel)
  {
    static_assert(std::is_nothrow_invocable_v<ModifierT&&, T&>);
    if(m_RecordsPerPage == 0)
    {
      return MakeErrorResult(-6040, "Temporary record page-cache modification has zero records per page.");
    }
    if(index >= m_Store.recordCount())
    {
      return MakeErrorResult(-6041, fmt::format("Temporary record page-cache modification index {} is outside the {}-record store.", index, m_Store.recordCount()));
    }
    auto pageResult = loadPage(index / m_RecordsPerPage, shouldCancel);
    if(pageResult.invalid())
    {
      return ConvertResult(std::move(pageResult));
    }
    auto& page = pageResult.value().get();
    std::forward<ModifierT>(modifier)(page.records[static_cast<usize>(index % m_RecordsPerPage)]);
    page.dirty = true;
    return {};
  }

  /**
   * @brief Updates one cached record and marks its page dirty for write-back.
   * @param index Zero-based record index.
   * @param value Replacement record value.
   * @param shouldCancel Cancellation flag.
   * @return A valid result or a cancellation, configuration, range, allocation, or store-I/O error.
   */
  Result<> write(uint64 index, const T& value, const std::atomic_bool& shouldCancel)
  {
    if(m_RecordsPerPage == 0)
    {
      return MakeErrorResult(-6040, "Temporary record page-cache has zero records per page");
    }
    if(index >= m_Store.recordCount())
    {
      return MakeErrorResult(-6041, "Temporary record page-cache request is outside the store");
    }
    auto pageResult = loadPage(index / m_RecordsPerPage, shouldCancel);
    if(pageResult.invalid())
    {
      return ConvertResult(std::move(pageResult));
    }
    auto& page = pageResult.value().get();
    page.records[static_cast<usize>(index % m_RecordsPerPage)] = value;
    page.dirty = true;
    return {};
  }

  /**
   * @brief Writes every dirty resident page to the backing record store.
   * @param shouldCancel Cancellation flag.
   * @return A valid result or the first backing-store or cancellation failure.
   */
  Result<> flush(const std::atomic_bool& shouldCancel)
  {
    for(auto& page : m_Pages)
    {
      auto result = flushPage(page, shouldCancel);
      if(result.invalid())
      {
        return result;
      }
    }
    return {};
  }

  usize cachedPageCount() const
  {
    return m_Pages.size();
  }

private:
  /**
   * @struct Page
   * @brief Stores one resident typed page and its write-back state.
   */
  struct Page
  {
    uint64 firstRecord = 0;
    uint64 recordCount = 0;
    bool dirty = false;
    std::vector<T> records;
  };

  /**
   * @brief Returns an MRU page, evicting and flushing the LRU page when the cache is full.
   * @param pageIndex Zero-based page index.
   * @param shouldCancel Cancellation flag.
   * @return A reference valid until a later cache operation evicts that page, or an error.
   */
  Result<std::reference_wrapper<Page>> loadPage(uint64 pageIndex, const std::atomic_bool& shouldCancel)
  {
    if(shouldCancel || m_RecordsPerPage == 0 || m_MaximumPages == 0 || sizeof(T) != m_Store.recordSize() || m_RecordsPerPage > m_Store.maxRecordsPerBatch() ||
       pageIndex > std::numeric_limits<uint64>::max() / m_RecordsPerPage)
    {
      return MakeErrorResult<std::reference_wrapper<Page>>(-6040, "Temporary record page-cache request is cancelled or invalid");
    }
    const uint64 firstRecord = pageIndex * m_RecordsPerPage;
    if(firstRecord >= m_Store.recordCount())
    {
      return MakeErrorResult<std::reference_wrapper<Page>>(-6041, "Temporary record page-cache request is outside the store");
    }
    if(!m_Pages.empty() && m_Pages.front().firstRecord == firstRecord)
    {
      return {std::ref(m_Pages.front())};
    }
    auto found = m_PageIndex.find(firstRecord);
    if(found != m_PageIndex.end())
    {
      m_Pages.splice(m_Pages.begin(), m_Pages, found->second);
      return {std::ref(m_Pages.front())};
    }
    const uint64 count = std::min(m_RecordsPerPage, m_Store.recordCount() - firstRecord);
    if(count > std::numeric_limits<usize>::max() || count > std::numeric_limits<usize>::max() / sizeof(T))
    {
      return MakeErrorResult<std::reference_wrapper<Page>>(-6042, "Temporary record page-cache page size overflows memory limits");
    }

    if(m_Pages.size() == m_MaximumPages)
    {
      auto result = flushPage(m_Pages.back(), shouldCancel);
      if(result.invalid())
      {
        return ConvertInvalidResult<std::reference_wrapper<Page>>(std::move(result));
      }
      m_PageIndex.erase(m_Pages.back().firstRecord);
      m_Pages.splice(m_Pages.begin(), m_Pages, std::prev(m_Pages.end()));
    }
    else
    {
      try
      {
        m_Pages.emplace_front();
      } catch(const std::exception&)
      {
        return MakeErrorResult<std::reference_wrapper<Page>>(-6044, "Temporary record page-cache allocation failed");
      }
    }

    Page& page = m_Pages.front();
    page.firstRecord = firstRecord;
    page.recordCount = count;
    page.dirty = false;
    try
    {
      page.records.resize(static_cast<usize>(count));
    } catch(const std::bad_alloc&)
    {
      m_Pages.pop_front();
      return MakeErrorResult<std::reference_wrapper<Page>>(-6044, "Temporary record page-cache allocation failed");
    } catch(const std::length_error&)
    {
      m_Pages.pop_front();
      return MakeErrorResult<std::reference_wrapper<Page>>(-6044, "Temporary record page-cache allocation failed");
    }
    auto bytes = nonstd::span<std::byte>(reinterpret_cast<std::byte*>(page.records.data()), page.records.size() * sizeof(T));
    auto readResult = m_Store.read(firstRecord, count, bytes, shouldCancel);
    if(readResult.invalid() || readResult.value() != count)
    {
      m_Pages.pop_front();
      return readResult.invalid() ? ConvertInvalidResult<std::reference_wrapper<Page>>(std::move(readResult)) :
                                    MakeErrorResult<std::reference_wrapper<Page>>(-6043, "Temporary record page-cache received a short page read");
    }
    try
    {
      const auto [indexIterator, inserted] = m_PageIndex.emplace(firstRecord, m_Pages.begin());
      static_cast<void>(indexIterator);
      if(!inserted)
      {
        m_Pages.pop_front();
        return MakeErrorResult<std::reference_wrapper<Page>>(-6044, "Temporary record page-cache index insertion failed");
      }
    } catch(const std::exception&)
    {
      if(!m_Pages.empty() && m_Pages.front().firstRecord == firstRecord)
      {
        m_Pages.pop_front();
      }
      return MakeErrorResult<std::reference_wrapper<Page>>(-6044, "Temporary record page-cache allocation failed");
    }
    return {std::ref(m_Pages.front())};
  }

  /**
   * @brief Writes one dirty page and clears its dirty state after a successful write.
   * @param page Page to write when dirty.
   * @param shouldCancel Cancellation flag.
   * @return Valid result or a backing-store or cancellation error.
   */
  Result<> flushPage(Page& page, const std::atomic_bool& shouldCancel)
  {
    if(!page.dirty)
    {
      return {};
    }
    auto bytes = nonstd::span<const std::byte>(reinterpret_cast<const std::byte*>(page.records.data()), page.records.size() * sizeof(T));
    auto result = m_Store.write(page.firstRecord, page.recordCount, bytes, shouldCancel);
    if(result.valid())
    {
      page.dirty = false;
    }
    return result;
  }

  ITemporaryRecordStore& m_Store;
  uint64 m_RecordsPerPage;
  usize m_MaximumPages;
  std::list<Page> m_Pages;
  std::unordered_map<uint64, typename std::list<Page>::iterator> m_PageIndex;
};
} // namespace nx::core
