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
class CreateGeometry2DAction : public IDataCreationAction
{
public:
  using DimensionType = std::vector<size_t>;

  static constexpr StringLiteral k_DefaultVerticesName = "SharedVertexList";
  static constexpr StringLiteral k_DefaultFacesName = "SharedTriList";

  /**
   * @brief Constructor to create the 2D geometry and allocate a default arrays for the shared vertex & shared face lists
   * @param geometryPath The path to the created geometry
   * @param numFaces The number of faces in the geometry
   * @param numVertices The number of vertices in the geometry
   * @param vertexAttributeMatrixName The name of the vertex AttributeMatrix to be created
   * @param faceAttributeMatrixName The name of the face AttributeMatrix to be created
   * @param sharedVerticesName The name of the shared vertex list array to be created
   * @param sharedFacesName The name of the shared face list array to be created
   */
  CreateGeometry2DAction(const DataPath& geometryPath, size_t numFaces, size_t numVertices, const std::string& vertexAttributeMatrixName, const std::string& faceAttributeMatrixName,
                         const std::string& sharedVerticesName, const std::string& sharedFacesName, std::string createdDataFormat = "")
  : IDataCreationAction(geometryPath)
  , m_NumFaces(numFaces)
  , m_NumVertices(numVertices)
  , m_VertexDataName(vertexAttributeMatrixName)
  , m_FaceDataName(faceAttributeMatrixName)
  , m_SharedVerticesName(sharedVerticesName)
  , m_SharedFacesName(sharedFacesName)
  , m_CreatedDataStoreFormat(createdDataFormat)
  {
  }

  /**
   * @brief Constructor to create the 2D geometry using existing vertices & faces arrays by either copying, moving, or referencing them
   * @param geometryPath The path to the created geometry
   * @param inputVerticesArrayPath The path to the existing vertices array
   * @param inputFacesArrayPath The path to the existing faces array
   * @param vertexAttributeMatrixName The name of the vertex AttributeMatrix to be created
   * @param faceAttributeMatrixName The name of the face AttributeMatrix to be created
   * @param arrayType Tells whether to copy, move, or reference the existing input vertices array
   */
  CreateGeometry2DAction(const DataPath& geometryPath, const DataPath& inputVerticesArrayPath, const DataPath& inputFacesArrayPath, const std::string& vertexAttributeMatrixName,
                         const std::string& faceAttributeMatrixName, const ArrayHandlingType& arrayType, std::string createdDataFormat = "")
  : IDataCreationAction(geometryPath)
  , m_VertexDataName(vertexAttributeMatrixName)
  , m_FaceDataName(faceAttributeMatrixName)
  , m_SharedVerticesName(inputVerticesArrayPath.getTargetName())
  , m_SharedFacesName(inputFacesArrayPath.getTargetName())
  , m_InputVertices(inputVerticesArrayPath)
  , m_InputFaces(inputFacesArrayPath)
  , m_ArrayHandlingType(arrayType)
  , m_CreatedDataStoreFormat(createdDataFormat)
  {
  }

  ~CreateGeometry2DAction() noexcept override = default;

  CreateGeometry2DAction(const CreateGeometry2DAction&) = delete;
  CreateGeometry2DAction(CreateGeometry2DAction&&) noexcept = delete;
  CreateGeometry2DAction& operator=(const CreateGeometry2DAction&) = delete;
  CreateGeometry2DAction& operator=(CreateGeometry2DAction&&) noexcept = delete;

  /**
   * @brief Applies this action's change to the given DataStructure in the given mode.
   * Returns any warnings/errors. On error, DataStructure is not guaranteed to be consistent.
   * @param dataStructure
   * @return
   */
  Result<> apply(DataStructure& dataStructure, Mode mode) const override
  {
    static constexpr StringLiteral prefix = "CreateGeometry2DAction: ";
    using MeshIndexType = IGeometry::MeshIndexType;
    using SharedTriList = IGeometry::SharedTriList;
    const DataPath faceDataPath = getFaceDataPath();
    const DataPath vertexDataPath = getVertexDataPath();

    // Check for empty Geometry DataPath
    if(getCreatedPath().empty())
    {
      return MakeErrorResult(-5501, fmt::format("{}CreateGeometry2DAction: Geometry Path cannot be empty", prefix));
    }

    // Check if the Geometry Path already exists
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
    // Get the Parent ID
    if(!dataStructure.getId(parentPath).has_value())
    {
      return MakeErrorResult(-5504, fmt::format("{}CreateGeometry2DAction: Parent Id was not available for path:'{}'", prefix, parentPath.toString()));
    }

    // Get the vertices list if we are using an existing array
    const auto vertices = dataStructure.getDataAs<Float32Array>(m_InputVertices);
    if(m_ArrayHandlingType != ArrayHandlingType::Create && vertices == nullptr)
    {
      return MakeErrorResult(-5505, fmt::format("{}CreateGeometry2DAction: Could not find vertices array at path '{}'", prefix, m_InputVertices.toString()));
    }

    // Get the faces list if we are using an existing array
    const auto faces = dataStructure.getDataAs<DataArray<MeshIndexType>>(m_InputFaces);
    if(m_ArrayHandlingType != ArrayHandlingType::Create && faces == nullptr)
    {
      return MakeErrorResult(-5506, fmt::format("{}CreateGeometry2DAction: Could not find faces array at path '{}'", prefix, m_InputFaces.toString()));
    }

    // Create the TriangleGeometry
    auto geometry2d = Geometry2DType::Create(dataStructure, getCreatedPath().getTargetName(), dataStructure.getId(parentPath).value());
    DimensionType faceTupleShape = {m_NumFaces};
    DimensionType vertexTupleShape = {m_NumVertices}; // We probably don't know how many Vertices there are but take what ever the developer sends us

    if(m_ArrayHandlingType == ArrayHandlingType::Copy)
    {
      faceTupleShape = faces->getTupleShape();
      vertexTupleShape = vertices->getTupleShape();

      std::shared_ptr<DataObject> vertexCopy = vertices->deepCopy(getCreatedPath().createChildPath(m_SharedVerticesName));
      const auto vertexArray = std::dynamic_pointer_cast<Float32Array>(vertexCopy);

      std::shared_ptr<DataObject> facesCopy = faces->deepCopy(getCreatedPath().createChildPath(m_SharedFacesName));
      const auto facesArray = std::dynamic_pointer_cast<DataArray<MeshIndexType>>(facesCopy);

      geometry2d->setFaceList(*facesArray);
      geometry2d->setVertices(*vertexArray);
    }
    else if(m_ArrayHandlingType == ArrayHandlingType::Move)
    {
      faceTupleShape = faces->getTupleShape();
      vertexTupleShape = vertices->getTupleShape();
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
      faceTupleShape = faces->getTupleShape();
      vertexTupleShape = vertices->getTupleShape();
      const auto geomId = geometry2d->getId();
      dataStructure.setAdditionalParent(vertices->getId(), geomId);
      dataStructure.setAdditionalParent(faces->getId(), geomId);
      geometry2d->setVertices(*vertices);
      geometry2d->setFaceList(*faces);
    }
    else
    {
      DataPath trianglesPath = getCreatedPath().createChildPath(m_SharedFacesName);
      // Create the default DataArray that will hold the FaceList and Vertices. We
      // size these to 1 because the Csv parser will resize them to the appropriate number of tuples
      complex::Result result = complex::CreateArray<MeshIndexType>(dataStructure, faceTupleShape, {Geometry2DType::k_NumVerts}, trianglesPath, mode, m_CreatedDataStoreFormat);
      if(result.invalid())
      {
        return MakeErrorResult(-5509, fmt::format("{}CreateGeometry2DAction: Could not allocate SharedTriList '{}'", prefix, trianglesPath.toString()));
      }
      SharedTriList* triangles = complex::ArrayFromPath<MeshIndexType>(dataStructure, trianglesPath);
      geometry2d->setFaceList(*triangles);

      // Create the Vertex Array with a component size of 3
      DataPath vertexPath = getCreatedPath().createChildPath(m_SharedVerticesName);

      result = complex::CreateArray<float>(dataStructure, vertexTupleShape, {3}, vertexPath, mode, m_CreatedDataStoreFormat);
      if(result.invalid())
      {
        return MakeErrorResult(-5510, fmt::format("{}CreateGeometry2DAction: Could not allocate SharedVertList '{}'", prefix, vertexPath.toString()));
      }
      Float32Array* vertexArray = complex::ArrayFromPath<float>(dataStructure, vertexPath);
      geometry2d->setVertices(*vertexArray);
    }

    // Create the vertex and face AttributeMatrix
    auto* faceAttributeMatrix = AttributeMatrix::Create(dataStructure, m_FaceDataName, faceTupleShape, geometry2d->getId());
    if(faceAttributeMatrix == nullptr)
    {
      return MakeErrorResult(-5511, fmt::format("{}CreateGeometry2DAction: Failed to create attribute matrix: '{}'", prefix, faceDataPath.toString()));
    }
    geometry2d->setFaceAttributeMatrix(*faceAttributeMatrix);

    auto* vertexAttributeMatrix = AttributeMatrix::Create(dataStructure, m_VertexDataName, vertexTupleShape, geometry2d->getId());
    if(vertexAttributeMatrix == nullptr)
    {
      return MakeErrorResult(-5512, fmt::format("CreateGeometry2DAction: Failed to create attribute matrix: '{}'", prefix, vertexDataPath.toString()));
    }
    geometry2d->setVertexAttributeMatrix(*vertexAttributeMatrix);

    return {};
  }

  /**
   * @brief Returns a copy of the action.
   * @return
   */
  UniquePointer clone() const override
  {
    auto action =
        std::unique_ptr<CreateGeometry2DAction>(new CreateGeometry2DAction(getCreatedPath(), m_NumFaces, m_NumVertices, m_VertexDataName, m_FaceDataName, m_SharedVerticesName, m_SharedFacesName));
    action->m_InputVertices = m_InputVertices;
    action->m_InputFaces = m_InputFaces;
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
  DataPath getFaceDataPath() const
  {
    return getCreatedPath().createChildPath(m_FaceDataName);
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
  IGeometry::MeshIndexType numFaces() const
  {
    return m_NumFaces;
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
  std::string m_CreatedDataStoreFormat;
};

using CreateTriangleGeometryAction = CreateGeometry2DAction<TriangleGeom>;
using CreateQuadGeometryAction = CreateGeometry2DAction<QuadGeom>;
} // namespace complex
