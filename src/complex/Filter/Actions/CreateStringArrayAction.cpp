#include "CreateStringArrayAction.hpp"

#include <fmt/core.h>

#include "complex/Common/TypeTraits.hpp"
#include "complex/DataStructure/EmptyDataStore.hpp"
#include "complex/DataStructure/StringArray.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"

using namespace complex;

namespace complex
{
CreateStringArrayAction::CreateStringArrayAction(const std::vector<usize>& tDims, const DataPath& path)
: IDataCreationAction(path)
, m_Dims(tDims)
{
}

CreateStringArrayAction::~CreateStringArrayAction() noexcept = default;

Result<> CreateStringArrayAction::apply(DataStructure& dataStructure, Mode mode) const
{
  auto parentPath = path().getParent();

  std::optional<DataObject::IdType> dataObjectId;

  DataObject* parentObject = nullptr;
  if(parentPath.getLength() != 0)
  {
    parentObject = dataStructure.getData(parentPath);
    if(parentObject == nullptr)
    {
      return MakeErrorResult(-380, fmt::format("CreateStringArrayAction:: Parent object '{}' does not exist", parentPath.toString()));
    }

    dataObjectId = parentObject->getId();
  }

  if(m_Dims.empty())
  {
    return MakeErrorResult(-381, fmt::format("CreateStringArrayAction: Tuple Shape was empty. Please set the number of tuples."));
  }

  std::string name = path().getTargetName();

  usize totalTuples = std::accumulate(m_Dims.cbegin(), m_Dims.cend(), static_cast<usize>(1), std::multiplies<>());

  std::vector<std::string> values(totalTuples);
  StringArray* array = StringArray::CreateWithValues(dataStructure, name, values, dataObjectId);
  if(array == nullptr)
  {
    return MakeErrorResult(-382, fmt::format("CreateStringArrayAction: Unable to create StringArray at '{}'", path().toString()));
  }
  return {};
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
