#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

/**
* This is example code to put in the Execute Method of the filter.


*/

namespace nx::core
{

struct SIMPLNXCORE_EXPORT ExtractInternalSurfacesFromTriangleGeometryInputValues
{
  MultiArraySelectionParameter::ValueType CopyTriangleArrayPaths;
  MultiArraySelectionParameter::ValueType CopyVertexArrayPaths;
  GeometrySelectionParameter::ValueType InputTriangleGeometryPath;
  VectorInt8Parameter::ValueType NodeTypeRange;
  ArraySelectionParameter::ValueType NodeTypesPath;
  DataGroupCreationParameter::ValueType OutputTriangleGeometryPath;
  DataObjectNameParameter::ValueType TriangleAttributeMatrixName;
  DataObjectNameParameter::ValueType VertexAttributeMatrixName;
};

/**
 * @class ExtractInternalSurfacesFromTriangleGeometry
 * @brief This algorithm implements support code for the ExtractInternalSurfacesFromTriangleGeometryFilter
 */

class SIMPLNXCORE_EXPORT ExtractInternalSurfacesFromTriangleGeometry
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

private:
  DataStructure& m_DataStructure;
  const ExtractInternalSurfacesFromTriangleGeometryInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
