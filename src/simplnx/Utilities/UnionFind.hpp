#pragma once

#include "simplnx/simplnx_export.hpp"

#include "simplnx/Common/Types.hpp"

#include <algorithm>
#include <vector>

namespace nx::core
{

/**
 * @class UnionFind
 * @brief Vector-based Union-Find (Disjoint Set) data structure for tracking
 * connected component equivalences during chunk-sequential processing.
 *
 * Uses union-by-rank and path-halving compression for near-O(1) amortized
 * find() and unite() operations. Internal storage uses contiguous vectors
 * indexed by label for cache-friendly access (no hash map overhead).
 *
 * Key features:
 * - Labels are contiguous integers starting from 1 (0 is unused/invalid)
 * - Grows dynamically as new labels are encountered
 * - Path halving in find() for near-O(1) amortized lookups
 * - Union-by-rank for balanced merges
 * - Accumulates sizes at each label during construction
 * - Single-pass flatten() for full path compression and size accumulation
 */
class SIMPLNX_EXPORT UnionFind
{
public:
  UnionFind()
  {
    // Index 0 is unused (labels start at 1). Initialize with a small capacity.
    constexpr usize k_InitialCapacity = 64;
    m_Parent.resize(k_InitialCapacity);
    m_Rank.resize(k_InitialCapacity, 0);
    m_Size.resize(k_InitialCapacity, 0);
    // Initialize all entries as self-parents
    for(usize i = 0; i < k_InitialCapacity; i++)
    {
      m_Parent[i] = static_cast<int64>(i);
    }
  }

  ~UnionFind() = default;

  UnionFind(const UnionFind&) = delete;
  UnionFind(UnionFind&&) noexcept = default;
  UnionFind& operator=(const UnionFind&) = delete;
  UnionFind& operator=(UnionFind&&) noexcept = default;

  /**
   * @brief Find the root label with path-halving compression.
   * Each node on the path is redirected to its grandparent, giving
   * near-O(1) amortized performance.
   * @param x Label to find
   * @return Root label
   */
  int64 find(int64 x)
  {
    ensureCapacity(x);

    // Path halving: point each node to its grandparent while walking
    while(m_Parent[x] != x)
    {
      m_Parent[x] = m_Parent[m_Parent[x]];
      x = m_Parent[x];
    }
    return x;
  }

  /**
   * @brief Unite two labels into the same equivalence class using union-by-rank.
   * @param a First label
   * @param b Second label
   */
  void unite(int64 a, int64 b)
  {
    int64 rootA = find(a);
    int64 rootB = find(b);

    if(rootA == rootB)
    {
      return;
    }

    if(m_Rank[rootA] < m_Rank[rootB])
    {
      m_Parent[rootA] = rootB;
    }
    else if(m_Rank[rootA] > m_Rank[rootB])
    {
      m_Parent[rootB] = rootA;
    }
    else
    {
      m_Parent[rootB] = rootA;
      m_Rank[rootA]++;
    }
  }

  /**
   * @brief Add to the size count for a label.
   * Sizes are accumulated at each label, not the root. They are
   * accumulated to roots during flatten().
   * @param label Label to update
   * @param count Number of voxels to add
   */
  void addSize(int64 label, uint64 count)
  {
    ensureCapacity(label);
    m_Size[label] += count;
  }

  /**
   * @brief Get the total size of a label's equivalence class.
   * Should only be called after flatten() for accurate totals.
   * @param label Label to query
   * @return Total number of voxels in the equivalence class
   */
  uint64 getSize(int64 label)
  {
    int64 root = find(label);
    return m_Size[root];
  }

  /**
   * @brief Flatten the union-find structure with full path compression
   * and accumulate all sizes to root labels.
   *
   * After flatten():
   * - Every label points directly to its root
   * - All sizes are accumulated at root labels
   * - Subsequent find() calls are O(1) (single lookup)
   */
  void flatten()
  {
    const usize count = m_Parent.size();

    // Full path compression: point every label directly to its root
    for(usize i = 1; i < count; i++)
    {
      m_Parent[i] = find(static_cast<int64>(i));
    }

    // Accumulate sizes to roots
    std::vector<uint64> rootSizes(count, 0);
    for(usize i = 1; i < count; i++)
    {
      rootSizes[m_Parent[i]] += m_Size[i];
    }
    m_Size = std::move(rootSizes);
  }

private:
  /**
   * @brief Ensure the internal vectors can hold index x.
   * Grows by doubling to amortize allocation cost.
   */
  void ensureCapacity(int64 x)
  {
    auto idx = static_cast<usize>(x);
    if(idx < m_Parent.size())
    {
      return;
    }

    usize newSize = std::max(idx + 1, m_Parent.size() * 2);
    usize oldSize = m_Parent.size();
    m_Parent.resize(newSize);
    m_Rank.resize(newSize, 0);
    m_Size.resize(newSize, 0);

    // Initialize new entries as self-parents
    for(usize i = oldSize; i < newSize; i++)
    {
      m_Parent[i] = static_cast<int64>(i);
    }
  }

  std::vector<int64> m_Parent;
  std::vector<int32> m_Rank;
  std::vector<uint64> m_Size;
};

} // namespace nx::core
