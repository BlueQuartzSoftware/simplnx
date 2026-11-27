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

  CreateGeometry3DAction(const DataPath& geometryPath, size_t numCells, size_t numVertices, std::string vertexAttributeMatrixName, std::string cellAttributeMatrixName, std::string sharedVerticesName,
                         std::string sharedCellsName)
  : IDataCreationAction(geometryPath)
  , m_NumCells(numCells)
  , m_NumVertices(numVertices)
  , m_VertexDataName(std::move(vertexAttributeMatrixName))
  , m_CellDataName(std::move(cellAttributeMatrixName))
  , m_SharedVerticesName(std::move(sharedVerticesName))
  , m_SharedCellsName(std::move(sharedCellsName))
  {
  }

  CreateGeometry3DAction(const DataPath& geometryPath, const DataPath& inputVerticesArrayPath, const DataPath& inputCellsArrayPath, std::string vertexAttributeMatrixName,
                         std::string cellAttributeMatrixName, const ArrayHandlingType& arrayType)
  : IDataCreationAction(geometryPath)
  , m_VertexDataName(std::move(vertexAttributeMatrixName))
  , m_CellDataName(std::move(cellAttributeMatrixName))
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
    static constexpr StringLiteralType k_Prefix = "CreateGeometry3DAction: ";
    using MeshIndexType = IGeometry::MeshIndexType;
    using SharedCellListType = IGeometry::SharedFaceListType;
    const DataPath cellDataPath = getCellDataPath();
    const DataPath vertexDataPath = getVertexDataPath();

    if(getCreatedPath().empty())
    {
      return MakeErrorResult(-5601, fmt::format("{}CreateGeometry3DAction: Geometry Path cannot be empty", k_Prefix));
    }

    const BaseGroup* parentObjectPtr = dataStructure.getDataAs<BaseGroup>(getCreatedPath());
    if(parentObjectPtr != nullptr)
    {
      return MakeErrorResult(-5602, fmt::format("{}CreateGeometry3DAction: DataObject already exists at path '{}'", k_Prefix, getCreatedPath().toString()));
    }

    const DataPath parentPath = getCreatedPath().getParent();
    if(!parentPath.empty())
    {
      const Result<LinkedPath> geomPath = dataStructure.makePath(parentPath);
      if(geomPath.invalid())
      {
        return MakeErrorResult(-5603, fmt::format("{}CreateGeometry3DAction: Geometry could not be created at path:'{}'", k_Prefix, getCreatedPath().toString()));
      }
    }
    if(!dataStructure.getId(parentPath).has_value())
    {
      return MakeErrorResult(-5604, fmt::format("{}CreateGeometry3DAction: Parent Id was not available for path:'{}'", k_Prefix, parentPath.toString()));
    }

    auto* const verticesPtr = dataStructure.getDataAs<Float32ArrayType>(m_InputVertices);
    if(m_ArrayHandlingType != ArrayHandlingType::Create && verticesPtr == nullptr)
    {
      return MakeErrorResult(-5605, fmt::format("{}CreateGeometry3DAction: Could not find vertices array at path '{}'", k_Prefix, m_InputVertices.toString()));
    }

    auto* const cellsPtr = dataStructure.getDataAs<DataArray<MeshIndexType>>(m_InputCells);
    if(m_ArrayHandlingType != ArrayHandlingType::Create && cellsPtr == nullptr)
    {
      return MakeErrorResult(-5606, fmt::format("{}CreateGeometry3DAction: Could not find cells array at path '{}'", k_Prefix, m_InputCells.toString()));
    }

    auto geometry3dPtr = Geometry3DType::Create(dataStructure, getCreatedPath().getTargetName(), dataStructure.getId(parentPath).value());
    DimensionType cellTupleShape = {m_NumCells};
    DimensionType vertexTupleShape = {m_NumVertices};

    if(m_ArrayHandlingType != ArrayHandlingType::Create)
    {
      cellTupleShape = cellsPtr->getTupleShape();
      vertexTupleShape = verticesPtr->getTupleShape();
    }

    if(m_ArrayHandlingType == ArrayHandlingType::Copy)
    {
      std::shared_ptr<DataObject> vertexCopy = verticesPtr->deepCopy(getCreatedPath().createChildPath(m_SharedVerticesName));
      const auto vertexArray = std::dynamic_pointer_cast<Float32ArrayType>(vertexCopy);

      std::shared_ptr<DataObject> cellsCopy = cellsPtr->deepCopy(getCreatedPath().createChildPath(m_SharedCellsName));
      const auto cellsArray = std::dynamic_pointer_cast<DataArray<MeshIndexType>>(cellsCopy);

      geometry3dPtr->setPolyhedraList(*cellsArray);
      geometry3dPtr->setVertices(*vertexArray);
    }
    else if(m_ArrayHandlingType == ArrayHandlingType::Move)
    {
      const auto geomId = geometry3dPtr->getId();

      const auto verticesId = verticesPtr->getId();
      dataStructure.setAdditionalParent(verticesId, geomId);
      const auto oldVertexParentId = dataStructure.getId(m_InputVertices.getParent());
      if(!oldVertexParentId.has_value())
      {
        return MakeErrorResult(-5607, fmt::format("{}CreateGeometry3DAction: Failed to remove vertices array '{}' from parent at path '{}' while moving array", k_Prefix, m_SharedVerticesName,
                                                  m_InputVertices.getParent().toString()));
      }
      dataStructure.removeParent(verticesId, oldVertexParentId.value());

      const auto cellsId = cellsPtr->getId();
      dataStructure.setAdditionalParent(cellsId, geomId);
      const auto oldCellParentId = dataStructure.getId(m_InputCells.getParent());
      if(!oldCellParentId.has_value())
      {
        return MakeErrorResult(-5608, fmt::format("{}CreateGeometry3DAction: Failed to remove cells array '{}' from parent at path '{}' while moving array", k_Prefix, m_SharedCellsName,
                                                  m_InputCells.getParent().toString()));
      }
      dataStructure.removeParent(cellsId, oldCellParentId.value());

      geometry3dPtr->setVertices(*verticesPtr);
      geometry3dPtr->setPolyhedraList(*cellsPtr);
    }
    else if(m_ArrayHandlingType == ArrayHandlingType::Reference)
    {
      const auto geomId = geometry3dPtr->getId();
      dataStructure.setAdditionalParent(verticesPtr->getId(), geomId);
      dataStructure.setAdditionalParent(cellsPtr->getId(), geomId);
      geometry3dPtr->setVertices(*verticesPtr);
      geometry3dPtr->setPolyhedraList(*cellsPtr);
    }
    else
    {
      const DataPath cellsPath = getCreatedPath().createChildPath(m_SharedCellsName);
      Result result = array_creation_utilities::CreateArray<MeshIndexType>(dataStructure, cellTupleShape, {Geometry3DType::k_NumVerts}, cellsPath, mode);
      if(result.invalid())
      {
        return MergeResults(result, MakeErrorResult(-5609, fmt::format("{}CreateGeometry3DAction: Could not allocate SharedCellList '{}'", k_Prefix, cellsPath.toString())));
      }
      auto* polyhedronListPtr = dataStructure.getDataAs<SharedCellListType>(cellsPath);
      if(polyhedronListPtr == nullptr)
      {
        throw std::runtime_error(fmt::format("DataPath does not point to a DataArray. DataPath: '{}'", cellsPath.toString()));
      }
      geometry3dPtr->setPolyhedraList(*polyhedronListPtr);

      // Vertices use three coordinates.
      const DataPath vertexPath = getCreatedPath().createChildPath(m_SharedVerticesName);

      result = array_creation_utilities::CreateArray<float>(dataStructure, vertexTupleShape, {3}, vertexPath, mode);
      if(result.invalid())
      {
        return MergeResults(result, MakeErrorResult(-5610, fmt::format("{}CreateGeometry3DAction: Could not allocate SharedVertList '{}'", k_Prefix, vertexPath.toString())));
      }
      auto* vertexArrayPtr = dataStructure.getDataAs<Float32ArrayType>(vertexPath);
      if(vertexArrayPtr == nullptr)
      {
        throw std::runtime_error(fmt::format("DataPath does not point to a DataArray. DataPath: '{}'", vertexPath.toString()));
      }
      geometry3dPtr->setVertices(*vertexArrayPtr);
    }

    auto* cellAttributeMatrixPtr = AttributeMatrix::Create(dataStructure, m_CellDataName, cellTupleShape, geometry3dPtr->getId());
    if(cellAttributeMatrixPtr == nullptr)
    {
      return MakeErrorResult(-5611, fmt::format("{}CreateGeometry3DAction: Failed to create attribute matrix: '{}'", k_Prefix, cellDataPath.toString()));
    }
    geometry3dPtr->setPolyhedraAttributeMatrix(*cellAttributeMatrixPtr);

    auto* vertexAttributeMatrixPtr = AttributeMatrix::Create(dataStructure, m_VertexDataName, vertexTupleShape, geometry3dPtr->getId());
    if(vertexAttributeMatrixPtr == nullptr)
    {
      return MakeErrorResult(-5612, fmt::format("{}CreateGeometry3DAction: Failed to create attribute matrix: '{}'", k_Prefix, vertexDataPath.toString()));
    }
    geometry3dPtr->setVertexAttributeMatrix(*vertexAttributeMatrixPtr);

    return {};
  }

  UniquePointerType clone() const override
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

using CreateTetrahedralGeometryActionType = CreateGeometry3DAction<TetrahedralGeom>;
using CreateHexahedralGeometryActionType = CreateGeometry3DAction<HexahedralGeom>;
} // namespace nx::core
