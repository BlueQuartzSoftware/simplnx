#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include <vector>

namespace nx::core
{

/**
 * @struct FillBadDataInputValues
 * @brief Holds all user-specified parameters for the FillBadData algorithm.
 *
 * This struct is populated by FillBadDataFilter and passed to the algorithm
 * dispatcher. It is shared between the BFS and CCL algorithm variants.
 */
struct SIMPLNXCORE_EXPORT FillBadDataInputValues
{
  int32 minAllowedDefectSizeValue;             ///< Minimum voxel count for a bad-data region to be preserved as a large defect (regions smaller than this are filled)
  bool storeAsNewPhase;                        ///< If true, large defect regions are assigned to a new phase (maxPhase + 1) for visualization
  DataPath featureIdsArrayPath;                ///< Path to the cell-level FeatureIds array (int32); voxels with value 0 are "bad data"
  DataPath cellPhasesArrayPath;                ///< Path to the cell-level Phases array (int32); only used when storeAsNewPhase is true
  std::vector<DataPath> ignoredDataArrayPaths; ///< Cell arrays that should NOT be updated during the fill (e.g., arrays the user wants to preserve)
  DataPath inputImageGeometry;                 ///< Path to the ImageGeom that defines the voxel grid dimensions
};

/**
 * @class FillBadData
 * @brief Dispatcher that selects between BFS (in-core) and CCL (out-of-core) algorithms
 * for filling bad data regions in an image geometry.
 *
 * This class does not contain algorithm logic itself. It inspects the storage
 * type of the FeatureIds array and delegates to one of two algorithm classes:
 *
 * - **FillBadDataBFS** (in-core): Uses breadth-first search (BFS) flood-fill
 *   with O(N) temporary buffers (neighbors array, visited flags). Efficient when
 *   data fits in RAM because BFS queue access is fast and random access to the
 *   contiguous in-memory buffer is O(1).
 *
 * - **FillBadDataCCL** (out-of-core): Uses a four-phase approach with scanline
 *   Connected Component Labeling (CCL) and Union-Find. Processes data in Z-slice
 *   buffers with strictly sequential access patterns. Avoids the random access
 *   pattern of BFS that causes catastrophic chunk load/evict cycles ("chunk
 *   thrashing") when data is stored on disk in compressed HDF5 chunks.
 *
 * The dispatch decision is made by DispatchAlgorithm<BFS, CCL>(), which checks
 * whether any input array uses out-of-core storage (or if the global
 * ForceOocAlgorithm() test flag is set).
 *
 * @see FillBadDataBFS for the in-core-optimized implementation.
 * @see FillBadDataCCL for the out-of-core-optimized implementation.
 * @see AlgorithmDispatch.hpp for the dispatch mechanism.
 */
class SIMPLNXCORE_EXPORT FillBadData
{
public:
  /**
   * @brief Constructs the dispatcher with the required context for algorithm selection.
   * @param dataStructure The data structure containing the arrays to process.
   * @param mesgHandler Handler for progress and informational messages.
   * @param shouldCancel Cancellation flag checked during execution.
   * @param inputValues Filter parameter values controlling fill behavior.
   */
  FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const FillBadDataInputValues* inputValues);
  ~FillBadData() noexcept;

  FillBadData(const FillBadData&) = delete;
  FillBadData(FillBadData&&) noexcept = delete;
  FillBadData& operator=(const FillBadData&) = delete;
  FillBadData& operator=(FillBadData&&) noexcept = delete;

  /**
   * @brief Dispatches to either BFS or CCL algorithm based on data residency.
   *
   * Checks whether the FeatureIds array uses out-of-core storage. If so (or if
   * ForceOocAlgorithm() is true), constructs and runs FillBadDataCCL. Otherwise,
   * constructs and runs FillBadDataBFS. Both produce identical results.
   *
   * @return Result indicating success or an error with a descriptive message.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;                        ///< Reference to the DataStructure containing all arrays
  const FillBadDataInputValues* m_InputValues = nullptr; ///< Non-owning pointer to the filter parameter values
  const std::atomic_bool& m_ShouldCancel;                ///< Cancellation flag checked during long-running phases
  const IFilter::MessageHandler& m_MessageHandler;       ///< Handler for emitting progress/informational messages
};

} // namespace nx::core
