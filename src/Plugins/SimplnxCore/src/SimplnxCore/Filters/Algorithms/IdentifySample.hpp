#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"

namespace nx::core
{

/**
 * @struct IdentifySampleInputValues
 * @brief Holds all user-specified parameters for the IdentifySample algorithm.
 *
 * This struct is populated by IdentifySampleFilter and passed to the algorithm
 * dispatcher. It is shared between the BFS and CCL algorithm variants, and
 * also by the slice-by-slice functor (IdentifySampleSliceBySliceFunctor).
 */
struct SIMPLNXCORE_EXPORT IdentifySampleInputValues
{
  BoolParameter::ValueType FillHoles;                           ///< If true, interior holes (bad-data regions fully enclosed by the sample) are filled
  GeometrySelectionParameter::ValueType InputImageGeometryPath; ///< Path to the ImageGeom defining the voxel grid dimensions
  ArraySelectionParameter::ValueType MaskArrayPath;             ///< Path to the boolean mask array (true = good/sample, false = bad/non-sample)
  BoolParameter::ValueType SliceBySlice;                        ///< If true, process each 2D slice independently instead of the full 3D volume
  ChoicesParameter::ValueType SliceBySlicePlaneIndex;           ///< Which orthogonal plane to slice along: 0 = XY, 1 = XZ, 2 = YZ
};

/**
 * @class IdentifySample
 * @brief Dispatcher that selects between BFS (in-core) and CCL (out-of-core) algorithms
 * for identifying the largest connected sample region in an image geometry.
 *
 * This class does not contain algorithm logic itself. It inspects the storage
 * type of the mask array and delegates to one of two algorithm classes:
 *
 * - **IdentifySampleBFS** (in-core): Uses BFS flood-fill with O(N) temporary
 *   bit vectors (checked, sample) to discover connected components. Fast when
 *   data fits in RAM due to O(1) random access, but causes chunk thrashing in
 *   OOC mode because BFS visits neighbors in an unpredictable wavefront pattern.
 *
 * - **IdentifySampleCCL** (out-of-core): Uses scanline Connected Component
 *   Labeling (CCL) with a rolling 2-slice label buffer and Vector Union-Find.
 *   Processes data in strict Z-slice sequential order, reading each slice
 *   exactly once. Uses a "replay" technique to avoid O(volume) label storage:
 *   the forward CCL scan is re-executed deterministically to re-derive labels
 *   on the fly during the apply phase.
 *
 * Both variants support the slice-by-slice mode, which delegates to the shared
 * IdentifySampleSliceBySliceFunctor (defined in IdentifySampleCommon.hpp).
 * Slice-by-slice mode uses BFS on individual 2D slices, which is always safe
 * because a single slice fits in memory regardless of OOC storage.
 *
 * The dispatch decision is made by DispatchAlgorithm<BFS, CCL>(), which checks
 * whether the mask array uses out-of-core storage (or if the global
 * ForceOocAlgorithm() test flag is set).
 *
 * @see IdentifySampleBFS for the in-core-optimized implementation.
 * @see IdentifySampleCCL for the out-of-core-optimized implementation.
 * @see IdentifySampleCommon.hpp for the shared slice-by-slice functor.
 * @see AlgorithmDispatch.hpp for the dispatch mechanism.
 */
class SIMPLNXCORE_EXPORT IdentifySample
{
public:
  /**
   * @brief Constructs the dispatcher with the required context for algorithm selection.
   * @param dataStructure The data structure containing the arrays to process.
   * @param mesgHandler Handler for progress and informational messages.
   * @param shouldCancel Cancellation flag checked during execution.
   * @param inputValues Filter parameter values controlling identification behavior.
   */
  IdentifySample(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, IdentifySampleInputValues* inputValues);
  ~IdentifySample() noexcept;

  IdentifySample(const IdentifySample&) = delete;
  IdentifySample(IdentifySample&&) noexcept = delete;
  IdentifySample& operator=(const IdentifySample&) = delete;
  IdentifySample& operator=(IdentifySample&&) noexcept = delete;

  /**
   * @brief Dispatches to either BFS or CCL algorithm based on data residency.
   *
   * Checks whether the mask array uses out-of-core storage. If so (or if
   * ForceOocAlgorithm() is true), constructs and runs IdentifySampleCCL.
   * Otherwise, constructs and runs IdentifySampleBFS. Both produce identical
   * results. In slice-by-slice mode, both variants delegate to the same
   * shared IdentifySampleSliceBySliceFunctor.
   *
   * @return Result indicating success or an error with a descriptive message.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;                           ///< Reference to the DataStructure containing all arrays
  const IdentifySampleInputValues* m_InputValues = nullptr; ///< Non-owning pointer to filter parameter values
  const std::atomic_bool& m_ShouldCancel;                   ///< Cancellation flag checked during long-running phases
  const IFilter::MessageHandler& m_MessageHandler;          ///< Handler for emitting progress/informational messages
};

} // namespace nx::core
