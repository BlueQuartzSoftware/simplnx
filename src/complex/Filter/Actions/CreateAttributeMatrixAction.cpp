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
  DataPath createdAttributeMatrixPath = getCreatedPath();
  // Check for empty Geometry DataPath
  if(createdAttributeMatrixPath.empty())
  {
    return MakeErrorResult(-65450, "CreateAttributeMatrixAction: AttributeMatrix Path cannot be empty");
  }
  // Check if the Geometry Path already exists
  BaseGroup* amObject = dataStructure.getDataAs<BaseGroup>(createdAttributeMatrixPath);
  if(amObject != nullptr)
  {
    return MakeErrorResult(-65451, fmt::format("CreateAttributeMatrixAction: DataObject already exists at path '{}'", createdAttributeMatrixPath.toString()));
  }
  DataPath parentPath = createdAttributeMatrixPath.getParent();
  if(!parentPath.empty())
  {
    Result<LinkedPath> amPath = dataStructure.makePath(parentPath);
    if(amPath.invalid())
    {
      return MakeErrorResult(-65452, fmt::format("CreateAttributeMatrixAction: AttributeMatrix could not be created at path:'{}'", createdAttributeMatrixPath.toString()));
    }
  }
  // Get the Parent ID
  if(!dataStructure.getId(parentPath).has_value())
  {
    return MakeErrorResult(-65453, fmt::format("CreateAttributeMatrixAction: Parent Id was not available for path:'{}'", parentPath.toString()));
  }

  auto* attributeMatrix = AttributeMatrix::Create(dataStructure, createdAttributeMatrixPath.getTargetName(), dataStructure.getId(parentPath).value());
  if(attributeMatrix == nullptr)
  {
    return MakeErrorResult(-65454, fmt::format("CreateAttributeMatrixAction: Failed to create AttributeMatrix: '{}'", createdAttributeMatrixPath.toString()));
  }
  attributeMatrix->setShape(m_TupleShape);

  return {};
}
} // namespace complex
