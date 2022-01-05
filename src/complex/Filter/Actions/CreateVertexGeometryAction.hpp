#pragma once

#include "complex/Common/Array.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/Geometry/AbstractGeometry.hpp"
#include "complex/DataStructure/Geometry/VertexGeom.hpp"
#include "complex/Filter/Output.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"
#include "complex/complex_export.hpp"

#include <fmt/core.h>

#include <utility>

namespace complex
{
/**
 * @brief Action for creating a Vertex Geometry in a DataStructure
 */

class CreateVertexGeometryAction : public IDataAction
{
public:
  CreateVertexGeometryAction() = delete;

  CreateVertexGeometryAction(DataPath geometryPath, AbstractGeometry::MeshIndexType numVertices)
  : m_GeometryPath(std::move(geometryPath))
  , m_NumVertices(numVertices)
  {
  }

  ~CreateVertexGeometryAction() noexcept = default;

  CreateVertexGeometryAction(const CreateVertexGeometryAction&) = delete;
  CreateVertexGeometryAction(CreateVertexGeometryAction&&) noexcept = delete;
  CreateVertexGeometryAction& operator=(const CreateVertexGeometryAction&) = delete;
  CreateVertexGeometryAction& operator=(CreateVertexGeometryAction&&) noexcept = delete;

  /**
   * @brief Applies this action's change to the given DataStructure in the given mode.
   * Returns any warnings/errors. On error, DataStructure is not guaranteed to be consistent.
   * @param dataStructure
   * @return
   */
  Result<> apply(DataStructure& dataStructure, Mode mode) const override
  {
    const std::string k_VertexDataName("SharedVertexList");

    // Check for empty Geometry DataPath
    if(m_GeometryPath.empty())
    {
      return MakeErrorResult(-220, "CreateGeometry2DAction: Geometry Path cannot be empty");
    }

    // Check if the Geometry Path already exists
    BaseGroup* parentObject = dataStructure.getDataAs<BaseGroup>(m_GeometryPath);
    if(parentObject != nullptr)
    {
      return MakeErrorResult(-222, fmt::format("CreateGeometry2DAction: DataObject already exists at path '{}'", m_GeometryPath.toString()));
    }

    DataPath parentPath = m_GeometryPath.getParent();
    if(!parentPath.empty())
    {
      Result<LinkedPath> geomPath = dataStructure.makePath(parentPath);
      if(geomPath.invalid())
      {
        return MakeErrorResult(-223, fmt::format("CreateGeometry2DAction: Geometry could not be created at path:'{}'", m_GeometryPath.toString()));
      }
    }
    // Get the Parent ID
    if(!dataStructure.getId(parentPath).has_value())
    {
      return MakeErrorResult(-224, fmt::format("CreateGeometry2DAction: Parent Id was not available for path:'{}'", parentPath.toString()));
    }

    // Create the VertexGeom
    VertexGeom* geometry2d = VertexGeom::Create(dataStructure, m_GeometryPath.getTargetName(), dataStructure.getId(parentPath).value());

    using MeshIndexType = AbstractGeometry::MeshIndexType;

    // Create the Vertex Array with a component size of 3
    DataPath vertexPath = m_GeometryPath.createChildPath(k_VertexDataName);
    std::vector<usize> tupleShape = {m_NumVertices}; // We don't probably know how many Vertices there are but take what ever the developer sends us
    std::vector<usize> componentShape = {3};

    Result<> result = complex::CreateArray<float>(dataStructure, tupleShape, componentShape, vertexPath, mode);
    if(result.invalid())
    {
      return result;
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
   * @brief Returns the number of vertices (estimated in some circumstances)
   * @return
   */
  AbstractGeometry::MeshIndexType numVertices() const
  {
    return m_NumVertices;
  }

private:
  DataPath m_GeometryPath;
  AbstractGeometry::MeshIndexType m_NumVertices;
};

} // namespace complex
