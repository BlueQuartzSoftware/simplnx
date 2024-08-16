#include "AlignGeometriesFilter.hpp"

#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry2D.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry3D.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/RectGridGeom.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <string>

using namespace nx::core;

namespace
{
FloatVec3 extractOrigin(const IGeometry& geometry)
{
  auto geomType = geometry.getGeomType();
  switch(geomType)
  {
  case IGeometry::Type::Image: {
    auto& image = dynamic_cast<const ImageGeom&>(geometry);
    return image.getOrigin();
  }
  case IGeometry::Type::RectGrid: {
    auto& rectGrid = dynamic_cast<const RectGridGeom&>(geometry);
    const auto& xBounds = rectGrid.getXBounds()->getDataStoreRef();
    const auto& yBounds = rectGrid.getYBounds()->getDataStoreRef();
    const auto& zBounds = rectGrid.getZBounds()->getDataStoreRef();
    FloatVec3 origin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
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
  case IGeometry::Type::Vertex: {
    auto& vertex = dynamic_cast<const VertexGeom&>(geometry);
    const auto& vertices = vertex.getVertices()->getDataStoreRef();
    FloatVec3 origin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());

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
  case IGeometry::Type::Edge: {
    const auto& edge = dynamic_cast<const EdgeGeom&>(geometry);
    const auto& vertices = edge.getVertices()->getDataStoreRef();
    FloatVec3 origin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());

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
  // 2D
  case IGeometry::Type::Triangle:
    [[fallthrough]];
  case IGeometry::Type::Quad: {
    const auto& geometry2d = dynamic_cast<const INodeGeometry2D&>(geometry);
    const auto& vertices = geometry2d.getVertices()->getDataStoreRef();
    FloatVec3 origin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());

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
  case IGeometry::Type::Hexahedral:
    [[fallthrough]];
  case IGeometry::Type::Tetrahedral: {
    const auto& geometry3d = dynamic_cast<const INodeGeometry3D&>(geometry);
    const auto& vertices = geometry3d.getVertices()->getDataStoreRef();
    FloatVec3 origin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());

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
    const auto& image = dynamic_cast<const ImageGeom&>(geometry);
    SizeVec3 dims = image.getDimensions();
    FloatVec3 origin = image.getOrigin();
    FloatVec3 res = image.getSpacing();

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
    auto& geometry2d = dynamic_cast<const INodeGeometry2D&>(geometry);
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

void translateGeometry(IGeometry& geometry, const FloatVec3& translation)
{
  switch(geometry.getGeomType())
  {
  case IGeometry::Type::Image: {
    auto& image = dynamic_cast<ImageGeom&>(geometry);
    FloatVec3 origin = image.getOrigin();
    origin[0] += translation[0];
    origin[1] += translation[1];
    origin[2] += translation[2];
    image.setOrigin(origin);
    return;
  }
  case IGeometry::Type::RectGrid: {
    auto& rectGrid = dynamic_cast<RectGridGeom&>(geometry);
    auto& xBounds = rectGrid.getXBounds()->getDataStoreRef();
    auto& yBounds = rectGrid.getYBounds()->getDataStoreRef();
    auto& zBounds = rectGrid.getZBounds()->getDataStoreRef();
    for(size_t i = 0; i < rectGrid.getNumXCells(); i++)
    {
      xBounds[i] += translation[0];
    }
    for(size_t i = 0; i < rectGrid.getNumYCells(); i++)
    {
      yBounds[i] += translation[1];
    }
    for(size_t i = 0; i < rectGrid.getNumZCells(); i++)
    {
      zBounds[i] += translation[2];
    }
    return;
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
    return;
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
    return;
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
    return;
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
    return;
  }
  }
}

} // namespace

namespace nx::core
{

//------------------------------------------------------------------------------
std::string AlignGeometriesFilter::name() const
{
  return FilterTraits<AlignGeometriesFilter>::name;
}

//------------------------------------------------------------------------------
std::string AlignGeometriesFilter::className() const
{
  return FilterTraits<AlignGeometriesFilter>::className;
}

//------------------------------------------------------------------------------
Uuid AlignGeometriesFilter::uuid() const
{
  return FilterTraits<AlignGeometriesFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string AlignGeometriesFilter::humanName() const
{
  return "Align Geometries";
}

//------------------------------------------------------------------------------
std::vector<std::string> AlignGeometriesFilter::defaultTags() const
{
  return {className(), "Match", "Align", "Geometry", "Move"};
}

//------------------------------------------------------------------------------
Parameters AlignGeometriesFilter::parameters() const
{
  GeometrySelectionParameter::AllowedTypes geomTypes = IGeometry::GetAllGeomTypes();

  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_MovingGeometry_Key, "Moving Geometry", "The geometry that will be moved.", DataPath(), geomTypes));
  params.insert(std::make_unique<GeometrySelectionParameter>(k_TargetGeometry_Key, "Fixed Geometry", "The geometry that does *not* move.", DataPath(), geomTypes));
  params.insert(std::make_unique<ChoicesParameter>(k_AlignmentType_Key, "Alignment Type", "The type of alignment to perform (Origin or Centroid.", 0, std::vector<std::string>{"Origin", "Centroid"}));
  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer AlignGeometriesFilter::clone() const
{
  return std::make_unique<AlignGeometriesFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult AlignGeometriesFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                              const std::atomic_bool& shouldCancel) const
{
  auto alignmentType = filterArgs.value<uint64>(k_AlignmentType_Key);

  if(alignmentType != 0 && alignmentType != 1)
  {
    std::string ss = fmt::format("Invalid selection for alignment type");
    return {MakeErrorResult<OutputActions>(-1, ss)};
  }

  OutputActions actions;
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> AlignGeometriesFilter::executeImpl(DataStructure& dataStructure, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                            const std::atomic_bool& shouldCancel) const
{
  auto movingGeometryPath = args.value<DataPath>(k_MovingGeometry_Key);
  auto targetGeometryPath = args.value<DataPath>(k_TargetGeometry_Key);
  auto alignmentType = args.value<uint64>(k_AlignmentType_Key);

  auto& moving = dataStructure.getDataRefAs<IGeometry>(movingGeometryPath);
  auto& target = dataStructure.getDataRefAs<IGeometry>(targetGeometryPath);

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

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_AlignmentTypeKey = "AlignmentType";
constexpr StringLiteral k_MovingGeometryKey = "MovingGeometry";
constexpr StringLiteral k_TargetGeometryKey = "TargetGeometry";
} // namespace SIMPL
} // namespace

Result<Arguments> AlignGeometriesFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = AlignGeometriesFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::ChoiceFilterParameterConverter>(args, json, SIMPL::k_AlignmentTypeKey, k_AlignmentType_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_MovingGeometryKey, k_MovingGeometry_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_TargetGeometryKey, k_TargetGeometry_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}

} // namespace nx::core
