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
 * @brief Action for creating an ImageGeometry in a DataStructure
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

    if(m_GeometryPath.empty())
    {
      return MakeErrorResult(-220, "CreateVertexGeometryAction: Geometry Path cannot be empty");
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
        return MakeErrorResult(-221, "CreateVertexGeometryAction Invalid path");
      }
    }

    BaseGroup* parentObject = dataStructure.getDataAs<BaseGroup>(parentId);
    if(parentObject->contains(name))
    {
      return MakeErrorResult(-222, fmt::format("CreateVertexGeometryAction DataObject already exists at path '{}'", m_GeometryPath.toString()));
    }

    // Create the VertexGeom
    VertexGeom* geometry2d = VertexGeom::Create(dataStructure, name, parentId);

    using MeshIndexType = AbstractGeometry::MeshIndexType;

    // Create the Vertex Array with a component size of 3
    DataPath vertexPath = m_GeometryPath.createChildPath(k_VertexDataName);
    std::vector<usize> tupleShape = {m_NumVertices}; // We don't probably know how many Vertices there are but take what ever the developer sends us
    std::vector<usize> componentShape = {3};

    Result<> result = complex::CreateArray<float>(dataStructure, tupleShape, componentShape, vertexPath, mode);
    if(result.invalid())
    {
      return MakeErrorResult(-224, fmt::format("CreateVertexGeometryAction Could not allocate SharedVertList '{}'", m_GeometryPath.toString()));
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
