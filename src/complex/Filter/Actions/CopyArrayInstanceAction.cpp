#include "CopyArrayInstanceAction.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/EmptyDataStore.hpp"
#include "complex/DataStructure/IDataStore.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"
#include "complex/Utilities/TemplateHelpers.hpp"

#include <fmt/core.h>

namespace
{
inline constexpr int32_t k_UnsupportedType_Error = -301;

}
using namespace complex;

namespace complex
{
CopyArrayInstanceAction::CopyArrayInstanceAction(const DataPath& selectedDataPath, const DataPath& createdDataPath)
: m_SelectedDataPath(selectedDataPath)
, m_CreatedDataPath(createdDataPath)
{
}

CopyArrayInstanceAction::~CopyArrayInstanceAction() noexcept = default;

#define CAI_BODY(type)                                                                                                                                                                                 \
  DataArray<type>* castInputArray = dynamic_cast<DataArray<type>*>(inputDataArray);                                                                                                                    \
  IDataStore::ShapeType tupleShape = castInputArray->getDataStore()->getTupleShape();                                                                                                            \
  IDataStore::ShapeType componentShape = castInputArray->getDataStore()->getComponentShape();                                                                                                    \
  return CreateArray<type>(dataStructure, tupleShape, componentShape, m_CreatedDataPath, mode);

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
  if(TemplateHelpers::CanDynamicCast<DataArray<size_t>>()(inputDataArray))
  {
    CAI_BODY(size_t)
  }

  return MakeErrorResult(k_UnsupportedType_Error, fmt::format("The input array at DataPath '{}' was of an unsupported type", m_SelectedDataPath.toString()));
}

DataPath CopyArrayInstanceAction::selectedDataPath() const
{
  return m_SelectedDataPath;
}

DataPath CopyArrayInstanceAction::createdDataPath() const
{
  return m_CreatedDataPath;
}

} // namespace complex
