#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

namespace nx::core
{
struct SIMPLNXCORE_EXPORT RegularGridSampleSurfaceMeshInputValues
{
  VectorUInt64Parameter::ValueType Dimensions;
  VectorFloat32Parameter::ValueType Spacing;
  VectorFloat32Parameter::ValueType Origin;
  DataPath TriangleGeometryPath;
  DataPath SurfaceMeshFaceLabelsArrayPath;
  DataPath ImageGeometryOutputPath;
  DataPath FeatureIdsArrayPath;
};

/**
 * @class RegularGridSampleSurfaceMesh
 * @brief Samples a TriangleGeometry onto a regular grid (ImageGeom) using
 * scanline rasterization. For each Z-slice of the grid, triangles are
 * intersected with the Z-plane to produce 2D edges. Each Y-scanline then
 * finds X-intersections with those edges, sorts them, and fills voxels
 * between crossings using face label toggling to assign feature IDs.
 */
class SIMPLNXCORE_EXPORT RegularGridSampleSurfaceMesh
{
public:
  RegularGridSampleSurfaceMesh(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, RegularGridSampleSurfaceMeshInputValues* inputValues);
  ~RegularGridSampleSurfaceMesh() noexcept;

  RegularGridSampleSurfaceMesh(const RegularGridSampleSurfaceMesh&) = delete;
  RegularGridSampleSurfaceMesh(RegularGridSampleSurfaceMesh&&) noexcept = delete;
  RegularGridSampleSurfaceMesh& operator=(const RegularGridSampleSurfaceMesh&) = delete;
  RegularGridSampleSurfaceMesh& operator=(RegularGridSampleSurfaceMesh&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const RegularGridSampleSurfaceMeshInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};
} // namespace nx::core
