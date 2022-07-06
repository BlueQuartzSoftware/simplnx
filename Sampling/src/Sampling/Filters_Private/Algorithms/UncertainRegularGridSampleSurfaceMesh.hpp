#pragma once

#include "Sampling/Sampling_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  UncertainRegularGridSampleSurfaceMeshInputValues inputValues;

  inputValues.SurfaceMeshFaceLabelsArrayPath = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  inputValues.XPoints = filterArgs.value<int32>(k_XPoints_Key);
  inputValues.YPoints = filterArgs.value<int32>(k_YPoints_Key);
  inputValues.ZPoints = filterArgs.value<int32>(k_ZPoints_Key);
  inputValues.Spacing = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Spacing_Key);
  inputValues.Origin = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Origin_Key);
  inputValues.Uncertainty = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Uncertainty_Key);
  inputValues.DataContainerName = filterArgs.value<DataPath>(k_DataContainerName_Key);
  inputValues.CellAttributeMatrixName = filterArgs.value<DataPath>(k_CellAttributeMatrixName_Key);
  inputValues.FeatureIdsArrayName = filterArgs.value<DataPath>(k_FeatureIdsArrayName_Key);

  return UncertainRegularGridSampleSurfaceMesh(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct SAMPLING_EXPORT UncertainRegularGridSampleSurfaceMeshInputValues
{
  DataPath SurfaceMeshFaceLabelsArrayPath;
  int32 XPoints;
  int32 YPoints;
  int32 ZPoints;
  VectorFloat32Parameter::ValueType Spacing;
  VectorFloat32Parameter::ValueType Origin;
  VectorFloat32Parameter::ValueType Uncertainty;
  DataPath DataContainerName;
  DataPath CellAttributeMatrixName;
  DataPath FeatureIdsArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class SAMPLING_EXPORT UncertainRegularGridSampleSurfaceMesh
{
public:
  UncertainRegularGridSampleSurfaceMesh(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                        UncertainRegularGridSampleSurfaceMeshInputValues* inputValues);
  ~UncertainRegularGridSampleSurfaceMesh() noexcept;

  UncertainRegularGridSampleSurfaceMesh(const UncertainRegularGridSampleSurfaceMesh&) = delete;
  UncertainRegularGridSampleSurfaceMesh(UncertainRegularGridSampleSurfaceMesh&&) noexcept = delete;
  UncertainRegularGridSampleSurfaceMesh& operator=(const UncertainRegularGridSampleSurfaceMesh&) = delete;
  UncertainRegularGridSampleSurfaceMesh& operator=(UncertainRegularGridSampleSurfaceMesh&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const UncertainRegularGridSampleSurfaceMeshInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
