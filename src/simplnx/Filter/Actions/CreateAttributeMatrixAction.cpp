#include "CreateAttributeMatrixAction.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/LinkedPath.hpp"

#include <fmt/core.h>

#include <utility>

namespace nx::core
{
//------------------------------------------------------------------------------
CreateAttributeMatrixAction::CreateAttributeMatrixAction(const DataPath& path, ShapeType shape)
: IDataCreationAction(path)
, m_TupleShape(std::move(shape))
{
}

CreateAttributeMatrixAction::~CreateAttributeMatrixAction() noexcept = default;

Result<> CreateAttributeMatrixAction::apply(DataStructure& dataStructure, Mode /*mode*/) const
{
  static constexpr StringLiteralType k_Prefix = "CreateAttributeMatrixAction: ";
  DataPath createdAttributeMatrixPath = getCreatedPath();
  // Check for empty Geometry DataPath
  if(createdAttributeMatrixPath.empty())
  {
    return MakeErrorResult(-5201, fmt::format("{} AttributeMatrix Path cannot be empty", k_Prefix));
  }
  // Check if the Geometry Path already exists
  BaseGroup* amObjectPtr = dataStructure.getDataAs<BaseGroup>(createdAttributeMatrixPath);
  if(amObjectPtr != nullptr)
  {
    return MakeErrorResult(-5203, fmt::format("{} DataObject already exists at path '{}'", k_Prefix, createdAttributeMatrixPath.toString()));
  }
  DataPath parentPath = createdAttributeMatrixPath.getParent();
  if(!parentPath.empty())
  {
    Result<LinkedPath> amPath = dataStructure.makePath(parentPath);
    if(amPath.invalid())
    {
      return MakeErrorResult(-5204, fmt::format("{} AttributeMatrix could not be created at path:'{}'", k_Prefix, createdAttributeMatrixPath.toString()));
    }
  }
  // Get the Parent ID
  if(!dataStructure.getId(parentPath).has_value())
  {
    return MakeErrorResult(-5205, fmt::format("{} Parent Id was not available for path:'{}'", k_Prefix, parentPath.toString()));
  }

  AttributeMatrix* amPtr = dataStructure.getDataAs<AttributeMatrix>(parentPath);
  if(amPtr != nullptr)
  {
    return MakeErrorResult(-5205, fmt::format("{} The Attribute Matrix path '{}' could not be created. Creating an Attribute Matrix inside another Attribute Matrix is not allowed. Please select a "
                                              "different parent path such as a geometry or group.",
                                              k_Prefix, createdAttributeMatrixPath.toString()));
  }

  auto* attributeMatrixPtr = AttributeMatrix::Create(dataStructure, createdAttributeMatrixPath.getTargetName(), m_TupleShape, dataStructure.getId(parentPath).value());
  if(attributeMatrixPtr == nullptr)
  {
    return MakeErrorResult(-5206, fmt::format("{} Failed to create AttributeMatrix: '{}'", k_Prefix, createdAttributeMatrixPath.toString()));
  }

  return {};
}

IDataAction::UniquePointerType CreateAttributeMatrixAction::clone() const
{
  return std::make_unique<CreateAttributeMatrixAction>(getCreatedPath(), m_TupleShape);
}

std::vector<DataPath> CreateAttributeMatrixAction::getAllCreatedPaths() const
{
  return {getCreatedPath()};
}
} // namespace nx::core
