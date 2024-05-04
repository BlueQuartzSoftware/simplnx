#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/SegmentFeatures.hpp"

namespace nx::core
{

struct ORIENTATIONANALYSIS_EXPORT CAxisSegmentFeaturesInputValues
{
  float32 MisorientationTolerance;
  bool UseMask;
  bool RandomizeFeatureIds;
  DataPath ImageGeometryPath;
  DataPath QuatsArrayPath;
  DataPath CellPhasesArrayPath;
  DataPath MaskArrayPath;
  DataPath CrystalStructuresArrayPath;
  DataPath FeatureIdsArrayName;
  DataPath CellFeatureAttributeMatrixName;
  DataPath ActiveArrayName;
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

  const std::atomic_bool& getCancel();

protected:
  int64 getSeed(int32 gnum, int64 nextSeed) const override;
  bool determineGrouping(int64 referencePoint, int64 neighborPoint, int32 gnum) const override;

private:
  const CAxisSegmentFeaturesInputValues* m_InputValues = nullptr;

  Float32Array* m_QuatsArray = nullptr;
  Int32Array* m_CellPhases = nullptr;
  std::unique_ptr<MaskCompare> m_GoodVoxelsArray = nullptr;
  Int32Array* m_FeatureIdsArray = nullptr;
};

} // namespace nx::core
