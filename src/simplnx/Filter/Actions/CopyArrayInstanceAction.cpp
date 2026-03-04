#include "CopyArrayInstanceAction.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/IDataStore.hpp"
#include "simplnx/Utilities/ArrayCreationUtilities.hpp"
#include "simplnx/Utilities/TemplateHelpers.hpp"

#include <fmt/core.h>

using namespace nx::core;

namespace
{
constexpr int32_t k_UnsupportedTypeError = -5001;

template <typename T>
Result<> DoCopy(AbstractDataArray* inputDataArray, DataStructure& dataStructure, DataPath&& path, IDataAction::Mode mode)
{
  auto* castInputArray = dynamic_cast<DataArray<T>*>(inputDataArray);
  ShapeType tupleShape = castInputArray->getDataStore()->getTupleShape();
  ShapeType componentShape = castInputArray->getDataStore()->getComponentShape();
  return ArrayCreationUtilities::CreateArray<T>(dataStructure, tupleShape, componentShape, path, mode);
}
} // namespace

namespace nx::core
{
CopyArrayInstanceAction::CopyArrayInstanceAction(const DataPath& selectedDataPath, const DataPath& createdDataPath)
: AbstractDataCreationAction(createdDataPath)
, m_SelectedDataPath(selectedDataPath)
{
}

CopyArrayInstanceAction::~CopyArrayInstanceAction() noexcept = default;

Result<> CopyArrayInstanceAction::apply(DataStructure& dataStructure, Mode mode) const
{
  auto* inputDataArray = dataStructure.getDataAs<AbstractDataArray>(m_SelectedDataPath);

  if(TemplateHelpers::CanDynamicCast<Float32Array>()(inputDataArray))
  {
    return ::DoCopy<float32>(inputDataArray, dataStructure, getCreatedPath(), mode);
  }
  if(TemplateHelpers::CanDynamicCast<Float64Array>()(inputDataArray))
  {
    return ::DoCopy<float64>(inputDataArray, dataStructure, getCreatedPath(), mode);
  }
  if(TemplateHelpers::CanDynamicCast<Int8Array>()(inputDataArray))
  {
    return ::DoCopy<int8>(inputDataArray, dataStructure, getCreatedPath(), mode);
  }
  if(TemplateHelpers::CanDynamicCast<UInt8Array>()(inputDataArray))
  {
    return ::DoCopy<uint8>(inputDataArray, dataStructure, getCreatedPath(), mode);
  }
  if(TemplateHelpers::CanDynamicCast<Int16Array>()(inputDataArray))
  {
    return ::DoCopy<int16>(inputDataArray, dataStructure, getCreatedPath(), mode);
  }
  if(TemplateHelpers::CanDynamicCast<UInt16Array>()(inputDataArray))
  {
    return ::DoCopy<uint16>(inputDataArray, dataStructure, getCreatedPath(), mode);
  }
  if(TemplateHelpers::CanDynamicCast<Int32Array>()(inputDataArray))
  {
    return ::DoCopy<int32>(inputDataArray, dataStructure, getCreatedPath(), mode);
  }
  if(TemplateHelpers::CanDynamicCast<UInt32Array>()(inputDataArray))
  {
    return ::DoCopy<uint32>(inputDataArray, dataStructure, getCreatedPath(), mode);
  }
  if(TemplateHelpers::CanDynamicCast<Int64Array>()(inputDataArray))
  {
    return ::DoCopy<int64>(inputDataArray, dataStructure, getCreatedPath(), mode);
  }
  if(TemplateHelpers::CanDynamicCast<UInt64Array>()(inputDataArray))
  {
    return ::DoCopy<uint64>(inputDataArray, dataStructure, getCreatedPath(), mode);
  }
  if(TemplateHelpers::CanDynamicCast<BoolArray>()(inputDataArray))
  {
    return ::DoCopy<bool>(inputDataArray, dataStructure, getCreatedPath(), mode);
  }

  static constexpr StringLiteral prefix = "CopyArrayInstanceAction: ";
  return MakeErrorResult(k_UnsupportedTypeError, fmt::format("{}The input array at DataPath '{}' was of an unsupported type", prefix, m_SelectedDataPath.toString()));
}

IDataAction::UniquePointer CopyArrayInstanceAction::clone() const
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
