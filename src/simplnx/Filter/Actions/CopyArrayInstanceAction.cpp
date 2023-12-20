#include "CopyArrayInstanceAction.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/IDataStore.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/TemplateHelpers.hpp"

#include <fmt/core.h>

namespace
{
inline constexpr int32_t k_UnsupportedTypeError = -5001;

}
using namespace nx::core;

namespace nx::core
{
CopyArrayInstanceAction::CopyArrayInstanceAction(const DataPath& selectedDataPath, const DataPath& createdDataPath)
: IDataCreationAction(createdDataPath)
, m_SelectedDataPath(selectedDataPath)
{
}

CopyArrayInstanceAction::~CopyArrayInstanceAction() noexcept = default;

#define CAI_BODY(type)                                                                                                                                                                                 \
  DataArray<type>* castInputArray = dynamic_cast<DataArray<type>*>(inputDataArray);                                                                                                                    \
  IDataStore::ShapeType tupleShape = castInputArray->getDataStore()->getTupleShape();                                                                                                                  \
  IDataStore::ShapeType componentShape = castInputArray->getDataStore()->getComponentShape();                                                                                                          \
  return CreateArray<type>(dataStructure, tupleShape, componentShape, getCreatedPath(), mode);

Result<> CopyArrayInstanceAction::apply(DataStructure& dataStructure, Mode mode) const
{
  auto* inputDataArray = dataStructure.getDataAs<IDataArray>(m_SelectedDataPath);

  if(TemplateHelpers::CanDynamicCast<Float32Array>()(inputDataArray))
  {
    CAI_BODY(float)
  }
  if(TemplateHelpers::CanDynamicCast<Float64Array>()(inputDataArray))
  {
    CAI_BODY(double)
  }
  if(TemplateHelpers::CanDynamicCast<Int8Array>()(inputDataArray))
  {
    CAI_BODY(int8_t)
  }
  if(TemplateHelpers::CanDynamicCast<UInt8Array>()(inputDataArray))
  {
    CAI_BODY(uint8_t)
  }
  if(TemplateHelpers::CanDynamicCast<Int16Array>()(inputDataArray))
  {
    CAI_BODY(int16_t)
  }
  if(TemplateHelpers::CanDynamicCast<UInt16Array>()(inputDataArray))
  {
    CAI_BODY(uint16_t)
  }
  if(TemplateHelpers::CanDynamicCast<Int32Array>()(inputDataArray))
  {
    CAI_BODY(int32_t)
  }
  if(TemplateHelpers::CanDynamicCast<UInt32Array>()(inputDataArray))
  {
    CAI_BODY(uint32_t)
  }
  if(TemplateHelpers::CanDynamicCast<Int64Array>()(inputDataArray))
  {
    CAI_BODY(int64_t)
  }
  if(TemplateHelpers::CanDynamicCast<UInt64Array>()(inputDataArray))
  {
    CAI_BODY(uint64_t)
  }
  if(TemplateHelpers::CanDynamicCast<BoolArray>()(inputDataArray))
  {
    CAI_BODY(bool)
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
