#pragma once

#include "simplnx/Common/Array.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/DataStructure/IDataStore.hpp"
#include "simplnx/Filter/Output.hpp"
#include "simplnx/Utilities/ArrayCreationUtilities.hpp"
#include "simplnx/simplnx_export.hpp"

#include <fmt/core.h>

#include <utility>

namespace nx::core
{
/**
 * @brief Action for creating a Vertex Geometry in a DataStructure
 */

class CreateVertexGeometryAction : public IDataCreationAction
{
public:
  /**
   * @brief Constructor to create the vertex geometry and allocate a default array for the shared vertex list
   * @param geometryPath The path to the created geometry
   * @param numVertices The number of vertices in the geometry
   * @param vertexAttributeMatrixName The name of the vertex AttributeMatrix to be created
   * @param sharedVertexListName The name of the shared vertex list array to be created
   */
  CreateVertexGeometryAction(const DataPath& geometryPath, IGeometry::MeshIndexType numVertices, const std::string& vertexAttributeMatrixName, const std::string& sharedVertexListName)
  : IDataCreationAction(geometryPath)
  , m_NumVertices(numVertices)
  , m_VertexDataName(vertexAttributeMatrixName)
  , m_SharedVertexListName(sharedVertexListName)
  {
  }

  /**
   * @brief Constructor to create the vertex geometry using an existing vertices array by either copying, moving, or referencing it
   * @param geometryPath The path to the created geometry
   * @param inputVerticesArrayPath The path to the existing vertices array
   * @param vertexAttributeMatrixName The name of the vertex AttributeMatrix to be created
   * @param arrayType Tells whether to copy, move, or reference the existing input vertices array
   */
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
   * @brief Applies this action's change to the given DataStructure in the given mode.
   * Returns any warnings/errors. On error, DataStructure is not guaranteed to be consistent.
   * @param dataStructure The DataStructure to modify
   * @param mode The mode (Preflight or Execute)
   * @return Result<> Result with any errors or warnings
   */
  Result<> apply(DataStructure& dataStructure, Mode mode) const override
  {
    static constexpr StringLiteral prefix = "CreateVertexGeometryAction: ";
    // Check for empty Geometry DataPath
    if(getCreatedPath().empty())
    {
      return MakeErrorResult(-6101, fmt::format("{}Geometry Path cannot be empty", prefix));
    }

    // Check if the Geometry Path already exists
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
    // Get the Parent ID
    if(!dataStructure.getId(parentPath).has_value())
    {
      return MakeErrorResult(-6104, fmt::format("{}Parent Id was not available for path:'{}'", prefix, parentPath.toString()));
    }

    // Get the vertices list if we are using an existing array
    const auto vertices = dataStructure.getDataAs<Float32Array>(m_InputVertices);
    if(m_ArrayHandlingType != ArrayHandlingType::Create && vertices == nullptr)
    {
      return MakeErrorResult(-6105, fmt::format("{}Could not find vertices array at path '{}'", prefix, m_InputVertices.toString()));
    }

    // Create the VertexGeom
    VertexGeom* vertexGeom = VertexGeom::Create(dataStructure, getCreatedPath().getTargetName(), dataStructure.getId(parentPath).value());

    ShapeType tupleShape = {m_NumVertices}; // We don't probably know how many Vertices there are but take what ever the developer sends us

    // For Copy/Move/Reference, read shapes and materialize OOC stores upfront
    if(m_ArrayHandlingType != ArrayHandlingType::Create)
    {
      tupleShape = vertices->getTupleShape();

      // If the source array has an OOC-backed store, materialize it into
      // an in-core store. The array may have been created OOC earlier in
      // the pipeline when it lived outside any geometry. Unstructured/poly
      // geometry topology arrays must be in-core for the visualization layer.
      if(vertices->getIDataStore()->getStoreType() == IDataStore::StoreType::OutOfCore)
      {
        auto inCoreStore = std::make_shared<DataStore<float32>>(tupleShape, ShapeType{3}, std::optional<float32>{});
        auto copyResult = vertices->getDataStoreRef().copyIntoBuffer(0, nonstd::span<float32>(inCoreStore->data(), inCoreStore->getSize()));
        if(copyResult.invalid())
        {
          return MakeErrorResult(-6107, fmt::format("{}Failed to materialize OOC vertices array '{}' into in-core store: {}", prefix, m_InputVertices.toString(),
                                                    copyResult.errors().empty() ? "unknown error" : copyResult.errors()[0].message));
        }
        vertices->setDataStore(std::move(inCoreStore));
      }
    }

    // Create the Vertex Array with a component size of 3
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

    // Create the Vertex AttributeMatrix
    auto* vertexAttributeMatrix = AttributeMatrix::Create(dataStructure, m_VertexDataName, tupleShape, vertexGeom->getId());
    if(vertexAttributeMatrix == nullptr)
    {
      return MakeErrorResult(-6107, fmt::format("{}Failed to create attribute matrix: '{}'", prefix, getVertexDataPath().toString()));
    }
    vertexGeom->setVertexAttributeMatrix(*vertexAttributeMatrix);

    return {};
  }

  /**
   * @brief Returns a copy of the action.
   * @return UniquePointer A unique pointer to the cloned action
   */
  UniquePointer clone() const override
  {
    auto action = std::unique_ptr<CreateVertexGeometryAction>(new CreateVertexGeometryAction(getCreatedPath(), m_NumVertices, m_VertexDataName, m_SharedVertexListName));
    action->m_InputVertices = m_InputVertices;
    action->m_ArrayHandlingType = m_ArrayHandlingType;
    return action;
  }

  /**
   * @brief Returns the path of the VertexGeometry to be created.
   * @return DataPath The geometry path
   */
  DataPath geometryPath() const
  {
    return getCreatedPath();
  }

  /**
   * @brief Returns the number of vertices (estimated in some circumstances).
   * @return IGeometry::MeshIndexType The number of vertices
   */
  IGeometry::MeshIndexType numVertices() const
  {
    return m_NumVertices;
  }

  /**
   * @brief Returns the path of the vertex data AttributeMatrix in the created geometry.
   * @return DataPath The vertex data path
   */
  DataPath getVertexDataPath() const
  {
    return getCreatedPath().createChildPath(m_VertexDataName);
  }

  /**
   * @brief Returns the path of the shared vertex list in the created geometry.
   * @return DataPath The shared vertex list path
   */
  DataPath getSharedVertexListDataPath() const
  {
    return getCreatedPath().createChildPath(m_SharedVertexListName);
  }

  /**
   * @brief Returns all of the DataPaths to be created.
   * @return std::vector<DataPath>
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
