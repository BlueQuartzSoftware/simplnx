#include "CreateAttributeMatrixAction.hpp"

#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/LinkedPath.hpp"

#include <fmt/core.h>

namespace complex
{
//------------------------------------------------------------------------------
CreateAttributeMatrixAction::CreateAttributeMatrixAction(const DataPath& path, const AttributeMatrix::ShapeType& shape)
: IDataCreationAction(path)
, m_TupleShape(shape)
{
}

CreateAttributeMatrixAction::~CreateAttributeMatrixAction() noexcept = default;

Result<> CreateAttributeMatrixAction::apply(DataStructure& dataStructure, Mode mode) const
{
  static constexpr StringLiteral prefix = "CreateAttributeMatrixAction: ";
  DataPath createdAttributeMatrixPath = getCreatedPath();
  // Check for empty Geometry DataPath
  if(createdAttributeMatrixPath.empty())
  {
    return MakeErrorResult(-5201, fmt::format("{}CreateAttributeMatrixAction: AttributeMatrix Path cannot be empty", prefix));
  }
  // Check if the Geometry Path already exists
  BaseGroup* amObject = dataStructure.getDataAs<BaseGroup>(createdAttributeMatrixPath);
  if(amObject != nullptr)
  {
    return MakeErrorResult(-5203, fmt::format("{}CreateAttributeMatrixAction: DataObject already exists at path '{}'", prefix, createdAttributeMatrixPath.toString()));
  }
  DataPath parentPath = createdAttributeMatrixPath.getParent();
  if(!parentPath.empty())
  {
    Result<LinkedPath> amPath = dataStructure.makePath(parentPath);
    if(amPath.invalid())
    {
      return MakeErrorResult(-5204, fmt::format("{}CreateAttributeMatrixAction: AttributeMatrix could not be created at path:'{}'", prefix, createdAttributeMatrixPath.toString()));
    }
  }
  // Get the Parent ID
  if(!dataStructure.getId(parentPath).has_value())
  {
    return MakeErrorResult(-5205, fmt::format("{}CreateAttributeMatrixAction: Parent Id was not available for path:'{}'", prefix, parentPath.toString()));
  }

  auto* attributeMatrix = AttributeMatrix::Create(dataStructure, createdAttributeMatrixPath.getTargetName(), m_TupleShape, dataStructure.getId(parentPath).value());
  if(attributeMatrix == nullptr)
  {
    return MakeErrorResult(-5206, fmt::format("{}CreateAttributeMatrixAction: Failed to create AttributeMatrix: '{}'", prefix, createdAttributeMatrixPath.toString()));
  }

  return {};
}

IDataAction::UniquePointer CreateAttributeMatrixAction::clone() const
{
  return std::make_unique<CreateAttributeMatrixAction>(getCreatedPath(), m_TupleShape);
}

std::vector<DataPath> CreateAttributeMatrixAction::getAllCreatedPaths() const
{
  return {getCreatedPath()};
}
} // namespace complex
