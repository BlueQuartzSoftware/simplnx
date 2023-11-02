#include "InitializeData.hpp"

#include "complex/Common/TypeTraits.hpp"
#include "complex/DataStructure/AbstractDataStore.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"
#include "complex/Utilities/FilterUtilities.hpp"

#include <fmt/core.h>

#include <array>
#include <chrono>
#include <limits>
#include <random>
#include <thread>

using namespace complex;

namespace
{
using RangeType = std::pair<float64, float64>;

enum InitializeType : uint64
{
  Default,
  FillValue,
  Incremental,
  Random
}

enum StepType : uint64
{
  Addition,
  Subtraction
}

InitializeData::InitType ConvertIndexToInitType(uint64 index)
{
  switch(index)
  {
  case to_underlying(InitializeData::InitType::Manual): {
    return InitializeData::InitType::Manual;
  }
  case to_underlying(InitializeData::InitType::Random): {
    return InitializeData::InitType::Random;
  }
  case to_underlying(InitializeData::InitType::RandomWithRange): {
    return InitializeData::InitType::RandomWithRange;
  }
  default: {
    throw std::runtime_error("InitializeData: Invalid value for InitType");
  }
  }
}

struct CheckInitializationFunctor
{
  template <class T>
  std::optional<Error> operator()(const IDataArray& dataArray, InitializeData::InitType initType, float64 initValue, const std::pair<float64, float64>& initRange)
  {
    std::string arrayName = dataArray.getName();

    if(initType == InitializeData::InitType::Manual)
    {
      if(initValue < static_cast<double>(std::numeric_limits<T>().lowest()) || initValue > static_cast<double>(std::numeric_limits<T>().max()))
      {
        return Error{-4000, fmt::format("{}: The initialization value could not be converted. The valid range is {} to {}", arrayName, std::numeric_limits<T>::min(), std::numeric_limits<T>::max())};
      }
    }
    else if(initType == InitializeData::InitType::RandomWithRange)
    {
      float64 min = initRange.first;
      float64 max = initRange.second;
      if(min > max)
      {
        return Error{-5550, fmt::format("{}: Invalid initialization range.  Minimum value is larger than maximum value.", arrayName)};
      }
      if(min < static_cast<double>(std::numeric_limits<T>().lowest()) || max > static_cast<double>(std::numeric_limits<T>().max()))
      {
        return Error{-4001, fmt::format("{}: The initialization range can only be from {} to {}", arrayName, std::numeric_limits<T>::min(), std::numeric_limits<T>::max())};
      }
      if(min == max)
      {
        return Error{-4002, fmt::format("{}: The initialization range must have differing values", arrayName)};
      }
    }

    return {};
  }
};

template <class T>
auto CreateRandomGenerator(T rangeMin, T rangeMax, uint64 seed)
{
  std::random_device randomDevice;           // Will be used to obtain a seed for the random number engine
  std::mt19937_64 generator(randomDevice()); // Standard mersenne_twister_engine seeded with rd()
  generator.seed(seed);

  if constexpr(std::is_integral_v<T>)
  {
    std::uniform_int_distribution<> distribution(rangeMin, rangeMax);
    return std::make_pair(distribution, generator);
  }
  else if constexpr(std::is_floating_point_v<T>)
  {
    std::uniform_real_distribution<T> distribution(rangeMin, rangeMax);
    return std::make_pair(distribution, generator);
  }
}

struct InitializeArrayFunctor
{
  template <class T>
  void operator()(IDataArray& dataArray, const std::array<usize, 3>& dims, uint64 xMin, uint64 xMax, uint64 yMin, uint64 yMax, uint64 zMin, uint64 zMax, InitializeData::InitType initType,
                  float64 initValue, const RangeType& initRange, uint64 seed)
  {
    T rangeMin;
    T rangeMax;
    if(initType == InitializeData::InitType::RandomWithRange)
    {
      rangeMin = static_cast<T>(initRange.first);
      rangeMax = static_cast<T>(initRange.second);
    }
    else
    {
      rangeMin = std::numeric_limits<T>().min();
      rangeMax = std::numeric_limits<T>().max();
    }

    auto& dataStore = dataArray.getIDataStoreRefAs<AbstractDataStore<T>>();

    auto&& [distribution, generator] = CreateRandomGenerator(rangeMin, rangeMax, seed);

    for(uint64 k = zMin; k < zMax + 1; k++)
    {
      for(uint64 j = yMin; j < yMax + 1; j++)
      {
        for(uint64 i = xMin; i < xMax + 1; i++)
        {
          usize index = (k * dims[0] * dims[1]) + (j * dims[0]) + i;

          if(initType == InitializeData::InitType::Manual)
          {
            T num = static_cast<T>(initValue);
            dataStore.fillTuple(index, num);
          }
          else
          {
            T randNum = distribution(generator);
            dataStore.fillTuple(index, randNum);
          }
        }
      }
    }
  }
};
} // namespace

namespace complex
{
//------------------------------------------------------------------------------
std::string InitializeData::name() const
{
  return FilterTraits<InitializeData>::name;
}

//------------------------------------------------------------------------------
std::string InitializeData::className() const
{
  return FilterTraits<InitializeData>::className;
}

//------------------------------------------------------------------------------
Uuid InitializeData::uuid() const
{
  return FilterTraits<InitializeData>::uuid;
}

//------------------------------------------------------------------------------
std::string InitializeData::humanName() const
{
  return "Initialize Data";
}

//------------------------------------------------------------------------------
std::vector<std::string> InitializeData::defaultTags() const
{
  return {className(), "Memory Management", "Initialize", "Create", "Generate", "Data"};
}

//------------------------------------------------------------------------------
Parameters InitializeData::parameters() const
{
  Parameters params;

  // TODO: restrict types
  params.insertSeparator(Parameters::Separator{"Required Data Objects"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseMultiCompArrays_Key, "Use Multiple Component Arrays", "When true options for multiple component arrays will be visible", false));
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_SingleCompArrayPaths_Key, "Single Component Arrays", "The data arrays in which to initialize the data", std::vector<DataPath>{},
                                                               MultiArraySelectionParameter::AllowedTypes{IArray::ArrayType::DataArray}, complex::GetAllNumericTypes(), MultiArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_MultiCompArrayPaths_Key, "Multi Component Arrays", "The data arrays in which to initialize the data", std::vector<DataPath>{},
                                                               MultiArraySelectionParameter::AllowedTypes{IArray::ArrayType::DataArray}, complex::GetAllNumericTypes()));

  params.insertSeperator("Data Initialization");
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_InitType_Key, "Initialization Type", "Method for detemining the what values of the data in the array should be initialized to",
                                                    0ULL, ChoicesParameter::Choices{"Fill Value", "Incremental", "Random", "Random With Range"})); // sequence dependent DO NOT REORDER
  params.insert(std::make_unique<Float64Parameter>(k_InitValue_Key, "Initialization Value", "This value will be used to fill the new array", 0.0));
  params.insert(std::make_unique<StringParameter>(k_MultiFillValue_key, "Multi Component Fill Values [Seperated with ;]",
                                                  "Specify values for each component. Ex: A 3-component array would be 6;8;12 and every tuple would have these same component values", "1;1;1"));
  
  params.insert(std::make_unique<Float64Parameter>(k_StartingFillValue_Key, "Starting Value", "The value to start incrementing from", 0.0));
  params.insert(std::make_unique<ChoicesParameter>(k_StepOperation_Key, "Starting Value", "The type of step operation to preform", 0ULL, ChoicesParameter::Choices{"Addition", "Subtraction"}));
  params.insert(std::make_unique<Float64Parameter>(k_StepValue_Key, "Increment/Step Value", "The number to increment/decrement the fill value by", 1.0));

  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseSeed_Key, "Use Seed for Random Generation", "When true the user will be able to put in a seed for random generation", false));
  params.insert(std::make_unique<NumberParameter<uint64>>(k_SeedValue_Key, "Seed Value", "The seed fed into the random generator", std::mt19937::default_seed));
  params.insert(std::make_unique<DataObjectNameParameter>(k_SeedArrayName_Key, "Stored Seed Value Array Name", "Name of array holding the seed value", "InitializeData SeedValue"));
  params.insert(std::make_unique<VectorFloat64Parameter>(k_InitRange_Key, "Initialization Range", "The initialization range if Random With Range Initialization Type is selected",
                                                         VectorFloat64Parameter::ValueType{0.0, 0.0}));
  
  // Associate the Linkable Parameter(s) to the children parameters that they control
  /* Using Multiple Components */
  params.linkParameters(k_UseMultiCompArrays_Key, k_MultiCompArraysPaths_Key, true);
  params.linkParameters(k_UseMultiCompArrays_Key, k_SingleCompArraysPaths_Key, false);
  params.linkParameters(k_UseMultiCompArrays_Key, k_MultiFillValue_Key, true);

  /* Using Fill Value */
  params.linkParameters(k_InitType_key, k_InitValue_Key, 0ULL);

  /* Using Incremental */
  params.linkParameters(k_InitType_key, k_StartingFillValue_Key, 1ULL);
  params.linkParameters(k_InitType_key, k_StepOperation_Key, 1ULL);
  params.linkParameters(k_InitType_key, k_StepValue_Key, 1ULL);

  /* Random */
  params.linkParameters(k_UseSeed_Key, k_SeedValue_Key, true);
  /* Random - Using Random */
  params.linkParameters(k_InitType_key, k_UseSeed_Key, 2ULL);
  /* Random - Using Random With Range */
  params.linkParameters(k_InitType_key, k_UseSeed_Key, 3ULL);
  params.linkParameters(k_InitType_key, k_InitRange_Key, 3ULL);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer InitializeData::clone() const
{
  return std::make_unique<InitializeData>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult InitializeData::preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  auto cellArrayPaths = args.value<MultiArraySelectionParameter::ValueType>(k_CellArrayPaths_Key);
  auto initValue = args.value<float64>(k_InitValue_Key);
  auto initRangeVec = args.value<std::vector<float64>>(k_InitRange_Key);
  auto pSeedArrayNameValue = args.value<std::string>(k_SeedArrayName_Key);
  auto initilizeTypeValue = static_cast<InitializeType>(args.value<uint64>(k_InitType_Key));

  InitType initType = ConvertIndexToInitType(initTypeIndex);
  RangeType initRange = {initRangeVec.at(0), initRangeVec.at(1)};

  if(cellArrayPaths.empty())
  {
    return {MakeErrorResult<OutputActions>(-5550, "At least one data array must be selected.")};
  }

  complex::Result<OutputActions> resultOutputActions;

  if(initializeTypeValue == InitializeType::Random)
  {
    auto createAction = std::make_unique<CreateArrayAction>(DataType::uint64, std::vector<usize>{1}, std::vector<usize>{1}, DataPath({seedArrayNameValue}));
    resultOutputActions.value().appendAction(std::move(createAction));
  }

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> InitializeData::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  auto cellArrayPaths = args.value<bool>(k_UseMultiCompArrays_Key) ? args.value<MultiArraySelectionParameter::ValueType>(k_MultiCompArraysPaths_Key) : args.value<MultiArraySelectionParameter::ValueType>(k_SingleCompArraysPaths_Key);
  auto initTypeIndex = args.value<uint64>(k_InitType_Key);
  auto initValue = args.value<float64>(k_InitValue_Key);
  auto initRangeVec = args.value<std::vector<float64>>(k_InitRange_Key);

  auto seed = args.value<std::mt19937_64::result_type>(k_SeedValue_Key);
  if(!args.value<bool>(k_UseSeed_Key))
  {
    seed = static_cast<std::mt19937_64::result_type>(std::chrono::steady_clock::now().time_since_epoch().count());
  }

  // Store Seed Value in Top Level Array
  data.getDataRefAs<UInt64Array>(DataPath({args.value<std::string>(k_SeedArrayName_Key)}))[0] = seed;

  InitType initType = ConvertIndexToInitType(initTypeIndex);
  RangeType initRange = {initRangeVec.at(0), initRangeVec.at(1)};

  for(const DataPath& path : cellArrayPaths)
  {
    auto& iDataArray = data.getDataRefAs<IDataArray>(path);

    ExecuteNeighborFunction(InitializeArrayFunctor{}, iDataArray.getDataType(), iDataArray, dims, xMin, xMax, yMin, yMax, zMin, zMax, initType, initValue, initRange, seed); // NO BOOL

    // Avoid the exact same seeding for each array
    seed++;
  }

  return {};
}
} // namespace complex
