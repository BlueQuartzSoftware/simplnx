#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

struct IdentifySampleInputValues;

/**
 * @class IdentifySampleBFS
 * @brief In-core BFS flood-fill algorithm for identifying the largest connected
 * sample region in an image geometry.
 *
 * This is the in-core-optimized implementation of the IdentifySample algorithm.
 * It uses breadth-first search (BFS) to discover connected components of "good"
 * voxels (mask == true), identifies the largest component as the sample, and
 * optionally fills interior holes in the sample boundary.
 *
 * ## Algorithm Overview
 *
 * The algorithm has two phases:
 *
 * **Phase 1 -- Find the largest connected component of good voxels.**
 * BFS flood-fill is initiated from each unvisited good voxel. The BFS expands
 * to all face-adjacent (6-connected) good neighbors, discovering one connected
 * component per seed. The component with the most voxels is recorded as "the
 * sample". After all components are found, any good voxels NOT in the largest
 * component are set to false (removing satellite regions and noise).
 *
 * **Phase 2 -- Fill interior holes (optional, when FillHoles is true).**
 * A second BFS pass runs on bad voxels (mask == false). Each connected component
 * of bad voxels is discovered via BFS. During expansion, a `touchesBoundary`
 * flag tracks whether any voxel in the component lies on the domain boundary
 * (x/y/z == 0 or max). If the component does NOT touch the boundary, it is
 * fully enclosed by the sample (an interior hole) and all its voxels are set
 * to true. Boundary-touching components are external empty space and left as-is.
 *
 * ## Memory Usage
 *
 * Uses O(N) temporary memory:
 * - `checked`: std::vector<bool> (1 bit per voxel), tracks BFS visited state
 * - `sample`: std::vector<bool> (1 bit per voxel), marks voxels in the largest component
 * - `currentVList`: std::vector<int64>, BFS queue (grows to component size)
 *
 * ## Why This is Not Suitable for Out-of-Core
 *
 * BFS expands outward from a seed in a wavefront pattern, visiting face-adjacent
 * neighbors in an order determined by the queue. When the goodVoxels mask array
 * is stored in compressed HDF5 chunks, each getValue() call may trigger chunk
 * decompression. The wavefront crosses chunk boundaries unpredictably, causing
 * the chunk cache to repeatedly load and evict the same chunks (chunk thrashing).
 * For large volumes, this can degrade performance by orders of magnitude. The
 * CCL variant avoids this by processing data in strict Z-slice sequential order.
 *
 * ## Slice-By-Slice Mode
 *
 * When the user enables slice-by-slice processing, this class delegates to the
 * shared IdentifySampleSliceBySliceFunctor (defined in IdentifySampleCommon.hpp),
 * which performs BFS on individual 2D slices. This is safe even for OOC data
 * because a single 2D slice fits in memory.
 *
 * @see IdentifySampleCCL for the out-of-core-optimized alternative.
 * @see IdentifySample for the dispatcher that selects between BFS and CCL.
 * @see IdentifySampleCommon.hpp for the shared slice-by-slice functor.
 * @see AlgorithmDispatch.hpp for the dispatch mechanism.
 */
class SIMPLNXCORE_EXPORT IdentifySampleBFS
{
public:
  /**
   * @brief Constructs the BFS sample identification algorithm with the required context.
   * @param dataStructure The data structure containing the arrays to process.
   * @param mesgHandler Handler for progress and informational messages.
   * @param shouldCancel Cancellation flag checked during execution.
   * @param inputValues Filter parameter values controlling identification behavior.
   */
  IdentifySampleBFS(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const IdentifySampleInputValues* inputValues);
  ~IdentifySampleBFS() noexcept;

  IdentifySampleBFS(const IdentifySampleBFS&) = delete;
  IdentifySampleBFS(IdentifySampleBFS&&) noexcept = delete;
  IdentifySampleBFS& operator=(const IdentifySampleBFS&) = delete;
  IdentifySampleBFS& operator=(IdentifySampleBFS&&) noexcept = delete;

  /**
   * @brief Executes the BFS flood-fill algorithm to identify the largest sample region.
   *
   * If slice-by-slice mode is enabled, delegates to IdentifySampleSliceBySliceFunctor.
   * Otherwise, runs the full 3D BFS algorithm (Phase 1 + optional Phase 2).
   * The algorithm modifies the mask array in place.
   *
   * @return Result indicating success or an error with a descriptive message.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;                           ///< Reference to the DataStructure containing all arrays
  const IdentifySampleInputValues* m_InputValues = nullptr; ///< Non-owning pointer to filter parameter values
  const std::atomic_bool& m_ShouldCancel;                   ///< Cancellation flag checked during BFS expansion
  const IFilter::MessageHandler& m_MessageHandler;          ///< Handler for emitting progress/informational messages
};

} // namespace nx::core
