#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/MaskCompareUtilities.hpp"
#include "simplnx/Utilities/SegmentFeatures.hpp"

#include <random>
#include <vector>

namespace nx::core
{

struct SIMPLNXCORE_EXPORT ScalarSegmentFeaturesInputValues
{
  int ScalarTolerance = 0;
  bool UseMask;
  bool RandomizeFeatureIds;
  bool IsPeriodic = false;
  SegmentFeatures::NeighborScheme NeighborScheme;
  DataPath ImageGeometryPath;
  DataPath InputDataPath;
  DataPath MaskArrayPath;
  DataPath FeatureIdsArrayPath;
  DataPath CellFeatureAttributeMatrixPath;
  DataPath ActiveArrayPath;
};

/**
 * @brief The ScalarSegmentFeatures class
 */
class SIMPLNXCORE_EXPORT ScalarSegmentFeatures : public SegmentFeatures
{
public:
  using FeatureIdsArrayType = Int32Array;
  using GoodVoxelsArrayType = BoolArray;

  ScalarSegmentFeatures(DataStructure& dataStructure, ScalarSegmentFeaturesInputValues* inputValues, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& mesgHandler);
  ~ScalarSegmentFeatures() noexcept override;

  ScalarSegmentFeatures(const ScalarSegmentFeatures&) = delete;
  ScalarSegmentFeatures(ScalarSegmentFeatures&&) noexcept = delete;
  ScalarSegmentFeatures& operator=(const ScalarSegmentFeatures&) = delete;
  ScalarSegmentFeatures& operator=(ScalarSegmentFeatures&&) noexcept = delete;

  Result<> operator()();

protected:
  int64 getSeed(int32 gnum, int64 nextSeed) const override;
  bool determineGrouping(int64 referencePoint, int64 neighborPoint, int32 gnum) const override;

  /**
   * @brief Checks whether a voxel can participate in scalar segmentation based on the mask.
   * @param point Linear voxel index.
   * @return true if the voxel passes the mask check (or no mask is used).
   */
  bool isValidVoxel(int64 point) const override;

  /**
   * @brief Determines whether two neighboring voxels belong to the same scalar segment.
   * @param point1 First voxel index.
   * @param point2 Second (neighbor) voxel index.
   * @return true if both voxels are valid and their scalar values are within tolerance.
   */
  bool areNeighborsSimilar(int64 point1, int64 point2) const override;

  /**
   * @brief Pre-loads input scalar and mask data for the given Z-slice into
   * rolling buffers, eliminating per-element OOC overhead during CCL.
   * @param iz Current Z-slice index, or -1 to disable buffering.
   * @param dimX X dimension of the grid.
   * @param dimY Y dimension of the grid.
   * @param dimZ Z dimension of the grid.
   */
  void prepareForSlice(int64 iz, int64 dimX, int64 dimY, int64 dimZ) override;

private:
  void allocateSliceBuffers(int64 dimX, int64 dimY);
  void deallocateSliceBuffers();

  const ScalarSegmentFeaturesInputValues* m_InputValues = nullptr;
  FeatureIdsArrayType* m_FeatureIdsArray = nullptr;
  GoodVoxelsArrayType* m_GoodVoxelsArray = nullptr;
  std::shared_ptr<SegmentFeatures::CompareFunctor> m_CompareFunctor;
  std::unique_ptr<MaskCompareUtilities::MaskCompare> m_GoodVoxels = nullptr;
  IDataArray* m_InputDataArray = nullptr;

  // Rolling 2-slot input buffers for OOC optimization.
  std::vector<float64> m_ScalarBuffer;
  std::vector<uint8> m_MaskBuffer;
  int64 m_BufSliceSize = 0;
  int64 m_BufferedSliceZ[2] = {-1, -1};
  bool m_UseSliceBuffers = false;
};
} // namespace nx::core
