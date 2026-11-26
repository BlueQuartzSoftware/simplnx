#include "CreateRectGridGeometryAction.hpp"

#include "simplnx/DataStructure/Geometry/RectGridGeom.hpp"
#include "simplnx/Utilities/ArrayCreationUtilities.hpp"

#include <fmt/core.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <memory>
#include <utility>

namespace nx::core
{
namespace
{
constexpr int32 k_BoundsCopyFailed = -5913;
constexpr int32 k_BoundsCountTooSmall = -5914;

Result<> InitializeBounds(Float32Array& array)
{
  auto& store = array.getDataStoreRef();
  std::array<float32, 4096> buffer{};
  float32 previous = -1.0f;
  for(usize offset = 0; offset < array.getNumberOfTuples();)
  {
    const usize count = std::min(buffer.size(), array.getNumberOfTuples() - offset);
    for(usize index = 0; index < count; ++index)
    {
      const float32 value = static_cast<float32>(offset + index);
      // For indices beyond float32's exact integer range, use the next representable bound.
      previous = value > previous ? value : std::nextafter(previous, std::numeric_limits<float32>::infinity());
      buffer[index] = previous;
    }
    if(auto result = store.copyFromBuffer(offset, nonstd::span<const float32>(buffer.data(), count)); result.invalid())
    {
      return result;
    }
    offset += count;
  }
  return {};
}
} // namespace

CreateRectGridGeometryAction::CreateRectGridGeometryAction(const DataPath& path, usize xBoundTuples, usize yBoundTuples, usize zBoundTuples, std::string cellAttributeMatrixName,
                                                           std::string xBoundsName, std::string yBoundsName, std::string zBoundsName)
: IDataCreationAction(path)
, m_NumXBoundTuples(xBoundTuples)
, m_NumYBoundTuples(yBoundTuples)
, m_NumZBoundTuples(zBoundTuples)
, m_CellDataName(std::move(cellAttributeMatrixName))
, m_XBoundsArrayName(std::move(xBoundsName))
, m_YBoundsArrayName(std::move(yBoundsName))
, m_ZBoundsArrayName(std::move(zBoundsName))
{
}

CreateRectGridGeometryAction::CreateRectGridGeometryAction(const DataPath& path, const DataPath& inputXBoundsPath, const DataPath& inputYBoundsPath, const DataPath& inputZBoundsPath,
                                                           std::string cellAttributeMatrixName, const ArrayHandlingType& arrayType)
: IDataCreationAction(path)
, m_CellDataName(std::move(cellAttributeMatrixName))
, m_XBoundsArrayName(inputXBoundsPath.getTargetName())
, m_YBoundsArrayName(inputYBoundsPath.getTargetName())
, m_ZBoundsArrayName(inputZBoundsPath.getTargetName())
, m_InputXBounds(inputXBoundsPath)
, m_InputYBounds(inputYBoundsPath)
, m_InputZBounds(inputZBoundsPath)
, m_ArrayHandlingType(arrayType)
{
}

CreateRectGridGeometryAction::~CreateRectGridGeometryAction() noexcept = default;

Result<> CreateRectGridGeometryAction::apply(DataStructure& dataStructure, Mode mode) const
{
  static constexpr StringLiteral prefix = "CreateRectGridGeometryAction: ";
  if(getCreatedPath().empty())
  {
    return MakeErrorResult(-5901, fmt::format("{}CreateRectGridGeometryAction: Geometry Path cannot be empty", prefix));
  }
  if(auto* parentObject = dataStructure.getDataAs<BaseGroup>(getCreatedPath()); parentObject != nullptr)
  {
    return MakeErrorResult(-5902, fmt::format("{}CreateRectGridGeometryAction: DataObject already exists at path '{}'", prefix, getCreatedPath().toString()));
  }
  auto* const xBounds = dataStructure.getDataAs<Float32Array>(m_InputXBounds);
  if(m_ArrayHandlingType != ArrayHandlingType::Create && xBounds == nullptr)
  {
    return MakeErrorResult(-5905, fmt::format("{}CreateRectGridGeometryAction: Could not find x bounds array at path '{}'", prefix, m_InputXBounds.toString()));
  }
  auto* const yBounds = dataStructure.getDataAs<Float32Array>(m_InputYBounds);
  if(m_ArrayHandlingType != ArrayHandlingType::Create && yBounds == nullptr)
  {
    return MakeErrorResult(-5906, fmt::format("{}CreateRectGridGeometryAction: Could not find y bounds array at path '{}'", prefix, m_InputYBounds.toString()));
  }
  auto* const zBounds = dataStructure.getDataAs<Float32Array>(m_InputZBounds);
  if(m_ArrayHandlingType != ArrayHandlingType::Create && zBounds == nullptr)
  {
    return MakeErrorResult(-5907, fmt::format("{}CreateRectGridGeometryAction: Could not find z bounds array at path '{}'", prefix, m_InputZBounds.toString()));
  }

  const auto addGeometryPath = [&](Result<> result) {
    for(auto& error : result.errors())
    {
      error.message = fmt::format("{}Geometry at '{}': {}", prefix, getCreatedPath().toString(), error.message);
    }
    return result;
  };
  const std::array<usize, 3> boundCounts = {m_NumXBoundTuples, m_NumYBoundTuples, m_NumZBoundTuples};
  if(m_ArrayHandlingType == ArrayHandlingType::Create)
  {
    constexpr std::array<char, 3> k_Axes = {'X', 'Y', 'Z'};
    for(usize axis = 0; axis < boundCounts.size(); ++axis)
    {
      if(boundCounts[axis] < 2)
      {
        return MakeErrorResult(k_BoundsCountTooSmall,
                               fmt::format("Rectilinear grid geometry at '{}' needs at least two {} bounds. Requested {}.", getCreatedPath().toString(), k_Axes[axis], boundCounts[axis]));
      }
    }
  }
  else if(auto result = RectGridGeom::ValidateBounds(xBounds, yBounds, zBounds); result.invalid())
  {
    return addGeometryPath(std::move(result));
  }

  DataPath parentPath = getCreatedPath().getParent();
  if(!parentPath.empty())
  {
    if(Result<LinkedPath> geomPath = dataStructure.makePath(parentPath); geomPath.invalid())
    {
      return MakeErrorResult(-5903, fmt::format("{}CreateRectGridGeometryAction: Geometry could not be created at path:'{}'", prefix, getCreatedPath().toString()));
    }
  }
  if(!dataStructure.getId(parentPath).has_value())
  {
    return MakeErrorResult(-5904, fmt::format("{}CreateRectGridGeometryAction: Parent Id was not available for path:'{}'", prefix, parentPath.toString()));
  }

  RectGridGeom* rectGridGeom = RectGridGeom::Create(dataStructure, getCreatedPath().getTargetName(), dataStructure.getId(parentPath).value());
  if(rectGridGeom == nullptr)
  {
    return MakeErrorResult(-5908, fmt::format("{}CreateRectGridGeometryAction: Failed to create RectGridGeometry:'{}'", prefix, getCreatedPath().toString()));
  }
  DimensionType dims = {m_NumXBoundTuples - 1, m_NumYBoundTuples - 1, m_NumZBoundTuples - 1};
  if(m_ArrayHandlingType != ArrayHandlingType::Create)
  {
    dims = {xBounds->getNumberOfTuples() - 1, yBounds->getNumberOfTuples() - 1, zBounds->getNumberOfTuples() - 1};
  }
  rectGridGeom->setDimensions(dims);

  // Cell AttributeMatrix tuple order is ZYX to match cell arrays.
  const DimensionType reversedDims(dims.rbegin(), dims.rend());
  auto* attributeMatrix = AttributeMatrix::Create(dataStructure, m_CellDataName, reversedDims, rectGridGeom->getId());
  if(attributeMatrix == nullptr)
  {
    return MakeErrorResult(-5909, fmt::format("{}CreateRectGridGeometryAction: Failed to create RectGridGeometry: '{}'", prefix, getCreatedPath().createChildPath(m_CellDataName).toString()));
  }

  rectGridGeom->setCellData(*attributeMatrix);

  if(m_ArrayHandlingType == ArrayHandlingType::Copy)
  {
    std::shared_ptr<DataObject> xCopy = xBounds->deepCopy(getCreatedPath().createChildPath(m_XBoundsArrayName));
    std::shared_ptr<DataObject> yCopy = yBounds->deepCopy(getCreatedPath().createChildPath(m_YBoundsArrayName));
    std::shared_ptr<DataObject> zCopy = zBounds->deepCopy(getCreatedPath().createChildPath(m_ZBoundsArrayName));
    const auto xBoundsArray = std::dynamic_pointer_cast<Float32Array>(xCopy);
    const auto yBoundsArray = std::dynamic_pointer_cast<Float32Array>(yCopy);
    const auto zBoundsArray = std::dynamic_pointer_cast<Float32Array>(zCopy);
    if(xBoundsArray == nullptr || yBoundsArray == nullptr || zBoundsArray == nullptr)
    {
      return MakeErrorResult(k_BoundsCopyFailed, fmt::format("Failed to copy bounds arrays for rectilinear grid geometry at '{}'.", getCreatedPath().toString()));
    }
    // The copies have the same values as the validated inputs.
    rectGridGeom->setXBoundsId(xBoundsArray->getId());
    rectGridGeom->setYBoundsId(yBoundsArray->getId());
    rectGridGeom->setZBoundsId(zBoundsArray->getId());
  }
  else if(m_ArrayHandlingType == ArrayHandlingType::Move)
  {
    const auto rectGeomId = rectGridGeom->getId();
    const auto xBoundId = xBounds->getId();
    dataStructure.setAdditionalParent(xBoundId, rectGeomId);
    const auto oldXParentId = dataStructure.getId(m_InputXBounds.getParent());
    if(!oldXParentId.has_value())
    {
      return MakeErrorResult(-5910, fmt::format("{}CreateRectGridGeometryAction: Failed to remove x bounds array '{}' from parent at path '{}' while moving array", prefix, m_XBoundsArrayName,
                                                m_InputXBounds.getParent().toString()));
    }
    dataStructure.removeParent(xBoundId, oldXParentId.value());

    const auto yBoundId = yBounds->getId();
    dataStructure.setAdditionalParent(yBoundId, rectGeomId);
    const auto oldYParentId = dataStructure.getId(m_InputYBounds.getParent());
    if(!oldYParentId.has_value())
    {
      return MakeErrorResult(-5911, fmt::format("{}CreateRectGridGeometryAction: Failed to remove y bounds array '{}' from parent at path '{}' while moving array", prefix, m_YBoundsArrayName,
                                                m_InputYBounds.getParent().toString()));
    }
    dataStructure.removeParent(yBoundId, oldYParentId.value());

    const auto zBoundId = zBounds->getId();
    dataStructure.setAdditionalParent(zBoundId, rectGeomId);
    const auto oldZParentId = dataStructure.getId(m_InputZBounds.getParent());
    if(!oldZParentId.has_value())
    {
      return MakeErrorResult(-5912, fmt::format("{}CreateRectGridGeometryAction: Failed to remove z bounds array '{}' from parent at path '{}' while moving array", prefix, m_ZBoundsArrayName,
                                                m_InputZBounds.getParent().toString()));
    }
    dataStructure.removeParent(zBoundId, oldZParentId.value());

    // Parent changes preserve the already validated bounds values.
    rectGridGeom->setXBoundsId(xBounds->getId());
    rectGridGeom->setYBoundsId(yBounds->getId());
    rectGridGeom->setZBoundsId(zBounds->getId());
  }
  else if(m_ArrayHandlingType == ArrayHandlingType::Reference)
  {
    const auto rectGeomId = rectGridGeom->getId();
    dataStructure.setAdditionalParent(xBounds->getId(), rectGeomId);
    dataStructure.setAdditionalParent(yBounds->getId(), rectGeomId);
    dataStructure.setAdditionalParent(zBounds->getId(), rectGeomId);
    // Parent changes preserve the already validated bounds values.
    rectGridGeom->setXBoundsId(xBounds->getId());
    rectGridGeom->setYBoundsId(yBounds->getId());
    rectGridGeom->setZBoundsId(zBounds->getId());
  }
  else
  {
    std::vector<Error> errors;
    auto* xBoundsArray = createBoundArray(dataStructure, mode, m_XBoundsArrayName, m_NumXBoundTuples, errors);
    auto* yBoundsArray = createBoundArray(dataStructure, mode, m_YBoundsArrayName, m_NumYBoundTuples, errors);
    auto* zBoundsArray = createBoundArray(dataStructure, mode, m_ZBoundsArrayName, m_NumZBoundTuples, errors);
    if(!errors.empty())
    {
      return addGeometryPath({nonstd::make_unexpected(std::move(errors))});
    }
    if(mode == Mode::Execute)
    {
      for(auto* bounds : {xBoundsArray, yBoundsArray, zBoundsArray})
      {
        if(auto result = InitializeBounds(*bounds); result.invalid())
        {
          return addGeometryPath(std::move(result));
        }
      }
    }
    if(auto result = rectGridGeom->setBounds(xBoundsArray, yBoundsArray, zBoundsArray); result.invalid())
    {
      return addGeometryPath(std::move(result));
    }
  }

  return {};
}

IDataAction::UniquePointer CreateRectGridGeometryAction::clone() const
{
  auto action = std::make_unique<CreateRectGridGeometryAction>(getCreatedPath(), m_NumXBoundTuples, m_NumYBoundTuples, m_NumZBoundTuples, m_CellDataName, m_XBoundsArrayName, m_YBoundsArrayName,
                                                               m_ZBoundsArrayName);
  action->m_InputXBounds = m_InputXBounds;
  action->m_InputYBounds = m_InputYBounds;
  action->m_InputZBounds = m_InputZBounds;
  action->m_ArrayHandlingType = m_ArrayHandlingType;
  return action;
}

Float32Array* CreateRectGridGeometryAction::createBoundArray(DataStructure& dataStructure, Mode mode, const std::string& arrayName, usize numTuples, std::vector<Error>& errors) const
{
  const DimensionType componentShape = {1};
  const DataPath boundsPath = getCreatedPath().createChildPath(arrayName);
  if(Result<> result = ArrayCreationUtilities::CreateArray<float32>(dataStructure, {numTuples}, componentShape, boundsPath, mode); result.invalid())
  {
    errors.insert(errors.end(), result.errors().begin(), result.errors().end());
    return nullptr;
  }
  auto* boundsArray = dataStructure.getDataAs<Float32Array>(boundsPath);
  if(boundsArray == nullptr)
  {
    throw std::runtime_error(fmt::format("DataPath does not point to a DataArray. DataPath: '{}'", boundsPath.toString()));
  }

  return boundsArray;
}

DataPath CreateRectGridGeometryAction::path() const
{
  return getCreatedPath();
}

const usize& CreateRectGridGeometryAction::xDims() const
{
  return m_NumXBoundTuples;
}

const usize& CreateRectGridGeometryAction::yDims() const
{
  return m_NumYBoundTuples;
}

const usize& CreateRectGridGeometryAction::zDims() const
{
  return m_NumZBoundTuples;
}

std::vector<DataPath> CreateRectGridGeometryAction::getAllCreatedPaths() const
{
  const auto topLevelCreatedPath = getCreatedPath();
  std::vector<DataPath> createdPaths = {topLevelCreatedPath, topLevelCreatedPath.createChildPath(m_CellDataName)};
  if(m_ArrayHandlingType == ArrayHandlingType::Create || m_ArrayHandlingType == ArrayHandlingType::Copy)
  {
    createdPaths.push_back(topLevelCreatedPath.createChildPath(m_XBoundsArrayName));
    createdPaths.push_back(topLevelCreatedPath.createChildPath(m_YBoundsArrayName));
    createdPaths.push_back(topLevelCreatedPath.createChildPath(m_ZBoundsArrayName));
  }
  return createdPaths;
}
} // namespace nx::core
