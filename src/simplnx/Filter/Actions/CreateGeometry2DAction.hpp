#pragma once

#include "simplnx/Common/Array.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/DataStructure/Geometry/QuadGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Filter/Output.hpp"
#include "simplnx/Utilities/ArrayCreationUtilities.hpp"
#include "simplnx/simplnx_export.hpp"

#include <fmt/core.h>

#include <utility>

namespace nx::core
{

/**
 * @class CreateGeometry2DAction
 * @brief Creates a two-dimensional geometry and its support arrays.
 * @tparam Geometry2DType Triangle or quad geometry type to create.
 *
 * Copy, Move, and Reference attach supplied topology arrays. Create allocates
 * new topology arrays and attribute matrices.
 */
template <typename Geometry2DType>
class CreateGeometry2DAction : public IDataCreationAction
{
public:
  using DimensionType = std::vector<size_t>;

  CreateGeometry2DAction(const DataPath& geometryPath, size_t numFaces, size_t numVertices, std::string vertexAttributeMatrixName, std::string faceAttributeMatrixName, std::string sharedVerticesName,
                         std::string sharedFacesName)
  : IDataCreationAction(geometryPath)
  , m_NumFaces(numFaces)
  , m_NumVertices(numVertices)
  , m_VertexDataName(std::move(vertexAttributeMatrixName))
  , m_FaceDataName(std::move(faceAttributeMatrixName))
  , m_SharedVerticesName(std::move(sharedVerticesName))
  , m_SharedFacesName(std::move(sharedFacesName))
  {
  }

  CreateGeometry2DAction(const DataPath& geometryPath, const DataPath& inputVerticesArrayPath, const DataPath& inputFacesArrayPath, std::string vertexAttributeMatrixName,
                         std::string faceAttributeMatrixName, const ArrayHandlingType& arrayType)
  : IDataCreationAction(geometryPath)
  , m_VertexDataName(std::move(vertexAttributeMatrixName))
  , m_FaceDataName(std::move(faceAttributeMatrixName))
  , m_SharedVerticesName(inputVerticesArrayPath.getTargetName())
  , m_SharedFacesName(inputFacesArrayPath.getTargetName())
  , m_InputVertices(inputVerticesArrayPath)
  , m_InputFaces(inputFacesArrayPath)
  , m_ArrayHandlingType(arrayType)
  {
  }

  ~CreateGeometry2DAction() noexcept override = default;

  CreateGeometry2DAction(const CreateGeometry2DAction&) = delete;
  CreateGeometry2DAction(CreateGeometry2DAction&&) noexcept = delete;
  CreateGeometry2DAction& operator=(const CreateGeometry2DAction&) = delete;
  CreateGeometry2DAction& operator=(CreateGeometry2DAction&&) noexcept = delete;

  /**
   * @brief Creates and configures the face geometry.
   * @param dataStructure Destination data structure.
   * @param mode Preflight or execute action mode.
   * @return Validation, allocation, or reparenting errors.
   *
   * Copy, Move, and Reference preserve the supplied topology-array storage.
   */
  Result<> apply(DataStructure& dataStructure, Mode mode) const override
  {
    static constexpr StringLiteral k_Prefix = "CreateGeometry2DAction: ";
    using MeshIndexType = IGeometry::MeshIndexType;
    using SharedTriListType = IGeometry::SharedTriList;
    const DataPath faceDataPath = getFaceDataPath();
    const DataPath vertexDataPath = getVertexDataPath();

    if(getCreatedPath().empty())
    {
      return MakeErrorResult(-5501, fmt::format("{}CreateGeometry2DAction: Geometry Path cannot be empty", k_Prefix));
    }

    const BaseGroup* parentObjectPtr = dataStructure.getDataAs<BaseGroup>(getCreatedPath());
    if(parentObjectPtr != nullptr)
    {
      return MakeErrorResult(-5502, fmt::format("{}CreateGeometry2DAction: DataObject already exists at path '{}'", k_Prefix, getCreatedPath().toString()));
    }

    const DataPath parentPath = getCreatedPath().getParent();
    if(!parentPath.empty())
    {
      const Result<LinkedPath> geomPath = dataStructure.makePath(parentPath);
      if(geomPath.invalid())
      {
        return MakeErrorResult(-5503, fmt::format("{}CreateGeometry2DAction: Geometry could not be created at path:'{}'", k_Prefix, getCreatedPath().toString()));
      }
    }
    if(!dataStructure.getId(parentPath).has_value())
    {
      return MakeErrorResult(-5504, fmt::format("{}CreateGeometry2DAction: Parent Id was not available for path:'{}'", k_Prefix, parentPath.toString()));
    }

    auto* const verticesPtr = dataStructure.getDataAs<Float32Array>(m_InputVertices);
    if(m_ArrayHandlingType != ArrayHandlingType::Create && verticesPtr == nullptr)
    {
      return MakeErrorResult(-5505, fmt::format("{}CreateGeometry2DAction: Could not find vertices array at path '{}'", k_Prefix, m_InputVertices.toString()));
    }

    auto* const facesPtr = dataStructure.getDataAs<DataArray<MeshIndexType>>(m_InputFaces);
    if(m_ArrayHandlingType != ArrayHandlingType::Create && facesPtr == nullptr)
    {
      return MakeErrorResult(-5506, fmt::format("{}CreateGeometry2DAction: Could not find faces array at path '{}'", k_Prefix, m_InputFaces.toString()));
    }

    auto geometry2dPtr = Geometry2DType::Create(dataStructure, getCreatedPath().getTargetName(), dataStructure.getId(parentPath).value());
    DimensionType faceTupleShape = {m_NumFaces};
    DimensionType vertexTupleShape = {m_NumVertices};

    if(m_ArrayHandlingType != ArrayHandlingType::Create)
    {
      faceTupleShape = facesPtr->getTupleShape();
      vertexTupleShape = verticesPtr->getTupleShape();
    }

    if(m_ArrayHandlingType == ArrayHandlingType::Copy)
    {
      std::shared_ptr<DataObject> vertexCopy = verticesPtr->deepCopy(getCreatedPath().createChildPath(m_SharedVerticesName));
      const auto vertexArray = std::dynamic_pointer_cast<Float32Array>(vertexCopy);

      std::shared_ptr<DataObject> facesCopy = facesPtr->deepCopy(getCreatedPath().createChildPath(m_SharedFacesName));
      const auto facesArray = std::dynamic_pointer_cast<DataArray<MeshIndexType>>(facesCopy);

      geometry2dPtr->setFaceList(*facesArray);
      geometry2dPtr->setVertices(*vertexArray);
    }
    else if(m_ArrayHandlingType == ArrayHandlingType::Move)
    {
      const auto geomId = geometry2dPtr->getId();

      const auto verticesId = verticesPtr->getId();
      dataStructure.setAdditionalParent(verticesId, geomId);
      const auto oldVertexParentId = dataStructure.getId(m_InputVertices.getParent());
      if(!oldVertexParentId.has_value())
      {
        return MakeErrorResult(-5507, fmt::format("{}CreateGeometry2DAction: Failed to remove vertices array '{}' from parent at path '{}' while moving array", k_Prefix, m_SharedVerticesName,
                                                  m_InputVertices.getParent().toString()));
      }
      dataStructure.removeParent(verticesId, oldVertexParentId.value());

      const auto facesId = facesPtr->getId();
      dataStructure.setAdditionalParent(facesId, geomId);
      const auto oldFaceParentId = dataStructure.getId(m_InputFaces.getParent());
      if(!oldFaceParentId.has_value())
      {
        return MakeErrorResult(-5508, fmt::format("{}CreateGeometry2DAction: Failed to remove faces array '{}' from parent at path '{}' while moving array", k_Prefix, m_SharedFacesName,
                                                  m_InputFaces.getParent().toString()));
      }
      dataStructure.removeParent(facesId, oldFaceParentId.value());

      geometry2dPtr->setVertices(*verticesPtr);
      geometry2dPtr->setFaceList(*facesPtr);
    }
    else if(m_ArrayHandlingType == ArrayHandlingType::Reference)
    {
      const auto geomId = geometry2dPtr->getId();
      dataStructure.setAdditionalParent(verticesPtr->getId(), geomId);
      dataStructure.setAdditionalParent(facesPtr->getId(), geomId);
      geometry2dPtr->setVertices(*verticesPtr);
      geometry2dPtr->setFaceList(*facesPtr);
    }
    else
    {
      DataPath trianglesPath = getCreatedPath().createChildPath(m_SharedFacesName);
      Result result = ArrayCreationUtilities::CreateArray<MeshIndexType>(dataStructure, faceTupleShape, {Geometry2DType::k_NumVerts}, trianglesPath, mode);
      if(result.invalid())
      {
        return MergeResults(result, MakeErrorResult(-5509, fmt::format("{}CreateGeometry2DAction: Could not allocate SharedTriList '{}'", k_Prefix, trianglesPath.toString())));
      }
      auto* trianglesPtr = dataStructure.getDataAs<SharedTriListType>(trianglesPath);
      if(trianglesPtr == nullptr)
      {
        throw std::runtime_error(fmt::format("DataPath does not point to a DataArray. DataPath: '{}'", trianglesPath.toString()));
      }
      geometry2dPtr->setFaceList(*trianglesPtr);

      // Vertices use three coordinates.
      DataPath vertexPath = getCreatedPath().createChildPath(m_SharedVerticesName);

      result = ArrayCreationUtilities::CreateArray<float32>(dataStructure, vertexTupleShape, {3}, vertexPath, mode);
      if(result.invalid())
      {
        return MergeResults(result, MakeErrorResult(-5510, fmt::format("{}CreateGeometry2DAction: Could not allocate SharedVertList '{}'", k_Prefix, vertexPath.toString())));
      }
      auto* vertexArrayPtr = dataStructure.getDataAs<Float32Array>(vertexPath);
      if(vertexArrayPtr == nullptr)
      {
        throw std::runtime_error(fmt::format("DataPath does not point to a DataArray. DataPath: '{}'", vertexPath.toString()));
      }
      geometry2dPtr->setVertices(*vertexArrayPtr);
    }

    auto* faceAttributeMatrixPtr = AttributeMatrix::Create(dataStructure, m_FaceDataName, faceTupleShape, geometry2dPtr->getId());
    if(faceAttributeMatrixPtr == nullptr)
    {
      return MakeErrorResult(-5511, fmt::format("{}CreateGeometry2DAction: Failed to create attribute matrix: '{}'", k_Prefix, faceDataPath.toString()));
    }
    geometry2dPtr->setFaceAttributeMatrix(*faceAttributeMatrixPtr);

    auto* vertexAttributeMatrixPtr = AttributeMatrix::Create(dataStructure, m_VertexDataName, vertexTupleShape, geometry2dPtr->getId());
    if(vertexAttributeMatrixPtr == nullptr)
    {
      return MakeErrorResult(-5512, fmt::format("{}CreateGeometry2DAction: Failed to create attribute matrix: '{}'", k_Prefix, vertexDataPath.toString()));
    }
    geometry2dPtr->setVertexAttributeMatrix(*vertexAttributeMatrixPtr);

    return {};
  }

  UniquePointer clone() const override
  {
    auto action =
        std::unique_ptr<CreateGeometry2DAction>(new CreateGeometry2DAction(getCreatedPath(), m_NumFaces, m_NumVertices, m_VertexDataName, m_FaceDataName, m_SharedVerticesName, m_SharedFacesName));
    action->m_InputVertices = m_InputVertices;
    action->m_InputFaces = m_InputFaces;
    action->m_ArrayHandlingType = m_ArrayHandlingType;
    return action;
  }

  const DataPath& geometryPath() const
  {
    return getCreatedPath();
  }

  DataPath getFaceDataPath() const
  {
    return getCreatedPath().createChildPath(m_FaceDataName);
  }

  DataPath getVertexDataPath() const
  {
    return getCreatedPath().createChildPath(m_VertexDataName);
  }

  IGeometry::MeshIndexType numFaces() const
  {
    return m_NumFaces;
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
    const auto topLevelCreatedPath = getCreatedPath();
    std::vector<DataPath> createdPaths = {topLevelCreatedPath, getFaceDataPath(), getVertexDataPath()};
    if(m_ArrayHandlingType == ArrayHandlingType::Create || m_ArrayHandlingType == ArrayHandlingType::Copy)
    {
      createdPaths.push_back(topLevelCreatedPath.createChildPath(m_SharedVerticesName));
      createdPaths.push_back(topLevelCreatedPath.createChildPath(m_SharedFacesName));
    }
    return createdPaths;
  }

protected:
  CreateGeometry2DAction() = default;

private:
  IGeometry::MeshIndexType m_NumFaces = 1;
  IGeometry::MeshIndexType m_NumVertices = Geometry2DType::k_NumVerts;
  std::string m_VertexDataName;
  std::string m_FaceDataName;
  std::string m_SharedVerticesName;
  std::string m_SharedFacesName;
  DataPath m_InputVertices;
  DataPath m_InputFaces;
  ArrayHandlingType m_ArrayHandlingType = ArrayHandlingType::Create;
};

using CreateTriangleGeometryAction = CreateGeometry2DAction<TriangleGeom>;
using CreateQuadGeometryAction = CreateGeometry2DAction<QuadGeom>;
} // namespace nx::core
