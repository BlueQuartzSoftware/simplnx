#pragma once

#include "complex/Common/Array.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/Geometry/IGeometry.hpp"
#include "complex/DataStructure/Geometry/QuadGeom.hpp"
#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/Filter/Output.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"
#include "complex/complex_export.hpp"

#include <fmt/core.h>

#include <utility>

namespace complex
{
/**
 * @brief Action for creating a Triangle or QuadGeometry in a DataStructure
 */
template <typename Geometry2DType>
class CreateGeometry1DAction : public IDataCreationAction
{
public:
  using DimensionType = std::vector<size_t>;

  static constexpr StringLiteral k_DefaultVerticesName = "SharedVertexList";
  static constexpr StringLiteral k_DefaultEdgesName = "SharedEdgeList";

  CreateGeometry1DAction() = delete;

  CreateGeometry1DAction(const DataPath& geometryPath, size_t numEdges, size_t numVertices, const std::string& vertexAttributeMatrixName, const std::string& edgeAttributeMatrixName)
  : IDataCreationAction(geometryPath)
  , m_NumEdges(numEdges)
  , m_NumVertices(numVertices)
  , m_VertexDataName(vertexAttributeMatrixName)
  , m_EdgeDataName(edgeAttributeMatrixName)
  {
  }

  ~CreateGeometry1DAction() noexcept = default;

  CreateGeometry1DAction(const CreateGeometry1DAction&) = delete;
  CreateGeometry1DAction(CreateGeometry1DAction&&) noexcept = delete;
  CreateGeometry1DAction& operator=(const CreateGeometry1DAction&) = delete;
  CreateGeometry1DAction& operator=(CreateGeometry1DAction&&) noexcept = delete;

  /**
   * @brief Applies this action's change to the given DataStructure in the given mode.
   * Returns any warnings/errors. On error, DataStructure is not guaranteed to be consistent.
   * @param dataStructure
   * @return
   */
  Result<> apply(DataStructure& dataStructure, Mode mode) const override
  {
    const std::string k_TriangleDataName(k_DefaultEdgesName);
    const std::string k_VertexDataName(k_DefaultVerticesName);
    DataPath faceDataPath = getEdgeDataPath();
    DataPath vertexDataPath = getVertexDataPath();

    // Check for empty Geometry DataPath
    if(getCreatedPath().empty())
    {
      return MakeErrorResult(-220, "CreateGeometry1DAction: Geometry Path cannot be empty");
    }

    // Check if the Geometry Path already exists
    BaseGroup* parentObject = dataStructure.getDataAs<BaseGroup>(getCreatedPath());
    if(parentObject != nullptr)
    {
      return MakeErrorResult(-221, fmt::format("CreateGeometry1DAction: DataObject already exists at path '{}'", getCreatedPath().toString()));
    }

    DataPath parentPath = getCreatedPath().getParent();
    if(!parentPath.empty())
    {
      Result<LinkedPath> geomPath = dataStructure.makePath(parentPath);
      if(geomPath.invalid())
      {
        return MakeErrorResult(-222, fmt::format("CreateGeometry1DAction: Geometry could not be created at path:'{}'", getCreatedPath().toString()));
      }
    }
    // Get the Parent ID
    if(!dataStructure.getId(parentPath).has_value())
    {
      return MakeErrorResult(-223, fmt::format("CreateGeometry1DAction: Parent Id was not available for path:'{}'", parentPath.toString()));
    }

    // Create the TriangleGeometry
    auto geometry2d = Geometry2DType::Create(dataStructure, getCreatedPath().getTargetName(), dataStructure.getId(parentPath).value());

    auto* faceAttributeMatrix = AttributeMatrix::Create(dataStructure, m_EdgeDataName, geometry2d->getId());
    if(faceAttributeMatrix == nullptr)
    {
      return MakeErrorResult(-224, fmt::format("CreateGeometry1DAction: Failed to create attribute matrix: '{}'", faceDataPath.toString()));
    }
    DimensionType faceTupleShape = {m_NumEdges};
    faceAttributeMatrix->setShape(faceTupleShape);
    geometry2d->setEdgeData(*faceAttributeMatrix);

    auto* vertexAttributeMatrix = AttributeMatrix::Create(dataStructure, m_VertexDataName, geometry2d->getId());
    if(vertexAttributeMatrix == nullptr)
    {
      return MakeErrorResult(-225, fmt::format("CreateGeometry1DAction: Failed to create attribute matrix: '{}'", vertexDataPath.toString()));
    }
    DimensionType vertexTupleShape = {m_NumVertices}; // We probably don't know how many Vertices there are but take what ever the developer sends us
    vertexAttributeMatrix->setShape(vertexTupleShape);
    geometry2d->setVertexData(*vertexAttributeMatrix);

    using MeshIndexType = IGeometry::MeshIndexType;
    using SharedEdgeList = IGeometry::SharedEdgeList;

    DataPath edgesPath = getCreatedPath().createChildPath(k_TriangleDataName);
    // Create the default DataArray that will hold the EdgeList and Vertices. We
    // size these to 1 because the Csv parser will resize them to the appropriate number of tuples
    complex::Result result = complex::CreateArray<MeshIndexType>(dataStructure, faceTupleShape, {2}, edgesPath, mode);
    if(result.invalid())
    {
      return MakeErrorResult(-226, fmt::format("CreateGeometry1DAction: Could not allocate SharedEdgeList '{}'", edgesPath.toString()));
    }
    SharedEdgeList* edges = complex::ArrayFromPath<MeshIndexType>(dataStructure, edgesPath);
    geometry2d->setEdges(*edges);

    // Create the Vertex Array with a component size of 3
    DataPath vertexPath = getCreatedPath().createChildPath(k_VertexDataName);

    result = complex::CreateArray<float>(dataStructure, vertexTupleShape, {3}, vertexPath, mode);
    if(result.invalid())
    {
      return MakeErrorResult(-227, fmt::format("CreateGeometry1DAction: Could not allocate SharedVertList '{}'", vertexPath.toString()));
    }
    Float32Array* vertexArray = complex::ArrayFromPath<float>(dataStructure, vertexPath);
    geometry2d->setVertices(*vertexArray);

    return {};
  }

  /**
   * @brief Returns the path of the ImageGeometry to be created.
   * @return
   */
  const DataPath& geometryPath() const
  {
    return getCreatedPath();
  }

  /**
   * @brief Returns the path of the ImageGeometry to be created.
   * @return
   */
  DataPath getEdgeDataPath() const
  {
    return getCreatedPath().createChildPath(m_EdgeDataName);
  }

  /**
   * @brief Returns the path of the ImageGeometry to be created.
   * @return
   */
  DataPath getVertexDataPath() const
  {
    return getCreatedPath().createChildPath(m_VertexDataName);
  }

  /**
   * @brief Returns the number of faces
   * @return
   */
  IGeometry::MeshIndexType numEdges() const
  {
    return m_NumEdges;
  }

  /**
   * @brief Returns the number of vertices (estimated in some circumstances)
   * @return
   */
  IGeometry::MeshIndexType numVertices() const
  {
    return m_NumVertices;
  }

  /**
   * @brief Returns all of the DataPaths to be created.
   * @return std::vector<DataPath>
   */
  std::vector<DataPath> getAllCreatedPaths() const override
  {
    auto topLevelCreatedPath = getCreatedPath();
    return {topLevelCreatedPath, getEdgeDataPath(), getVertexDataPath(), topLevelCreatedPath.createChildPath(k_DefaultEdgesName), topLevelCreatedPath.createChildPath(k_DefaultVerticesName)};
  }

private:
  IGeometry::MeshIndexType m_NumEdges;
  IGeometry::MeshIndexType m_NumVertices;
  std::string m_VertexDataName;
  std::string m_EdgeDataName;
};

using CreateEdgeGeometryAction = CreateGeometry1DAction<EdgeGeom>;
} // namespace complex
