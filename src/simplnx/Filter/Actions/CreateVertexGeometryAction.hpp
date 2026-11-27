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

#include <memory>
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
  /**
   * @brief Creates an action that allocates a new vertex coordinate array.
   * @param geometryPath Path of the VertexGeom to create.
   * @param numVertices Number of vertices to allocate.
   * @param vertexAttributeMatrixName Name of the vertex attribute matrix.
   * @param sharedVertexListName Name of the vertex coordinate array.
   * @param units Length unit of the vertex coordinates. The default is Meter. Use Unspecified when the coordinates are not lengths.
   */
  CreateVertexGeometryAction(const DataPath& geometryPath, IGeometry::MeshIndexType numVertices, std::string vertexAttributeMatrixName, std::string sharedVertexListName,
                             IGeometry::LengthUnit units = IGeometry::LengthUnit::Meter)
  : IDataCreationAction(geometryPath)
  , m_NumVertices(numVertices)
  , m_VertexDataName(std::move(vertexAttributeMatrixName))
  , m_SharedVertexListName(std::move(sharedVertexListName))
  , m_Units(units)
  {
  }

  /**
   * @brief Creates an action that uses an existing Float32 vertex coordinate array.
   * @param geometryPath Path of the VertexGeom to create.
   * @param inputVerticesArrayPath Path of the existing vertex coordinate array.
   * @param vertexAttributeMatrixName Name of the vertex attribute matrix.
   * @param arrayType Copy, Move, or Reference handling of the input array.
   * @param units Length unit of the vertex coordinates. The default is Meter. Use Unspecified when the coordinates are not lengths.
   */
  CreateVertexGeometryAction(const DataPath& geometryPath, const DataPath& inputVerticesArrayPath, std::string vertexAttributeMatrixName, const ArrayHandlingType& arrayType,
                             IGeometry::LengthUnit units = IGeometry::LengthUnit::Meter)
  : IDataCreationAction(geometryPath)
  , m_VertexDataName(std::move(vertexAttributeMatrixName))
  , m_SharedVertexListName(inputVerticesArrayPath.getTargetName())
  , m_InputVertices(inputVerticesArrayPath)
  , m_ArrayHandlingType(arrayType)
  , m_Units(units)
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
    static constexpr StringLiteralType k_Prefix = "CreateVertexGeometryAction: ";
    if(getCreatedPath().empty())
    {
      return MakeErrorResult(-6101, fmt::format("{}Geometry Path cannot be empty", k_Prefix));
    }

    BaseGroup* parentObjectPtr = dataStructure.getDataAs<BaseGroup>(getCreatedPath());
    if(parentObjectPtr != nullptr)
    {
      return MakeErrorResult(-6102, fmt::format("{}DataObject already exists at path '{}'", k_Prefix, getCreatedPath().toString()));
    }

    DataPath parentPath = getCreatedPath().getParent();
    if(!parentPath.empty())
    {
      Result<LinkedPath> geomPath = dataStructure.makePath(parentPath);
      if(geomPath.invalid())
      {
        return MakeErrorResult(-6103, fmt::format("{}Geometry could not be created at path:'{}'", k_Prefix, getCreatedPath().toString()));
      }
    }
    if(!dataStructure.getId(parentPath).has_value())
    {
      return MakeErrorResult(-6104, fmt::format("{}Parent Id was not available for path:'{}'", k_Prefix, parentPath.toString()));
    }

    auto* const verticesPtr = dataStructure.getDataAs<Float32ArrayType>(m_InputVertices);
    if(m_ArrayHandlingType != ArrayHandlingType::Create && verticesPtr == nullptr)
    {
      return MakeErrorResult(-6105, fmt::format("{}Could not find vertices array at path '{}'", k_Prefix, m_InputVertices.toString()));
    }

    VertexGeom* vertexGeomPtr = VertexGeom::Create(dataStructure, getCreatedPath().getTargetName(), dataStructure.getId(parentPath).value());
    if(vertexGeomPtr == nullptr)
    {
      return MakeErrorResult(-6108,
                             fmt::format("{}Vertex Geometry could not be created at path '{}'. Choose an unused path under a DataGroup or another geometry.", k_Prefix, getCreatedPath().toString()));
    }
    vertexGeomPtr->setUnits(m_Units);

    ShapeType tupleShape = {m_NumVertices};

    if(m_ArrayHandlingType != ArrayHandlingType::Create)
    {
      tupleShape = verticesPtr->getTupleShape();
    }

    if(m_ArrayHandlingType == ArrayHandlingType::Copy)
    {
      std::shared_ptr<DataObject> copy = verticesPtr->deepCopy(getCreatedPath().createChildPath(m_SharedVertexListName));
      const auto vertexArray = std::dynamic_pointer_cast<Float32ArrayType>(copy);

      vertexGeomPtr->setVertices(*vertexArray);
    }
    else if(m_ArrayHandlingType == ArrayHandlingType::Move)
    {
      const auto geomId = vertexGeomPtr->getId();
      const auto verticesId = verticesPtr->getId();
      dataStructure.setAdditionalParent(verticesId, geomId);
      const auto oldParentId = dataStructure.getId(m_InputVertices.getParent());
      if(!oldParentId.has_value())
      {
        return MakeErrorResult(
            -6106, fmt::format("{}Failed to remove vertices array '{}' from parent at path '{}' while moving array", k_Prefix, m_SharedVertexListName, m_InputVertices.getParent().toString()));
      }
      dataStructure.removeParent(verticesId, oldParentId.value());
      vertexGeomPtr->setVertices(*verticesPtr);
    }
    else if(m_ArrayHandlingType == ArrayHandlingType::Reference)
    {
      dataStructure.setAdditionalParent(verticesPtr->getId(), vertexGeomPtr->getId());
      vertexGeomPtr->setVertices(*verticesPtr);
    }
    else
    {
      const DataPath vertexPath = getCreatedPath().createChildPath(m_SharedVertexListName);
      const ShapeType componentShape = {3};

      Result<> result = array_creation_utilities::CreateArray<float32>(dataStructure, tupleShape, componentShape, vertexPath, mode);
      if(result.invalid())
      {
        return result;
      }
      auto* vertexArrayPtr = dataStructure.getDataAs<Float32ArrayType>(vertexPath);
      if(vertexArrayPtr == nullptr)
      {
        throw std::runtime_error(fmt::format("DataPath does not point to a DataArray. DataPath: '{}'", vertexPath.toString()));
      }
      vertexGeomPtr->setVertices(*vertexArrayPtr);
    }

    auto* vertexAttributeMatrixPtr = AttributeMatrix::Create(dataStructure, m_VertexDataName, tupleShape, vertexGeomPtr->getId());
    if(vertexAttributeMatrixPtr == nullptr)
    {
      return MakeErrorResult(-6107, fmt::format("{}Failed to create attribute matrix: '{}'", k_Prefix, getVertexDataPath().toString()));
    }
    vertexGeomPtr->setVertexAttributeMatrix(*vertexAttributeMatrixPtr);

    return {};
  }

  UniquePointerType clone() const override
  {
    auto action = std::make_unique<CreateVertexGeometryAction>(getCreatedPath(), m_NumVertices, m_VertexDataName, m_SharedVertexListName, m_Units);
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

  IGeometry::LengthUnit units() const
  {
    return m_Units;
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
  IGeometry::LengthUnit m_Units = IGeometry::LengthUnit::Meter;
};

} // namespace nx::core
