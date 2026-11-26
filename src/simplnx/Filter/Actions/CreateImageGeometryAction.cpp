#include "CreateImageGeometryAction.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"

#include <fmt/core.h>

#include <utility>

using namespace nx::core;

namespace nx::core
{
CreateImageGeometryAction::CreateImageGeometryAction(const DataPath& path, DimensionType dims, OriginType origin, SpacingType spacing, std::string cellAttributeMatrixName, IGeometry::LengthUnit units)
: IDataCreationAction(path)
, m_Dims(std::move(dims))
, m_Origin(std::move(origin))
, m_Spacing(std::move(spacing))
, m_Units(units)
, m_CellDataName(std::move(cellAttributeMatrixName))
{
}

CreateImageGeometryAction::~CreateImageGeometryAction() noexcept = default;

Result<> CreateImageGeometryAction::apply(DataStructure& dataStructure, Mode /*mode*/) const
{
  static constexpr StringLiteral k_Prefix = "CreateImageGeometryAction: ";
  // Check for empty Geometry DataPath
  if(getCreatedPath().empty())
  {
    return MakeErrorResult(-5701, fmt::format("{}Geometry Path cannot be empty", k_Prefix));
  }
  // Check if the Geometry Path already exists
  BaseGroup* parentObjectPtr = dataStructure.getDataAs<BaseGroup>(getCreatedPath());
  if(parentObjectPtr != nullptr)
  {
    return MakeErrorResult(-5702, fmt::format("{}DataObject already exists at path '{}'", k_Prefix, getCreatedPath().toString()));
  }
  const auto addGeometryPath = [&](Result<> result) {
    for(auto& error : result.errors())
    {
      error.message = fmt::format("{}Geometry at '{}': {}", k_Prefix, getCreatedPath().toString(), error.message);
    }
    return result;
  };
  if(m_Origin.size() < 3 || m_Spacing.size() < 3)
  {
    constexpr int32 k_MissingMetadataComponents = -5707;
    return MakeErrorResult(k_MissingMetadataComponents, fmt::format("Image geometry at '{}' requires three origin and spacing components. Received {} origin and {} spacing components.",
                                                                    getCreatedPath().toString(), m_Origin.size(), m_Spacing.size()));
  }
  const FloatVec3 origin = m_Origin;
  const FloatVec3 spacing = m_Spacing;
  if(auto result = ImageGeom::ValidateOrigin(origin); result.invalid())
  {
    return addGeometryPath(std::move(result));
  }
  if(auto result = ImageGeom::ValidateSpacing(spacing); result.invalid())
  {
    return addGeometryPath(std::move(result));
  }

  DataPath parentPath = getCreatedPath().getParent();
  if(!parentPath.empty())
  {
    Result<LinkedPath> geomPath = dataStructure.makePath(parentPath);
    if(geomPath.invalid())
    {
      return MakeErrorResult(-5703, fmt::format("{}Geometry could not be created at path:'{}'", k_Prefix, getCreatedPath().toString()));
    }
  }
  // Get the Parent ID
  if(!dataStructure.getId(parentPath).has_value())
  {
    return MakeErrorResult(-5704, fmt::format("{}Parent Id was not available for path:'{}'", k_Prefix, parentPath.toString()));
  }

  // Create the ImageGeometry
  ImageGeom* imageGeomPtr = ImageGeom::Create(dataStructure, getCreatedPath().getTargetName(), dataStructure.getId(parentPath));
  if(imageGeomPtr == nullptr)
  {
    return MakeErrorResult(-5705, fmt::format("{}Failed to create ImageGeometry:'{}'", k_Prefix, getCreatedPath().toString()));
  }
  imageGeomPtr->setDimensions(m_Dims);
  if(auto result = imageGeomPtr->setOrigin(origin); result.invalid())
  {
    return addGeometryPath(std::move(result));
  }
  if(auto result = imageGeomPtr->setSpacing(spacing); result.invalid())
  {
    return addGeometryPath(std::move(result));
  }
  imageGeomPtr->setUnits(m_Units);

  const DimensionType reversedDims(m_Dims.rbegin(), m_Dims.rend());
  auto* attributeMatrixPtr = AttributeMatrix::Create(dataStructure, m_CellDataName, reversedDims, imageGeomPtr->getId());
  if(attributeMatrixPtr == nullptr)
  {
    return MakeErrorResult(-5706, fmt::format("{}Failed to create ImageGeometry: '{}'", k_Prefix, getCreatedPath().createChildPath(m_CellDataName).toString()));
  }

  imageGeomPtr->setCellData(*attributeMatrixPtr);

  return {};
}

IDataAction::UniquePointer CreateImageGeometryAction::clone() const
{
  return std::make_unique<CreateImageGeometryAction>(getCreatedPath(), m_Dims, m_Origin, m_Spacing, m_CellDataName, m_Units);
}

DataPath CreateImageGeometryAction::path() const
{
  return getCreatedPath();
}

const CreateImageGeometryAction::DimensionType& CreateImageGeometryAction::dims() const
{
  return m_Dims;
}

const CreateImageGeometryAction::OriginType& CreateImageGeometryAction::origin() const
{
  return m_Origin;
}

const CreateImageGeometryAction::SpacingType& CreateImageGeometryAction::spacing() const
{
  return m_Spacing;
}

std::string CreateImageGeometryAction::cellAttributeMatrixName() const
{
  return m_CellDataName;
}

IGeometry::LengthUnit CreateImageGeometryAction::units() const
{
  return m_Units;
}

std::vector<DataPath> CreateImageGeometryAction::getAllCreatedPaths() const
{
  auto topLevelCreatedPath = getCreatedPath();
  return {topLevelCreatedPath, topLevelCreatedPath.createChildPath(m_CellDataName)};
}
} // namespace nx::core
