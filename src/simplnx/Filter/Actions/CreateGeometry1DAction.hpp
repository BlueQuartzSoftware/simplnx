#pragma once

#include "simplnx/Common/Array.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/Filter/Output.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/simplnx_export.hpp"

#include <fmt/core.h>

#include <utility>

namespace nx::core
{
/**
 * @brief Action for creating an Edge Geometry in a DataStructure
 */
template <typename Geometry1DType>
class CreateGeometry1DAction : public IDataCreationAction
{
public:
  using DimensionType = std::vector<size_t>;

  static constexpr StringLiteral k_DefaultVerticesName = "SharedVertexList";
  static constexpr StringLiteral k_DefaultEdgesName = "SharedEdgeList";

  /**
   * @brief Constructor to create the 1D geometry and allocate a default arrays for the shared vertex & shared edge lists
   * @param geometryPath The path to the created geometry
   * @param numEdges The number of edges in the geometry
   * @param numVertices The number of vertices in the geometry
   * @param vertexAttributeMatrixName The name of the vertex AttributeMatrix to be created
   * @param edgeAttributeMatrixName The name of the edge AttributeMatrix to be created
   * @param sharedVerticesName The name of the shared vertex list array to be created
   * @param sharedEdgesName The name of the shared edge list array to be created
   */
  CreateGeometry1DAction(const DataPath& geometryPath, size_t numEdges, size_t numVertices, const std::string& vertexAttributeMatrixName, const std::string& edgeAttributeMatrixName,
                         const std::string& sharedVerticesName, const std::string& sharedEdgesName, std::string createdDataFormat = "")
  : IDataCreationAction(geometryPath)
  , m_NumEdges(numEdges)
  , m_NumVertices(numVertices)
  , m_VertexDataName(vertexAttributeMatrixName)
  , m_EdgeDataName(edgeAttributeMatrixName)
  , m_SharedVerticesName(sharedVerticesName)
  , m_SharedEdgesName(sharedEdgesName)
  , m_CreatedDataStoreFormat(createdDataFormat)
  {
  }

  /**
   * @brief Constructor to create the 1D geometry using existing vertices & edges arrays by either copying, moving, or referencing them
   * @param geometryPath The path to the created geometry
   * @param inputVerticesArrayPath The path to the existing vertices array
   * @param inputEdgesArrayPath The path to the existing edges array
   * @param vertexAttributeMatrixName The name of the vertex AttributeMatrix to be created
   * @param edgeAttributeMatrixName The name of the edge AttributeMatrix to be created
   * @param arrayType Tells whether to copy, move, or reference the existing input vertices array
   */
  CreateGeometry1DAction(const DataPath& geometryPath, const DataPath& inputVerticesArrayPath, const DataPath& inputEdgesArrayPath, const std::string& vertexAttributeMatrixName,
                         const std::string& edgeAttributeMatrixName, const ArrayHandlingType& arrayType, std::string createdDataFormat = "")
  : IDataCreationAction(geometryPath)
  , m_VertexDataName(vertexAttributeMatrixName)
  , m_EdgeDataName(edgeAttributeMatrixName)
  , m_SharedVerticesName(inputVerticesArrayPath.getTargetName())
  , m_SharedEdgesName(inputEdgesArrayPath.getTargetName())
  , m_InputVertices(inputVerticesArrayPath)
  , m_InputEdges(inputEdgesArrayPath)
  , m_ArrayHandlingType(arrayType)
  , m_CreatedDataStoreFormat(createdDataFormat)
  {
  }

  ~CreateGeometry1DAction() noexcept override = default;

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
    static constexpr StringLiteral prefix = "CreateGeometry1DAction: ";

    using MeshIndexType = IGeometry::MeshIndexType;
    using SharedEdgeList = IGeometry::SharedEdgeList;
    DataPath edgeDataPath = getEdgeDataPath();
    DataPath vertexDataPath = getVertexDataPath();

    // Check for empty Geometry DataPath
    if(getCreatedPath().empty())
    {
      return MakeErrorResult(-5401, fmt::format("{}CreateGeometry1DAction: Geometry Path cannot be empty", prefix));
    }

    // Check if the Geometry Path already exists
    const BaseGroup* parentObject = dataStructure.getDataAs<BaseGroup>(getCreatedPath());
    if(parentObject != nullptr)
    {
      return MakeErrorResult(-5402, fmt::format("{}CreateGeometry1DAction: DataObject already exists at path '{}'", prefix, getCreatedPath().toString()));
    }

    DataPath parentPath = getCreatedPath().getParent();
    if(!parentPath.empty())
    {
      Result<LinkedPath> geomPath = dataStructure.makePath(parentPath);
      if(geomPath.invalid())
      {
        return MakeErrorResult(-5403, fmt::format("{}CreateGeometry1DAction: Geometry could not be created at path:'{}'", prefix, getCreatedPath().toString()));
      }
    }
    // Get the Parent ID
    if(!dataStructure.getId(parentPath).has_value())
    {
      return MakeErrorResult(-5404, fmt::format("{}CreateGeometry1DAction: Parent Id was not available for path:'{}'", prefix, parentPath.toString()));
    }

    // Get the vertices list if we are using an existing array
    const auto vertices = dataStructure.getDataAs<Float32Array>(m_InputVertices);
    if(m_ArrayHandlingType != ArrayHandlingType::Create && vertices == nullptr)
    {
      return MakeErrorResult(-5405, fmt::format("{}CreateGeometry1DAction: Could not find vertices array at path '{}'", prefix, m_InputVertices.toString()));
    }

    // Get the faces list if we are using an existing array
    const auto edges = dataStructure.getDataAs<DataArray<MeshIndexType>>(m_InputEdges);
    if(m_ArrayHandlingType != ArrayHandlingType::Create && edges == nullptr)
    {
      return MakeErrorResult(-5406, fmt::format("{}CreateGeometry1DAction: Could not find edges array at path '{}'", prefix, m_InputEdges.toString()));
    }

    // Create the EdgeGeometry
    auto geometry1d = Geometry1DType::Create(dataStructure, getCreatedPath().getTargetName(), dataStructure.getId(parentPath).value());
    DimensionType edgeTupleShape = {m_NumEdges};
    DimensionType vertexTupleShape = {m_NumVertices}; // We probably don't know how many Vertices there are but take what ever the developer sends us

    if(m_ArrayHandlingType == ArrayHandlingType::Copy)
    {
      edgeTupleShape = edges->getTupleShape();
      vertexTupleShape = vertices->getTupleShape();

      std::shared_ptr<DataObject> vertexCopy = vertices->deepCopy(getCreatedPath().createChildPath(m_SharedVerticesName));
      const auto vertexArray = std::dynamic_pointer_cast<Float32Array>(vertexCopy);

      std::shared_ptr<DataObject> edgesCopy = edges->deepCopy(getCreatedPath().createChildPath(m_SharedEdgesName));
      const auto edgesArray = std::dynamic_pointer_cast<DataArray<MeshIndexType>>(edgesCopy);

      geometry1d->setEdgeList(*edgesArray);
      geometry1d->setVertices(*vertexArray);
    }
    else if(m_ArrayHandlingType == ArrayHandlingType::Move)
    {
      edgeTupleShape = edges->getTupleShape();
      vertexTupleShape = vertices->getTupleShape();
      const auto geomId = geometry1d->getId();

      const auto verticesId = vertices->getId();
      dataStructure.setAdditionalParent(verticesId, geomId);
      const auto oldVertexParentId = dataStructure.getId(m_InputVertices.getParent());
      if(!oldVertexParentId.has_value())
      {
        return MakeErrorResult(-5407, fmt::format("{}CreateGeometry1DAction: Failed to remove vertices array '{}' from parent at path '{}' while moving array", prefix, m_SharedVerticesName,
                                                  m_InputVertices.getParent().toString()));
      }
      dataStructure.removeParent(verticesId, oldVertexParentId.value());

      const auto edgesId = edges->getId();
      dataStructure.setAdditionalParent(edgesId, geomId);
      const auto oldEdgeParentId = dataStructure.getId(m_InputEdges.getParent());
      if(!oldEdgeParentId.has_value())
      {
        return MakeErrorResult(-5408, fmt::format("{}CreateGeometry1DAction: Failed to remove edges array '{}' from parent at path '{}' while moving array", prefix, m_SharedEdgesName,
                                                  m_InputEdges.getParent().toString()));
      }
      dataStructure.removeParent(edgesId, oldEdgeParentId.value());

      geometry1d->setVertices(*vertices);
      geometry1d->setEdgeList(*edges);
    }
    else if(m_ArrayHandlingType == ArrayHandlingType::Reference)
    {
      edgeTupleShape = edges->getTupleShape();
      vertexTupleShape = vertices->getTupleShape();
      const auto geomId = geometry1d->getId();
      dataStructure.setAdditionalParent(vertices->getId(), geomId);
      dataStructure.setAdditionalParent(edges->getId(), geomId);
      geometry1d->setVertices(*vertices);
      geometry1d->setEdgeList(*edges);
    }
    else
    {
      DataPath edgesPath = getCreatedPath().createChildPath(m_SharedEdgesName);
      // Create the default DataArray that will hold the EdgeList and Vertices. We
      // size these to 1 because the Csv parser will resize them to the appropriate number of tuples
      nx::core::Result result = nx::core::CreateArray<MeshIndexType>(dataStructure, edgeTupleShape, {2}, edgesPath, mode, m_CreatedDataStoreFormat);
      if(result.invalid())
      {
        return MakeErrorResult(-5409, fmt::format("{}CreateGeometry1DAction: Could not allocate SharedEdgeList '{}'", prefix, edgesPath.toString()));
      }
      SharedEdgeList* createdEdges = nx::core::ArrayFromPath<MeshIndexType>(dataStructure, edgesPath);
      geometry1d->setEdgeList(*createdEdges);

      // Create the Vertex Array with a component size of 3
      DataPath vertexPath = getCreatedPath().createChildPath(m_SharedVerticesName);

      result = nx::core::CreateArray<float>(dataStructure, vertexTupleShape, {3}, vertexPath, mode, m_CreatedDataStoreFormat);
      if(result.invalid())
      {
        return MakeErrorResult(-5410, fmt::format("{}CreateGeometry1DAction: Could not allocate SharedVertList '{}'", prefix, vertexPath.toString()));
      }
      Float32Array* vertexArray = nx::core::ArrayFromPath<float>(dataStructure, vertexPath);
      geometry1d->setVertices(*vertexArray);
    }

    // Create the vertex and edge AttributeMatrix
    auto* edgeAttributeMatrix = AttributeMatrix::Create(dataStructure, m_EdgeDataName, edgeTupleShape, geometry1d->getId());
    if(edgeAttributeMatrix == nullptr)
    {
      return MakeErrorResult(-5411, fmt::format("{}CreateGeometry1DAction: Failed to create attribute matrix: '{}'", prefix, edgeDataPath.toString()));
    }
    geometry1d->setEdgeAttributeMatrix(*edgeAttributeMatrix);

    auto* vertexAttributeMatrix = AttributeMatrix::Create(dataStructure, m_VertexDataName, vertexTupleShape, geometry1d->getId());
    if(vertexAttributeMatrix == nullptr)
    {
      return MakeErrorResult(-5412, fmt::format("{}CreateGeometry1DAction: Failed to create attribute matrix: '{}'", prefix, vertexDataPath.toString()));
    }
    geometry1d->setVertexAttributeMatrix(*vertexAttributeMatrix);

    return {};
  }

  /**
   * @brief Returns a copy of the action.
   * @return
   */
  UniquePointer clone() const override
  {
    auto action =
        std::unique_ptr<CreateGeometry1DAction>(new CreateGeometry1DAction(getCreatedPath(), m_NumEdges, m_NumVertices, m_VertexDataName, m_EdgeDataName, m_SharedVerticesName, m_SharedEdgesName));
    action->m_InputVertices = m_InputVertices;
    action->m_InputEdges = m_InputEdges;
    action->m_ArrayHandlingType = m_ArrayHandlingType;
    return action;
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
   * @brief Returns the number of edges
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
    std::vector<DataPath> createdPaths = {topLevelCreatedPath, getEdgeDataPath(), getVertexDataPath()};
    if(m_ArrayHandlingType == ArrayHandlingType::Create || m_ArrayHandlingType == ArrayHandlingType::Copy)
    {
      createdPaths.push_back(topLevelCreatedPath.createChildPath(m_SharedVerticesName));
      createdPaths.push_back(topLevelCreatedPath.createChildPath(m_SharedEdgesName));
    }
    return createdPaths;
  }

protected:
  CreateGeometry1DAction() = default;

private:
  IGeometry::MeshIndexType m_NumEdges = 1;
  IGeometry::MeshIndexType m_NumVertices = 2;
  std::string m_VertexDataName;
  std::string m_EdgeDataName;
  std::string m_SharedVerticesName;
  std::string m_SharedEdgesName;
  DataPath m_InputVertices;
  DataPath m_InputEdges;
  ArrayHandlingType m_ArrayHandlingType = ArrayHandlingType::Create;
  std::string m_CreatedDataStoreFormat;
};

using CreateEdgeGeometryAction = CreateGeometry1DAction<EdgeGeom>;
} // namespace nx::core
