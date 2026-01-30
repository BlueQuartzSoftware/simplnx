#include "AlignGeometries.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry2D.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry3D.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/RectGridGeom.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"

using namespace nx::core;

namespace
{
FloatVec3 extractOrigin(const IGeometry& geometry)
{
  auto geomType = geometry.getGeomType();
  switch(geomType)
  {
  case IGeometry::Type::Image: {
    auto& imageGeom = dynamic_cast<const ImageGeom&>(geometry);
    return imageGeom.getOrigin();
  }
  case IGeometry::Type::RectGrid: {
    auto& rectGridGeom = dynamic_cast<const RectGridGeom&>(geometry);
    const auto& xBoundsRef = rectGridGeom.getXBounds()->getDataStoreRef();
    const auto& yBoundsRef = rectGridGeom.getYBounds()->getDataStoreRef();
    const auto& zBoundsRef = rectGridGeom.getZBounds()->getDataStoreRef();
    FloatVec3 origin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    for(size_t i = 0; i < xBoundsRef.getNumberOfTuples(); i++)
    {
      if(xBoundsRef[i] < origin[0])
      {
        origin[0] = xBoundsRef[i];
      }
    }
    for(size_t i = 0; i < yBoundsRef.getNumberOfTuples(); i++)
    {
      if(yBoundsRef[i] < origin[1])
      {
        origin[1] = yBoundsRef[i];
      }
    }
    for(size_t i = 0; i < zBoundsRef.getNumberOfTuples(); i++)
    {
      if(zBoundsRef[i] < origin[2])
      {
        origin[2] = zBoundsRef[i];
      }
    }
    return origin;
  }
  case IGeometry::Type::Vertex: {
    auto& vertexGeom = dynamic_cast<const VertexGeom&>(geometry);
    const auto& verticesRef = vertexGeom.getVertices()->getDataStoreRef();
    FloatVec3 origin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());

    for(size_t i = 0; i < vertexGeom.getNumberOfVertices(); i++)
    {
      for(size_t j = 0; j < 3; j++)
      {
        if(verticesRef[3 * i + j] < origin[j])
        {
          origin[j] = verticesRef[3 * i + j];
        }
      }
    }
    return origin;
  }
  case IGeometry::Type::Edge: {
    const auto& edgeGeom = dynamic_cast<const EdgeGeom&>(geometry);
    const auto& verticesRef = edgeGeom.getVertices()->getDataStoreRef();
    FloatVec3 origin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());

    for(size_t i = 0; i < edgeGeom.getNumberOfVertices(); i++)
    {
      for(size_t j = 0; j < 3; j++)
      {
        if(verticesRef[3 * i + j] < origin[j])
        {
          origin[j] = verticesRef[3 * i + j];
        }
      }
    }
    return origin;
  }
  // 2D
  case IGeometry::Type::Triangle:
    [[fallthrough]];
  case IGeometry::Type::Quad: {
    const auto& geometry2dGeom = dynamic_cast<const INodeGeometry2D&>(geometry);
    const auto& verticesRef = geometry2dGeom.getVertices()->getDataStoreRef();
    FloatVec3 origin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());

    for(size_t i = 0; i < geometry2dGeom.getNumberOfVertices(); i++)
    {
      for(size_t j = 0; j < 3; j++)
      {
        if(verticesRef[3 * i + j] < origin[j])
        {
          origin[j] = verticesRef[3 * i + j];
        }
      }
    }
    return origin;
  }
  // 3D
  case IGeometry::Type::Hexahedral:
    [[fallthrough]];
  case IGeometry::Type::Tetrahedral: {
    const auto& geometry3dGeom = dynamic_cast<const INodeGeometry3D&>(geometry);
    const auto& verticesRef = geometry3dGeom.getVertices()->getDataStoreRef();
    FloatVec3 origin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());

    for(size_t i = 0; i < geometry3dGeom.getNumberOfVertices(); i++)
    {
      for(size_t j = 0; j < 3; j++)
      {
        if(verticesRef[3 * i + j] < origin[j])
        {
          origin[j] = verticesRef[3 * i + j];
        }
      }
    }
    return origin;
  }
  default:
    break;
  }

  FloatVec3 origin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
  return origin;
}

FloatVec3 extractCentroid(const IGeometry& geometry)
{
  FloatVec3 centroid(0.0f, 0.0f, 0.0f);
  switch(geometry.getGeomType())
  {
  case IGeometry::Type::Image: {
    const auto& imageGeom = dynamic_cast<const ImageGeom&>(geometry);
    SizeVec3 dims = imageGeom.getDimensions();
    FloatVec3 origin = imageGeom.getOrigin();
    FloatVec3 res = imageGeom.getSpacing();

    centroid[0] = (static_cast<float>(dims[0]) * res[0] / 2.0f) + origin[0];
    centroid[1] = (static_cast<float>(dims[1]) * res[1] / 2.0f) + origin[1];
    centroid[2] = (static_cast<float>(dims[2]) * res[2] / 2.0f) + origin[2];
    return centroid;
  }
  case IGeometry::Type::RectGrid: {
    const auto& rectGridGeom = dynamic_cast<const RectGridGeom&>(geometry);
    const auto& xBoundsRef = rectGridGeom.getXBounds()->getDataStoreRef();
    const auto& yBoundsRef = rectGridGeom.getYBounds()->getDataStoreRef();
    const auto& zBoundsRef = rectGridGeom.getZBounds()->getDataStoreRef();
    float min[3] = {std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
    float max[3] = {std::numeric_limits<float>::min(), std::numeric_limits<float>::min(), std::numeric_limits<float>::min()};
    for(size_t i = 0; i < xBoundsRef.getNumberOfTuples(); i++)
    {
      if(xBoundsRef[i] < min[0])
      {
        min[0] = xBoundsRef[i];
      }
      if(xBoundsRef[i] > max[0])
      {
        max[0] = xBoundsRef[i];
      }
    }
    for(size_t i = 0; i < yBoundsRef.getNumberOfTuples(); i++)
    {
      if(yBoundsRef[i] < min[1])
      {
        min[1] = yBoundsRef[i];
      }
      if(yBoundsRef[i] > max[1])
      {
        max[1] = yBoundsRef[i];
      }
    }
    for(size_t i = 0; i < zBoundsRef.getNumberOfTuples(); i++)
    {
      if(zBoundsRef[i] < min[2])
      {
        min[2] = zBoundsRef[i];
      }
      if(zBoundsRef[i] > max[2])
      {
        max[2] = zBoundsRef[i];
      }
    }
    centroid[0] = (max[0] - min[0]) / 2.0f;
    centroid[1] = (max[1] - min[1]) / 2.0f;
    centroid[2] = (max[2] - min[2]) / 2.0f;
    return centroid;
  }
  case IGeometry::Type::Vertex: {
    const auto& vertexGeom = dynamic_cast<const VertexGeom&>(geometry);
    const auto& verticesRef = vertexGeom.getVertices()->getDataStoreRef();
    centroid[0] = 0.0f;
    centroid[1] = 0.0f;
    centroid[2] = 0.0f;
    for(size_t i = 0; i < vertexGeom.getNumberOfVertices(); i++)
    {
      centroid[0] += verticesRef[3 * i + 0];
      centroid[1] += verticesRef[3 * i + 1];
      centroid[2] += verticesRef[3 * i + 2];
    }
    centroid[0] /= static_cast<float>(vertexGeom.getNumberOfVertices());
    centroid[1] /= static_cast<float>(vertexGeom.getNumberOfVertices());
    centroid[2] /= static_cast<float>(vertexGeom.getNumberOfVertices());
    return centroid;
  }
  case IGeometry::Type::Edge: {
    const auto& edgeGeom = dynamic_cast<const EdgeGeom&>(geometry);
    const auto& verticesRef = edgeGeom.getVertices()->getDataStoreRef();
    centroid[0] = 0.0f;
    centroid[1] = 0.0f;
    centroid[2] = 0.0f;
    for(size_t i = 0; i < edgeGeom.getNumberOfVertices(); i++)
    {
      centroid[0] += verticesRef[3 * i + 0];
      centroid[1] += verticesRef[3 * i + 1];
      centroid[2] += verticesRef[3 * i + 2];
    }
    centroid[0] /= static_cast<float>(edgeGeom.getNumberOfVertices());
    centroid[1] /= static_cast<float>(edgeGeom.getNumberOfVertices());
    centroid[2] /= static_cast<float>(edgeGeom.getNumberOfVertices());
    return centroid;
  }
    // 2D Types
  case IGeometry::Type::Triangle:
    [[fallthrough]];
  case IGeometry::Type::Quad: {
    auto& geometry2dGeom = dynamic_cast<const INodeGeometry2D&>(geometry);
    const auto& verticesRef = geometry2dGeom.getVertices()->getDataStoreRef();
    centroid[0] = 0.0f;
    centroid[1] = 0.0f;
    centroid[2] = 0.0f;
    for(size_t i = 0; i < geometry2dGeom.getNumberOfVertices(); i++)
    {
      centroid[0] += verticesRef[3 * i + 0];
      centroid[1] += verticesRef[3 * i + 1];
      centroid[2] += verticesRef[3 * i + 2];
    }
    centroid[0] /= static_cast<float>(geometry2dGeom.getNumberOfVertices());
    centroid[1] /= static_cast<float>(geometry2dGeom.getNumberOfVertices());
    centroid[2] /= static_cast<float>(geometry2dGeom.getNumberOfVertices());
    return centroid;
  }
    // 3D Types
  case IGeometry::Type::Hexahedral:
    [[fallthrough]];
  case IGeometry::Type::Tetrahedral: {
    const auto& geometry3dGeom = dynamic_cast<const INodeGeometry3D&>(geometry);
    const auto& verticesRef = geometry3dGeom.getVertices()->getDataStoreRef();
    centroid[0] = 0.0f;
    centroid[1] = 0.0f;
    centroid[2] = 0.0f;
    for(size_t i = 0; i < geometry3dGeom.getNumberOfVertices(); i++)
    {
      centroid[0] += verticesRef[3 * i + 0];
      centroid[1] += verticesRef[3 * i + 1];
      centroid[2] += verticesRef[3 * i + 2];
    }
    centroid[0] /= static_cast<float>(geometry3dGeom.getNumberOfVertices());
    centroid[1] /= static_cast<float>(geometry3dGeom.getNumberOfVertices());
    centroid[2] /= static_cast<float>(geometry3dGeom.getNumberOfVertices());
    return centroid;
  }
  }

  return centroid;
}

void translateGeometry(IGeometry& geometry, const FloatVec3& translation)
{
  switch(geometry.getGeomType())
  {
  case IGeometry::Type::Image: {
    auto& imageGeom = dynamic_cast<ImageGeom&>(geometry);
    FloatVec3 origin = imageGeom.getOrigin();
    origin[0] += translation[0];
    origin[1] += translation[1];
    origin[2] += translation[2];
    imageGeom.setOrigin(origin);
    return;
  }
  case IGeometry::Type::RectGrid: {
    auto& rectGridGeom = dynamic_cast<RectGridGeom&>(geometry);
    auto& xBoundsRef = rectGridGeom.getXBounds()->getDataStoreRef();
    auto& yBoundsRef = rectGridGeom.getYBounds()->getDataStoreRef();
    auto& zBoundsRef = rectGridGeom.getZBounds()->getDataStoreRef();
    for(size_t i = 0; i < rectGridGeom.getNumXCells(); i++)
    {
      xBoundsRef[i] += translation[0];
    }
    for(size_t i = 0; i < rectGridGeom.getNumYCells(); i++)
    {
      yBoundsRef[i] += translation[1];
    }
    for(size_t i = 0; i < rectGridGeom.getNumZCells(); i++)
    {
      zBoundsRef[i] += translation[2];
    }
    return;
  }
  case IGeometry::Type::Vertex: {
    auto& vertexGeom = dynamic_cast<VertexGeom&>(geometry);
    auto& verticesRef = vertexGeom.getVertices()->getDataStoreRef();
    for(size_t i = 0; i < vertexGeom.getNumberOfVertices(); i++)
    {
      verticesRef[3 * i + 0] += translation[0];
      verticesRef[3 * i + 1] += translation[1];
      verticesRef[3 * i + 2] += translation[2];
    }
    return;
  }
  case IGeometry::Type::Edge: {
    auto& edgeGeom = dynamic_cast<EdgeGeom&>(geometry);
    auto& verticesRef = edgeGeom.getVertices()->getDataStoreRef();
    for(size_t i = 0; i < edgeGeom.getNumberOfVertices(); i++)
    {
      verticesRef[3 * i + 0] += translation[0];
      verticesRef[3 * i + 1] += translation[1];
      verticesRef[3 * i + 2] += translation[2];
    }
    return;
  }
    // 2D Geometries
  case IGeometry::Type::Quad:
    [[fallthrough]];
  case IGeometry::Type::Triangle: {
    auto& geometry2dGeom = dynamic_cast<INodeGeometry2D&>(geometry);
    auto& verticesRef = geometry2dGeom.getVertices()->getDataStoreRef();
    for(size_t i = 0; i < geometry2dGeom.getNumberOfVertices(); i++)
    {
      verticesRef[3 * i + 0] += translation[0];
      verticesRef[3 * i + 1] += translation[1];
      verticesRef[3 * i + 2] += translation[2];
    }
    return;
  }
    // 3D Geometries
  case IGeometry::Type::Hexahedral:
    [[fallthrough]];
  case IGeometry::Type::Tetrahedral: {
    auto& geometry3dGeom = dynamic_cast<INodeGeometry3D&>(geometry);
    auto& verticesRef = geometry3dGeom.getVertices()->getDataStoreRef();
    for(size_t i = 0; i < geometry3dGeom.getNumberOfVertices(); i++)
    {
      verticesRef[3 * i + 0] += translation[0];
      verticesRef[3 * i + 1] += translation[1];
      verticesRef[3 * i + 2] += translation[2];
    }
    return;
  }
  }
}

} // namespace

// -----------------------------------------------------------------------------
AlignGeometries::AlignGeometries(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, AlignGeometriesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
AlignGeometries::~AlignGeometries() noexcept = default;

// -----------------------------------------------------------------------------
Result<> AlignGeometries::operator()()
{

  auto movingGeometryPath = m_InputValues->InputMovingGeometryPath;
  auto targetGeometryPath = m_InputValues->InputTargetGeometryPath;
  auto alignmentType = m_InputValues->AlignmentTypeIndex;

  auto& movingGeom = m_DataStructure.getDataRefAs<IGeometry>(movingGeometryPath);
  auto& targetGeom = m_DataStructure.getDataRefAs<IGeometry>(targetGeometryPath);

  if(alignmentType == 0)
  {
    FloatVec3 movingOrigin = extractOrigin(movingGeom);
    FloatVec3 targetOrigin = extractOrigin(targetGeom);

    FloatVec3 translation = {targetOrigin[0] - movingOrigin[0], targetOrigin[1] - movingOrigin[1], targetOrigin[2] - movingOrigin[2]};
    translateGeometry(movingGeom, translation);
  }
  else if(alignmentType == 1)
  {
    FloatVec3 movingCentroid = extractCentroid(movingGeom);
    FloatVec3 targetCentroid = extractCentroid(targetGeom);

    FloatVec3 translation = {targetCentroid[0] - movingCentroid[0], targetCentroid[1] - movingCentroid[1], targetCentroid[2] - movingCentroid[2]};
    translateGeometry(movingGeom, translation);
  }
  else
  {
    std::string ss = fmt::format("Invalid selection for alignment type");
    return {MakeErrorResult<>(-13423, ss)};
  }

  return {};
}
