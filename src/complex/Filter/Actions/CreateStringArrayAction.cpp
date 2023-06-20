#include "CreateStringArrayAction.hpp"

#include "complex/DataStructure/EmptyDataStore.hpp"
#include "complex/DataStructure/StringArray.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"

#include <fmt/core.h>

using namespace complex;

namespace complex
{
CreateStringArrayAction::CreateStringArrayAction(const std::vector<usize>& tDims, const DataPath& path, const std::string& initializeValue)
: IDataCreationAction(path)
, m_Dims(tDims)
, m_InitializeValue(initializeValue)
{
}

CreateStringArrayAction::~CreateStringArrayAction() noexcept = default;

Result<> CreateStringArrayAction::apply(DataStructure& dataStructure, Mode mode) const
{
  static constexpr StringLiteral prefix = "CreateStringArrayAction: ";
  auto parentPath = path().getParent();

  std::optional<DataObject::IdType> dataObjectId;

  DataObject* parentObject = nullptr;
  if(parentPath.getLength() != 0)
  {
    parentObject = dataStructure.getData(parentPath);
    if(parentObject == nullptr)
    {
      return MakeErrorResult(-6001, fmt::format("{}CreateStringArrayAction:: Parent object '{}' does not exist", prefix, parentPath.toString()));
    }

    dataObjectId = parentObject->getId();
  }

  if(m_Dims.empty())
  {
    return MakeErrorResult(-6002, fmt::format("{}CreateStringArrayAction: Tuple Shape was empty. Please set the number of tuples.", prefix));
  }

  std::string name = path().getTargetName();

  usize totalTuples = std::accumulate(m_Dims.cbegin(), m_Dims.cend(), static_cast<usize>(1), std::multiplies<>());

  std::vector<std::string> values(totalTuples, m_InitializeValue);
  StringArray* array = StringArray::CreateWithValues(dataStructure, name, values, dataObjectId);
  if(array == nullptr)
  {
    if(parentObject != nullptr && parentObject->getDataObjectType() == DataObject::Type::AttributeMatrix)
    {
      auto* attrMatrix = dynamic_cast<AttributeMatrix*>(parentObject);
      std::string amShape = fmt::format("Attribute Matrix Tuple Dims: {}", fmt::join(attrMatrix->getShape(), " x "));
      std::string arrayShape = fmt::format("String Array Tuple Shape: {}", fmt::join(m_Dims, " x "));
      return MakeErrorResult(-6003, fmt::format("{}Unable to create String Array '{}' inside Attribute matrix '{}'. Mismatch of tuple dimensions. The created String Array must have the same tuple "
                                                "dimensions or the same total number of tuples.\n{}\n{}",
                                                prefix, name, parentPath.toString(), amShape, arrayShape));
    }
    else
    {
      return MakeErrorResult(-6004, fmt::format("{}CreateStringArrayAction: Unable to create StringArray at '{}'", prefix, path().toString()));
    }
  }
  return {};
}

IDataAction::UniquePointer CreateStringArrayAction::clone() const
{
  return std::make_unique<CreateStringArrayAction>(m_Dims, getCreatedPath());
}

const std::vector<usize>& CreateStringArrayAction::dims() const
{
  return m_Dims;
}

DataPath CreateStringArrayAction::path() const
{
  return getCreatedPath();
}

std::vector<DataPath> CreateStringArrayAction::getAllCreatedPaths() const
{
  return {getCreatedPath()};
}
} // namespace complex
