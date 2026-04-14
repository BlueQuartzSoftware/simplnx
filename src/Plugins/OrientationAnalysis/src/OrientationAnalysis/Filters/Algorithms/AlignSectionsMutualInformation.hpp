#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Utilities/AlignSections.hpp"

#include <EbsdLib/LaueOps/LaueOps.h>

namespace nx::core
{
/**
 * @struct AlignSectionsMutualInformationInputValues
 * @brief Holds all user-supplied parameters for the AlignSectionsMutualInformation algorithm.
 */
struct ORIENTATIONANALYSIS_EXPORT AlignSectionsMutualInformationInputValues
{
  DataPath ImageGeometryPath; ///< Path to the ImageGeom defining the 3D voxel grid
  bool UseMask = false;       ///< Whether to exclude masked voxels from alignment
  DataPath MaskArrayPath;     ///< Path to the Bool/UInt8 mask array (only used when UseMask is true)

  float32 MisorientationTolerance = 0.0f; ///< Misorientation tolerance (degrees) used to segment features within each Z-slice
  DataPath QuatsArrayPath;                ///< Path to the Float32 quaternion array (4 components per cell)
  DataPath CellPhasesArrayPath;           ///< Path to the Int32 cell phases array
  DataPath CrystalStructuresArrayPath;    ///< Path to the UInt32 crystal structures ensemble array

  bool StoreAlignmentShifts = false;  ///< Whether to write computed shifts into output DataArrays
  DataPath AlignmentAMPath;           ///< Path to the Attribute Matrix storing alignment shift output arrays
  DataPath SlicesArrayPath;           ///< Path to the UInt32 array recording which slices were compared
  DataPath RelativeShiftsArrayPath;   ///< Path to the Int64 array recording per-iteration relative X/Y shifts
  DataPath CumulativeShiftsArrayPath; ///< Path to the Int64 array recording cumulative X/Y shifts
};

/**
 * @class AlignSectionsMutualInformation
 * @brief Aligns consecutive Z-sections by maximizing mutual information of
 * orientation-based feature segmentations between adjacent slices.
 *
 * Unlike the misorientation-based alignment which compares individual voxel
 * orientations, this algorithm first segments each 2D Z-slice into temporary
 * "features" (groups of voxels whose orientations are within MisorientationTolerance).
 * It then computes the mutual information between the feature-ID maps of
 * adjacent slices. Mutual information measures the statistical dependence
 * between two discrete distributions -- here, the joint probability of
 * feature-ID pairs vs. their marginal probabilities. Higher mutual information
 * indicates better alignment because features in neighboring slices overlap
 * more consistently.
 *
 * The shift search uses the same 7x7 iterative-refinement approach as
 * AlignSectionsMisorientation, but maximizes mutual information instead of
 * minimizing misorientation count.
 *
 * ## Out-of-Core (OOC) Optimization
 *
 * The original algorithm stored all per-slice feature IDs in a single
 * totalPoints-sized array and pre-segmented every slice up front, requiring
 * the full quaternion, phase, and feature-ID arrays to be resident in memory.
 * The optimized version uses a rolling 2-slice approach:
 *   1. Bulk-reads one Z-slice of quaternions, phases, and mask via
 *      copyIntoBuffer() into local std::vectors.
 *   2. Flood-fills the slice locally via formFeaturesForSlice() to produce
 *      a per-slice feature-ID vector (indices 0 to sliceVoxels-1).
 *   3. Computes mutual information using only the current and reference
 *      feature-ID vectors.
 *   4. After convergence, swaps the current feature-ID vector into the
 *      reference slot (O(1) std::swap) for the next iteration.
 *
 * This reduces memory from O(totalVoxels) to O(2 * sliceVoxels) and ensures
 * each slice's data is read exactly once via a single sequential bulk I/O
 * operation, eliminating chunk thrashing on OOC DataStores.
 */
class ORIENTATIONANALYSIS_EXPORT AlignSectionsMutualInformation : public AlignSections
{
public:
  /**
   * @brief Constructs the algorithm with all required references and parameters.
   * @param dataStructure The DataStructure containing all input/output arrays.
   * @param mesgHandler Handler for sending progress messages to the UI.
   * @param shouldCancel Atomic flag checked between iterations to support cancellation.
   * @param inputValues User-supplied parameters controlling the alignment behavior.
   */
  AlignSectionsMutualInformation(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                 AlignSectionsMutualInformationInputValues* inputValues);
  ~AlignSectionsMutualInformation() noexcept override;

  AlignSectionsMutualInformation(const AlignSectionsMutualInformation&) = delete;
  AlignSectionsMutualInformation(AlignSectionsMutualInformation&&) noexcept = delete;
  AlignSectionsMutualInformation& operator=(const AlignSectionsMutualInformation&) = delete;
  AlignSectionsMutualInformation& operator=(AlignSectionsMutualInformation&&) noexcept = delete;

  /**
   * @brief Executes the mutual-information-based section alignment algorithm.
   * @return Result<> indicating success or any errors encountered during execution.
   */
  Result<> operator()();

protected:
  /**
   * @brief Computes the optimal X-Y shift for each pair of adjacent Z-slices
   * by maximizing mutual information of per-slice feature segmentations.
   *
   * Uses a rolling 2-slice buffering strategy: each Z-slice is bulk-read once,
   * flood-filled into a local feature-ID vector, and then the current vector
   * is swapped into the reference slot after convergence.
   *
   * @param xShifts Output vector of cumulative X shifts per slice (size = numSlices).
   * @param yShifts Output vector of cumulative Y shifts per slice (size = numSlices).
   * @return Success or error result.
   */
  Result<> findShifts(std::vector<int64>& xShifts, std::vector<int64>& yShifts) override;

private:
  /**
   * @brief Flood-fills a single Z-slice to identify features based on
   * misorientation tolerance, operating on pre-buffered slice data.
   *
   * Uses a region-growing algorithm: starting from an unassigned seed voxel,
   * expands to 4-connected in-plane neighbors (up, down, left, right) whose
   * misorientation with the current voxel is below the tolerance. Each
   * connected component receives a unique feature ID.
   *
   * This method works identically for both in-core and OOC paths because the
   * caller provides pre-buffered raw pointers to the slice's data, so no
   * DataStore access occurs inside the flood fill.
   *
   * @param quats Pointer to the slice's quaternion data (4 float32 components per voxel).
   * @param phases Pointer to the slice's phase data (1 int32 per voxel).
   * @param mask Pointer to the slice's mask data (nullptr if mask is not used).
   * @param featureIds Output vector of per-voxel feature IDs (must be pre-zeroed, size = dimX*dimY).
   * @param dimX Number of voxels in X dimension.
   * @param dimY Number of voxels in Y dimension.
   * @param misorientationTolerance Misorientation tolerance in radians.
   * @param useMask Whether to use the mask array.
   * @param orientationOps Laue orientation operators for symmetry-aware misorientation.
   * @param crystalStructures Crystal structure IDs indexed by phase number.
   * @return The number of features found in this slice (feature IDs run from 1 to return-value-1).
   */
  int32 formFeaturesForSlice(const float32* quats, const int32* phases, const uint8* mask, std::vector<int32>& featureIds, int64 dimX, int64 dimY, float32 misorientationTolerance, bool useMask,
                             const std::vector<ebsdlib::LaueOps::Pointer>& orientationOps, const std::vector<uint32>& crystalStructures);

  DataStructure& m_DataStructure;                                           ///< Reference to the DataStructure containing all arrays
  const AlignSectionsMutualInformationInputValues* m_InputValues = nullptr; ///< Non-owning pointer to the user-supplied parameters
  const std::atomic_bool& m_ShouldCancel;                                   ///< Atomic flag for cooperative cancellation
  const IFilter::MessageHandler& m_MessageHandler;                          ///< Handler for sending progress messages to the UI
};

} // namespace nx::core
