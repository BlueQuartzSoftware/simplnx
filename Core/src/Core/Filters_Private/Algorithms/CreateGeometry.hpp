#pragma once

#include "Core/Core_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  CreateGeometryInputValues inputValues;

  inputValues.GeometryType = filterArgs.value<ChoicesParameter::ValueType>(k_GeometryType_Key);
  inputValues.TreatWarningsAsErrors = filterArgs.value<bool>(k_TreatWarningsAsErrors_Key);
  inputValues.ArrayHandling = filterArgs.value<ChoicesParameter::ValueType>(k_ArrayHandling_Key);
  inputValues.DataContainerName = filterArgs.value<DataPath>(k_DataContainerName_Key);
  inputValues.Dimensions = filterArgs.value<VectorInt32Parameter::ValueType>(k_Dimensions_Key);
  inputValues.Origin = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Origin_Key);
  inputValues.Spacing = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Spacing_Key);
  inputValues.ImageCellAttributeMatrixName = filterArgs.value<DataPath>(k_ImageCellAttributeMatrixName_Key);
  inputValues.XBoundsArrayPath = filterArgs.value<DataPath>(k_XBoundsArrayPath_Key);
  inputValues.YBoundsArrayPath = filterArgs.value<DataPath>(k_YBoundsArrayPath_Key);
  inputValues.ZBoundsArrayPath = filterArgs.value<DataPath>(k_ZBoundsArrayPath_Key);
  inputValues.RectGridCellAttributeMatrixName = filterArgs.value<DataPath>(k_RectGridCellAttributeMatrixName_Key);
  inputValues.BoxDimensions = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_BoxDimensions_Key);
  inputValues.SharedVertexListArrayPath0 = filterArgs.value<DataPath>(k_SharedVertexListArrayPath0_Key);
  inputValues.VertexAttributeMatrixName0 = filterArgs.value<DataPath>(k_VertexAttributeMatrixName0_Key);
  inputValues.SharedVertexListArrayPath1 = filterArgs.value<DataPath>(k_SharedVertexListArrayPath1_Key);
  inputValues.SharedEdgeListArrayPath = filterArgs.value<DataPath>(k_SharedEdgeListArrayPath_Key);
  inputValues.VertexAttributeMatrixName1 = filterArgs.value<DataPath>(k_VertexAttributeMatrixName1_Key);
  inputValues.EdgeAttributeMatrixName = filterArgs.value<DataPath>(k_EdgeAttributeMatrixName_Key);
  inputValues.SharedVertexListArrayPath2 = filterArgs.value<DataPath>(k_SharedVertexListArrayPath2_Key);
  inputValues.SharedTriListArrayPath = filterArgs.value<DataPath>(k_SharedTriListArrayPath_Key);
  inputValues.VertexAttributeMatrixName2 = filterArgs.value<DataPath>(k_VertexAttributeMatrixName2_Key);
  inputValues.FaceAttributeMatrixName0 = filterArgs.value<DataPath>(k_FaceAttributeMatrixName0_Key);
  inputValues.SharedVertexListArrayPath3 = filterArgs.value<DataPath>(k_SharedVertexListArrayPath3_Key);
  inputValues.SharedQuadListArrayPath = filterArgs.value<DataPath>(k_SharedQuadListArrayPath_Key);
  inputValues.VertexAttributeMatrixName3 = filterArgs.value<DataPath>(k_VertexAttributeMatrixName3_Key);
  inputValues.FaceAttributeMatrixName1 = filterArgs.value<DataPath>(k_FaceAttributeMatrixName1_Key);
  inputValues.SharedVertexListArrayPath4 = filterArgs.value<DataPath>(k_SharedVertexListArrayPath4_Key);
  inputValues.SharedTetListArrayPath = filterArgs.value<DataPath>(k_SharedTetListArrayPath_Key);
  inputValues.VertexAttributeMatrixName4 = filterArgs.value<DataPath>(k_VertexAttributeMatrixName4_Key);
  inputValues.TetCellAttributeMatrixName = filterArgs.value<DataPath>(k_TetCellAttributeMatrixName_Key);
  inputValues.SharedVertexListArrayPath5 = filterArgs.value<DataPath>(k_SharedVertexListArrayPath5_Key);
  inputValues.SharedHexListArrayPath = filterArgs.value<DataPath>(k_SharedHexListArrayPath_Key);
  inputValues.VertexAttributeMatrixName5 = filterArgs.value<DataPath>(k_VertexAttributeMatrixName5_Key);
  inputValues.HexCellAttributeMatrixName = filterArgs.value<DataPath>(k_HexCellAttributeMatrixName_Key);

  return CreateGeometry(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct CORE_EXPORT CreateGeometryInputValues
{
  ChoicesParameter::ValueType GeometryType;
  bool TreatWarningsAsErrors;
  ChoicesParameter::ValueType ArrayHandling;
  DataPath DataContainerName;
  VectorInt32Parameter::ValueType Dimensions;
  VectorFloat32Parameter::ValueType Origin;
  VectorFloat32Parameter::ValueType Spacing;
  DataPath ImageCellAttributeMatrixName;
  DataPath XBoundsArrayPath;
  DataPath YBoundsArrayPath;
  DataPath ZBoundsArrayPath;
  DataPath RectGridCellAttributeMatrixName;
  <<<NOT_IMPLEMENTED>>> BoxDimensions;
  DataPath SharedVertexListArrayPath0;
  DataPath VertexAttributeMatrixName0;
  DataPath SharedVertexListArrayPath1;
  DataPath SharedEdgeListArrayPath;
  DataPath VertexAttributeMatrixName1;
  DataPath EdgeAttributeMatrixName;
  DataPath SharedVertexListArrayPath2;
  DataPath SharedTriListArrayPath;
  DataPath VertexAttributeMatrixName2;
  DataPath FaceAttributeMatrixName0;
  DataPath SharedVertexListArrayPath3;
  DataPath SharedQuadListArrayPath;
  DataPath VertexAttributeMatrixName3;
  DataPath FaceAttributeMatrixName1;
  DataPath SharedVertexListArrayPath4;
  DataPath SharedTetListArrayPath;
  DataPath VertexAttributeMatrixName4;
  DataPath TetCellAttributeMatrixName;
  DataPath SharedVertexListArrayPath5;
  DataPath SharedHexListArrayPath;
  DataPath VertexAttributeMatrixName5;
  DataPath HexCellAttributeMatrixName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class CORE_EXPORT CreateGeometry
{
public:
  CreateGeometry(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CreateGeometryInputValues* inputValues);
  ~CreateGeometry() noexcept;

  CreateGeometry(const CreateGeometry&) = delete;
  CreateGeometry(CreateGeometry&&) noexcept = delete;
  CreateGeometry& operator=(const CreateGeometry&) = delete;
  CreateGeometry& operator=(CreateGeometry&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const CreateGeometryInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
