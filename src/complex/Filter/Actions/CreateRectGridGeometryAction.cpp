#include "CreateRectGridGeometryAction.hpp"

#include <fmt/core.h>

#include "complex/DataStructure/Geometry/RectGridGeom.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"

using namespace complex;

namespace complex
{
CreateRectGridGeometryAction::CreateRectGridGeometryAction(const DataPath& path, usize xBoundTuples, usize yBoundTuples, usize zBoundTuples, const std::string& cellAttributeMatrixName,
                                                           const std::string& xBoundsName, const std::string& yBoundsName, const std::string& zBoundsName)
: IDataCreationAction(path)
, m_NumXBoundTuples(xBoundTuples)
, m_NumYBoundTuples(yBoundTuples)
, m_NumZBoundTuples(zBoundTuples)
, m_CellDataName(cellAttributeMatrixName)
, m_XBoundsArrayName(xBoundsName)
, m_YBoundsArrayName(yBoundsName)
, m_ZBoundsArrayName(zBoundsName)
{
}

CreateRectGridGeometryAction::~CreateRectGridGeometryAction() noexcept = default;

Result<> CreateRectGridGeometryAction::apply(DataStructure& dataStructure, Mode mode) const
{
  // Check for empty Geometry DataPath
  if(getCreatedPath().empty())
  {
    return MakeErrorResult(-220, "CreateRectGridGeometryAction: Geometry Path cannot be empty");
  }
  // Check if the Geometry Path already exists
  BaseGroup* parentObject = dataStructure.getDataAs<BaseGroup>(getCreatedPath());
  if(parentObject != nullptr)
  {
    return MakeErrorResult(-222, fmt::format("CreateRectGridGeometryAction: DataObject already exists at path '{}'", getCreatedPath().toString()));
  }
  DataPath parentPath = getCreatedPath().getParent();
  if(!parentPath.empty())
  {
    Result<LinkedPath> geomPath = dataStructure.makePath(parentPath);
    if(geomPath.invalid())
    {
      return MakeErrorResult(-223, fmt::format("CreateRectGridGeometryAction: Geometry could not be created at path:'{}'", getCreatedPath().toString()));
    }
  }
  // Get the Parent ID
  if(!dataStructure.getId(parentPath).has_value())
  {
    return MakeErrorResult(-224, fmt::format("CreateRectGridGeometryAction: Parent Id was not available for path:'{}'", parentPath.toString()));
  }

  // Create the RectGridGeometry
  RectGridGeom* rectGridGeom = RectGridGeom::Create(dataStructure, getCreatedPath().getTargetName(), dataStructure.getId(parentPath).value());
  if(rectGridGeom == nullptr)
  {
    return MakeErrorResult(-225, fmt::format("CreateRectGridGeometryAction: Failed to create ImageGeometry:'{}'", getCreatedPath().toString()));
  }
  DimensionType dims = {m_NumXBoundTuples - 1, m_NumYBoundTuples - 1, m_NumZBoundTuples - 1};
  rectGridGeom->setDimensions(dims);

  auto* attributeMatrix = AttributeMatrix::Create(dataStructure, m_CellDataName, rectGridGeom->getId());
  if(attributeMatrix == nullptr)
  {
    return MakeErrorResult(-226, fmt::format("CreateRectGridGeometryAction: Failed to create ImageGeometry: '{}'", getCreatedPath().createChildPath(m_CellDataName).toString()));
  }
  attributeMatrix->setShape(std::move(dims));

  rectGridGeom->setCellData(*attributeMatrix);

  // create the bounds arrays
  DimensionType componentShape = {1};
  DataPath xBoundsPath = getCreatedPath().createChildPath(m_XBoundsArrayName);
  Result<> xResult = CreateArray<float32>(dataStructure, {m_NumXBoundTuples}, componentShape, xBoundsPath, mode);
  if(xResult.invalid())
  {
    return xResult;
  }
  Float32Array* xBoundsArray = complex::ArrayFromPath<float>(dataStructure, xBoundsPath);

  DataPath yBoundsPath = getCreatedPath().createChildPath(m_YBoundsArrayName);
  Result<> yResult = CreateArray<float32>(dataStructure, {m_NumYBoundTuples}, componentShape, yBoundsPath, mode);
  if(yResult.invalid())
  {
    return yResult;
  }
  Float32Array* yBoundsArray = complex::ArrayFromPath<float>(dataStructure, yBoundsPath);

  DataPath zBoundsPath = getCreatedPath().createChildPath(m_ZBoundsArrayName);
  Result<> zResult = CreateArray<float32>(dataStructure, {m_NumZBoundTuples}, componentShape, zBoundsPath, mode);
  if(zResult.invalid())
  {
    return zResult;
  }
  Float32Array* zBoundsArray = complex::ArrayFromPath<float>(dataStructure, zBoundsPath);

  rectGridGeom->setBounds(xBoundsArray, yBoundsArray, zBoundsArray);

  return {};
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
  auto topLevelCreatedPath = getCreatedPath();
  return {topLevelCreatedPath, topLevelCreatedPath.createChildPath(m_CellDataName), topLevelCreatedPath.createChildPath(m_XBoundsArrayName), topLevelCreatedPath.createChildPath(m_YBoundsArrayName),
          topLevelCreatedPath.createChildPath(m_ZBoundsArrayName)};
}
} // namespace complex
