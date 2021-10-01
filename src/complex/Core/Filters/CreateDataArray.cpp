#include "CreateDataArray.hpp"

#include "complex/Core/Parameters/ArrayCreationParameter.hpp"
#include "complex/Core/Parameters/ChoicesParameter.hpp"
#include "complex/Core/Parameters/NumberParameter.hpp"
#include "complex/Core/Parameters/NumericTypeParameter.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"

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
  return FilterTraits<CreateDataArray>::name.str();
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
  params.insert(std::make_unique<NumericTypeParameter>(k_NumericType_Key.str(), "Numeric Type", "Numeric Type of data to create", NumericType::int32));
  params.insert(std::make_unique<UInt64Parameter>(k_NumComps_Key.str(), "Number of Components", "Number of components", 1));
  params.insert(std::make_unique<UInt64Parameter>(k_NumTuples_Key.str(), "Number of Tuples", "Number of tuples", 0));
  params.insert(std::make_unique<ArrayCreationParameter>(k_DataPath_Key.str(), "Created Array", "Array storing the file data", DataPath{}));
  return params;
}

IFilter::UniquePointer CreateDataArray::clone() const
{
  return std::make_unique<CreateDataArray>();
}

Result<OutputActions> CreateDataArray::preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler) const
{
  auto numericType = args.value<NumericType>(k_NumericType_Key.view());
  auto components = args.value<uint64>(k_NumComps_Key.view());
  auto numTuples = args.value<uint64>(k_NumTuples_Key.view());
  auto dataArrayPath = args.value<DataPath>(k_DataPath_Key.view());

  auto action = std::make_unique<CreateArrayAction>(numericType, std::vector<usize>{numTuples}, components, dataArrayPath);

  OutputActions actions;
  actions.actions.push_back(std::move(action));

  return {std::move(actions)};
}

Result<> CreateDataArray::executeImpl(DataStructure& data, const Arguments& args, const MessageHandler& messageHandler) const
{
  auto numericType = args.value<NumericType>(k_NumericType_Key.view());
  auto components = args.value<uint64>(k_NumComps_Key.view());
  auto numTuples = args.value<uint64>(k_NumTuples_Key.view());
  auto path = args.value<DataPath>(k_DataPath_Key.view());

  switch(numericType)
  {
  case NumericType::int8: {
    auto& dataArray = ArrayFromPath<int8>(data, path);
    auto& v = *(dataArray.getDataStore());
    std::fill(v.begin(), v.end(), 0);
    break;
  }
  case NumericType::uint8: {
    auto& dataArray = ArrayFromPath<uint8>(data, path);
    auto& v = *(dataArray.getDataStore());
    std::fill(v.begin(), v.end(), 0);
    break;
  }
  case NumericType::int16: {
    auto& dataArray = ArrayFromPath<int16>(data, path);
    auto& v = *(dataArray.getDataStore());
    std::fill(v.begin(), v.end(), 0);
    break;
  }
  case NumericType::uint16: {
    auto& dataArray = ArrayFromPath<uint16>(data, path);
    auto& v = *(dataArray.getDataStore());
    std::fill(v.begin(), v.end(), 0);
    break;
  }
  case NumericType::int32: {
    auto& dataArray = ArrayFromPath<int32>(data, path);
    auto& v = *(dataArray.getDataStore());
    std::fill(v.begin(), v.end(), 0);
    break;
  }
  case NumericType::uint32: {
    auto& dataArray = ArrayFromPath<uint32>(data, path);
    auto& v = *(dataArray.getDataStore());
    std::fill(v.begin(), v.end(), 0);
    break;
  }
  case NumericType::int64: {
    auto& dataArray = ArrayFromPath<int64>(data, path);
    auto& v = *(dataArray.getDataStore());
    std::fill(v.begin(), v.end(), 0LL);
    break;
  }
  case NumericType::uint64: {
    auto& dataArray = ArrayFromPath<uint64>(data, path);
    auto& v = *(dataArray.getDataStore());
    std::fill(v.begin(), v.end(), 0ULL);
    break;
  }
  case NumericType::float32: {
    auto& dataArray = ArrayFromPath<float32>(data, path);
    auto& v = *(dataArray.getDataStore());
    std::fill(v.begin(), v.end(), 0.0f);
    break;
  }
  case NumericType::float64: {
    auto& dataArray = ArrayFromPath<float64>(data, path);
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
