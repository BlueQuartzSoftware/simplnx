#include "AlignGeometries.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry2D.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry3D.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/RectGridGeom.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"

#include <algorithm>
#include <array>

using namespace nx::core;

namespace
{
FloatVec3Type ExtractOrigin(const IGeometry& geometry)
{
  auto geomType = geometry.getGeomType();
  switch(geomType)
  {
  case IGeometry::Type::Image: {
    const auto& image = dynamic_cast<const ImageGeom&>(geometry);
    return image.getOrigin();
  }
  case IGeometry::Type::RectGrid: {
    const auto& rectGrid = dynamic_cast<const RectGridGeom&>(geometry);
    const auto& xBounds = rectGrid.getXBounds()->getDataStoreRef();
    const auto& yBounds = rectGrid.getYBounds()->getDataStoreRef();
    const auto& zBounds = rectGrid.getZBounds()->getDataStoreRef();
    FloatVec3Type origin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    for(size_t i = 0; i < xBounds.getNumberOfTuples(); i++)
    {
      origin[0] = std::min(xBounds[i], origin[0]);
    }
    for(size_t i = 0; i < yBounds.getNumberOfTuples(); i++)
    {
      origin[1] = std::min(yBounds[i], origin[1]);
    }
    for(size_t i = 0; i < zBounds.getNumberOfTuples(); i++)
    {
      origin[2] = std::min(zBounds[i], origin[2]);
    }
    return origin;
  }
  case IGeometry::Type::Vertex: {
    const auto& vertex = dynamic_cast<const VertexGeom&>(geometry);
    const auto& vertices = vertex.getVertices()->getDataStoreRef();
    FloatVec3Type origin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());

    for(size_t i = 0; i < vertex.getNumberOfVertices(); i++)
    {
      for(size_t j = 0; j < 3; j++)
      {
        origin[j] = std::min(vertices[3 * i + j], origin[j]);
      }
    }
    return origin;
  }
  case IGeometry::Type::Edge: {
    const auto& edge = dynamic_cast<const EdgeGeom&>(geometry);
    const auto& vertices = edge.getVertices()->getDataStoreRef();
    FloatVec3Type origin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());

    for(size_t i = 0; i < edge.getNumberOfVertices(); i++)
    {
      for(size_t j = 0; j < 3; j++)
      {
        origin[j] = std::min(vertices[3 * i + j], origin[j]);
      }
    }
    return origin;
  }
  // 2D
  case IGeometry::Type::Triangle:
    [[fallthrough]];
  case IGeometry::Type::Quad: {
    const auto& geometry2d = dynamic_cast<const INodeGeometry2D&>(geometry);
    const auto& vertices = geometry2d.getVertices()->getDataStoreRef();
    FloatVec3Type origin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());

    for(size_t i = 0; i < geometry2d.getNumberOfVertices(); i++)
    {
      for(size_t j = 0; j < 3; j++)
      {
        origin[j] = std::min(vertices[3 * i + j], origin[j]);
      }
    }
    return origin;
  }
  // 3D
  case IGeometry::Type::Hexahedral:
    [[fallthrough]];
  case IGeometry::Type::Tetrahedral: {
    const auto& geometry3d = dynamic_cast<const INodeGeometry3D&>(geometry);
    const auto& vertices = geometry3d.getVertices()->getDataStoreRef();
    FloatVec3Type origin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());

    for(size_t i = 0; i < geometry3d.getNumberOfVertices(); i++)
    {
      for(size_t j = 0; j < 3; j++)
      {
        origin[j] = std::min(vertices[3 * i + j], origin[j]);
      }
    }
    return origin;
  }
  default:
    break;
  }

  FloatVec3Type origin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
  return origin;
}

FloatVec3Type ExtractCentroid(const IGeometry& geometry)
{
  FloatVec3Type centroid(0.0f, 0.0f, 0.0f);
  switch(geometry.getGeomType())
  {
  case IGeometry::Type::Image: {
    const auto& image = dynamic_cast<const ImageGeom&>(geometry);
    SizeVec3Type dims = image.getDimensions();
    FloatVec3Type origin = image.getOrigin();
    FloatVec3Type res = image.getSpacing();

    centroid[0] = (static_cast<float>(dims[0]) * res[0] / 2.0f) + origin[0];
    centroid[1] = (static_cast<float>(dims[1]) * res[1] / 2.0f) + origin[1];
    centroid[2] = (static_cast<float>(dims[2]) * res[2] / 2.0f) + origin[2];
    return centroid;
  }
  case IGeometry::Type::RectGrid: {
    const auto& rectGrid = dynamic_cast<const RectGridGeom&>(geometry);
    const auto& xBounds = rectGrid.getXBounds()->getDataStoreRef();
    const auto& yBounds = rectGrid.getYBounds()->getDataStoreRef();
    const auto& zBounds = rectGrid.getZBounds()->getDataStoreRef();
    float min[3] = {std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
    float max[3] = {std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest()};
    for(size_t i = 0; i < xBounds.getNumberOfTuples(); i++)
    {
      min[0] = std::min(xBounds[i], min[0]);
      max[0] = std::max(xBounds[i], max[0]);
    }
    for(size_t i = 0; i < yBounds.getNumberOfTuples(); i++)
    {
      min[1] = std::min(yBounds[i], min[1]);
      max[1] = std::max(yBounds[i], max[1]);
    }
    for(size_t i = 0; i < zBounds.getNumberOfTuples(); i++)
    {
      min[2] = std::min(zBounds[i], min[2]);
      max[2] = std::max(zBounds[i], max[2]);
    }
    centroid[0] = (max[0] + min[0]) / 2.0f;
    centroid[1] = (max[1] + min[1]) / 2.0f;
    centroid[2] = (max[2] + min[2]) / 2.0f;
    return centroid;
  }
  case IGeometry::Type::Vertex: {
    const auto& vertex = dynamic_cast<const VertexGeom&>(geometry);
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
  case IGeometry::Type::Edge: {
    const auto& edge = dynamic_cast<const EdgeGeom&>(geometry);
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
  case IGeometry::Type::Triangle:
    [[fallthrough]];
  case IGeometry::Type::Quad: {
    const auto& geometry2d = dynamic_cast<const INodeGeometry2D&>(geometry);
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
  case IGeometry::Type::Hexahedral:
    [[fallthrough]];
  case IGeometry::Type::Tetrahedral: {
    const auto& geometry3d = dynamic_cast<const INodeGeometry3D&>(geometry);
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
  }

  return centroid;
}

Result<> TranslateGeometry(IGeometry& geometry, const FloatVec3Type& translation)
{
  switch(geometry.getGeomType())
  {
  case IGeometry::Type::Image: {
    auto& image = dynamic_cast<ImageGeom&>(geometry);
    FloatVec3Type origin = image.getOrigin();
    origin[0] += translation[0];
    origin[1] += translation[1];
    origin[2] += translation[2];
    return image.setOrigin(origin);
  }
  case IGeometry::Type::RectGrid: {
    auto& rectGrid = dynamic_cast<RectGridGeom&>(geometry);
    std::array<Float32ArrayType*, 3> bounds{rectGrid.getXBounds(), rectGrid.getYBounds(), rectGrid.getZBounds()};
    std::array<AbstractDataStore<float32>*, 3> stores{};
    constexpr std::array<char, 3> k_Axes{'X', 'Y', 'Z'};
    for(usize axis = 0; axis < bounds.size(); ++axis)
    {
      auto* storePtr = &bounds[axis]->getDataStoreRef();
      for(usize earlier = 0; earlier < axis; ++earlier)
      {
        if(storePtr != stores[earlier])
        {
          continue;
        }
        if(translation[axis] != translation[earlier])
        {
          return MakeErrorResult(-69201, fmt::format("Cannot translate rectilinear grid geometry '{}': the {} and {} bounds share storage but require different offsets ({} and {}). "
                                                     "Use separate bounds arrays or an alignment that applies the same offset to the shared axes.",
                                                     rectGrid.getName(), k_Axes[earlier], k_Axes[axis], translation[earlier], translation[axis]));
        }

        // The earlier axis already represents these values for validation and writing.
        bounds[axis] = nullptr;
        break;
      }
      if(bounds[axis] != nullptr)
      {
        stores[axis] = storePtr;
      }
    }
    auto validation = RectGridGeom::ValidateBounds(bounds[0], bounds[1], bounds[2], translation);
    if(validation.invalid())
    {
      for(auto& error : validation.errors())
      {
        error.message = fmt::format("Cannot translate rectilinear grid geometry '{}': {}", rectGrid.getName(), error.message);
      }
      return validation;
    }
    for(usize axis = 0; axis < stores.size(); ++axis)
    {
      auto* storePtr = stores[axis];
      if(storePtr == nullptr)
      {
        continue;
      }
      for(usize i = 0; i < storePtr->getNumberOfTuples(); ++i)
      {
        (*storePtr)[i] += translation[axis];
      }
    }
    return {};
  }
  case IGeometry::Type::Vertex: {
    auto& vertex = dynamic_cast<VertexGeom&>(geometry);
    auto& vertices = vertex.getVertices()->getDataStoreRef();
    for(size_t i = 0; i < vertex.getNumberOfVertices(); i++)
    {
      vertices[3 * i + 0] += translation[0];
      vertices[3 * i + 1] += translation[1];
      vertices[3 * i + 2] += translation[2];
    }
    return {};
  }
  case IGeometry::Type::Edge: {
    auto& edge = dynamic_cast<EdgeGeom&>(geometry);
    auto& vertices = edge.getVertices()->getDataStoreRef();
    for(size_t i = 0; i < edge.getNumberOfVertices(); i++)
    {
      vertices[3 * i + 0] += translation[0];
      vertices[3 * i + 1] += translation[1];
      vertices[3 * i + 2] += translation[2];
    }
    return {};
  }
    // 2D Geometries
  case IGeometry::Type::Quad:
    [[fallthrough]];
  case IGeometry::Type::Triangle: {
    auto& geometry2d = dynamic_cast<INodeGeometry2D&>(geometry);
    auto& vertices = geometry2d.getVertices()->getDataStoreRef();
    for(size_t i = 0; i < geometry2d.getNumberOfVertices(); i++)
    {
      vertices[3 * i + 0] += translation[0];
      vertices[3 * i + 1] += translation[1];
      vertices[3 * i + 2] += translation[2];
    }
    return {};
  }
    // 3D Geometries
  case IGeometry::Type::Hexahedral:
    [[fallthrough]];
  case IGeometry::Type::Tetrahedral: {
    auto& geometry3d = dynamic_cast<INodeGeometry3D&>(geometry);
    auto& vertices = geometry3d.getVertices()->getDataStoreRef();
    for(size_t i = 0; i < geometry3d.getNumberOfVertices(); i++)
    {
      vertices[3 * i + 0] += translation[0];
      vertices[3 * i + 1] += translation[1];
      vertices[3 * i + 2] += translation[2];
    }
    return {};
  }
  }
  return {};
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

// -----------------------------------------------------------------------------
Result<> AlignGeometries::operator()()
{

  auto movingGeometryPath = m_InputValues->input_moving_geometry_path;
  auto targetGeometryPath = m_InputValues->input_target_geometry_path;
  auto alignmentType = m_InputValues->alignment_type_index;

  auto& moving = m_DataStructure.getDataRefAs<IGeometry>(movingGeometryPath);
  auto& target = m_DataStructure.getDataRefAs<IGeometry>(targetGeometryPath);

  if(alignmentType == 0)
  {
    FloatVec3Type movingOrigin = ExtractOrigin(moving);
    if(m_ShouldCancel)
    {
      return {};
    }
    FloatVec3Type targetOrigin = ExtractOrigin(target);
    if(m_ShouldCancel)
    {
      return {};
    }

    float translation[3] = {targetOrigin[0] - movingOrigin[0], targetOrigin[1] - movingOrigin[1], targetOrigin[2] - movingOrigin[2]};
    return TranslateGeometry(moving, translation);
  }
  if(alignmentType == 1)
  {
    FloatVec3Type movingCentroid = ExtractCentroid(moving);
    if(m_ShouldCancel)
    {
      return {};
    }
    FloatVec3Type targetCentroid = ExtractCentroid(target);
    if(m_ShouldCancel)
    {
      return {};
    }

    float translation[3] = {targetCentroid[0] - movingCentroid[0], targetCentroid[1] - movingCentroid[1], targetCentroid[2] - movingCentroid[2]};
    return TranslateGeometry(moving, translation);
  }
  return MakeErrorResult(-13423, "Invalid selection for alignment type");
}
