#include "CreateDataArray.hpp"

#include "complex/Core/Parameters/ArrayCreationParameter.hpp"
#include "complex/Core/Parameters/ChoicesParameter.hpp"
#include "complex/Core/Parameters/NumberParameter.hpp"
#include "complex/Core/Parameters/NumericTypeParameter.hpp"
#include "complex/DataStructure/DataArray.hpp"

namespace complex
{

namespace
{
template <class T>
DataArray<T>& ArrayFromPath(DataStructure& data, const DataPath& path)
{
  DataObject* object = data.getData(path);
  DataArray<T>* dataArray = dynamic_cast<DataArray<T>*>(object);
  if(dataArray == nullptr)
  {
    throw std::runtime_error("Can't obtain DataArray");
  }
  return *dataArray;
}
} // namespace

std::string CreateDataArray::name() const
{
  return FilterTraits<CreateDataArray>::name;
}

Uuid CreateDataArray::uuid() const
{
  return FilterTraits<CreateDataArray>::uuid;
}

std::string CreateDataArray::humanName() const
{
  return "CreateDataArray";
}

Parameters CreateDataArray::parameters() const
{
  Parameters params;
  params.insert(std::make_unique<NumericTypeParameter>(k_NumericType_Key, "Numeric Type", "Numeric Type of data to create", NumericType::i32));
  params.insert(std::make_unique<UInt64Parameter>(k_NumComps_Key, "Number of Components", "Number of components", 1));
  params.insert(std::make_unique<UInt64Parameter>(k_NumTuples_Key, "Number of Tuples", "Number of tuples", 0));
  params.insert(std::make_unique<ArrayCreationParameter>(k_DataPath_Key, "Created Array", "Array storing the file data", DataPath{}));
  return params;
}

IFilter::UniquePointer CreateDataArray::clone() const
{
  return std::make_unique<CreateDataArray>();
}

Result<OutputActions> CreateDataArray::preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler) const
{
  auto numericType = args.value<NumericType>(k_NumericType_Key);
  auto components = args.value<u64>(k_NumComps_Key);
  auto numTuples = args.value<u64>(k_NumTuples_Key);
  auto dataArrayPath = args.value<DataPath>(k_DataPath_Key);

  auto action = std::make_unique<CreateArrayAction>(numericType, std::vector<size_t>{numTuples}, components, dataArrayPath);

  OutputActions actions;
  actions.actions.push_back(std::move(action));

  return {std::move(actions)};
}

Result<> CreateDataArray::executeImpl(DataStructure& data, const Arguments& args, const MessageHandler& messageHandler) const
{
  auto numericType = args.value<NumericType>(k_NumericType_Key);
  auto components = args.value<u64>(k_NumComps_Key);
  auto numTuples = args.value<u64>(k_NumTuples_Key);
  auto path = args.value<DataPath>(k_DataPath_Key);

  switch(numericType)
  {
  case NumericType::i8: {
    auto& dataArray = ArrayFromPath<i8>(data, path);
    auto& v = *(dataArray.getDataStore());
    std::fill(v.begin(), v.end(), 0);
    break;
  }
  case NumericType::u8: {
    auto& dataArray = ArrayFromPath<u8>(data, path);
    auto& v = *(dataArray.getDataStore());
    std::fill(v.begin(), v.end(), 0);
    break;
  }
  case NumericType::i16: {
    auto& dataArray = ArrayFromPath<i16>(data, path);
    auto& v = *(dataArray.getDataStore());
    std::fill(v.begin(), v.end(), 0);
    break;
  }
  case NumericType::u16: {
    auto& dataArray = ArrayFromPath<u16>(data, path);
    auto& v = *(dataArray.getDataStore());
    std::fill(v.begin(), v.end(), 0);
    break;
  }
  case NumericType::i32: {
    auto& dataArray = ArrayFromPath<i32>(data, path);
    auto& v = *(dataArray.getDataStore());
    std::fill(v.begin(), v.end(), 0);
    break;
  }
  case NumericType::u32: {
    auto& dataArray = ArrayFromPath<u32>(data, path);
    auto& v = *(dataArray.getDataStore());
    std::fill(v.begin(), v.end(), 0);
    break;
  }
  case NumericType::i64: {
    auto& dataArray = ArrayFromPath<i64>(data, path);
    auto& v = *(dataArray.getDataStore());
    std::fill(v.begin(), v.end(), 0LL);
    break;
  }
  case NumericType::u64: {
    auto& dataArray = ArrayFromPath<u64>(data, path);
    auto& v = *(dataArray.getDataStore());
    std::fill(v.begin(), v.end(), 0ULL);
    break;
  }
  case NumericType::f32: {
    auto& dataArray = ArrayFromPath<f32>(data, path);
    auto& v = *(dataArray.getDataStore());
    std::fill(v.begin(), v.end(), 0.0f);
    break;
  }
  case NumericType::f64: {
    auto& dataArray = ArrayFromPath<f64>(data, path);
    auto& v = *(dataArray.getDataStore());
    std::fill(v.begin(), v.end(), 0.0);
    break;
  }
  default:
    throw std::runtime_error("Invalid type");
  }

  return {};
}
} // namespace complex
