#include "Output.hpp"

#include <fmt/core.h>

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/EmptyDataStore.hpp"

using namespace complex;

namespace
{
template <class T>
IDataStore<T>* CreateDataStore(usize tupleSize, usize tupleCount, IDataAction::Mode mode)
{
  switch(mode)
  {
  case IDataAction::Mode::Preflight: {
    return new EmptyDataStore<T>(tupleSize, tupleCount);
  }
  case IDataAction::Mode::Execute: {
    return new DataStore<T>(tupleSize, tupleCount);
  }
  default: {
    throw std::runtime_error("Invalid mode");
  }
  }
}

template <class T>
Result<> CreateArray(DataStructure& dataStructure, const std::vector<usize>& dims, uint64 nComp, const DataPath& path, IDataAction::Mode mode)
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

  uint64 nTuples = std::accumulate(dims.cbegin(), dims.cend(), static_cast<uint64>(0));

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
CreateArrayAction::CreateArrayAction(NumericType type, const std::vector<usize>& dims, uint64 nComp, const DataPath& path)
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
  case NumericType::int8: {
    return CreateArray<int8>(dataStructure, m_Dims, m_NComp, m_Path, mode);
  }
  case NumericType::uint8: {
    return CreateArray<uint8>(dataStructure, m_Dims, m_NComp, m_Path, mode);
  }
  case NumericType::int16: {
    return CreateArray<int16>(dataStructure, m_Dims, m_NComp, m_Path, mode);
  }
  case NumericType::uint16: {
    return CreateArray<uint16>(dataStructure, m_Dims, m_NComp, m_Path, mode);
  }
  case NumericType::int32: {
    return CreateArray<int32>(dataStructure, m_Dims, m_NComp, m_Path, mode);
  }
  case NumericType::uint32: {
    return CreateArray<uint32>(dataStructure, m_Dims, m_NComp, m_Path, mode);
  }
  case NumericType::int64: {
    return CreateArray<int64>(dataStructure, m_Dims, m_NComp, m_Path, mode);
  }
  case NumericType::uint64: {
    return CreateArray<uint64>(dataStructure, m_Dims, m_NComp, m_Path, mode);
  }
  case NumericType::float32: {
    return CreateArray<float32>(dataStructure, m_Dims, m_NComp, m_Path, mode);
  }
  case NumericType::float64: {
    return CreateArray<float64>(dataStructure, m_Dims, m_NComp, m_Path, mode);
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

uint64 CreateArrayAction::numComponents() const
{
  return m_NComp;
}

DataPath CreateArrayAction::path() const
{
  return m_Path;
}
} // namespace complex
