#pragma once

#include "VolumeMeshing/VolumeMeshing_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  VMFindDistanceFieldFromTriangleGeometryInputValues inputValues;

  inputValues.DistanceFieldType = filterArgs.value<ChoicesParameter::ValueType>(k_DistanceFieldType_Key);
  inputValues.Spacing = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Spacing_Key);
  inputValues.StoreClosestTriangle = filterArgs.value<bool>(k_StoreClosestTriangle_Key);
  inputValues.TriangleDataContainerName = filterArgs.value<DataPath>(k_TriangleDataContainerName_Key);
  inputValues.ImageDataContainerName = filterArgs.value<DataPath>(k_ImageDataContainerName_Key);
  inputValues.CellAttributeMatrixName = filterArgs.value<StringParameter::ValueType>(k_CellAttributeMatrixName_Key);
  inputValues.SignedDistanceFieldName = filterArgs.value<StringParameter::ValueType>(k_SignedDistanceFieldName_Key);
  inputValues.ClosestTriangleName = filterArgs.value<StringParameter::ValueType>(k_ClosestTriangleName_Key);

  return VMFindDistanceFieldFromTriangleGeometry(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct VOLUMEMESHING_EXPORT VMFindDistanceFieldFromTriangleGeometryInputValues
{
  ChoicesParameter::ValueType DistanceFieldType;
  VectorFloat32Parameter::ValueType Spacing;
  bool StoreClosestTriangle;
  DataPath TriangleDataContainerName;
  DataPath ImageDataContainerName;
  StringParameter::ValueType CellAttributeMatrixName;
  StringParameter::ValueType SignedDistanceFieldName;
  StringParameter::ValueType ClosestTriangleName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class VOLUMEMESHING_EXPORT VMFindDistanceFieldFromTriangleGeometry
{
public:
  VMFindDistanceFieldFromTriangleGeometry(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                          VMFindDistanceFieldFromTriangleGeometryInputValues* inputValues);
  ~VMFindDistanceFieldFromTriangleGeometry() noexcept;

  VMFindDistanceFieldFromTriangleGeometry(const VMFindDistanceFieldFromTriangleGeometry&) = delete;
  VMFindDistanceFieldFromTriangleGeometry(VMFindDistanceFieldFromTriangleGeometry&&) noexcept = delete;
  VMFindDistanceFieldFromTriangleGeometry& operator=(const VMFindDistanceFieldFromTriangleGeometry&) = delete;
  VMFindDistanceFieldFromTriangleGeometry& operator=(VMFindDistanceFieldFromTriangleGeometry&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const VMFindDistanceFieldFromTriangleGeometryInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
