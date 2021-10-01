#include "Output.hpp"

#include <numeric>

#include <fmt/core.h>

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/EmptyDataStore.hpp"

using namespace complex;

namespace
{
/**
 * @brief Creates a new DataStore object. IT IS THE RESPONSIBILITY OF THE CALLER
 * TO DISPOSE OF THIS POINTER USING 'delete'
 * @tparam T
 * @param numComponents
 * @param numTuples
 * @param mode
 * @return
 */
template <class T>
IDataStore<T>* CreateDataStore(usize numComponents, usize numTuples, IDataAction::Mode mode)
{
  switch(mode)
  {
  case IDataAction::Mode::Preflight: {
    return new EmptyDataStore<T>({numTuples}, {numComponents});
  }
  case IDataAction::Mode::Execute: {
    return new DataStore<T>({numTuples}, {numComponents});
  }
  default: {
    throw std::runtime_error("Invalid mode");
  }
  }
}

template <class T>
Result<> CreateArray(DataStructure& dataStructure, const std::vector<usize>& dims, u64 nComp, const DataPath& path, IDataAction::Mode mode)
{
  auto parentPath = path.getParent();

  std::optional<DataObject::IdType> id;

  if(parentPath.getLength() != 0)
  {
    auto parentObject = dataStructure.getData(parentPath);
    if(parentObject == nullptr)
    {
      return {nonstd::make_unexpected(std::vector<Error>{{-1, fmt::format("Parent object \"{}\" does not exist", parentPath.toString())}})};
    }

    id = parentObject->getId();
  }

  usize last = path.getLength() - 1;

  std::string name = path[last];

  u64 nTuples = std::accumulate(dims.cbegin(), dims.cend(), static_cast<u64>(0));

  auto* store = CreateDataStore<T>(nComp, nTuples, mode);
  auto dataArray = DataArray<T>::Create(dataStructure, name, store, id);
  if(dataArray == nullptr)
  {
    return {nonstd::make_unexpected(std::vector<Error>{{-2, fmt::format("Unable to create DataArray at \"{}\"", path.toString())}})};
  }

  return {};
}
} // namespace

namespace complex
{
CreateArrayAction::CreateArrayAction(NumericType type, const std::vector<usize>& dims, u64 nComp, const DataPath& path)
: m_Type(type)
, m_Dims(dims)
, m_NComp(nComp)
, m_Path(path)
{
}

CreateArrayAction::~CreateArrayAction() noexcept = default;

Result<> CreateArrayAction::apply(DataStructure& dataStructure, Mode mode) const
{
  switch(m_Type)
  {
  case NumericType::i8: {
    return CreateArray<i8>(dataStructure, m_Dims, m_NComp, m_Path, mode);
  }
  case NumericType::u8: {
    return CreateArray<u8>(dataStructure, m_Dims, m_NComp, m_Path, mode);
  }
  case NumericType::i16: {
    return CreateArray<i16>(dataStructure, m_Dims, m_NComp, m_Path, mode);
  }
  case NumericType::u16: {
    return CreateArray<u16>(dataStructure, m_Dims, m_NComp, m_Path, mode);
  }
  case NumericType::i32: {
    return CreateArray<i32>(dataStructure, m_Dims, m_NComp, m_Path, mode);
  }
  case NumericType::u32: {
    return CreateArray<u32>(dataStructure, m_Dims, m_NComp, m_Path, mode);
  }
  case NumericType::i64: {
    return CreateArray<i64>(dataStructure, m_Dims, m_NComp, m_Path, mode);
  }
  case NumericType::u64: {
    return CreateArray<u64>(dataStructure, m_Dims, m_NComp, m_Path, mode);
  }
  case NumericType::f32: {
    return CreateArray<f32>(dataStructure, m_Dims, m_NComp, m_Path, mode);
  }
  case NumericType::f64: {
    return CreateArray<f64>(dataStructure, m_Dims, m_NComp, m_Path, mode);
  }
  default:
    throw std::runtime_error("Invalid type");
  }
}

NumericType CreateArrayAction::type() const
{
  return m_Type;
}

std::vector<usize> CreateArrayAction::dims() const
{
  return m_Dims;
}

u64 CreateArrayAction::numComponents() const
{
  return m_NComp;
}

DataPath CreateArrayAction::path() const
{
  return m_Path;
}
} // namespace complex
