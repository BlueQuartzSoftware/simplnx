#pragma once

#include "simplnx/simplnx_export.hpp"

#include "simplnx/Common/Types.hpp"

#include <algorithm>
#include <vector>

namespace nx::core
{

/**
 * @class UnionFind
 * @brief Tracks connected-component equivalences with dense integer labels.
 *
 * Union by rank and path halving give near-constant amortized operations.
 * Contiguous vectors avoid hash lookup. Storage grows with the largest label,
 * so callers must use dense positive labels. Label zero is invalid.
 *
 * addSize() records counts at the supplied labels. flatten() moves all counts to
 * roots and fully compresses paths. The class is mutable and not thread-safe.
 */
class SIMPLNX_EXPORT UnionFind
{
public:
  UnionFind()
  {
    // Reserve index zero and a small initial range for positive labels.
    constexpr usize k_InitialCapacity = 64;
    m_Parent.resize(k_InitialCapacity);
    m_Rank.resize(k_InitialCapacity, 0);
    m_Size.resize(k_InitialCapacity, 0);
    // New label slots start as independent sets.
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
   * @brief Finds a root and applies path-halving compression.
   * @param x Specifies a positive label.
   * @return Root label for x.
   * @pre x is greater than zero and is representable as usize.
   */
  int64 find(int64 x)
  {
    ensureCapacity(x);

    // Point each visited label to its grandparent while walking to the root.
    while(m_Parent[x] != x)
    {
      m_Parent[x] = m_Parent[m_Parent[x]];
      x = m_Parent[x];
    }
    return x;
  }

  /**
   * @brief Unites two equivalence classes by rank.
   * @param a Specifies the first positive label.
   * @param b Specifies the second positive label.
   * @pre Both labels are greater than zero and are representable as usize.
   *
   * This operation does not move size counts. Call flatten() after all unions.
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
   * @brief Adds a count to one label before flattening.
   * @param label Specifies a positive label.
   * @param count Specifies the count to add.
   * @pre label is representable as usize. The addition does not overflow uint64.
   *
   * The count stays at label until flatten() accumulates it at the root.
   */
  void addSize(int64 label, uint64 count)
  {
    ensureCapacity(label);
    m_Size[label] += count;
  }

  /**
   * @brief Gets the accumulated size of one equivalence class.
   * @param label Specifies a positive label.
   * @return Count stored at the label root.
   * @pre label is greater than zero and is representable as usize.
   * @pre flatten() completed after the most recent union or size addition.
   */
  uint64 getSize(int64 label)
  {
    int64 root = find(label);
    return m_Size[root];
  }

  /**
   * @brief Fully compresses paths and accumulates sizes at roots.
   * @pre The accumulated size for each root fits uint64.
   *
   * Each allocated label points directly to its root after this call. A later
   * union or size addition requires another flatten() before accurate size queries.
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
   * @brief Grows internal vectors to contain one label.
   * @param x Specifies a positive label index.
   * @pre x is representable as usize. Growth arithmetic and allocation succeed.
   *
   * Capacity doubles when that is larger than the required label range.
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

    // New label slots start as independent sets.
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
