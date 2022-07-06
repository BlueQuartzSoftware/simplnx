#pragma once

#include "DREAM3DReview/DREAM3DReview_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ExtractInternalSurfacesFromTriangleGeometryInputValues inputValues;

  inputValues.TriangleDataContainerName = filterArgs.value<DataPath>(k_TriangleDataContainerName_Key);
  inputValues.NodeTypesArrayPath = filterArgs.value<DataPath>(k_NodeTypesArrayPath_Key);
  inputValues.InternalTrianglesName = filterArgs.value<StringParameter::ValueType>(k_InternalTrianglesName_Key);

  return ExtractInternalSurfacesFromTriangleGeometry(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct DREAM3DREVIEW_EXPORT ExtractInternalSurfacesFromTriangleGeometryInputValues
{
  DataPath TriangleDataContainerName;
  DataPath NodeTypesArrayPath;
  StringParameter::ValueType InternalTrianglesName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class DREAM3DREVIEW_EXPORT ExtractInternalSurfacesFromTriangleGeometry
{
public:
  ExtractInternalSurfacesFromTriangleGeometry(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                              ExtractInternalSurfacesFromTriangleGeometryInputValues* inputValues);
  ~ExtractInternalSurfacesFromTriangleGeometry() noexcept;

  ExtractInternalSurfacesFromTriangleGeometry(const ExtractInternalSurfacesFromTriangleGeometry&) = delete;
  ExtractInternalSurfacesFromTriangleGeometry(ExtractInternalSurfacesFromTriangleGeometry&&) noexcept = delete;
  ExtractInternalSurfacesFromTriangleGeometry& operator=(const ExtractInternalSurfacesFromTriangleGeometry&) = delete;
  ExtractInternalSurfacesFromTriangleGeometry& operator=(ExtractInternalSurfacesFromTriangleGeometry&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ExtractInternalSurfacesFromTriangleGeometryInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
