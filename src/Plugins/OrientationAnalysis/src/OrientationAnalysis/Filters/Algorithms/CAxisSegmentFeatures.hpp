#pragma once

#include <array>

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/MaskCompareUtilities.hpp"
#include "simplnx/Utilities/SegmentFeatures.hpp"

namespace nx::core
{

/**
 * @struct CAxisSegmentFeaturesInputValues
 * @brief Identifies c-axis segmentation inputs.
 *
 * MisorientationTolerance is in radians. ActiveArrayPath reserves tuple zero
 * for the background feature.
 */
struct ORIENTATIONANALYSIS_EXPORT CAxisSegmentFeaturesInputValues
{
  float32 MisorientationTolerance = 0.0f;
  bool UseMask = false;
  bool RandomizeFeatureIds = false;
  SegmentFeatures::NeighborScheme NeighborScheme{};
  DataPath ImageGeometryPath;
  DataPath QuatsArrayPath;
  DataPath CellPhasesArrayPath;
  DataPath MaskArrayPath;
  DataPath CrystalStructuresArrayPath;
  DataPath FeatureIdsArrayPath;
  DataPath CellFeatureAttributeMatrixPath;
  DataPath ActiveArrayPath;
};

/**
 * @class CAxisSegmentFeatures
 * @brief Segments hexagonal EBSD cells by c-axis alignment.
 *
 * The algorithm compares sample-frame [0001] directions. Parallel and
 * antiparallel c axes are equivalent. Positive unmasked phase IDs must select
 * a hexagonal crystal structure.
 *
 * SegmentFeatures performs connected-component labeling one slice at a time.
 * Two local slices supply quaternion, phase, and mask data for normal and
 * periodic comparisons. Bulk reads avoid per-element OOC access. Direct array
 * access is retained outside CCL and gives no generic DataStore thread-safety
 * guarantee.
 */
class ORIENTATIONANALYSIS_EXPORT CAxisSegmentFeatures : public SegmentFeatures
{
public:
  /**
   * @brief Initializes c-axis segmentation.
   * @param dataStructure Provides selected arrays and the geometry.
   * @param mesgHandler Supplies progress messages.
   * @param shouldCancel Signals cancellation.
   * @param inputValues Identifies segmentation settings.
   * @pre dataStructure, mesgHandler, shouldCancel, and inputValues outlive this
   *      executor.
   */
  CAxisSegmentFeatures(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CAxisSegmentFeaturesInputValues* inputValues);
  /**
   * @brief Destroys the c-axis segmentation executor.
   */
  ~CAxisSegmentFeatures() noexcept override;

  CAxisSegmentFeatures(const CAxisSegmentFeatures&) = delete;
  CAxisSegmentFeatures(CAxisSegmentFeatures&&) noexcept = delete;
  CAxisSegmentFeatures& operator=(const CAxisSegmentFeatures&) = delete;
  CAxisSegmentFeatures& operator=(CAxisSegmentFeatures&&) noexcept = delete;

  /**
   * @brief Executes c-axis connected-component labeling.
   * @return Success, or a phase, mask, or bulk-I/O error.
   */
  Result<> operator()();

protected:
  /**
   * @brief Checks whether a voxel can seed or join a feature.
   * @param point Identifies the voxel.
   * @return True if the mask permits the voxel and its phase is positive.
   *
   * Active slice buffers avoid a DataStore access. CCL rejects a nonresident
   * slice instead of reading it directly.
   */
  bool isValidVoxel(int64 point) const override;

  /**
   * @brief Checks c-axis alignment for neighboring voxels.
   * @param point1 Identifies the source voxel.
   * @param point2 Identifies the neighbor voxel.
   * @return True if the cells share a phase and meet the c-axis tolerance.
   *
   * The comparison accepts antiparallel c axes. Active buffers keep CCL reads
   * local.
   */
  bool areNeighborsSimilar(int64 point1, int64 point2) const override;

  /**
   * @brief Loads a Z slice into a two-slot LRU buffer.
   * @param iz Identifies the slice, or a negative value to disable buffering.
   * @param dimX Specifies the X dimension.
   * @param dimY Specifies the Y dimension.
   * @param dimZ Specifies the Z dimension.
   * @return Success, or a mask or source bulk-I/O error.
   *
   * Resident slices are reused. LRU replacement supports periodic comparisons.
   */
  Result<> prepareForSlice(int64 iz, int64 dimX, int64 dimY, int64 dimZ) override;

private:
  const CAxisSegmentFeaturesInputValues* m_InputValues = nullptr;

  Float32Array* m_QuatsArray = nullptr;
  Int32Array* m_CellPhases = nullptr;
  std::unique_ptr<MaskCompareUtilities::MaskCompare> m_GoodVoxelsArray = nullptr;
  Int32Array* m_FeatureIdsArray = nullptr;

  /**
   * @brief Allocates two local X-Y slice buffers.
   * @param dimX Specifies the X dimension.
   * @param dimY Specifies the Y dimension.
   */
  void allocateSliceBuffers(int64 dimX, int64 dimY);

  /**
   * @brief Releases local slice buffers.
   */
  void deallocateSliceBuffers();

  std::vector<float32> m_QuatBuffer;
  std::vector<int32> m_PhaseBuffer;
  std::vector<uint8> m_MaskBuffer;
  int64 m_BufSliceSize = 0;
  std::array<int64, 2> m_BufferedSliceZ = {-1, -1};
  std::array<uint64, 2> m_BufferUseSequence = {0, 0};
  uint64 m_NextBufferUseSequence = 1;
  bool m_UseSliceBuffers = false;
};

} // namespace nx::core
