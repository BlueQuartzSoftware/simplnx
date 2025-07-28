#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  TriangleDihedralAngleInputValues inputValues;
  inputValues.InputTriangleGeometryPath = filterArgs.value<GeometrySelectionParameter::ValueType>(input_triangle_geometry_path);
  inputValues.SurfaceMeshTriangleDihedralAnglesArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(surface_mesh_triangle_dihedral_angles_array_name);
  return TriangleDihedralAngle(dataStructure, messageHandler, shouldCancel, &inputValues)();

*/

namespace nx::core
{

struct SIMPLNXCORE_EXPORT TriangleDihedralAngleInputValues
{
  GeometrySelectionParameter::ValueType InputTriangleGeometryPath;
  DataObjectNameParameter::ValueType SurfaceMeshTriangleDihedralAnglesArrayName;
};

/**
 * @class TriangleDihedralAngle
 * @brief This algorithm implements support code for the TriangleDihedralAngleFilter
 */

class SIMPLNXCORE_EXPORT TriangleDihedralAngle
{
public:
  TriangleDihedralAngle(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, TriangleDihedralAngleInputValues* inputValues);
  ~TriangleDihedralAngle() noexcept;

  TriangleDihedralAngle(const TriangleDihedralAngle&) = delete;
  TriangleDihedralAngle(TriangleDihedralAngle&&) noexcept = delete;
  TriangleDihedralAngle& operator=(const TriangleDihedralAngle&) = delete;
  TriangleDihedralAngle& operator=(TriangleDihedralAngle&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const TriangleDihedralAngleInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
