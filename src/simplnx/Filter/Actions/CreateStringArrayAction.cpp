#include "CreateStringArrayAction.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/BaseGroup.hpp"
#include "simplnx/DataStructure/EmptyDataStore.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <fmt/core.h>

#include <numeric>

using namespace nx::core;

namespace nx::core
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

  const std::string shapeContext = fmt::format("String array '{}' tuple", path().toString());
  auto tupleCount = ComputeDimensionProduct(m_Dims, shapeContext, DimensionConversion::k_StringTupleOverflow);
  if(tupleCount.invalid())
  {
    return ConvertResult(std::move(tupleCount));
  }
  const usize totalTuples = tupleCount.value();
  const usize capacity = std::vector<std::string>().max_size();
  if(totalTuples > capacity)
  {
    return MakeErrorResult(DimensionConversion::k_StringCapacity, fmt::format("String array '{}' tuple dimensions [{}] require {} strings, above storage capacity {}. Use a smaller shape.",
                                                                              path().toString(), StringUtilities::formatDimensions(m_Dims), totalTuples, capacity));
  }

  // Distinguish failure modes BEFORE calling CreateWithValues so the error message
  // tells the caller exactly what's wrong instead of always blaming dim mismatch.
  // CreateWithValues -> AttemptToAddObject -> parent->canInsert() returns false for
  // multiple distinct reasons (name collision, dim mismatch, cycle); the previous
  // implementation's catch-all "Mismatch of tuple dimensions" message reported
  // identical dims when the actual problem was a duplicate name.
  if(auto* parentGroup = dynamic_cast<BaseGroup*>(parentObject); parentGroup != nullptr)
  {
    if(parentGroup->contains(name))
    {
      return MakeErrorResult(-6005, fmt::format("{}A DataObject named '{}' already exists inside '{}'. Either remove the existing object first or use a different name. "
                                                "(If you are running a multi-filter pipeline where an upstream filter already created this array, the downstream filter "
                                                "should reuse it via a selection parameter rather than re-creating it.)",
                                                prefix, name, parentPath.toString()));
    }
  }

  if(auto* attrMatrix = dynamic_cast<AttributeMatrix*>(parentObject); attrMatrix != nullptr)
  {
    const auto& amShape = attrMatrix->getShape();
    const usize amTotalTuples = std::accumulate(amShape.cbegin(), amShape.cend(), static_cast<usize>(1), std::multiplies<>());
    if(amTotalTuples != totalTuples)
    {
      return MakeErrorResult(-6003, fmt::format("{}Unable to create String Array '{}' inside Attribute matrix '{}'. Mismatch of tuple dimensions. The created String Array must have the same tuple "
                                                "dimensions or the same total number of tuples.\nAttribute Matrix Tuple Dims: {}\nString Array Tuple Shape: {}",
                                                prefix, name, parentPath.toString(), fmt::join(amShape, " x "), fmt::join(m_Dims, " x ")));
    }
  }

  std::vector<std::string> values(totalTuples, m_InitializeValue);
  StringArray* array = StringArray::CreateWithValues(dataStructure, name, m_Dims, values, dataObjectId);
  if(array == nullptr)
  {
    // The pre-checks above should have caught the common failure modes; if we
    // get here it's an unexpected condition (e.g. cycle detection or a parent
    // group type that does not accept arrays).
    return MakeErrorResult(-6004,
                           fmt::format("{}Unable to create StringArray at '{}'. The parent '{}' rejected the new object for an unexpected reason.", prefix, path().toString(), parentPath.toString()));
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
} // namespace nx::core
