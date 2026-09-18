#pragma once

#include "simplnx/Utilities/BoundedRecordPageCache.hpp"

#include <limits>
#include <new>

/**
 * @namespace nx::core
 * @brief Contains simplnx core types and functions.
 */
namespace nx::core
{
/**
 * @class ExternalEquivalence
 * @brief Disjoint-set equivalence structure whose node array lives in a temporary record store.
 *
 * Connected-component and clustering algorithms can create labels proportional
 * to the input size. Keeping those labels in a bounded page cache avoids an
 * equally large resident parent/rank/size vector. Union-by-rank and path halving
 * limit record traffic, while the lowest original representative is tracked
 * explicitly so results remain deterministic regardless of union order.
 *
 * The object owns the record store and its cache. It is not thread-safe.
 */
class ExternalEquivalence
{
public:
  /**
   * @struct Node
   * @brief Fixed-width disjoint-set record persisted by the temporary store.
   */
  struct Node
  {
    uint64 parent = 0;
    uint64 size = 0;
    uint64 rank = 0;
    /**
     * @brief Stores the lowest original label in the component.
     */
    uint64 representative = 0;
    /**
     * @brief Distinguishes the current record layout from compatible legacy records.
     */
    uint64 initialized = 0;
  };
  /**
   * @brief Validates and creates externally backed equivalence state.
   * @param store Writable owned store with one Node record per possible label.
   * @param recordsPerPage Node count loaded by one bounded cache page.
   * @param maxPages Hard bound on simultaneously resident node pages.
   * @param initialComponentSize Size assigned when a label is initialized lazily.
   * @return The owned equivalence object or a configuration/allocation error.
   */
  static Result<std::unique_ptr<ExternalEquivalence>> Create(std::unique_ptr<ITemporaryRecordStore> store, uint64 recordsPerPage, usize maxPages, uint64 initialComponentSize = 1)
  {
    if(store == nullptr || store->isReadOnly() || store->recordSize() != sizeof(Node) || store->recordCount() == 0 || recordsPerPage == 0 || maxPages == 0 ||
       recordsPerPage > store->maxRecordsPerBatch())
    {
      return MakeErrorResult<std::unique_ptr<ExternalEquivalence>>(-6051, "Invalid external equivalence temporary-store configuration");
    }
    try
    {
      return {std::unique_ptr<ExternalEquivalence>(new ExternalEquivalence(std::move(store), recordsPerPage, maxPages, initialComponentSize))};
    } catch(const std::bad_alloc&)
    {
      return MakeErrorResult<std::unique_ptr<ExternalEquivalence>>(-6051, "External equivalence allocation failed");
    }
  }

private:
  /**
   * @brief Takes ownership of a validated store and constructs its page cache.
   * @param store Validated writable Node store.
   * @param recordsPerPage Node count in one cache page.
   * @param maxPages Maximum resident page count.
   * @param initialComponentSize Size for a lazily initialized label.
   */
  ExternalEquivalence(std::unique_ptr<ITemporaryRecordStore> store, uint64 recordsPerPage, usize maxPages, uint64 initialComponentSize)
  : m_Store(std::move(store))
  , m_Cache(*m_Store, recordsPerPage, maxPages)
  , m_InitialComponentSize(initialComponentSize)
  {
  }

public:
  /**
   * @brief Finds the deterministic lowest representative of a label's component.
   * @param label Label to resolve and initialize when necessary.
   * @param cancel Cancellation flag.
   * @return The representative or a range, cache-I/O, or cancellation error.
   */
  Result<uint64> find(uint64 label, const std::atomic_bool& cancel)
  {
    auto root = findRoot(label, cancel);
    if(root.invalid())
      return root;
    auto rootNode = node(root.value(), cancel);
    if(rootNode.invalid())
      return ConvertInvalidResult<uint64>(std::move(rootNode));
    return {rootNode.value().representative};
  }
  /**
   * @brief Unites two components by rank while combining size and lowest-representative state.
   * @param left Label in the first component.
   * @param right Label in the second component.
   * @param cancel Cancellation flag.
   * @return A valid result or a range, overflow, cache-I/O, or cancellation error.
   */
  Result<> unite(uint64 left, uint64 right, const std::atomic_bool& cancel)
  {
    auto l = findRoot(left, cancel);
    auto r = findRoot(right, cancel);
    if(l.invalid())
      return ConvertResult(std::move(l));
    if(r.invalid())
      return ConvertResult(std::move(r));
    if(l.value() == r.value())
      return {};
    uint64 parent = l.value();
    uint64 child = r.value();
    auto p = node(parent, cancel);
    auto c = node(child, cancel);
    if(p.invalid())
      return ConvertResult(std::move(p));
    if(c.invalid())
      return ConvertResult(std::move(c));
    if(p.value().rank < c.value().rank)
    {
      std::swap(parent, child);
      std::swap(p.value(), c.value());
    }
    if(p.value().size > std::numeric_limits<uint64>::max() - c.value().size)
      return MakeErrorResult(-6052, "External equivalence component size overflow");
    p.value().size += c.value().size;
    p.value().representative = std::min(p.value().representative, c.value().representative);
    c.value().parent = parent;
    if(p.value().rank == c.value().rank)
    {
      p.value().rank++;
    }
    auto a = m_Cache.write(parent, p.value(), cancel);
    if(a.invalid())
      return a;
    return m_Cache.write(child, c.value(), cancel);
  }
  /**
   * @brief Returns the accumulated size stored at a label's resolved root.
   * @param label Label to resolve and initialize when necessary.
   * @param cancel Cancellation flag.
   * @return Component size or a range, cache-I/O, or cancellation error.
   */
  Result<uint64> componentSize(uint64 label, const std::atomic_bool& cancel)
  {
    auto root = findRoot(label, cancel);
    if(root.invalid())
      return root;
    auto n = node(root.value(), cancel);
    if(n.invalid())
      return ConvertInvalidResult<uint64>(std::move(n));
    return {n.value().size};
  }
  /**
   * @brief Adds to a component's root size without a resident per-label size array.
   * @param label Label in the component.
   * @param increment Size to add.
   * @param cancel Cancellation flag.
   * @return Valid result or a range, overflow, cache-I/O, or cancellation error.
   */
  Result<> addSize(uint64 label, uint64 increment, const std::atomic_bool& cancel)
  {
    auto root = findRoot(label, cancel);
    if(root.invalid())
      return ConvertResult(std::move(root));
    auto n = node(root.value(), cancel);
    if(n.invalid())
      return ConvertResult(std::move(n));
    if(n.value().size > std::numeric_limits<uint64>::max() - increment)
      return MakeErrorResult(-6052, "External equivalence component size overflow");
    n.value().size += increment;
    return m_Cache.write(root.value(), n.value(), cancel);
  }
  /**
   * @brief Flushes dirty equivalence pages before another phase reads the store.
   * @param cancel Cancellation flag.
   * @return Valid result or a backing-store or cancellation error.
   */
  Result<> flush(const std::atomic_bool& cancel)
  {
    return m_Cache.flush(cancel);
  }

private:
  /**
   * @brief Resolves a structural root with path-halving writes.
   * @param label Label to resolve and initialize when necessary.
   * @param cancel Cancellation flag.
   * @return Root label or a range, cache-I/O, or cancellation error.
   */
  Result<uint64> findRoot(uint64 label, const std::atomic_bool& cancel)
  {
    auto n = node(label, cancel);
    if(n.invalid())
      return ConvertInvalidResult<uint64>(std::move(n));
    uint64 current = label;
    while(n.value().parent != current)
    {
      const uint64 parent = n.value().parent;
      auto parentNode = node(parent, cancel);
      if(parentNode.invalid())
        return ConvertInvalidResult<uint64>(std::move(parentNode));
      if(parentNode.value().parent != parent)
      {
        n.value().parent = parentNode.value().parent;
        auto writeResult = m_Cache.write(current, n.value(), cancel);
        if(writeResult.invalid())
          return ConvertResultTo<uint64>(std::move(writeResult), uint64{});
      }
      current = parent;
      n = std::move(parentNode);
    }
    return {current};
  }
  /**
   * @brief Reads and lazily initializes one node.
   * @param label Label and record index.
   * @param cancel Cancellation flag.
   * @return Node value or a range, cache-I/O, or cancellation error.
   *
   * A nonzero size without the marker identifies a compatible two- or three-field record layout.
   */
  Result<Node> node(uint64 label, const std::atomic_bool& cancel)
  {
    if(label >= m_Store->recordCount())
      return MakeErrorResult<Node>(-6050, "External equivalence label exceeds declared store capacity");
    auto n = m_Cache.read(label, cancel);
    if(n.invalid())
      return n;
    if(n.value().initialized != k_InitializedMarker)
    {
      if(n.value().size == 0)
      {
        n.value() = {label, m_InitialComponentSize, 0, label, k_InitializedMarker};
      }
      else
      {
        // Preserve records that use the compatible two- or three-field Node layout.
        n.value().representative = n.value().parent == label ? label : 0;
        n.value().initialized = k_InitializedMarker;
      }
      auto r = m_Cache.write(label, n.value(), cancel);
      if(r.invalid())
        return ConvertInvalidResult<Node>(std::move(r));
    }
    return n;
  }
  /**
   * @brief Marks records that use the current five-field Node layout.
   */
  static inline constexpr uint64 k_InitializedMarker = 0x45515549564E4F44ULL;
  std::unique_ptr<ITemporaryRecordStore> m_Store;
  BoundedRecordPageCache<Node> m_Cache;
  uint64 m_InitialComponentSize = 1;
};
} // namespace nx::core
