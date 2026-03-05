#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/MaskCompareUtilities.hpp"
#include "simplnx/Utilities/SegmentFeatures.hpp"

#include <EbsdLib/LaueOps/LaueOps.h>

#include <vector>

namespace nx::core
{

/**
 * @brief The EBSDSegmentFeaturesInputValues struct
 */
struct ORIENTATIONANALYSIS_EXPORT EBSDSegmentFeaturesInputValues
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
  bool IsPeriodic;
};

/**
 * @brief
 */
class ORIENTATIONANALYSIS_EXPORT EBSDSegmentFeatures : public SegmentFeatures
{
public:
  using FeatureIdsArrayType = Int32Array;

  EBSDSegmentFeatures(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, EBSDSegmentFeaturesInputValues* inputValues);
  ~EBSDSegmentFeatures() noexcept override;

  EBSDSegmentFeatures(const EBSDSegmentFeatures&) = delete;            // Copy Constructor Not Implemented
  EBSDSegmentFeatures(EBSDSegmentFeatures&&) = delete;                 // Move Constructor Not Implemented
  EBSDSegmentFeatures& operator=(const EBSDSegmentFeatures&) = delete; // Copy Assignment Not Implemented
  EBSDSegmentFeatures& operator=(EBSDSegmentFeatures&&) = delete;      // Move Assignment Not Implemented

  Result<> operator()();

protected:
  int64_t getSeed(int32 gnum, int64 nextSeed) const override;
  bool determineGrouping(int64 referencePoint, int64 neighborPoint, int32 gnum) const override;

  /**
   * @brief Checks whether a voxel can participate in EBSD segmentation based on mask and phase.
   * @param point Linear voxel index.
   * @return true if the voxel passes mask and phase checks.
   */
  bool isValidVoxel(int64 point) const override;

  /**
   * @brief Determines whether two neighboring voxels belong to the same EBSD segment.
   * @param point1 First voxel index.
   * @param point2 Second (neighbor) voxel index.
   * @return true if both voxels share the same phase and their misorientation is within tolerance.
   */
  bool areNeighborsSimilar(int64 point1, int64 point2) const override;

private:
  const EBSDSegmentFeaturesInputValues* m_InputValues = nullptr;
  Float32Array* m_QuatsArray = nullptr;
  FeatureIdsArrayType* m_CellPhases = nullptr;
  std::unique_ptr<MaskCompareUtilities::MaskCompare> m_GoodVoxelsArray = nullptr;
  DataArray<uint32>* m_CrystalStructures = nullptr;

  FeatureIdsArrayType* m_FeatureIdsArray = nullptr;

  std::vector<ebsdlib::LaueOps::Pointer> m_OrientationOps;
};

} // namespace nx::core
