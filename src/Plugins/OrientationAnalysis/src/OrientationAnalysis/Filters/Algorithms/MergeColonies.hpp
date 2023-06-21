#pragma once

#include "OrientationAnalysis/Filters/Algorithms/GroupFeatures.hpp"
#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "complex/DataStructure/AttributeMatrix.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include "EbsdLib/LaueOps/LaueOps.h"

namespace complex
{
struct ORIENTATIONANALYSIS_EXPORT MergeColoniesInputValues
{
  float32 AxisTolerance;
  float32 AngleTolerance;
  DataPath FeaturePhasesPath;
  DataPath AvgQuatsPath;
  DataPath FeatureIdsPath;
  DataPath CellPhasesPath;
  DataPath CrystalStructuresPath;
  DataPath CellParentIdsPath;
  DataPath CellFeatureAMPath;
  DataPath FeatureParentIdsPath;
  DataPath ActivePath;
  bool RandomizeParentIds = true;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class ORIENTATIONANALYSIS_EXPORT MergeColonies : public GroupFeatures
{
  using LaueOpsShPtrType = std::shared_ptr<LaueOps>;
  using LaueOpsContainer = std::vector<LaueOpsShPtrType>;

public:
  MergeColonies(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, MergeColoniesInputValues* inputValues,
                GroupFeaturesInputValues* groupInputValues);
  ~MergeColonies() noexcept;

  MergeColonies(const MergeColonies&) = delete;
  MergeColonies(MergeColonies&&) noexcept = delete;
  MergeColonies& operator=(const MergeColonies&) = delete;
  MergeColonies& operator=(MergeColonies&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

  int getSeed(int32 newFid) override;
  bool determineGrouping(int32 referenceFeature, int32 neighborFeature, int32 newFid) override;
  void characterize_colonies();

private:
  DataStructure& m_DataStructure;
  const MergeColoniesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
  LaueOpsContainer m_OrientationOps;
  Int32Array& m_FeatureParentIds;
  Int32Array& m_FeaturePhases;
  Float32Array& m_AvgQuats;
  UInt32Array& m_CrystalStructures;
  float32 m_AxisToleranceRad = 0.0;
  float32 m_AngleTolerance = 1.0;
};
} // namespace complex
