#include "CreateRectGridGeometryAction.hpp"

#include "simplnx/DataStructure/Geometry/RectGridGeom.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"

#include <fmt/core.h>

#include <memory>

namespace nx::core
{
CreateRectGridGeometryAction::CreateRectGridGeometryAction(const DataPath& path, usize xBoundTuples, usize yBoundTuples, usize zBoundTuples, const std::string& cellAttributeMatrixName,
                                                           const std::string& xBoundsName, const std::string& yBoundsName, const std::string& zBoundsName, std::string createdDataFormat)
: IDataCreationAction(path)
, m_NumXBoundTuples(xBoundTuples)
, m_NumYBoundTuples(yBoundTuples)
, m_NumZBoundTuples(zBoundTuples)
, m_CellDataName(cellAttributeMatrixName)
, m_XBoundsArrayName(xBoundsName)
, m_YBoundsArrayName(yBoundsName)
, m_ZBoundsArrayName(zBoundsName)
, m_CreatedDataStoreFormat(createdDataFormat)
{
}

CreateRectGridGeometryAction::CreateRectGridGeometryAction(const DataPath& path, const DataPath& inputXBoundsPath, const DataPath& inputYBoundsPath, const DataPath& inputZBoundsPath,
                                                           const std::string& cellAttributeMatrixName, const ArrayHandlingType& arrayType, std::string createdDataFormat)
: IDataCreationAction(path)
, m_CellDataName(cellAttributeMatrixName)
, m_XBoundsArrayName(inputXBoundsPath.getTargetName())
, m_YBoundsArrayName(inputYBoundsPath.getTargetName())
, m_ZBoundsArrayName(inputZBoundsPath.getTargetName())
, m_InputXBounds(inputXBoundsPath)
, m_InputYBounds(inputYBoundsPath)
, m_InputZBounds(inputZBoundsPath)
, m_ArrayHandlingType(arrayType)
, m_CreatedDataStoreFormat(createdDataFormat)
{
}

CreateRectGridGeometryAction::~CreateRectGridGeometryAction() noexcept = default;

Result<> CreateRectGridGeometryAction::apply(DataStructure& dataStructure, Mode mode) const
{
  static constexpr StringLiteral prefix = "CreateRectGridGeometryAction: ";
  // Check for empty Geometry DataPath
  if(getCreatedPath().empty())
  {
    return MakeErrorResult(-5901, fmt::format("{}CreateRectGridGeometryAction: Geometry Path cannot be empty", prefix));
  }
  // Check if the Geometry Path already exists
  if(auto* parentObject = dataStructure.getDataAs<BaseGroup>(getCreatedPath()); parentObject != nullptr)
  {
    return MakeErrorResult(-5902, fmt::format("{}CreateRectGridGeometryAction: DataObject already exists at path '{}'", prefix, getCreatedPath().toString()));
  }
  DataPath parentPath = getCreatedPath().getParent();
  if(!parentPath.empty())
  {
    if(Result<LinkedPath> geomPath = dataStructure.makePath(parentPath); geomPath.invalid())
    {
      return MakeErrorResult(-5903, fmt::format("{}CreateRectGridGeometryAction: Geometry could not be created at path:'{}'", prefix, getCreatedPath().toString()));
    }
  }
  // Get the Parent ID
  if(!dataStructure.getId(parentPath).has_value())
  {
    return MakeErrorResult(-5904, fmt::format("{}CreateRectGridGeometryAction: Parent Id was not available for path:'{}'", prefix, parentPath.toString()));
  }

  const auto xBounds = dataStructure.getDataAs<Float32Array>(m_InputXBounds);
  if(m_ArrayHandlingType != ArrayHandlingType::Create && xBounds == nullptr)
  {
    return MakeErrorResult(-5905, fmt::format("{}CreateRectGridGeometryAction: Could not find x bounds array at path '{}'", prefix, m_InputXBounds.toString()));
  }
  const auto yBounds = dataStructure.getDataAs<Float32Array>(m_InputYBounds);
  if(m_ArrayHandlingType != ArrayHandlingType::Create && yBounds == nullptr)
  {
    return MakeErrorResult(-5906, fmt::format("{}CreateRectGridGeometryAction: Could not find y bounds array at path '{}'", prefix, m_InputYBounds.toString()));
  }
  const auto zBounds = dataStructure.getDataAs<Float32Array>(m_InputZBounds);
  if(m_ArrayHandlingType != ArrayHandlingType::Create && zBounds == nullptr)
  {
    return MakeErrorResult(-5907, fmt::format("{}CreateRectGridGeometryAction: Could not find z bounds array at path '{}'", prefix, m_InputZBounds.toString()));
  }

  // Create the RectGridGeometry
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

  auto* attributeMatrix = AttributeMatrix::Create(dataStructure, m_CellDataName, dims, rectGridGeom->getId());
  if(attributeMatrix == nullptr)
  {
    return MakeErrorResult(-5909, fmt::format("{}CreateRectGridGeometryAction: Failed to create RectGridGeometry: '{}'", prefix, getCreatedPath().createChildPath(m_CellDataName).toString()));
  }

  rectGridGeom->setCellData(*attributeMatrix);

  // create the bounds arrays
  Result<> results;
  if(m_ArrayHandlingType == ArrayHandlingType::Copy)
  {
    std::shared_ptr<DataObject> xCopy = xBounds->deepCopy(getCreatedPath().createChildPath(m_XBoundsArrayName));
    std::shared_ptr<DataObject> yCopy = yBounds->deepCopy(getCreatedPath().createChildPath(m_YBoundsArrayName));
    std::shared_ptr<DataObject> zCopy = zBounds->deepCopy(getCreatedPath().createChildPath(m_ZBoundsArrayName));
    const auto xBoundsArray = std::dynamic_pointer_cast<Float32Array>(xCopy);
    const auto yBoundsArray = std::dynamic_pointer_cast<Float32Array>(yCopy);
    const auto zBoundsArray = std::dynamic_pointer_cast<Float32Array>(zCopy);
    rectGridGeom->setBounds(xBoundsArray.get(), yBoundsArray.get(), zBoundsArray.get());
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

    rectGridGeom->setBounds(xBounds, yBounds, zBounds);
  }
  else if(m_ArrayHandlingType == ArrayHandlingType::Reference)
  {
    const auto rectGeomId = rectGridGeom->getId();
    dataStructure.setAdditionalParent(xBounds->getId(), rectGeomId);
    dataStructure.setAdditionalParent(yBounds->getId(), rectGeomId);
    dataStructure.setAdditionalParent(zBounds->getId(), rectGeomId);
    rectGridGeom->setBounds(xBounds, yBounds, zBounds);
  }
  else
  {
    const Float32Array* xBoundsArray = createBoundArray(dataStructure, mode, m_XBoundsArrayName, m_NumXBoundTuples, results.errors());
    const Float32Array* yBoundsArray = createBoundArray(dataStructure, mode, m_YBoundsArrayName, m_NumYBoundTuples, results.errors());
    const Float32Array* zBoundsArray = createBoundArray(dataStructure, mode, m_ZBoundsArrayName, m_NumZBoundTuples, results.errors());
    rectGridGeom->setBounds(xBoundsArray, yBoundsArray, zBoundsArray);
  }

  return results;
}

IDataAction::UniquePointer CreateRectGridGeometryAction::clone() const
{
  auto action = std::unique_ptr<CreateRectGridGeometryAction>(
      new CreateRectGridGeometryAction(getCreatedPath(), m_NumXBoundTuples, m_NumYBoundTuples, m_NumZBoundTuples, m_CellDataName, m_XBoundsArrayName, m_YBoundsArrayName, m_ZBoundsArrayName));
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
  if(Result<> result = CreateArray<float32>(dataStructure, {numTuples}, componentShape, boundsPath, mode, m_CreatedDataStoreFormat); result.invalid())
  {
    errors.insert(errors.end(), result.errors().begin(), result.errors().end());
    return nullptr;
  }
  Float32Array* boundsArray = ArrayFromPath<float>(dataStructure, boundsPath);

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
} // namespace complex
