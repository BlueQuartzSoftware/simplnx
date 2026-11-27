#include "CopyArrayInstanceAction.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/IDataStore.hpp"
#include "simplnx/Utilities/ArrayCreationUtilities.hpp"
#include "simplnx/Utilities/TemplateHelpers.hpp"

#include <fmt/core.h>

#include <utility>

using namespace nx::core;

namespace
{
constexpr int32_t k_UnsupportedTypeError = -5001;

template <typename T>
Result<> DoCopy(IDataArray* inputDataArray, DataStructure& dataStructure, DataPath&& path, IDataAction::Mode mode)
{
  auto* castInputArrayPtr = dynamic_cast<DataArray<T>*>(inputDataArray);
  ShapeType tupleShape = castInputArrayPtr->getDataStore()->getTupleShape();
  ShapeType componentShape = castInputArrayPtr->getDataStore()->getComponentShape();
  return ArrayCreationUtilities::CreateArray<T>(dataStructure, tupleShape, componentShape, path, mode);
}
} // namespace

namespace nx::core
{
CopyArrayInstanceAction::CopyArrayInstanceAction(DataPath selectedDataPath, const DataPath& createdDataPath)
: IDataCreationAction(createdDataPath)
, m_SelectedDataPath(std::move(selectedDataPath))
{
}

CopyArrayInstanceAction::~CopyArrayInstanceAction() noexcept = default;

Result<> CopyArrayInstanceAction::apply(DataStructure& dataStructure, Mode mode) const
{
  auto* inputDataArrayPtr = dataStructure.getDataAs<IDataArray>(m_SelectedDataPath);

  if(template_helpers::CanDynamicCast<Float32Array>()(inputDataArrayPtr))
  {
    return ::DoCopy<float32>(inputDataArrayPtr, dataStructure, getCreatedPath(), mode);
  }
  if(template_helpers::CanDynamicCast<Float64Array>()(inputDataArrayPtr))
  {
    return ::DoCopy<float64>(inputDataArrayPtr, dataStructure, getCreatedPath(), mode);
  }
  if(template_helpers::CanDynamicCast<Int8Array>()(inputDataArrayPtr))
  {
    return ::DoCopy<int8>(inputDataArrayPtr, dataStructure, getCreatedPath(), mode);
  }
  if(template_helpers::CanDynamicCast<UInt8Array>()(inputDataArrayPtr))
  {
    return ::DoCopy<uint8>(inputDataArrayPtr, dataStructure, getCreatedPath(), mode);
  }
  if(template_helpers::CanDynamicCast<Int16Array>()(inputDataArrayPtr))
  {
    return ::DoCopy<int16>(inputDataArrayPtr, dataStructure, getCreatedPath(), mode);
  }
  if(template_helpers::CanDynamicCast<UInt16Array>()(inputDataArrayPtr))
  {
    return ::DoCopy<uint16>(inputDataArrayPtr, dataStructure, getCreatedPath(), mode);
  }
  if(template_helpers::CanDynamicCast<Int32Array>()(inputDataArrayPtr))
  {
    return ::DoCopy<int32>(inputDataArrayPtr, dataStructure, getCreatedPath(), mode);
  }
  if(template_helpers::CanDynamicCast<UInt32Array>()(inputDataArrayPtr))
  {
    return ::DoCopy<uint32>(inputDataArrayPtr, dataStructure, getCreatedPath(), mode);
  }
  if(template_helpers::CanDynamicCast<Int64Array>()(inputDataArrayPtr))
  {
    return ::DoCopy<int64>(inputDataArrayPtr, dataStructure, getCreatedPath(), mode);
  }
  if(template_helpers::CanDynamicCast<UInt64Array>()(inputDataArrayPtr))
  {
    return ::DoCopy<uint64>(inputDataArrayPtr, dataStructure, getCreatedPath(), mode);
  }
  if(template_helpers::CanDynamicCast<BoolArray>()(inputDataArrayPtr))
  {
    return ::DoCopy<bool>(inputDataArrayPtr, dataStructure, getCreatedPath(), mode);
  }

  static constexpr StringLiteralType k_Prefix = "CopyArrayInstanceAction: ";
  return MakeErrorResult(k_UnsupportedTypeError, fmt::format("{}The input array at DataPath '{}' was of an unsupported type", k_Prefix, m_SelectedDataPath.toString()));
}

IDataAction::UniquePointerType CopyArrayInstanceAction::clone() const
{
  return std::make_unique<CopyArrayInstanceAction>(m_SelectedDataPath, createdDataPath());
}

DataPath CopyArrayInstanceAction::selectedDataPath() const
{
  return m_SelectedDataPath;
}

DataPath CopyArrayInstanceAction::createdDataPath() const
{
  return getCreatedPath();
}

std::vector<DataPath> CopyArrayInstanceAction::getAllCreatedPaths() const
{
  return {getCreatedPath()};
}

} // namespace nx::core
