#pragma once

#include "complex/Common/Array.hpp"
#include "complex/DataStructure/Geometry/AbstractGeometry.hpp"
#include "complex/Filter/Output.hpp"
#include "complex/complex_export.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/DataStructure/Geometry/QuadGeom.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"

#include <fmt/core.h>

#include <utility>


namespace complex
{
/**
 * @brief Action for creating an ImageGeometry in a DataStructure
 */
template<typename Geometry2DType>
class CreateGeometry2DAction : public IDataAction
{
public:
  using DimensionType = std::vector<size_t>;
  using OriginType = std::vector<float>;
  using SpacingType = std::vector<float>;

  CreateGeometry2DAction() = delete;

  CreateGeometry2DAction(DataPath geometryPath, AbstractGeometry::MeshIndexType numFaces, AbstractGeometry::MeshIndexType numVertices)
  : m_GeometryPath(std::move(geometryPath))
  , m_NumFaces(numFaces)
  , m_NumVertices(numVertices)
  {
  }

  ~CreateGeometry2DAction()noexcept = default;

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
    const std::string k_TriangleDataName("SharedTriList");
    const std::string k_VertexDataName("SharedVertexList");

    if(m_GeometryPath.empty())
    {
      return MakeErrorResult(-220, "CreateGeometry2DAction: Geometry Path cannot be empty");
    }

    usize size = m_GeometryPath.getLength();
    std::string name = (size == 1 ? m_GeometryPath[0] : m_GeometryPath[size - 1]);

    std::optional<DataObject::IdType> parentId;
    if(size > 1)
    {
      DataPath parentPath = m_GeometryPath.getParent();
      parentId = dataStructure.getId(parentPath);
      if(!parentId.has_value())
      {
        return MakeErrorResult(-221, "CreateGeometry2DAction: Invalid path");
      }
    }

    BaseGroup* parentObject = dataStructure.getDataAs<BaseGroup>(parentId);
    if(parentObject->contains(name))
    {
      return MakeErrorResult(-222, fmt::format("CreateGeometry2DAction: DataObject already exists at path '{}'", m_GeometryPath.toString()));
    }

    // Create the TriangleGeometry
    auto geometry2d = Geometry2DType::Create(dataStructure, name, parentId);

    using MeshIndexType = AbstractGeometry::MeshIndexType;
    using SharedTriList = AbstractGeometry::SharedTriList;

    DataPath trianglesPath = m_GeometryPath.createChildPath(k_TriangleDataName);
    // Create the default DataArray that will hold the FaceList and Vertices. We
    // size these to 1 because the Csv parser will resize them to the appropriate number of tuples
    std::vector<size_t> tupleShape = {m_NumFaces};
    complex::Result result = complex::CreateArray<MeshIndexType>(dataStructure, tupleShape, 3, trianglesPath, mode);
    if(result.invalid())
    {
      return MakeErrorResult(-223, fmt::format("CreateGeometry2DAction: Could not allocate SharedTriList '{}'", trianglesPath.toString()));
    }
    SharedTriList* triangles = complex::ArrayFromPath<MeshIndexType>(dataStructure, trianglesPath);
    geometry2d->setFaces(triangles);

    // Create the Vertex Array with a component size of 3
    DataPath vertexPath = m_GeometryPath.createChildPath(k_VertexDataName);
    tupleShape = {m_NumVertices}; // We don't probably know how many Vertices there are but take what ever the developer sends us

    result = complex::CreateArray<float>(dataStructure, tupleShape, 3, vertexPath, mode);
    if(result.invalid())
    {
      return MakeErrorResult(-224, fmt::format("CreateGeometry2DAction: Could not allocate SharedVertList '{}'", trianglesPath.toString()));
    }
    Float32Array* vertexArray = complex::ArrayFromPath<float>(dataStructure, vertexPath);
    geometry2d->setVertices(vertexArray);

    return {};
  }

  /**
   * @brief Returns the path of the ImageGeometry to be created.
   * @return
   */
  const DataPath& geometryPath() const
  {
    return m_GeometryPath;
  }

  /**
   * @brief Returns the number of faces
   * @return
   */
  AbstractGeometry::MeshIndexType numFaces() const
  {
    return m_NumFaces;
  }

  /**
   * @brief Returns the number of vertices (estimated in some circumstances)
   * @return
   */
  AbstractGeometry::MeshIndexType numVertices() const
  {
    return m_NumVertices;
  }



private:
  DataPath m_GeometryPath;
  AbstractGeometry::MeshIndexType m_NumFaces;
  AbstractGeometry::MeshIndexType m_NumVertices;
};

using CreateTriangleGeometryAction = CreateGeometry2DAction<TriangleGeom>;
using CreateQuadGeometryAction = CreateGeometry2DAction<QuadGeom>;
} // namespace complex
