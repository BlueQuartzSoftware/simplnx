#pragma once

#include "simplnx/Common/Array.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/HexahedralGeom.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/DataStructure/Geometry/TetrahedralGeom.hpp"
#include "simplnx/Filter/Output.hpp"
#include "simplnx/Utilities/ArrayCreationUtilities.hpp"
#include "simplnx/simplnx_export.hpp"

#include <fmt/core.h>

#include <utility>

namespace nx::core
{

/**
 * @class CreateGeometry3DAction
 * @brief Creates a three-dimensional geometry and its support arrays.
 * @tparam Geometry3DType Tetrahedral or hexahedral geometry type to create.
 *
 * Copy, Move, and Reference attach supplied topology arrays. Create allocates
 * new topology arrays and attribute matrices.
 */
template <typename Geometry3DType>
class CreateGeometry3DAction : public IDataCreationAction
{
public:
  using DimensionType = std::vector<size_t>;

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

  CreateGeometry3DAction(const DataPath& geometryPath, const DataPath& inputVerticesArrayPath, const DataPath& inputCellsArrayPath, const std::string& vertexAttributeMatrixName,
                         const std::string& cellAttributeMatrixName, const ArrayHandlingType& arrayType)
  : IDataCreationAction(geometryPath)
  , m_VertexDataName(vertexAttributeMatrixName)
  , m_CellDataName(cellAttributeMatrixName)
  , m_SharedVerticesName(inputVerticesArrayPath.getTargetName())
  , m_SharedCellsName(inputCellsArrayPath.getTargetName())
  , m_InputVertices(inputVerticesArrayPath)
  , m_InputCells(inputCellsArrayPath)
  , m_ArrayHandlingType(arrayType)
  {
  }

  ~CreateGeometry3DAction() noexcept override = default;

  CreateGeometry3DAction(const CreateGeometry3DAction&) = delete;
  CreateGeometry3DAction(CreateGeometry3DAction&&) noexcept = delete;
  CreateGeometry3DAction& operator=(const CreateGeometry3DAction&) = delete;
  CreateGeometry3DAction& operator=(CreateGeometry3DAction&&) noexcept = delete;

  /**
   * @brief Creates and configures the cell geometry.
   * @param dataStructure Destination data structure.
   * @param mode Preflight or execute action mode.
   * @return Validation, allocation, or reparenting errors.
   *
   * Copy, Move, and Reference preserve the supplied topology-array storage.
   */
  Result<> apply(DataStructure& dataStructure, Mode mode) const override
  {
    static constexpr StringLiteral prefix = "CreateGeometry3DAction: ";
    using MeshIndexType = IGeometry::MeshIndexType;
    using SharedCellList = IGeometry::SharedFaceList;
    const DataPath cellDataPath = getCellDataPath();
    const DataPath vertexDataPath = getVertexDataPath();

    if(getCreatedPath().empty())
    {
      return MakeErrorResult(-5601, fmt::format("{}CreateGeometry3DAction: Geometry Path cannot be empty", prefix));
    }

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
    if(!dataStructure.getId(parentPath).has_value())
    {
      return MakeErrorResult(-5604, fmt::format("{}CreateGeometry3DAction: Parent Id was not available for path:'{}'", prefix, parentPath.toString()));
    }

    const auto vertices = dataStructure.getDataAs<Float32Array>(m_InputVertices);
    if(m_ArrayHandlingType != ArrayHandlingType::Create && vertices == nullptr)
    {
      return MakeErrorResult(-5605, fmt::format("{}CreateGeometry3DAction: Could not find vertices array at path '{}'", prefix, m_InputVertices.toString()));
    }

    const auto cells = dataStructure.getDataAs<DataArray<MeshIndexType>>(m_InputCells);
    if(m_ArrayHandlingType != ArrayHandlingType::Create && cells == nullptr)
    {
      return MakeErrorResult(-5606, fmt::format("{}CreateGeometry3DAction: Could not find cells array at path '{}'", prefix, m_InputCells.toString()));
    }

    auto geometry3d = Geometry3DType::Create(dataStructure, getCreatedPath().getTargetName(), dataStructure.getId(parentPath).value());
    DimensionType cellTupleShape = {m_NumCells};
    DimensionType vertexTupleShape = {m_NumVertices};

    if(m_ArrayHandlingType != ArrayHandlingType::Create)
    {
      cellTupleShape = cells->getTupleShape();
      vertexTupleShape = vertices->getTupleShape();
    }

    if(m_ArrayHandlingType == ArrayHandlingType::Copy)
    {
      std::shared_ptr<DataObject> vertexCopy = vertices->deepCopy(getCreatedPath().createChildPath(m_SharedVerticesName));
      const auto vertexArray = std::dynamic_pointer_cast<Float32Array>(vertexCopy);

      std::shared_ptr<DataObject> cellsCopy = cells->deepCopy(getCreatedPath().createChildPath(m_SharedCellsName));
      const auto cellsArray = std::dynamic_pointer_cast<DataArray<MeshIndexType>>(cellsCopy);

      geometry3d->setPolyhedraList(*cellsArray);
      geometry3d->setVertices(*vertexArray);
    }
    else if(m_ArrayHandlingType == ArrayHandlingType::Move)
    {
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
      const auto geomId = geometry3d->getId();
      dataStructure.setAdditionalParent(vertices->getId(), geomId);
      dataStructure.setAdditionalParent(cells->getId(), geomId);
      geometry3d->setVertices(*vertices);
      geometry3d->setPolyhedraList(*cells);
    }
    else
    {
      const DataPath cellsPath = getCreatedPath().createChildPath(m_SharedCellsName);
      Result result = ArrayCreationUtilities::CreateArray<MeshIndexType>(dataStructure, cellTupleShape, {Geometry3DType::k_NumVerts}, cellsPath, mode);
      if(result.invalid())
      {
        return MergeResults(result, MakeErrorResult(-5609, fmt::format("{}CreateGeometry3DAction: Could not allocate SharedCellList '{}'", prefix, cellsPath.toString())));
      }
      auto* polyhedronList = dataStructure.getDataAs<SharedCellList>(cellsPath);
      if(polyhedronList == nullptr)
      {
        throw std::runtime_error(fmt::format("DataPath does not point to a DataArray. DataPath: '{}'", cellsPath.toString()));
      }
      geometry3d->setPolyhedraList(*polyhedronList);

      // Vertices use three coordinates.
      const DataPath vertexPath = getCreatedPath().createChildPath(m_SharedVerticesName);

      result = ArrayCreationUtilities::CreateArray<float>(dataStructure, vertexTupleShape, {3}, vertexPath, mode);
      if(result.invalid())
      {
        return MergeResults(result, MakeErrorResult(-5610, fmt::format("{}CreateGeometry3DAction: Could not allocate SharedVertList '{}'", prefix, vertexPath.toString())));
      }
      auto* vertexArray = dataStructure.getDataAs<Float32Array>(vertexPath);
      if(vertexArray == nullptr)
      {
        throw std::runtime_error(fmt::format("DataPath does not point to a DataArray. DataPath: '{}'", vertexPath.toString()));
      }
      geometry3d->setVertices(*vertexArray);
    }

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

  UniquePointer clone() const override
  {
    auto action =
        std::unique_ptr<CreateGeometry3DAction>(new CreateGeometry3DAction(getCreatedPath(), m_NumCells, m_NumVertices, m_VertexDataName, m_CellDataName, m_SharedVerticesName, m_SharedCellsName));
    action->m_InputVertices = m_InputVertices;
    action->m_InputCells = m_InputCells;
    action->m_ArrayHandlingType = m_ArrayHandlingType;
    return action;
  }

  const DataPath& geometryPath() const
  {
    return getCreatedPath();
  }

  DataPath getCellDataPath() const
  {
    return getCreatedPath().createChildPath(m_CellDataName);
  }

  DataPath getVertexDataPath() const
  {
    return getCreatedPath().createChildPath(m_VertexDataName);
  }

  IGeometry::MeshIndexType numCells() const
  {
    return m_NumCells;
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
};

using CreateTetrahedralGeometryAction = CreateGeometry3DAction<TetrahedralGeom>;
using CreateHexahedralGeometryAction = CreateGeometry3DAction<HexahedralGeom>;
} // namespace nx::core
