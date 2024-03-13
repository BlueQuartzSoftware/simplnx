#pragma once

#include "simplnx/Common/Array.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/HexahedralGeom.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/DataStructure/Geometry/TetrahedralGeom.hpp"
#include "simplnx/Filter/Output.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/simplnx_export.hpp"

#include <fmt/core.h>

#include <utility>

namespace nx::core
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

  /**
   * @brief Constructor to create the 3D geometry and allocate a default arrays for the shared vertex & shared calls lists
   * @param geometryPath The path to the created geometry
   * @param numCells The number of cells in the geometry
   * @param numVertices The number of vertices in the geometry
   * @param vertexAttributeMatrixName The name of the vertex AttributeMatrix to be created
   * @param cellAttributeMatrixName The name of the cell AttributeMatrix to be created
   * @param sharedVerticesName The name of the shared vertex list array to be created
   * @param sharedCellsName The name of the shared cell list array to be created
   */
  CreateGeometry3DAction(const DataPath& geometryPath, size_t numCells, size_t numVertices, const std::string& vertexAttributeMatrixName, const std::string& cellAttributeMatrixName,
                         const std::string& sharedVerticesName, const std::string& sharedCellsName, std::string createdDataFormat = "")
  : IDataCreationAction(geometryPath)
  , m_NumCells(numCells)
  , m_NumVertices(numVertices)
  , m_VertexDataName(vertexAttributeMatrixName)
  , m_CellDataName(cellAttributeMatrixName)
  , m_SharedVerticesName(sharedVerticesName)
  , m_SharedCellsName(sharedCellsName)
  , m_CreatedDataStoreFormat(createdDataFormat)
  {
  }

  /**
   * @brief Constructor to create the 3D geometry using existing vertices & cells arrays by either copying, moving, or referencing them
   * @param geometryPath The path to the created geometry
   * @param inputVerticesArrayPath The path to the existing vertices array
   * @param inputCellsArrayPath The path to the existing cells array
   * @param vertexAttributeMatrixName The name of the vertex AttributeMatrix to be created
   * @param cellAttributeMatrixName The name of the cell AttributeMatrix to be created
   * @param arrayType Tells whether to copy, move, or reference the existing input vertices array
   */
  CreateGeometry3DAction(const DataPath& geometryPath, const DataPath& inputVerticesArrayPath, const DataPath& inputCellsArrayPath, const std::string& vertexAttributeMatrixName,
                         const std::string& cellAttributeMatrixName, const ArrayHandlingType& arrayType, std::string createdDataFormat = "")
  : IDataCreationAction(geometryPath)
  , m_VertexDataName(vertexAttributeMatrixName)
  , m_CellDataName(cellAttributeMatrixName)
  , m_SharedVerticesName(inputVerticesArrayPath.getTargetName())
  , m_SharedCellsName(inputCellsArrayPath.getTargetName())
  , m_InputVertices(inputVerticesArrayPath)
  , m_InputCells(inputCellsArrayPath)
  , m_ArrayHandlingType(arrayType)
  , m_CreatedDataStoreFormat(createdDataFormat)
  {
  }

  ~CreateGeometry3DAction() noexcept override = default;

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
    static constexpr StringLiteral prefix = "CreateGeometry3DAction: ";
    using MeshIndexType = IGeometry::MeshIndexType;
    using SharedCellList = IGeometry::SharedFaceList;
    const DataPath cellDataPath = getCellDataPath();
    const DataPath vertexDataPath = getVertexDataPath();

    // Check for empty Geometry DataPath
    if(getCreatedPath().empty())
    {
      return MakeErrorResult(-5601, fmt::format("{}CreateGeometry3DAction: Geometry Path cannot be empty", prefix));
    }

    // Check if the Geometry Path already exists
    const BaseGroup* parentObject = dataStructure.getDataAs<BaseGroup>(getCreatedPath());
    if(parentObject != nullptr)
    {
      return MakeErrorResult(-5602, fmt::format("{}CreateGeometry3DAction: DataObject already exists at path '{}'", prefix, getCreatedPath().toString()));
    }

    const DataPath parentPath = getCreatedPath().getParent();
    if(!parentPath.empty())
    {
      const Result<LinkedPath> geomPath = dataStructure.makePath(parentPath);
      if(geomPath.invalid())
      {
        return MakeErrorResult(-5603, fmt::format("{}CreateGeometry3DAction: Geometry could not be created at path:'{}'", prefix, getCreatedPath().toString()));
      }
    }
    // Get the Parent ID
    if(!dataStructure.getId(parentPath).has_value())
    {
      return MakeErrorResult(-5604, fmt::format("{}CreateGeometry3DAction: Parent Id was not available for path:'{}'", prefix, parentPath.toString()));
    }

    // Get the vertices list if we are using an existing array
    const auto vertices = dataStructure.getDataAs<Float32Array>(m_InputVertices);
    if(m_ArrayHandlingType != ArrayHandlingType::Create && vertices == nullptr)
    {
      return MakeErrorResult(-5605, fmt::format("{}CreateGeometry3DAction: Could not find vertices array at path '{}'", prefix, m_InputVertices.toString()));
    }

    // Get the faces list if we are using an existing array
    const auto cells = dataStructure.getDataAs<DataArray<MeshIndexType>>(m_InputCells);
    if(m_ArrayHandlingType != ArrayHandlingType::Create && cells == nullptr)
    {
      return MakeErrorResult(-5606, fmt::format("{}CreateGeometry3DAction: Could not find cells array at path '{}'", prefix, m_InputCells.toString()));
    }

    // Create the Geometry
    auto geometry3d = Geometry3DType::Create(dataStructure, getCreatedPath().getTargetName(), dataStructure.getId(parentPath).value());
    DimensionType cellTupleShape = {m_NumCells};
    DimensionType vertexTupleShape = {m_NumVertices}; // We probably don't know how many Vertices there are but take what ever the developer sends us

    if(m_ArrayHandlingType == ArrayHandlingType::Copy)
    {
      cellTupleShape = cells->getTupleShape();
      vertexTupleShape = vertices->getTupleShape();

      std::shared_ptr<DataObject> vertexCopy = vertices->deepCopy(getCreatedPath().createChildPath(m_SharedVerticesName));
      const auto vertexArray = std::dynamic_pointer_cast<Float32Array>(vertexCopy);

      std::shared_ptr<DataObject> cellsCopy = cells->deepCopy(getCreatedPath().createChildPath(m_SharedCellsName));
      const auto cellsArray = std::dynamic_pointer_cast<DataArray<MeshIndexType>>(cellsCopy);

      geometry3d->setPolyhedraList(*cellsArray);
      geometry3d->setVertices(*vertexArray);
    }
    else if(m_ArrayHandlingType == ArrayHandlingType::Move)
    {
      cellTupleShape = cells->getTupleShape();
      vertexTupleShape = vertices->getTupleShape();
      const auto geomId = geometry3d->getId();

      const auto verticesId = vertices->getId();
      dataStructure.setAdditionalParent(verticesId, geomId);
      const auto oldVertexParentId = dataStructure.getId(m_InputVertices.getParent());
      if(!oldVertexParentId.has_value())
      {
        return MakeErrorResult(-5607, fmt::format("{}CreateGeometry3DAction: Failed to remove vertices array '{}' from parent at path '{}' while moving array", prefix, m_SharedVerticesName,
                                                  m_InputVertices.getParent().toString()));
      }
      dataStructure.removeParent(verticesId, oldVertexParentId.value());

      const auto cellsId = cells->getId();
      dataStructure.setAdditionalParent(cellsId, geomId);
      const auto oldCellParentId = dataStructure.getId(m_InputCells.getParent());
      if(!oldCellParentId.has_value())
      {
        return MakeErrorResult(-5608, fmt::format("{}CreateGeometry3DAction: Failed to remove cells array '{}' from parent at path '{}' while moving array", prefix, m_SharedCellsName,
                                                  m_InputCells.getParent().toString()));
      }
      dataStructure.removeParent(cellsId, oldCellParentId.value());

      geometry3d->setVertices(*vertices);
      geometry3d->setPolyhedraList(*cells);
    }
    else if(m_ArrayHandlingType == ArrayHandlingType::Reference)
    {
      cellTupleShape = cells->getTupleShape();
      vertexTupleShape = vertices->getTupleShape();
      const auto geomId = geometry3d->getId();
      dataStructure.setAdditionalParent(vertices->getId(), geomId);
      dataStructure.setAdditionalParent(cells->getId(), geomId);
      geometry3d->setVertices(*vertices);
      geometry3d->setPolyhedraList(*cells);
    }
    else
    {
      const DataPath cellsPath = getCreatedPath().createChildPath(m_SharedCellsName);
      // Create the default DataArray that will hold the CellList and Vertices.
      Result result = CreateArray<MeshIndexType>(dataStructure, cellTupleShape, {Geometry3DType::k_NumVerts}, cellsPath, mode, m_CreatedDataStoreFormat);
      if(result.invalid())
      {
        return MergeResults(result, MakeErrorResult(-5609, fmt::format("{}CreateGeometry3DAction: Could not allocate SharedCellList '{}'", prefix, cellsPath.toString())));
      }
      SharedCellList* polyhedronList = ArrayFromPath<MeshIndexType>(dataStructure, cellsPath);
      geometry3d->setPolyhedraList(*polyhedronList);

      // Create the Vertex Array with a component size of 3
      const DataPath vertexPath = getCreatedPath().createChildPath(m_SharedVerticesName);

      result = CreateArray<float>(dataStructure, vertexTupleShape, {3}, vertexPath, mode, m_CreatedDataStoreFormat);
      if(result.invalid())
      {
        return MergeResults(result, MakeErrorResult(-5610, fmt::format("{}CreateGeometry3DAction: Could not allocate SharedVertList '{}'", prefix, vertexPath.toString())));
      }
      Float32Array* vertexArray = ArrayFromPath<float>(dataStructure, vertexPath);
      geometry3d->setVertices(*vertexArray);
    }

    // Create the vertex and cell AttributeMatrix
    auto* cellAttributeMatrix = AttributeMatrix::Create(dataStructure, m_CellDataName, cellTupleShape, geometry3d->getId());
    if(cellAttributeMatrix == nullptr)
    {
      return MakeErrorResult(-5611, fmt::format("{}CreateGeometry3DAction: Failed to create attribute matrix: '{}'", prefix, cellDataPath.toString()));
    }
    geometry3d->setPolyhedraAttributeMatrix(*cellAttributeMatrix);

    auto* vertexAttributeMatrix = AttributeMatrix::Create(dataStructure, m_VertexDataName, vertexTupleShape, geometry3d->getId());
    if(vertexAttributeMatrix == nullptr)
    {
      return MakeErrorResult(-5612, fmt::format("{}CreateGeometry3DAction: Failed to create attribute matrix: '{}'", prefix, vertexDataPath.toString()));
    }
    geometry3d->setVertexAttributeMatrix(*vertexAttributeMatrix);

    return {};
  }

  /**
   * @brief Returns a copy of the action.
   * @return
   */
  UniquePointer clone() const override
  {
    auto action =
        std::unique_ptr<CreateGeometry3DAction>(new CreateGeometry3DAction(getCreatedPath(), m_NumCells, m_NumVertices, m_VertexDataName, m_CellDataName, m_SharedVerticesName, m_SharedCellsName));
    action->m_InputVertices = m_InputVertices;
    action->m_InputCells = m_InputCells;
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
    const auto topLevelCreatedPath = getCreatedPath();
    std::vector<DataPath> createdPaths = {topLevelCreatedPath, getCellDataPath(), getVertexDataPath()};
    if(m_ArrayHandlingType == ArrayHandlingType::Create || m_ArrayHandlingType == ArrayHandlingType::Copy)
    {
      createdPaths.push_back(topLevelCreatedPath.createChildPath(m_SharedVerticesName));
      createdPaths.push_back(topLevelCreatedPath.createChildPath(m_SharedCellsName));
    }
    return createdPaths;
  }

protected:
  CreateGeometry3DAction() = default;

private:
  IGeometry::MeshIndexType m_NumCells = 1;
  IGeometry::MeshIndexType m_NumVertices = Geometry3DType::k_NumVerts;
  std::string m_VertexDataName;
  std::string m_CellDataName;
  std::string m_SharedVerticesName;
  std::string m_SharedCellsName;
  DataPath m_InputVertices;
  DataPath m_InputCells;
  ArrayHandlingType m_ArrayHandlingType = ArrayHandlingType::Create;
  std::string m_CreatedDataStoreFormat;
};

using CreateTetrahedralGeometryAction = CreateGeometry3DAction<TetrahedralGeom>;
using CreateHexahedralGeometryAction = CreateGeometry3DAction<HexahedralGeom>;
} // namespace nx::core
