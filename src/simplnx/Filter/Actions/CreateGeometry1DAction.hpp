#pragma once

#include "simplnx/Common/Array.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/Filter/Output.hpp"
#include "simplnx/Utilities/ArrayCreationUtilities.hpp"
#include "simplnx/simplnx_export.hpp"

#include <fmt/core.h>

#include <utility>

namespace nx::core
{

/**
 * @class CreateGeometry1DAction
 * @brief Creates a one-dimensional geometry and its support arrays.
 * @tparam Geometry1DType Edge-geometry type to create.
 *
 * Copy, Move, and Reference attach supplied topology arrays. Create allocates
 * new topology arrays and attribute matrices.
 */
template <typename Geometry1DType>
class CreateGeometry1DAction : public IDataCreationAction
{
public:
  using DimensionType = std::vector<size_t>;

  CreateGeometry1DAction(const DataPath& geometryPath, size_t numEdges, size_t numVertices, const std::string& vertexAttributeMatrixName, const std::string& edgeAttributeMatrixName,
                         const std::string& sharedVerticesName, const std::string& sharedEdgesName)
  : IDataCreationAction(geometryPath)
  , m_NumEdges(numEdges)
  , m_NumVertices(numVertices)
  , m_VertexDataName(vertexAttributeMatrixName)
  , m_EdgeDataName(edgeAttributeMatrixName)
  , m_SharedVerticesName(sharedVerticesName)
  , m_SharedEdgesName(sharedEdgesName)
  {
  }

  CreateGeometry1DAction(const DataPath& geometryPath, const DataPath& inputVerticesArrayPath, const DataPath& inputEdgesArrayPath, const std::string& vertexAttributeMatrixName,
                         const std::string& edgeAttributeMatrixName, const ArrayHandlingType& arrayType)
  : IDataCreationAction(geometryPath)
  , m_VertexDataName(vertexAttributeMatrixName)
  , m_EdgeDataName(edgeAttributeMatrixName)
  , m_SharedVerticesName(inputVerticesArrayPath.getTargetName())
  , m_SharedEdgesName(inputEdgesArrayPath.getTargetName())
  , m_InputVertices(inputVerticesArrayPath)
  , m_InputEdges(inputEdgesArrayPath)
  , m_ArrayHandlingType(arrayType)
  {
  }

  ~CreateGeometry1DAction() noexcept override = default;

  CreateGeometry1DAction(const CreateGeometry1DAction&) = delete;
  CreateGeometry1DAction(CreateGeometry1DAction&&) noexcept = delete;
  CreateGeometry1DAction& operator=(const CreateGeometry1DAction&) = delete;
  CreateGeometry1DAction& operator=(CreateGeometry1DAction&&) noexcept = delete;

  /**
   * @brief Creates and configures the edge geometry.
   * @param dataStructure Destination data structure.
   * @param mode Preflight or execute action mode.
   * @return Validation, allocation, or reparenting errors.
   *
   * Copy, Move, and Reference preserve the supplied topology-array storage.
   */
  Result<> apply(DataStructure& dataStructure, Mode mode) const override
  {
    static constexpr StringLiteral prefix = "CreateGeometry1DAction: ";

    using MeshIndexType = IGeometry::MeshIndexType;
    using SharedEdgeList = IGeometry::SharedEdgeList;
    DataPath edgeDataPath = getEdgeDataPath();
    DataPath vertexDataPath = getVertexDataPath();

    if(getCreatedPath().empty())
    {
      return MakeErrorResult(-5401, fmt::format("{}CreateGeometry1DAction: Geometry Path cannot be empty", prefix));
    }

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
    if(!dataStructure.getId(parentPath).has_value())
    {
      return MakeErrorResult(-5404, fmt::format("{}CreateGeometry1DAction: Parent Id was not available for path:'{}'", prefix, parentPath.toString()));
    }

    const auto vertices = dataStructure.getDataAs<Float32Array>(m_InputVertices);
    if(m_ArrayHandlingType != ArrayHandlingType::Create && vertices == nullptr)
    {
      return MakeErrorResult(-5405, fmt::format("{}CreateGeometry1DAction: Could not find vertices array at path '{}'", prefix, m_InputVertices.toString()));
    }

    const auto edges = dataStructure.getDataAs<DataArray<MeshIndexType>>(m_InputEdges);
    if(m_ArrayHandlingType != ArrayHandlingType::Create && edges == nullptr)
    {
      return MakeErrorResult(-5406, fmt::format("{}CreateGeometry1DAction: Could not find edges array at path '{}'", prefix, m_InputEdges.toString()));
    }

    auto geometry1d = Geometry1DType::Create(dataStructure, getCreatedPath().getTargetName(), dataStructure.getId(parentPath).value());
    DimensionType edgeTupleShape = {m_NumEdges};
    DimensionType vertexTupleShape = {m_NumVertices};

    if(m_ArrayHandlingType != ArrayHandlingType::Create)
    {
      edgeTupleShape = edges->getTupleShape();
      vertexTupleShape = vertices->getTupleShape();
    }

    if(m_ArrayHandlingType == ArrayHandlingType::Copy)
    {
      std::shared_ptr<DataObject> vertexCopy = vertices->deepCopy(getCreatedPath().createChildPath(m_SharedVerticesName));
      const auto vertexArray = std::dynamic_pointer_cast<Float32Array>(vertexCopy);

      std::shared_ptr<DataObject> edgesCopy = edges->deepCopy(getCreatedPath().createChildPath(m_SharedEdgesName));
      const auto edgesArray = std::dynamic_pointer_cast<DataArray<MeshIndexType>>(edgesCopy);

      geometry1d->setEdgeList(*edgesArray);
      geometry1d->setVertices(*vertexArray);
    }
    else if(m_ArrayHandlingType == ArrayHandlingType::Move)
    {
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
      const auto geomId = geometry1d->getId();
      dataStructure.setAdditionalParent(vertices->getId(), geomId);
      dataStructure.setAdditionalParent(edges->getId(), geomId);
      geometry1d->setVertices(*vertices);
      geometry1d->setEdgeList(*edges);
    }
    else
    {
      DataPath edgesPath = getCreatedPath().createChildPath(m_SharedEdgesName);
      Result result = ArrayCreationUtilities::CreateArray<MeshIndexType>(dataStructure, edgeTupleShape, {2}, edgesPath, mode);
      if(result.invalid())
      {
        return MergeResults(result, MakeErrorResult(-5409, fmt::format("{}CreateGeometry1DAction: Could not allocate SharedEdgeList '{}'", prefix, edgesPath.toString())));
      }
      auto* createdEdges = dataStructure.getDataAs<SharedEdgeList>(edgesPath);
      if(createdEdges == nullptr)
      {
        throw std::runtime_error(fmt::format("DataPath does not point to a DataArray. DataPath: '{}'", edgesPath.toString()));
      }
      geometry1d->setEdgeList(*createdEdges);

      // Vertices use three coordinates.
      DataPath vertexPath = getCreatedPath().createChildPath(m_SharedVerticesName);

      result = ArrayCreationUtilities::CreateArray<float32>(dataStructure, vertexTupleShape, {3}, vertexPath, mode);
      if(result.invalid())
      {
        return MergeResults(result, MakeErrorResult(-5410, fmt::format("{}CreateGeometry1DAction: Could not allocate SharedVertList '{}'", prefix, vertexPath.toString())));
      }
      auto* vertexArray = dataStructure.getDataAs<Float32Array>(vertexPath);
      if(vertexArray == nullptr)
      {
        throw std::runtime_error(fmt::format("DataPath does not point to a DataArray. DataPath: '{}'", vertexPath.toString()));
      }
      geometry1d->setVertices(*vertexArray);
    }

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

  UniquePointer clone() const override
  {
    auto action =
        std::unique_ptr<CreateGeometry1DAction>(new CreateGeometry1DAction(getCreatedPath(), m_NumEdges, m_NumVertices, m_VertexDataName, m_EdgeDataName, m_SharedVerticesName, m_SharedEdgesName));
    action->m_InputVertices = m_InputVertices;
    action->m_InputEdges = m_InputEdges;
    action->m_ArrayHandlingType = m_ArrayHandlingType;
    return action;
  }

  const DataPath& geometryPath() const
  {
    return getCreatedPath();
  }

  DataPath getEdgeDataPath() const
  {
    return getCreatedPath().createChildPath(m_EdgeDataName);
  }

  DataPath getVertexDataPath() const
  {
    return getCreatedPath().createChildPath(m_VertexDataName);
  }

  IGeometry::MeshIndexType numEdges() const
  {
    return m_NumEdges;
  }

  IGeometry::MeshIndexType numVertices() const
  {
    return m_NumVertices;
  }

  /**
   * @brief Returns paths created by this action.
   * @return Geometry and attribute-matrix paths. Create and Copy also return
   * topology-array paths.
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
};

using CreateEdgeGeometryAction = CreateGeometry1DAction<EdgeGeom>;
} // namespace nx::core
