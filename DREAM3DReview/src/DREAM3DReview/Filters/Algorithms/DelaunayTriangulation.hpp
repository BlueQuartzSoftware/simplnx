#pragma once

#include "DREAM3DReview/DREAM3DReview_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  DelaunayTriangulationInputValues inputValues;

  inputValues.Offset = filterArgs.value<float64>(k_Offset_Key);
  inputValues.Tolerance = filterArgs.value<float64>(k_Tolerance_Key);
  inputValues.TriangulateByFeature = filterArgs.value<bool>(k_TriangulateByFeature_Key);
  inputValues.InputGeometry = filterArgs.value<DataPath>(k_InputGeometry_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.TriangleDataContainerName = filterArgs.value<StringParameter::ValueType>(k_TriangleDataContainerName_Key);
  inputValues.VertexAttributeMatrixName = filterArgs.value<StringParameter::ValueType>(k_VertexAttributeMatrixName_Key);
  inputValues.FaceAttributeMatrixName = filterArgs.value<StringParameter::ValueType>(k_FaceAttributeMatrixName_Key);

  return DelaunayTriangulation(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct DREAM3DREVIEW_EXPORT DelaunayTriangulationInputValues
{
  float64 Offset;
  float64 Tolerance;
  bool TriangulateByFeature;
  DataPath InputGeometry;
  DataPath FeatureIdsArrayPath;
  StringParameter::ValueType TriangleDataContainerName;
  StringParameter::ValueType VertexAttributeMatrixName;
  StringParameter::ValueType FaceAttributeMatrixName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class DREAM3DREVIEW_EXPORT DelaunayTriangulation
{
public:
  DelaunayTriangulation(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, DelaunayTriangulationInputValues* inputValues);
  ~DelaunayTriangulation() noexcept;

  DelaunayTriangulation(const DelaunayTriangulation&) = delete;
  DelaunayTriangulation(DelaunayTriangulation&&) noexcept = delete;
  DelaunayTriangulation& operator=(const DelaunayTriangulation&) = delete;
  DelaunayTriangulation& operator=(DelaunayTriangulation&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const DelaunayTriangulationInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
