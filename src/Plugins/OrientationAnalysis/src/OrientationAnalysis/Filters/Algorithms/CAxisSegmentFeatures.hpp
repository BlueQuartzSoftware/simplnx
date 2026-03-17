#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/MaskCompareUtilities.hpp"
#include "simplnx/Utilities/SegmentFeatures.hpp"

namespace nx::core
{

struct ORIENTATIONANALYSIS_EXPORT CAxisSegmentFeaturesInputValues
{
  float32 MisorientationTolerance;
  bool UseMask;
  bool RandomizeFeatureIds;
  SegmentFeatures::NeighborScheme NeighborScheme;
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
 * @brief This filter segments the Features by grouping neighboring Cells that satisfy the C-axis misalignment tolerance, i.e., have misalignment angle less than the value set by the user.
 */

class ORIENTATIONANALYSIS_EXPORT CAxisSegmentFeatures : public SegmentFeatures
{
public:
  CAxisSegmentFeatures(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CAxisSegmentFeaturesInputValues* inputValues);
  ~CAxisSegmentFeatures() noexcept override;

  CAxisSegmentFeatures(const CAxisSegmentFeatures&) = delete;
  CAxisSegmentFeatures(CAxisSegmentFeatures&&) noexcept = delete;
  CAxisSegmentFeatures& operator=(const CAxisSegmentFeatures&) = delete;
  CAxisSegmentFeatures& operator=(CAxisSegmentFeatures&&) noexcept = delete;

  Result<> operator()();

protected:
  int64 getSeed(int32 gnum, int64 nextSeed) const override;
  bool determineGrouping(int64 referencePoint, int64 neighborPoint, int32 gnum) const override;

  /**
   * @brief Checks whether a voxel can participate in C-axis segmentation based on mask and phase.
   * @param point Linear voxel index.
   * @return true if the voxel passes mask and phase checks.
   */
  bool isValidVoxel(int64 point) const override;

  /**
   * @brief Determines whether two neighboring voxels belong to the same C-axis segment.
   * @param point1 First voxel index.
   * @param point2 Second (neighbor) voxel index.
   * @return true if both voxels share the same phase and their C-axis misalignment is within tolerance.
   */
  bool areNeighborsSimilar(int64 point1, int64 point2) const override;

  void prepareForSlice(int64 iz, int64 dimX, int64 dimY, int64 dimZ) override;

private:
  const CAxisSegmentFeaturesInputValues* m_InputValues = nullptr;

  Float32Array* m_QuatsArray = nullptr;
  Int32Array* m_CellPhases = nullptr;
  std::unique_ptr<MaskCompareUtilities::MaskCompare> m_GoodVoxelsArray = nullptr;
  Int32Array* m_FeatureIdsArray = nullptr;

  void allocateSliceBuffers(int64 dimX, int64 dimY);
  void deallocateSliceBuffers();

  // Rolling 2-slot input buffers for OOC optimization.
  std::vector<float32> m_QuatBuffer;
  std::vector<int32> m_PhaseBuffer;
  std::vector<uint8> m_MaskBuffer;
  int64 m_BufSliceSize = 0;
  int64 m_BufferedSliceZ[2] = {-1, -1};
  bool m_UseSliceBuffers = false;
};

} // namespace nx::core
