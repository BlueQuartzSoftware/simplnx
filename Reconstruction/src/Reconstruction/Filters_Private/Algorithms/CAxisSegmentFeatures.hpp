#pragma once

#include "Reconstruction/Reconstruction_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  CAxisSegmentFeaturesInputValues inputValues;

  inputValues.MisorientationTolerance = filterArgs.value<float32>(k_MisorientationTolerance_Key);
  inputValues.UseGoodVoxels = filterArgs.value<bool>(k_UseGoodVoxels_Key);
  inputValues.QuatsArrayPath = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  inputValues.CellPhasesArrayPath = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  inputValues.GoodVoxelsArrayPath = filterArgs.value<DataPath>(k_GoodVoxelsArrayPath_Key);
  inputValues.CrystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  inputValues.FeatureIdsArrayName = filterArgs.value<DataPath>(k_FeatureIdsArrayName_Key);
  inputValues.CellFeatureAttributeMatrixName = filterArgs.value<DataPath>(k_CellFeatureAttributeMatrixName_Key);
  inputValues.ActiveArrayName = filterArgs.value<DataPath>(k_ActiveArrayName_Key);

  return CAxisSegmentFeatures(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct RECONSTRUCTION_EXPORT CAxisSegmentFeaturesInputValues
{
  float32 MisorientationTolerance;
  bool UseGoodVoxels;
  DataPath QuatsArrayPath;
  DataPath CellPhasesArrayPath;
  DataPath GoodVoxelsArrayPath;
  DataPath CrystalStructuresArrayPath;
  DataPath FeatureIdsArrayName;
  DataPath CellFeatureAttributeMatrixName;
  DataPath ActiveArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class RECONSTRUCTION_EXPORT CAxisSegmentFeatures
{
public:
  CAxisSegmentFeatures(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CAxisSegmentFeaturesInputValues* inputValues);
  ~CAxisSegmentFeatures() noexcept;

  CAxisSegmentFeatures(const CAxisSegmentFeatures&) = delete;
  CAxisSegmentFeatures(CAxisSegmentFeatures&&) noexcept = delete;
  CAxisSegmentFeatures& operator=(const CAxisSegmentFeatures&) = delete;
  CAxisSegmentFeatures& operator=(CAxisSegmentFeatures&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const CAxisSegmentFeaturesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
