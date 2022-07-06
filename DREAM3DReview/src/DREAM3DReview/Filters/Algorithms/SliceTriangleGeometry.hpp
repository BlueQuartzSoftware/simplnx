#pragma once

#include "DREAM3DReview/DREAM3DReview_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  SliceTriangleGeometryInputValues inputValues;

  inputValues.SliceDirection = filterArgs.value<VectorFloat32Parameter::ValueType>(k_SliceDirection_Key);
  inputValues.SliceRange = filterArgs.value<ChoicesParameter::ValueType>(k_SliceRange_Key);
  inputValues.Zstart = filterArgs.value<float32>(k_Zstart_Key);
  inputValues.Zend = filterArgs.value<float32>(k_Zend_Key);
  inputValues.SliceResolution = filterArgs.value<float32>(k_SliceResolution_Key);
  inputValues.HaveRegionIds = filterArgs.value<bool>(k_HaveRegionIds_Key);
  inputValues.CADDataContainerName = filterArgs.value<DataPath>(k_CADDataContainerName_Key);
  inputValues.RegionIdArrayPath = filterArgs.value<DataPath>(k_RegionIdArrayPath_Key);
  inputValues.SliceDataContainerName = filterArgs.value<StringParameter::ValueType>(k_SliceDataContainerName_Key);
  inputValues.EdgeAttributeMatrixName = filterArgs.value<DataPath>(k_EdgeAttributeMatrixName_Key);
  inputValues.SliceIdArrayName = filterArgs.value<DataPath>(k_SliceIdArrayName_Key);
  inputValues.SliceAttributeMatrixName = filterArgs.value<DataPath>(k_SliceAttributeMatrixName_Key);

  return SliceTriangleGeometry(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct DREAM3DREVIEW_EXPORT SliceTriangleGeometryInputValues
{
  VectorFloat32Parameter::ValueType SliceDirection;
  ChoicesParameter::ValueType SliceRange;
  float32 Zstart;
  float32 Zend;
  float32 SliceResolution;
  bool HaveRegionIds;
  DataPath CADDataContainerName;
  DataPath RegionIdArrayPath;
  StringParameter::ValueType SliceDataContainerName;
  DataPath EdgeAttributeMatrixName;
  DataPath SliceIdArrayName;
  DataPath SliceAttributeMatrixName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class DREAM3DREVIEW_EXPORT SliceTriangleGeometry
{
public:
  SliceTriangleGeometry(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, SliceTriangleGeometryInputValues* inputValues);
  ~SliceTriangleGeometry() noexcept;

  SliceTriangleGeometry(const SliceTriangleGeometry&) = delete;
  SliceTriangleGeometry(SliceTriangleGeometry&&) noexcept = delete;
  SliceTriangleGeometry& operator=(const SliceTriangleGeometry&) = delete;
  SliceTriangleGeometry& operator=(SliceTriangleGeometry&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const SliceTriangleGeometryInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
