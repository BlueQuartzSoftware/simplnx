#pragma once

#include "complex/Common/Array.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/Geometry/IGeometry.hpp"
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

class CreateVertexGeometryAction : public IDataCreationAction
{
public:
  static constexpr StringLiteral k_SharedVertexListName = "SharedVertexList";

  CreateVertexGeometryAction() = delete;

  CreateVertexGeometryAction(const DataPath& geometryPath, IGeometry::MeshIndexType numVertices, const std::string& vertexAttributeMatrixName)
  : IDataCreationAction(geometryPath)
  , m_NumVertices(numVertices)
  , m_VertexDataName(vertexAttributeMatrixName)
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
    // Check for empty Geometry DataPath
    if(getCreatedPath().empty())
    {
      return MakeErrorResult(-220, "CreateVertexGeometryAction: Geometry Path cannot be empty");
    }

    // Check if the Geometry Path already exists
    BaseGroup* parentObject = dataStructure.getDataAs<BaseGroup>(getCreatedPath());
    if(parentObject != nullptr)
    {
      return MakeErrorResult(-222, fmt::format("CreateVertexGeometryAction: DataObject already exists at path '{}'", getCreatedPath().toString()));
    }

    DataPath parentPath = getCreatedPath().getParent();
    if(!parentPath.empty())
    {
      Result<LinkedPath> geomPath = dataStructure.makePath(parentPath);
      if(geomPath.invalid())
      {
        return MakeErrorResult(-223, fmt::format("CreateVertexGeometryAction: Geometry could not be created at path:'{}'", getCreatedPath().toString()));
      }
    }
    // Get the Parent ID
    if(!dataStructure.getId(parentPath).has_value())
    {
      return MakeErrorResult(-224, fmt::format("CreateVertexGeometryAction: Parent Id was not available for path:'{}'", parentPath.toString()));
    }

    // Create the VertexGeom
    VertexGeom* geometry2d = VertexGeom::Create(dataStructure, getCreatedPath().getTargetName(), dataStructure.getId(parentPath).value());

    using MeshIndexType = IGeometry::MeshIndexType;

    // Create the Vertex Array with a component size of 3
    DataPath vertexPath = getCreatedPath().createChildPath(k_SharedVertexListName);
    std::vector<usize> tupleShape = {m_NumVertices}; // We don't probably know how many Vertices there are but take what ever the developer sends us
    std::vector<usize> componentShape = {3};

    Result<> result = complex::CreateArray<float>(dataStructure, tupleShape, componentShape, vertexPath, mode);
    if(result.invalid())
    {
      return result;
    }
    Float32Array* vertexArray = complex::ArrayFromPath<float>(dataStructure, vertexPath);
    geometry2d->setVertices(*vertexArray);

    auto* vertexAttributeMatrix = AttributeMatrix::Create(dataStructure, m_VertexDataName, geometry2d->getId());
    if(vertexAttributeMatrix == nullptr)
    {
      return MakeErrorResult(-225, fmt::format("CreateGeometry2DAction: Failed to create attribute matrix: '{}'", getVertexDataPath().toString()));
    }
    vertexAttributeMatrix->setShape(tupleShape);
    geometry2d->setVertexAttributeMatrix(*vertexAttributeMatrix);

    return {};
  }

  /**
   * @brief Returns the path of the ImageGeometry to be created.
   * @return DataPath
   */
  DataPath geometryPath() const
  {
    return getCreatedPath();
  }

  /**
   * @brief Returns the number of vertices (estimated in some circumstances)
   * @return IGeometry::MeshIndexType
   */
  IGeometry::MeshIndexType numVertices() const
  {
    return m_NumVertices;
  }

  /**
   * @brief Returns the path of the vertex data in the created geometry.
   * @return
   */
  DataPath getVertexDataPath() const
  {
    return getCreatedPath().createChildPath(m_VertexDataName);
  }

  /**
   * @brief Returns all of the DataPaths to be created.
   * @return std::vector<DataPath>
   */
  std::vector<DataPath> getAllCreatedPaths() const override
  {
    auto topLevelCreatedPath = getCreatedPath();
    return {topLevelCreatedPath, getVertexDataPath(), topLevelCreatedPath.createChildPath(k_SharedVertexListName)};
  }

private:
  IGeometry::MeshIndexType m_NumVertices;
  std::string m_VertexDataName;
};

} // namespace complex
