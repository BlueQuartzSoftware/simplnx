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

  CreateGeometry2DAction(const DataPath& geometryPath, size_t numFaces, size_t numVertices, const std::string& vertexAttributeMatrixName, const std::string& faceAttributeMatrixName,
                         const std::string& sharedVerticesName, const std::string& sharedFacesName)
  : IDataCreationAction(geometryPath)
  , m_NumFaces(numFaces)
  , m_NumVertices(numVertices)
  , m_VertexDataName(vertexAttributeMatrixName)
  , m_FaceDataName(faceAttributeMatrixName)
  , m_SharedVerticesName(sharedVerticesName)
  , m_SharedFacesName(sharedFacesName)
  {
  }

  CreateGeometry2DAction(const DataPath& geometryPath, const DataPath& inputVerticesArrayPath, const DataPath& inputFacesArrayPath, const std::string& vertexAttributeMatrixName,
                         const std::string& faceAttributeMatrixName, const ArrayHandlingType& arrayType)
  : IDataCreationAction(geometryPath)
  , m_VertexDataName(vertexAttributeMatrixName)
  , m_FaceDataName(faceAttributeMatrixName)
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
    static constexpr StringLiteral prefix = "CreateGeometry2DAction: ";
    using MeshIndexType = IGeometry::MeshIndexType;
    using SharedTriList = IGeometry::SharedTriList;
    const DataPath faceDataPath = getFaceDataPath();
    const DataPath vertexDataPath = getVertexDataPath();

    if(getCreatedPath().empty())
    {
      return MakeErrorResult(-5501, fmt::format("{}CreateGeometry2DAction: Geometry Path cannot be empty", prefix));
    }

    const BaseGroup* parentObject = dataStructure.getDataAs<BaseGroup>(getCreatedPath());
    if(parentObject != nullptr)
    {
      return MakeErrorResult(-5502, fmt::format("{}CreateGeometry2DAction: DataObject already exists at path '{}'", prefix, getCreatedPath().toString()));
    }

    const DataPath parentPath = getCreatedPath().getParent();
    if(!parentPath.empty())
    {
      const Result<LinkedPath> geomPath = dataStructure.makePath(parentPath);
      if(geomPath.invalid())
      {
        return MakeErrorResult(-5503, fmt::format("{}CreateGeometry2DAction: Geometry could not be created at path:'{}'", prefix, getCreatedPath().toString()));
      }
    }
    if(!dataStructure.getId(parentPath).has_value())
    {
      return MakeErrorResult(-5504, fmt::format("{}CreateGeometry2DAction: Parent Id was not available for path:'{}'", prefix, parentPath.toString()));
    }

    const auto vertices = dataStructure.getDataAs<Float32Array>(m_InputVertices);
    if(m_ArrayHandlingType != ArrayHandlingType::Create && vertices == nullptr)
    {
      return MakeErrorResult(-5505, fmt::format("{}CreateGeometry2DAction: Could not find vertices array at path '{}'", prefix, m_InputVertices.toString()));
    }

    const auto faces = dataStructure.getDataAs<DataArray<MeshIndexType>>(m_InputFaces);
    if(m_ArrayHandlingType != ArrayHandlingType::Create && faces == nullptr)
    {
      return MakeErrorResult(-5506, fmt::format("{}CreateGeometry2DAction: Could not find faces array at path '{}'", prefix, m_InputFaces.toString()));
    }

    auto geometry2d = Geometry2DType::Create(dataStructure, getCreatedPath().getTargetName(), dataStructure.getId(parentPath).value());
    DimensionType faceTupleShape = {m_NumFaces};
    DimensionType vertexTupleShape = {m_NumVertices};

    if(m_ArrayHandlingType != ArrayHandlingType::Create)
    {
      faceTupleShape = faces->getTupleShape();
      vertexTupleShape = vertices->getTupleShape();
    }

    if(m_ArrayHandlingType == ArrayHandlingType::Copy)
    {
      std::shared_ptr<DataObject> vertexCopy = vertices->deepCopy(getCreatedPath().createChildPath(m_SharedVerticesName));
      const auto vertexArray = std::dynamic_pointer_cast<Float32Array>(vertexCopy);

      std::shared_ptr<DataObject> facesCopy = faces->deepCopy(getCreatedPath().createChildPath(m_SharedFacesName));
      const auto facesArray = std::dynamic_pointer_cast<DataArray<MeshIndexType>>(facesCopy);

      geometry2d->setFaceList(*facesArray);
      geometry2d->setVertices(*vertexArray);
    }
    else if(m_ArrayHandlingType == ArrayHandlingType::Move)
    {
      const auto geomId = geometry2d->getId();

      const auto verticesId = vertices->getId();
      dataStructure.setAdditionalParent(verticesId, geomId);
      const auto oldVertexParentId = dataStructure.getId(m_InputVertices.getParent());
      if(!oldVertexParentId.has_value())
      {
        return MakeErrorResult(-5507, fmt::format("{}CreateGeometry2DAction: Failed to remove vertices array '{}' from parent at path '{}' while moving array", prefix, m_SharedVerticesName,
                                                  m_InputVertices.getParent().toString()));
      }
      dataStructure.removeParent(verticesId, oldVertexParentId.value());

      const auto facesId = faces->getId();
      dataStructure.setAdditionalParent(facesId, geomId);
      const auto oldFaceParentId = dataStructure.getId(m_InputFaces.getParent());
      if(!oldFaceParentId.has_value())
      {
        return MakeErrorResult(-5508, fmt::format("{}CreateGeometry2DAction: Failed to remove faces array '{}' from parent at path '{}' while moving array", prefix, m_SharedFacesName,
                                                  m_InputFaces.getParent().toString()));
      }
      dataStructure.removeParent(facesId, oldFaceParentId.value());

      geometry2d->setVertices(*vertices);
      geometry2d->setFaceList(*faces);
    }
    else if(m_ArrayHandlingType == ArrayHandlingType::Reference)
    {
      const auto geomId = geometry2d->getId();
      dataStructure.setAdditionalParent(vertices->getId(), geomId);
      dataStructure.setAdditionalParent(faces->getId(), geomId);
      geometry2d->setVertices(*vertices);
      geometry2d->setFaceList(*faces);
    }
    else
    {
      DataPath trianglesPath = getCreatedPath().createChildPath(m_SharedFacesName);
      Result result = ArrayCreationUtilities::CreateArray<MeshIndexType>(dataStructure, faceTupleShape, {Geometry2DType::k_NumVerts}, trianglesPath, mode);
      if(result.invalid())
      {
        return MergeResults(result, MakeErrorResult(-5509, fmt::format("{}CreateGeometry2DAction: Could not allocate SharedTriList '{}'", prefix, trianglesPath.toString())));
      }
      auto* triangles = dataStructure.getDataAs<SharedTriList>(trianglesPath);
      if(triangles == nullptr)
      {
        throw std::runtime_error(fmt::format("DataPath does not point to a DataArray. DataPath: '{}'", trianglesPath.toString()));
      }
      geometry2d->setFaceList(*triangles);

      // Vertices use three coordinates.
      DataPath vertexPath = getCreatedPath().createChildPath(m_SharedVerticesName);

      result = ArrayCreationUtilities::CreateArray<float32>(dataStructure, vertexTupleShape, {3}, vertexPath, mode);
      if(result.invalid())
      {
        return MergeResults(result, MakeErrorResult(-5510, fmt::format("{}CreateGeometry2DAction: Could not allocate SharedVertList '{}'", prefix, vertexPath.toString())));
      }
      auto* vertexArray = dataStructure.getDataAs<Float32Array>(vertexPath);
      if(vertexArray == nullptr)
      {
        throw std::runtime_error(fmt::format("DataPath does not point to a DataArray. DataPath: '{}'", vertexPath.toString()));
      }
      geometry2d->setVertices(*vertexArray);
    }

    auto* faceAttributeMatrix = AttributeMatrix::Create(dataStructure, m_FaceDataName, faceTupleShape, geometry2d->getId());
    if(faceAttributeMatrix == nullptr)
    {
      return MakeErrorResult(-5511, fmt::format("{}CreateGeometry2DAction: Failed to create attribute matrix: '{}'", prefix, faceDataPath.toString()));
    }
    geometry2d->setFaceAttributeMatrix(*faceAttributeMatrix);

    auto* vertexAttributeMatrix = AttributeMatrix::Create(dataStructure, m_VertexDataName, vertexTupleShape, geometry2d->getId());
    if(vertexAttributeMatrix == nullptr)
    {
      return MakeErrorResult(-5512, fmt::format("{}CreateGeometry2DAction: Failed to create attribute matrix: '{}'", prefix, vertexDataPath.toString()));
    }
    geometry2d->setVertexAttributeMatrix(*vertexAttributeMatrix);

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
