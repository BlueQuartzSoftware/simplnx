#pragma once

#include "VolumeMeshing/VolumeMeshing_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  VMFillLevelSetWithTetrahedraInputValues inputValues;

  inputValues.AutoDetectFeatures = filterArgs.value<bool>(k_AutoDetectFeatures_Key);
  inputValues.AngleThreshold = filterArgs.value<float64>(k_AngleThreshold_Key);
  inputValues.TriangleDataContainerName = filterArgs.value<DataPath>(k_TriangleDataContainerName_Key);
  inputValues.LevelSetArrayPath = filterArgs.value<DataPath>(k_LevelSetArrayPath_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.TetrahedralDataContainerName = filterArgs.value<DataPath>(k_TetrahedralDataContainerName_Key);
  inputValues.VertexAttributeMatrixName = filterArgs.value<StringParameter::ValueType>(k_VertexAttributeMatrixName_Key);
  inputValues.CellAttributeMatrixName = filterArgs.value<StringParameter::ValueType>(k_CellAttributeMatrixName_Key);
  inputValues.TetFeatureIdsName = filterArgs.value<StringParameter::ValueType>(k_TetFeatureIdsName_Key);

  return VMFillLevelSetWithTetrahedra(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct VOLUMEMESHING_EXPORT VMFillLevelSetWithTetrahedraInputValues
{
  bool AutoDetectFeatures;
  float64 AngleThreshold;
  DataPath TriangleDataContainerName;
  DataPath LevelSetArrayPath;
  DataPath FeatureIdsArrayPath;
  DataPath TetrahedralDataContainerName;
  StringParameter::ValueType VertexAttributeMatrixName;
  StringParameter::ValueType CellAttributeMatrixName;
  StringParameter::ValueType TetFeatureIdsName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class VOLUMEMESHING_EXPORT VMFillLevelSetWithTetrahedra
{
public:
  VMFillLevelSetWithTetrahedra(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, VMFillLevelSetWithTetrahedraInputValues* inputValues);
  ~VMFillLevelSetWithTetrahedra() noexcept;

  VMFillLevelSetWithTetrahedra(const VMFillLevelSetWithTetrahedra&) = delete;
  VMFillLevelSetWithTetrahedra(VMFillLevelSetWithTetrahedra&&) noexcept = delete;
  VMFillLevelSetWithTetrahedra& operator=(const VMFillLevelSetWithTetrahedra&) = delete;
  VMFillLevelSetWithTetrahedra& operator=(VMFillLevelSetWithTetrahedra&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const VMFillLevelSetWithTetrahedraInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
