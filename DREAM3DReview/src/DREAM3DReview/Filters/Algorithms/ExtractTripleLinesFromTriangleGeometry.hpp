#pragma once

#include "DREAM3DReview/DREAM3DReview_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ExtractTripleLinesFromTriangleGeometryInputValues inputValues;

  inputValues.SmoothTripleLines = filterArgs.value<bool>(k_SmoothTripleLines_Key);
  inputValues.NodeTypesArrayPath = filterArgs.value<DataPath>(k_NodeTypesArrayPath_Key);
  inputValues.EdgeGeometry = filterArgs.value<StringParameter::ValueType>(k_EdgeGeometry_Key);
  inputValues.VertexAttributeMatrixName = filterArgs.value<StringParameter::ValueType>(k_VertexAttributeMatrixName_Key);
  inputValues.NodeTypesArrayName = filterArgs.value<StringParameter::ValueType>(k_NodeTypesArrayName_Key);
  inputValues.EdgeAttributeMatrixName = filterArgs.value<StringParameter::ValueType>(k_EdgeAttributeMatrixName_Key);

  return ExtractTripleLinesFromTriangleGeometry(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct DREAM3DREVIEW_EXPORT ExtractTripleLinesFromTriangleGeometryInputValues
{
  bool SmoothTripleLines;
  DataPath NodeTypesArrayPath;
  StringParameter::ValueType EdgeGeometry;
  StringParameter::ValueType VertexAttributeMatrixName;
  StringParameter::ValueType NodeTypesArrayName;
  StringParameter::ValueType EdgeAttributeMatrixName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class DREAM3DREVIEW_EXPORT ExtractTripleLinesFromTriangleGeometry
{
public:
  ExtractTripleLinesFromTriangleGeometry(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                         ExtractTripleLinesFromTriangleGeometryInputValues* inputValues);
  ~ExtractTripleLinesFromTriangleGeometry() noexcept;

  ExtractTripleLinesFromTriangleGeometry(const ExtractTripleLinesFromTriangleGeometry&) = delete;
  ExtractTripleLinesFromTriangleGeometry(ExtractTripleLinesFromTriangleGeometry&&) noexcept = delete;
  ExtractTripleLinesFromTriangleGeometry& operator=(const ExtractTripleLinesFromTriangleGeometry&) = delete;
  ExtractTripleLinesFromTriangleGeometry& operator=(ExtractTripleLinesFromTriangleGeometry&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ExtractTripleLinesFromTriangleGeometryInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
