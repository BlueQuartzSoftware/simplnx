#include "CreateStringArrayAction.hpp"

#include "simplnx/DataStructure/EmptyDataStore.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <fmt/core.h>

#include <utility>

using namespace nx::core;

namespace nx::core
{
CreateStringArrayAction::CreateStringArrayAction(const std::vector<usize>& tDims, const DataPath& path, std::string initializeValue)
: IDataCreationAction(path)
, m_Dims(tDims)
, m_InitializeValue(std::move(initializeValue))
{
}

CreateStringArrayAction::~CreateStringArrayAction() noexcept = default;

Result<> CreateStringArrayAction::apply(DataStructure& dataStructure, Mode /*mode*/) const
{
  static constexpr StringLiteralType k_Prefix = "CreateStringArrayAction: ";
  auto parentPath = path().getParent();

  std::optional<DataObject::IdType> dataObjectId;

  DataObject* parentObjectPtr = nullptr;
  if(parentPath.getLength() != 0)
  {
    parentObjectPtr = dataStructure.getData(parentPath);
    if(parentObjectPtr == nullptr)
    {
      return MakeErrorResult(-6001, fmt::format("{}CreateStringArrayAction:: Parent object '{}' does not exist", k_Prefix, parentPath.toString()));
    }

    dataObjectId = parentObjectPtr->getId();
  }

  if(m_Dims.empty())
  {
    return MakeErrorResult(-6002, fmt::format("{}CreateStringArrayAction: Tuple Shape was empty. Please set the number of tuples.", k_Prefix));
  }

  std::string name = path().getTargetName();

  const std::string shapeContext = fmt::format("String array '{}' tuple", path().toString());
  auto tupleCount = ComputeDimensionProduct(m_Dims, shapeContext, dimension_conversion::k_StringTupleOverflow);
  if(tupleCount.invalid())
  {
    return ConvertResult(std::move(tupleCount));
  }
  const usize totalTuples = tupleCount.value();
  const usize capacity = std::vector<std::string>().max_size();
  if(totalTuples > capacity)
  {
    return MakeErrorResult(dimension_conversion::k_StringCapacity, fmt::format("String array '{}' tuple dimensions [{}] require {} strings, above storage capacity {}. Use a smaller shape.",
                                                                               path().toString(), string_utilities::FormatDimensions(m_Dims), totalTuples, capacity));
  }

  std::vector<std::string> values(totalTuples, m_InitializeValue);
  StringArray* arrayPtr = StringArray::CreateWithValues(dataStructure, name, m_Dims, values, dataObjectId);
  if(arrayPtr == nullptr)
  {
    if(parentObjectPtr != nullptr && parentObjectPtr->getDataObjectType() == DataObject::Type::AttributeMatrix)
    {
      auto* attrMatrixPtr = dynamic_cast<AttributeMatrix*>(parentObjectPtr);
      std::string amShape = fmt::format("Attribute Matrix Tuple Dims: {}", fmt::join(attrMatrixPtr->getShape(), " x "));
      std::string arrayShape = fmt::format("String Array Tuple Shape: {}", fmt::join(m_Dims, " x "));
      return MakeErrorResult(-6003, fmt::format("{}Unable to create String Array '{}' inside Attribute matrix '{}'. Mismatch of tuple dimensions. The created String Array must have the same tuple "
                                                "dimensions or the same total number of tuples.\n{}\n{}",
                                                k_Prefix, name, parentPath.toString(), amShape, arrayShape));
    }

    return MakeErrorResult(-6004, fmt::format("{}CreateStringArrayAction: Unable to create StringArray at '{}'", k_Prefix, path().toString()));
  }
  return {};
}

IDataAction::UniquePointerType CreateStringArrayAction::clone() const
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
} // namespace nx::core
