#pragma once

#include "complex/Common/Array.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/Geometry/HexahedralGeom.hpp"
#include "complex/DataStructure/Geometry/IGeometry.hpp"
#include "complex/DataStructure/Geometry/TetrahedralGeom.hpp"
#include "complex/Filter/Output.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"
#include "complex/complex_export.hpp"

#include <fmt/core.h>

#include <utility>

namespace complex
{
/**
 * @brief Action for creating a Tetrahedral or Hexehedral Geometry in a DataStructure
 */
template <typename Geometry3DType>
class CreateGeometry3DAction : public IDataCreationAction
{
public:
  using DimensionType = std::vector<size_t>;

  static constexpr StringLiteral k_DefaultVerticesName = "SharedVertexList";
  static constexpr StringLiteral k_DefaultCellsName = "SharedCellList";

  CreateGeometry3DAction() = delete;

  CreateGeometry3DAction(const DataPath& geometryPath, size_t numCells, size_t numVertices, const std::string& vertexAttributeMatrixName, const std::string& cellAttributeMatrixName,
                         const std::string& sharedVerticesName, const std::string& sharedCellsName)
  : IDataCreationAction(geometryPath)
  , m_NumCells(numCells)
  , m_NumVertices(numVertices)
  , m_VertexDataName(vertexAttributeMatrixName)
  , m_CellDataName(cellAttributeMatrixName)
  , m_SharedVerticesName(sharedVerticesName)
  , m_SharedCellsName(sharedCellsName)
  {
  }

  ~CreateGeometry3DAction() noexcept = default;

  CreateGeometry3DAction(const CreateGeometry3DAction&) = delete;
  CreateGeometry3DAction(CreateGeometry3DAction&&) noexcept = delete;
  CreateGeometry3DAction& operator=(const CreateGeometry3DAction&) = delete;
  CreateGeometry3DAction& operator=(CreateGeometry3DAction&&) noexcept = delete;

  /**
   * @brief Applies this action's change to the given DataStructure in the given mode.
   * Returns any warnings/errors. On error, DataStructure is not guaranteed to be consistent.
   * @param dataStructure
   * @return
   */
  Result<> apply(DataStructure& dataStructure, Mode mode) const override
  {
    DataPath cellDataPath = getCellDataPath();
    DataPath vertexDataPath = getVertexDataPath();

    // Check for empty Geometry DataPath
    if(getCreatedPath().empty())
    {
      return MakeErrorResult(-220, "CreateGeometry3DAction: Geometry Path cannot be empty");
    }

    // Check if the Geometry Path already exists
    BaseGroup* parentObject = dataStructure.getDataAs<BaseGroup>(getCreatedPath());
    if(parentObject != nullptr)
    {
      return MakeErrorResult(-221, fmt::format("CreateGeometry3DAction: DataObject already exists at path '{}'", getCreatedPath().toString()));
    }

    DataPath parentPath = getCreatedPath().getParent();
    if(!parentPath.empty())
    {
      Result<LinkedPath> geomPath = dataStructure.makePath(parentPath);
      if(geomPath.invalid())
      {
        return MakeErrorResult(-222, fmt::format("CreateGeometry3DAction: Geometry could not be created at path:'{}'", getCreatedPath().toString()));
      }
    }
    // Get the Parent ID
    if(!dataStructure.getId(parentPath).has_value())
    {
      return MakeErrorResult(-223, fmt::format("CreateGeometry3DAction: Parent Id was not available for path:'{}'", parentPath.toString()));
    }

    // Create the Geometry
    auto geometry3d = Geometry3DType::Create(dataStructure, getCreatedPath().getTargetName(), dataStructure.getId(parentPath).value());

    auto* cellAttributeMatrix = AttributeMatrix::Create(dataStructure, m_CellDataName, geometry3d->getId());
    if(cellAttributeMatrix == nullptr)
    {
      return MakeErrorResult(-224, fmt::format("CreateGeometry3DAction: Failed to create attribute matrix: '{}'", cellDataPath.toString()));
    }
    DimensionType cellTupleShape = {m_NumCells};
    cellAttributeMatrix->setShape(cellTupleShape);
    geometry3d->setPolyhedraAttributeMatrix(*cellAttributeMatrix);

    auto* vertexAttributeMatrix = AttributeMatrix::Create(dataStructure, m_VertexDataName, geometry3d->getId());
    if(vertexAttributeMatrix == nullptr)
    {
      return MakeErrorResult(-225, fmt::format("CreateGeometry3DAction: Failed to create attribute matrix: '{}'", vertexDataPath.toString()));
    }
    DimensionType vertexTupleShape = {m_NumVertices}; // We probably don't know how many Vertices there are but take what ever the developer sends us
    vertexAttributeMatrix->setShape(vertexTupleShape);
    geometry3d->setVertexAttributeMatrix(*vertexAttributeMatrix);

    using MeshIndexType = IGeometry::MeshIndexType;
    using SharedCellList = IGeometry::SharedFaceList;

    DataPath cellsPath = getCreatedPath().createChildPath(m_SharedCellsName);
    // Create the default DataArray that will hold the CellList and Vertices.
    complex::Result result = complex::CreateArray<MeshIndexType>(dataStructure, cellTupleShape, {Geometry3DType::k_NumVerts}, cellsPath, mode);
    if(result.invalid())
    {
      return MakeErrorResult(-226, fmt::format("CreateGeometry3DAction: Could not allocate SharedCellList '{}'", cellsPath.toString()));
    }
    SharedCellList* cells = complex::ArrayFromPath<MeshIndexType>(dataStructure, cellsPath);
    geometry3d->setPolyhedraList(*cells);

    // Create the Vertex Array with a component size of 3
    DataPath vertexPath = getCreatedPath().createChildPath(m_SharedVerticesName);

    result = complex::CreateArray<float>(dataStructure, vertexTupleShape, {3}, vertexPath, mode);
    if(result.invalid())
    {
      return MakeErrorResult(-227, fmt::format("CreateGeometry3DAction: Could not allocate SharedVertList '{}'", vertexPath.toString()));
    }
    Float32Array* vertexArray = complex::ArrayFromPath<float>(dataStructure, vertexPath);
    geometry3d->setVertices(*vertexArray);

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
   * @brief Returns the path of the ImageGeometry to be created.
   * @return
   */
  DataPath getCellDataPath() const
  {
    return getCreatedPath().createChildPath(m_CellDataName);
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
   * @brief Returns the number of cells
   * @return
   */
  IGeometry::MeshIndexType numCells() const
  {
    return m_NumCells;
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
    auto topLevelCreatedPath = getCreatedPath();
    return {topLevelCreatedPath, getCellDataPath(), getVertexDataPath(), topLevelCreatedPath.createChildPath(m_SharedCellsName), topLevelCreatedPath.createChildPath(m_SharedVerticesName)};
  }

private:
  IGeometry::MeshIndexType m_NumCells;
  IGeometry::MeshIndexType m_NumVertices;
  std::string m_VertexDataName;
  std::string m_CellDataName;
  std::string m_SharedVerticesName;
  std::string m_SharedCellsName;
};

using CreateTetrahedralGeometryAction = CreateGeometry3DAction<TetrahedralGeom>;
using CreateHexahedralGeometryAction = CreateGeometry3DAction<HexahedralGeom>;
} // namespace complex
