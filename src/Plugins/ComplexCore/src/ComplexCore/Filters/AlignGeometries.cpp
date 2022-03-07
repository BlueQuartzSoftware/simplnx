#include "AlignGeometries.hpp"

#include "complex/DataStructure/Geometry/AbstractGeometry.hpp"
#include "complex/DataStructure/Geometry/AbstractGeometry2D.hpp"
#include "complex/DataStructure/Geometry/AbstractGeometry3D.hpp"
#include "complex/DataStructure/Geometry/EdgeGeom.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/DataStructure/Geometry/RectGridGeom.hpp"
#include "complex/DataStructure/Geometry/VertexGeom.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"

#include <string>

namespace
{
constexpr complex::int32 k_EMPTY_PARAMETER = -123;

complex::FloatVec3 extractOrigin(const complex::AbstractGeometry& geometry)
{
  auto geomType = geometry.getGeomType();
  switch(geomType)
  {
  case complex::AbstractGeometry::Type::Image: {
    auto& image = dynamic_cast<const complex::ImageGeom&>(geometry);
    return image.getOrigin();
  }
  case complex::AbstractGeometry::Type::RectGrid: {
    auto& rectGrid = dynamic_cast<const complex::RectGridGeom&>(geometry);
    const auto& xBounds = rectGrid.getXBounds()->getDataStoreRef();
    const auto& yBounds = rectGrid.getYBounds()->getDataStoreRef();
    const auto& zBounds = rectGrid.getZBounds()->getDataStoreRef();
    complex::FloatVec3 origin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    for(size_t i = 0; i < xBounds.getNumberOfTuples(); i++)
    {
      if(xBounds[i] < origin[0])
      {
        origin[0] = xBounds[i];
      }
    }
    for(size_t i = 0; i < yBounds.getNumberOfTuples(); i++)
    {
      if(yBounds[i] < origin[1])
      {
        origin[1] = yBounds[i];
      }
    }
    for(size_t i = 0; i < zBounds.getNumberOfTuples(); i++)
    {
      if(zBounds[i] < origin[2])
      {
        origin[2] = zBounds[i];
      }
    }
    return origin;
  }
  break;
  case complex::AbstractGeometry::Type::Vertex: {
    auto& vertex = dynamic_cast<const complex::VertexGeom&>(geometry);
    const auto& vertices = vertex.getVertices()->getDataStoreRef();
    complex::FloatVec3 origin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());

    for(size_t i = 0; i < vertex.getNumberOfVertices(); i++)
    {
      for(size_t j = 0; j < 3; j++)
      {
        if(vertices[3 * i + j] < origin[j])
        {
          origin[j] = vertices[3 * i + j];
        }
      }
    }
    return origin;
  }
  break;
  case complex::AbstractGeometry::Type::Edge: {
    const auto& edge = dynamic_cast<const complex::EdgeGeom&>(geometry);
    const auto& vertices = edge.getVertices()->getDataStoreRef();
    complex::FloatVec3 origin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());

    for(size_t i = 0; i < edge.getNumberOfVertices(); i++)
    {
      for(size_t j = 0; j < 3; j++)
      {
        if(vertices[3 * i + j] < origin[j])
        {
          origin[j] = vertices[3 * i + j];
        }
      }
    }
    return origin;
  }
  break;
  // 2D
  case complex::AbstractGeometry::Type::Triangle:
    [[fallthrough]];
  case complex::AbstractGeometry::Type::Quad:
    {
      const auto& geometry2d = dynamic_cast<const complex::AbstractGeometry2D&>(geometry);
      const auto& vertices = geometry2d.getVertices()->getDataStoreRef();
      complex::FloatVec3 origin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());

      for(size_t i = 0; i < geometry2d.getNumberOfVertices(); i++)
      {
        for(size_t j = 0; j < 3; j++)
        {
          if(vertices[3 * i + j] < origin[j])
          {
            origin[j] = vertices[3 * i + j];
          }
        }
      }
      return origin;
    }
  // 3D
  case complex::AbstractGeometry::Type::Hexahedral:
    [[falthrough]] case complex::AbstractGeometry::Type::Tetrahedral:
    {
      const auto& geometry3d = dynamic_cast<const complex::AbstractGeometry3D&>(geometry);
      const auto& vertices = geometry3d.getVertices()->getDataStoreRef();
      complex::FloatVec3 origin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());

      for(size_t i = 0; i < geometry3d.getNumberOfVertices(); i++)
      {
        for(size_t j = 0; j < 3; j++)
        {
          if(vertices[3 * i + j] < origin[j])
          {
            origin[j] = vertices[3 * i + j];
          }
        }
      }
      return origin;
    }
  case complex::AbstractGeometry::Type::Unknown:
    break;
  case complex::AbstractGeometry::Type::Any:
    break;
  default:
    break;
  }

  complex::FloatVec3 origin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
  return origin;
}

complex::FloatVec3 extractCentroid(const complex::AbstractGeometry& geometry)
{
  complex::FloatVec3 centroid(0.0f, 0.0f, 0.0f);
  switch(geometry.getGeomType())
  {
  case complex::AbstractGeometry::Type::Image: {
    const auto& image = dynamic_cast<const complex::ImageGeom&>(geometry);
    complex::SizeVec3 dims = image.getDimensions();
    complex::FloatVec3 origin = image.getOrigin();
    complex::FloatVec3 res = image.getSpacing();

    centroid[0] = (static_cast<float>(dims[0]) * res[0] / 2.0f) + origin[0];
    centroid[1] = (static_cast<float>(dims[1]) * res[1] / 2.0f) + origin[1];
    centroid[2] = (static_cast<float>(dims[2]) * res[2] / 2.0f) + origin[2];
    return centroid;
  }
  case complex::AbstractGeometry::Type::RectGrid: {
    const auto& rectGrid = dynamic_cast<const complex::RectGridGeom&>(geometry);
    const auto& xBounds = rectGrid.getXBounds()->getDataStoreRef();
    const auto& yBounds = rectGrid.getYBounds()->getDataStoreRef();
    const auto& zBounds = rectGrid.getZBounds()->getDataStoreRef();
    float min[3] = {std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
    float max[3] = {std::numeric_limits<float>::min(), std::numeric_limits<float>::min(), std::numeric_limits<float>::min()};
    for(size_t i = 0; i < xBounds.getNumberOfTuples(); i++)
    {
      if(xBounds[i] < min[0])
      {
        min[0] = xBounds[i];
      }
      if(xBounds[i] > max[0])
      {
        max[0] = xBounds[i];
      }
    }
    for(size_t i = 0; i < yBounds.getNumberOfTuples(); i++)
    {
      if(yBounds[i] < min[1])
      {
        min[1] = yBounds[i];
      }
      if(yBounds[i] > max[1])
      {
        max[1] = yBounds[i];
      }
    }
    for(size_t i = 0; i < zBounds.getNumberOfTuples(); i++)
    {
      if(zBounds[i] < min[2])
      {
        min[2] = zBounds[i];
      }
      if(zBounds[i] > max[2])
      {
        max[2] = zBounds[i];
      }
    }
    centroid[0] = (max[0] - min[0]) / 2.0f;
    centroid[1] = (max[1] - min[1]) / 2.0f;
    centroid[2] = (max[2] - min[2]) / 2.0f;
    return centroid;
  }
  case complex::AbstractGeometry::Type::Vertex: {
    const auto& vertex = dynamic_cast<const complex::VertexGeom&>(geometry);
    const auto& vertices = vertex.getVertices()->getDataStoreRef();
    centroid[0] = 0.0f;
    centroid[1] = 0.0f;
    centroid[2] = 0.0f;
    for(size_t i = 0; i < vertex.getNumberOfVertices(); i++)
    {
      centroid[0] += vertices[3 * i + 0];
      centroid[1] += vertices[3 * i + 1];
      centroid[2] += vertices[3 * i + 2];
    }
    centroid[0] /= static_cast<float>(vertex.getNumberOfVertices());
    centroid[1] /= static_cast<float>(vertex.getNumberOfVertices());
    centroid[2] /= static_cast<float>(vertex.getNumberOfVertices());
    return centroid;
  }
  case complex::AbstractGeometry::Type::Edge: {
    const auto& edge = dynamic_cast<const complex::EdgeGeom&>(geometry);
    const auto& vertices = edge.getVertices()->getDataStoreRef();
    centroid[0] = 0.0f;
    centroid[1] = 0.0f;
    centroid[2] = 0.0f;
    for(size_t i = 0; i < edge.getNumberOfVertices(); i++)
    {
      centroid[0] += vertices[3 * i + 0];
      centroid[1] += vertices[3 * i + 1];
      centroid[2] += vertices[3 * i + 2];
    }
    centroid[0] /= static_cast<float>(edge.getNumberOfVertices());
    centroid[1] /= static_cast<float>(edge.getNumberOfVertices());
    centroid[2] /= static_cast<float>(edge.getNumberOfVertices());
    return centroid;
  }
    // 2D Types
  case complex::AbstractGeometry::Type::Triangle:
    [[falthrough]] case complex::AbstractGeometry::Type::Quad:
    {
      auto& geometry2d = dynamic_cast<const complex::AbstractGeometry2D&>(geometry);
      const auto& vertices = geometry2d.getVertices()->getDataStoreRef();
      centroid[0] = 0.0f;
      centroid[1] = 0.0f;
      centroid[2] = 0.0f;
      for(size_t i = 0; i < geometry2d.getNumberOfVertices(); i++)
      {
        centroid[0] += vertices[3 * i + 0];
        centroid[1] += vertices[3 * i + 1];
        centroid[2] += vertices[3 * i + 2];
      }
      centroid[0] /= static_cast<float>(geometry2d.getNumberOfVertices());
      centroid[1] /= static_cast<float>(geometry2d.getNumberOfVertices());
      centroid[2] /= static_cast<float>(geometry2d.getNumberOfVertices());
      return centroid;
    }
    // 3D Types
  case complex::AbstractGeometry::Type::Hexahedral:
    [[falthrough]] case complex::AbstractGeometry::Type::Tetrahedral:
    {
      const auto& geometry3d = dynamic_cast<const complex::AbstractGeometry3D&>(geometry);
      const auto& vertices = geometry3d.getVertices()->getDataStoreRef();
      centroid[0] = 0.0f;
      centroid[1] = 0.0f;
      centroid[2] = 0.0f;
      for(size_t i = 0; i < geometry3d.getNumberOfVertices(); i++)
      {
        centroid[0] += vertices[3 * i + 0];
        centroid[1] += vertices[3 * i + 1];
        centroid[2] += vertices[3 * i + 2];
      }
      centroid[0] /= static_cast<float>(geometry3d.getNumberOfVertices());
      centroid[1] /= static_cast<float>(geometry3d.getNumberOfVertices());
      centroid[2] /= static_cast<float>(geometry3d.getNumberOfVertices());
      return centroid;
    }
  case complex::AbstractGeometry::Type::Unknown:
    break;
  case complex::AbstractGeometry::Type::Any:
    break;
  }

  return centroid;
}

void translateGeometry(complex::AbstractGeometry& geometry, const complex::FloatVec3& translation)
{
  switch(geometry.getGeomType())
  {
  case complex::AbstractGeometry::Type::Image: {
    auto& image = dynamic_cast<complex::ImageGeom&>(geometry);
    complex::FloatVec3 origin = image.getOrigin();
    origin[0] += translation[0];
    origin[1] += translation[1];
    origin[2] += translation[2];
    image.setOrigin(origin);
    return;
  }
  case complex::AbstractGeometry::Type::RectGrid: {
    auto& rectGrid = dynamic_cast<complex::RectGridGeom&>(geometry);
    auto& xBounds = rectGrid.getXBounds()->getDataStoreRef();
    auto& yBounds = rectGrid.getYBounds()->getDataStoreRef();
    auto& zBounds = rectGrid.getZBounds()->getDataStoreRef();
    for(size_t i = 0; i < rectGrid.getNumXPoints(); i++)
    {
      xBounds[i] += translation[0];
    }
    for(size_t i = 0; i < rectGrid.getNumYPoints(); i++)
    {
      yBounds[i] += translation[1];
    }
    for(size_t i = 0; i < rectGrid.getNumZPoints(); i++)
    {
      zBounds[i] += translation[2];
    }
    return;
  }
  case complex::AbstractGeometry::Type::Vertex: {
    auto& vertex = dynamic_cast<complex::VertexGeom&>(geometry);
    auto& vertices = vertex.getVertices()->getDataStoreRef();
    for(size_t i = 0; i < vertex.getNumberOfVertices(); i++)
    {
      vertices[3 * i + 0] += translation[0];
      vertices[3 * i + 1] += translation[1];
      vertices[3 * i + 2] += translation[2];
    }
    return;
  }
  case complex::AbstractGeometry::Type::Edge: {
    auto& edge = dynamic_cast<complex::EdgeGeom&>(geometry);
    auto& vertices = edge.getVertices()->getDataStoreRef();
    for(size_t i = 0; i < edge.getNumberOfVertices(); i++)
    {
      vertices[3 * i + 0] += translation[0];
      vertices[3 * i + 1] += translation[1];
      vertices[3 * i + 2] += translation[2];
    }
    return;
  }
    // 2D Geometries
  case complex::AbstractGeometry::Type::Quad:
    [[falthrough]] case complex::AbstractGeometry::Type::Triangle:
    {
      auto& geometry2d = dynamic_cast<complex::AbstractGeometry2D&>(geometry);
      auto& vertices = geometry2d.getVertices()->getDataStoreRef();
      for(size_t i = 0; i < geometry2d.getNumberOfVertices(); i++)
      {
        vertices[3 * i + 0] += translation[0];
        vertices[3 * i + 1] += translation[1];
        vertices[3 * i + 2] += translation[2];
      }
      return;
    }
    // 3D Geometries
  case complex::AbstractGeometry::Type::Hexahedral:
    [[falthrough]] case complex::AbstractGeometry::Type::Tetrahedral:
    {
      auto& geometry3d = dynamic_cast<complex::AbstractGeometry3D&>(geometry);
      auto& vertices = geometry3d.getVertices()->getDataStoreRef();
      for(size_t i = 0; i < geometry3d.getNumberOfVertices(); i++)
      {
        vertices[3 * i + 0] += translation[0];
        vertices[3 * i + 1] += translation[1];
        vertices[3 * i + 2] += translation[2];
      }
      return;
    }
  }
}

} // namespace

namespace complex
{

std::string AlignGeometries::name() const
{
  return FilterTraits<AlignGeometries>::name;
}

std::string AlignGeometries::className() const
{
  return FilterTraits<AlignGeometries>::className;
}

Uuid AlignGeometries::uuid() const
{
  return FilterTraits<AlignGeometries>::uuid;
}

std::string AlignGeometries::humanName() const
{
  return "Align Geometries";
}

Parameters AlignGeometries::parameters() const
{
  GeometrySelectionParameter::AllowedTypes geomTypes{AbstractGeometry::Type::Any};

  Parameters params;
  params.insert(std::make_unique<GeometrySelectionParameter>(k_MovingGeometry_Key, "Moving Geometry", "Numeric Type of data to create", DataPath(), geomTypes));
  params.insert(std::make_unique<GeometrySelectionParameter>(k_TargetGeometry_Key, "Target Geometry", "Number of components", DataPath(), geomTypes));
  params.insert(std::make_unique<ChoicesParameter>(k_AlignmentType_Key, "Alignment Type", "Alignment Type", 0, std::vector<std::string>{"Origin", "Centroid"}));
  return params;
}

IFilter::UniquePointer AlignGeometries::clone() const
{
  return std::make_unique<AlignGeometries>();
}

IFilter::PreflightResult AlignGeometries::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                        const std::atomic_bool& shouldCancel) const
{
  auto movingGeometryPath = filterArgs.value<DataPath>(k_MovingGeometry_Key);
  auto targetGeometryPath = filterArgs.value<DataPath>(k_TargetGeometry_Key);
  auto alignmentType = filterArgs.value<uint64>(k_AlignmentType_Key);

  auto* movingGeometry = dataStructure.getDataAs<AbstractGeometry>(movingGeometryPath);
  auto* targetGeometry = dataStructure.getDataAs<AbstractGeometry>(targetGeometryPath);

  if(alignmentType != 0 && alignmentType != 1)
  {
    std::string ss = fmt::format("Invalid selection for alignment type");
    return {MakeErrorResult<OutputActions>(-1, ss)};
  }

  OutputActions actions;
  return {std::move(actions)};
}

Result<> AlignGeometries::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  auto movingGeometryPath = args.value<DataPath>(k_MovingGeometry_Key);
  auto targetGeometryPath = args.value<DataPath>(k_TargetGeometry_Key);
  auto alignmentType = args.value<uint64>(k_AlignmentType_Key);

  auto& moving = data.getDataRefAs<AbstractGeometry>(movingGeometryPath);
  auto& target = data.getDataRefAs<AbstractGeometry>(targetGeometryPath);

  if(alignmentType == 0)
  {
    FloatVec3 movingOrigin = extractOrigin(moving);
    FloatVec3 targetOrigin = extractOrigin(target);

    float translation[3] = {targetOrigin[0] - movingOrigin[0], targetOrigin[1] - movingOrigin[1], targetOrigin[2] - movingOrigin[2]};
    translateGeometry(moving, translation);
  }
  else if(alignmentType == 1)
  {
    FloatVec3 movingCentroid = extractCentroid(moving);
    FloatVec3 targetCentroid = extractCentroid(target);

    float translation[3] = {targetCentroid[0] - movingCentroid[0], targetCentroid[0] - movingCentroid[0], targetCentroid[0] - movingCentroid[0]};
    translateGeometry(moving, translation);
  }
  else
  {
    std::string ss = fmt::format("Invalid selection for alignment type");
    return {MakeErrorResult<>(-1, ss)};
  }

  return {};
}
} // namespace complex
