#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry2D.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT SliceTriangleGeometryInputValues
{
  // VectorFloat32Parameter::ValueType SliceDirection;
  ChoicesParameter::ValueType SliceRange;
  float32 Zstart;
  float32 Zend;
  float32 SliceResolution;
  bool HaveRegionIds;
  DataPath CADDataContainerName;
  DataPath RegionIdArrayPath;
  DataGroupCreationParameter::ValueType SliceDataContainerName;
  DataObjectNameParameter::ValueType EdgeAttributeMatrixName;
  DataObjectNameParameter::ValueType SliceIdArrayName;
  DataObjectNameParameter::ValueType SliceAttributeMatrixName;
};

/**
 * @class SliceTriangleGeometry
 * @brief This filter slices an input Triangle Geometry, producing an Edge Geometry
 */

class SIMPLNXCORE_EXPORT SliceTriangleGeometry
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

protected:
  char rayIntersectsPlane(float32 d, const std::array<float32, 3>& q, const std::array<float32, 3>& r, std::array<float32, 3>& p);
  usize determineBoundsAndNumSlices(float32& minDim, float32& maxDim, usize numTris, INodeGeometry2D::SharedFaceList& tris, INodeGeometry0D::SharedVertexList& triVerts);

private:
  DataStructure& m_DataStructure;
  const SliceTriangleGeometryInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
