#pragma once

#include "simplnx/Common/Array.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/Filter/Output.hpp"
#include "simplnx/Utilities/ArrayCreationUtilities.hpp"
#include "simplnx/simplnx_export.hpp"

#include <fmt/core.h>

#include <utility>

namespace nx::core
{

/**
 * @class CreateVertexGeometryAction
 * @brief Creates a VertexGeom and its vertex support arrays.
 *
 * Copy, Move, and Reference attach a supplied vertex array. Create allocates a
 * new vertex array and attribute matrix.
 */

class CreateVertexGeometryAction : public IDataCreationAction
{
public:
  CreateVertexGeometryAction(const DataPath& geometryPath, IGeometry::MeshIndexType numVertices, const std::string& vertexAttributeMatrixName, const std::string& sharedVertexListName)
  : IDataCreationAction(geometryPath)
  , m_NumVertices(numVertices)
  , m_VertexDataName(vertexAttributeMatrixName)
  , m_SharedVertexListName(sharedVertexListName)
  {
  }

  CreateVertexGeometryAction(const DataPath& geometryPath, const DataPath& inputVerticesArrayPath, const std::string& vertexAttributeMatrixName, const ArrayHandlingType& arrayType)
  : IDataCreationAction(geometryPath)
  , m_VertexDataName(vertexAttributeMatrixName)
  , m_SharedVertexListName(inputVerticesArrayPath.getTargetName())
  , m_InputVertices(inputVerticesArrayPath)
  , m_ArrayHandlingType(arrayType)
  {
  }

  ~CreateVertexGeometryAction() noexcept override = default;

  CreateVertexGeometryAction(const CreateVertexGeometryAction&) = delete;
  CreateVertexGeometryAction(CreateVertexGeometryAction&&) noexcept = delete;
  CreateVertexGeometryAction& operator=(const CreateVertexGeometryAction&) = delete;
  CreateVertexGeometryAction& operator=(CreateVertexGeometryAction&&) noexcept = delete;

  /**
   * @brief Creates and configures the VertexGeom.
   * @param dataStructure Destination data structure.
   * @param mode Preflight or execute action mode.
   * @return Validation, allocation, or reparenting errors.
   *
   * Copy, Move, and Reference preserve the supplied vertex-array storage.
   */
  Result<> apply(DataStructure& dataStructure, Mode mode) const override
  {
    static constexpr StringLiteral prefix = "CreateVertexGeometryAction: ";
    if(getCreatedPath().empty())
    {
      return MakeErrorResult(-6101, fmt::format("{}Geometry Path cannot be empty", prefix));
    }

    BaseGroup* parentObject = dataStructure.getDataAs<BaseGroup>(getCreatedPath());
    if(parentObject != nullptr)
    {
      return MakeErrorResult(-6102, fmt::format("{}DataObject already exists at path '{}'", prefix, getCreatedPath().toString()));
    }

    DataPath parentPath = getCreatedPath().getParent();
    if(!parentPath.empty())
    {
      Result<LinkedPath> geomPath = dataStructure.makePath(parentPath);
      if(geomPath.invalid())
      {
        return MakeErrorResult(-6103, fmt::format("{}Geometry could not be created at path:'{}'", prefix, getCreatedPath().toString()));
      }
    }
    if(!dataStructure.getId(parentPath).has_value())
    {
      return MakeErrorResult(-6104, fmt::format("{}Parent Id was not available for path:'{}'", prefix, parentPath.toString()));
    }

    const auto vertices = dataStructure.getDataAs<Float32Array>(m_InputVertices);
    if(m_ArrayHandlingType != ArrayHandlingType::Create && vertices == nullptr)
    {
      return MakeErrorResult(-6105, fmt::format("{}Could not find vertices array at path '{}'", prefix, m_InputVertices.toString()));
    }

    VertexGeom* vertexGeom = VertexGeom::Create(dataStructure, getCreatedPath().getTargetName(), dataStructure.getId(parentPath).value());

    ShapeType tupleShape = {m_NumVertices};

    if(m_ArrayHandlingType != ArrayHandlingType::Create)
    {
      tupleShape = vertices->getTupleShape();
    }

    if(m_ArrayHandlingType == ArrayHandlingType::Copy)
    {
      std::shared_ptr<DataObject> copy = vertices->deepCopy(getCreatedPath().createChildPath(m_SharedVertexListName));
      const auto vertexArray = std::dynamic_pointer_cast<Float32Array>(copy);

      vertexGeom->setVertices(*vertexArray);
    }
    else if(m_ArrayHandlingType == ArrayHandlingType::Move)
    {
      const auto geomId = vertexGeom->getId();
      const auto verticesId = vertices->getId();
      dataStructure.setAdditionalParent(verticesId, geomId);
      const auto oldParentId = dataStructure.getId(m_InputVertices.getParent());
      if(!oldParentId.has_value())
      {
        return MakeErrorResult(
            -6106, fmt::format("{}Failed to remove vertices array '{}' from parent at path '{}' while moving array", prefix, m_SharedVertexListName, m_InputVertices.getParent().toString()));
      }
      dataStructure.removeParent(verticesId, oldParentId.value());
      vertexGeom->setVertices(*vertices);
    }
    else if(m_ArrayHandlingType == ArrayHandlingType::Reference)
    {
      dataStructure.setAdditionalParent(vertices->getId(), vertexGeom->getId());
      vertexGeom->setVertices(*vertices);
    }
    else
    {
      const DataPath vertexPath = getCreatedPath().createChildPath(m_SharedVertexListName);
      const ShapeType componentShape = {3};

      Result<> result = ArrayCreationUtilities::CreateArray<float32>(dataStructure, tupleShape, componentShape, vertexPath, mode);
      if(result.invalid())
      {
        return result;
      }
      auto* vertexArray = dataStructure.getDataAs<Float32Array>(vertexPath);
      if(vertexArray == nullptr)
      {
        throw std::runtime_error(fmt::format("DataPath does not point to a DataArray. DataPath: '{}'", vertexPath.toString()));
      }
      vertexGeom->setVertices(*vertexArray);
    }

    auto* vertexAttributeMatrix = AttributeMatrix::Create(dataStructure, m_VertexDataName, tupleShape, vertexGeom->getId());
    if(vertexAttributeMatrix == nullptr)
    {
      return MakeErrorResult(-6107, fmt::format("{}Failed to create attribute matrix: '{}'", prefix, getVertexDataPath().toString()));
    }
    vertexGeom->setVertexAttributeMatrix(*vertexAttributeMatrix);

    return {};
  }

  UniquePointer clone() const override
  {
    auto action = std::unique_ptr<CreateVertexGeometryAction>(new CreateVertexGeometryAction(getCreatedPath(), m_NumVertices, m_VertexDataName, m_SharedVertexListName));
    action->m_InputVertices = m_InputVertices;
    action->m_ArrayHandlingType = m_ArrayHandlingType;
    return action;
  }

  DataPath geometryPath() const
  {
    return getCreatedPath();
  }

  IGeometry::MeshIndexType numVertices() const
  {
    return m_NumVertices;
  }

  DataPath getVertexDataPath() const
  {
    return getCreatedPath().createChildPath(m_VertexDataName);
  }

  DataPath getSharedVertexListDataPath() const
  {
    return getCreatedPath().createChildPath(m_SharedVertexListName);
  }

  /**
   * @brief Returns paths created by this action.
   * @return Geometry and attribute-matrix paths. Create and Copy also return
   * the vertex-array path.
   */
  std::vector<DataPath> getAllCreatedPaths() const override
  {
    const auto topLevelCreatedPath = getCreatedPath();
    std::vector<DataPath> createdPaths = {topLevelCreatedPath, getVertexDataPath()};
    if(m_ArrayHandlingType == ArrayHandlingType::Create || m_ArrayHandlingType == ArrayHandlingType::Copy)
    {
      createdPaths.push_back(topLevelCreatedPath.createChildPath(m_SharedVertexListName));
    }
    return createdPaths;
  }

protected:
  CreateVertexGeometryAction() = default;

private:
  IGeometry::MeshIndexType m_NumVertices = 1;
  std::string m_VertexDataName;
  std::string m_SharedVertexListName;
  DataPath m_InputVertices;
  ArrayHandlingType m_ArrayHandlingType = ArrayHandlingType::Create;
};

} // namespace nx::core
