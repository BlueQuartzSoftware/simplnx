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
  CreateGeometry2DAction() = delete;

  CreateGeometry2DAction(const DataPath& geometryPath, IGeometry::MeshIndexType numFaces, IGeometry::MeshIndexType numVertices)
  : IDataCreationAction(geometryPath)
  , m_NumFaces(numFaces)
  , m_NumVertices(numVertices)
  {
  }

  ~CreateGeometry2DAction() noexcept = default;

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

    // Check for empty Geometry DataPath
    if(getCreatedPath().empty())
    {
      return MakeErrorResult(-220, "CreateGeometry2DAction: Geometry Path cannot be empty");
    }

    // Check if the Geometry Path already exists
    BaseGroup* parentObject = dataStructure.getDataAs<BaseGroup>(getCreatedPath());
    if(parentObject != nullptr)
    {
      return MakeErrorResult(-222, fmt::format("CreateGeometry2DAction: DataObject already exists at path '{}'", getCreatedPath().toString()));
    }

    DataPath parentPath = getCreatedPath().getParent();
    if(!parentPath.empty())
    {
      Result<LinkedPath> geomPath = dataStructure.makePath(parentPath);
      if(geomPath.invalid())
      {
        return MakeErrorResult(-223, fmt::format("CreateGeometry2DAction: Geometry could not be created at path:'{}'", getCreatedPath().toString()));
      }
    }
    // Get the Parent ID
    if(!dataStructure.getId(parentPath).has_value())
    {
      return MakeErrorResult(-224, fmt::format("CreateGeometry2DAction: Parent Id was not available for path:'{}'", parentPath.toString()));
    }

    // Create the TriangleGeometry
    auto geometry2d = Geometry2DType::Create(dataStructure, getCreatedPath().getTargetName(), dataStructure.getId(parentPath).value());

    using MeshIndexType = IGeometry::MeshIndexType;
    using SharedTriList = IGeometry::SharedTriList;

    DataPath trianglesPath = getCreatedPath().createChildPath(k_TriangleDataName);
    // Create the default DataArray that will hold the FaceList and Vertices. We
    // size these to 1 because the Csv parser will resize them to the appropriate number of tuples
    std::vector<size_t> tupleShape = {m_NumFaces};
    complex::Result result = complex::CreateArray<MeshIndexType>(dataStructure, tupleShape, {3}, trianglesPath, mode);
    if(result.invalid())
    {
      return MakeErrorResult(-223, fmt::format("CreateGeometry2DAction: Could not allocate SharedTriList '{}'", trianglesPath.toString()));
    }
    SharedTriList* triangles = complex::ArrayFromPath<MeshIndexType>(dataStructure, trianglesPath);
    geometry2d->setFaces(*triangles);

    // Create the Vertex Array with a component size of 3
    DataPath vertexPath = getCreatedPath().createChildPath(k_VertexDataName);
    tupleShape = {m_NumVertices}; // We don't probably know how many Vertices there are but take what ever the developer sends us

    result = complex::CreateArray<float>(dataStructure, tupleShape, {3}, vertexPath, mode);
    if(result.invalid())
    {
      return MakeErrorResult(-224, fmt::format("CreateGeometry2DAction: Could not allocate SharedVertList '{}'", trianglesPath.toString()));
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

private:
  IGeometry::MeshIndexType m_NumFaces;
  IGeometry::MeshIndexType m_NumVertices;
};

using CreateTriangleGeometryAction = CreateGeometry2DAction<TriangleGeom>;
using CreateQuadGeometryAction = CreateGeometry2DAction<QuadGeom>;
} // namespace complex
