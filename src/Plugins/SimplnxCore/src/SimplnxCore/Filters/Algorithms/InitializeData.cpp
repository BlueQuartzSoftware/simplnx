

#include "InitializeData.hpp"

#include "simplnx/Common/TypeTraits.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

#include <chrono>
#include <limits>

using namespace nx::core;

namespace
{

// At the current time this code could be simplified with a bool in the incremental template, HOWEVER,
// it was done this way to allow for expansion of operations down the line multiplication, division, etc.
template <bool UseAddition, bool UseSubtraction>
struct IncrementalOptions
{
  static constexpr bool UsingAddition = UseAddition;
  static constexpr bool UsingSubtraction = UseSubtraction;
};

using AdditionT = IncrementalOptions<true, false>;
using SubtractionT = IncrementalOptions<false, true>;

template <typename T>
void ValueFill(AbstractDataStore<T>& dataStore, const std::vector<std::string>& stringValues)
{
  usize numComp = dataStore.getNumberOfComponents(); // We checked that the values string is greater than max comps size so proceed check free

  if(numComp > 1)
  {
    std::vector<T> values;

    for(const auto& str : stringValues)
    {
      values.emplace_back(ConvertTo<T>::convert(str).value());
    }

    usize numTup = dataStore.getNumberOfTuples();

    for(usize tup = 0; tup < numTup; tup++)
    {
      for(usize comp = 0; comp < numComp; comp++)
      {
        dataStore[tup * numComp + comp] = values[comp];
      }
    }
  }
  else
  {
    Result<T> result = ConvertTo<T>::convert(stringValues[0]);
    T value = result.value();
    dataStore.fill(value);
  }
}

template <typename T, class IncrementalOptions = AdditionT>
void IncrementalFill(AbstractDataStore<T>& dataStore, const std::vector<std::string>& startValues, const std::vector<std::string>& stepValues)
{
  usize numComp = dataStore.getNumberOfComponents(); // We checked that the values string is greater than max comps size so proceed check free

  std::vector<T> values(numComp);
  std::vector<T> steps(numComp);

  for(usize comp = 0; comp < numComp; comp++)
  {
    Result<T> result = ConvertTo<T>::convert(startValues[comp]);
    values[comp] = result.value();
    if constexpr(!std::is_same_v<T, bool>)
    {
      result = ConvertTo<T>::convert(stepValues[comp]);
      steps[comp] = result.value();
    }
  }

  usize numTup = dataStore.getNumberOfTuples();

  if constexpr(std::is_same_v<T, bool>)
  {
    for(usize comp = 0; comp < numComp; comp++)
    {
      dataStore[comp] = values[comp];

      if constexpr(IncrementalOptions::UsingAddition)
      {
        values[comp] = ConvertTo<uint8>::convert(stepValues[comp]).value() != 0 ? true : values[comp];
      }
      if constexpr(IncrementalOptions::UsingSubtraction)
      {
        values[comp] = ConvertTo<uint8>::convert(stepValues[comp]).value() != 0 ? false : values[comp];
      }
    }

    for(usize tup = 1; tup < numTup; tup++)
    {
      for(usize comp = 0; comp < numComp; comp++)
      {
        dataStore[tup * numComp + comp] = values[comp];
      }
    }
  }

  if constexpr(!std::is_same_v<T, bool>)
  {
    for(usize tup = 0; tup < numTup; tup++)
    {
      for(usize comp = 0; comp < numComp; comp++)
      {
        dataStore[tup * numComp + comp] = values[comp];

        if constexpr(IncrementalOptions::UsingAddition)
        {
          values[comp] += steps[comp];
        }
        if constexpr(IncrementalOptions::UsingSubtraction)
        {
          values[comp] -= steps[comp];
        }
      }
    }
  }
}

template <typename T, bool Ranged, class DistributionT>
void RandomFill(std::vector<DistributionT>& dist, AbstractDataStore<T>& dataStore, const uint64 seed, const bool standardizeSeed)
{
  usize numComp = dataStore.getNumberOfComponents(); // We checked that the values string is greater than max comps size so proceed check free

  std::vector<std::mt19937_64> generators(numComp, std::mt19937_64{});

  for(usize comp = 0; comp < numComp; comp++)
  {
    generators[comp].seed((standardizeSeed ? seed : seed + comp)); // If standardizing seed all generators use the same else, use modified seeds
  }

  usize numTup = dataStore.getNumberOfTuples();

  for(usize tup = 0; tup < numTup; tup++)
  {
    for(usize comp = 0; comp < numComp; comp++)
    {
      if constexpr(std::is_floating_point_v<T>)
      {
        if constexpr(Ranged)
        {
          dataStore[tup * numComp + comp] = static_cast<T>(dist[comp](generators[comp]));
        }
        if constexpr(!Ranged)
        {
          if constexpr(std::is_signed_v<T>)
          {
            dataStore[tup * numComp + comp] = static_cast<T>(dist[comp](generators[comp]) * (std::numeric_limits<T>::max() - 1) * (((rand() & 1) == 0) ? 1 : -1));
          }
          if constexpr(!std::is_signed_v<T>)
          {
            dataStore[tup * numComp + comp] = static_cast<T>(dist[comp](generators[comp]) * std::numeric_limits<T>::max());
          }
        }
      }
      if constexpr(!std::is_floating_point_v<T>)
      {
        dataStore[tup * numComp + comp] = static_cast<T>(dist[comp](generators[comp]));
      }
    }
  }
}

template <typename T, class... ArgsT>
void FillIncForwarder(const StepType& stepType, ArgsT&&... args)
{
  switch(stepType)
  {
  case StepType::Addition: {
    ::IncrementalFill<T, AdditionT>(std::forward<ArgsT>(args)...);
    return;
  }
  case StepType::Subtraction: {
    ::IncrementalFill<T, SubtractionT>(std::forward<ArgsT>(args)...);
    return;
  }
  }
}

template <typename T, bool Ranged, class... ArgsT>
void FillRandomForwarder(const std::vector<T>& range, usize numComp, ArgsT&&... args)
{
  if constexpr(std::is_same_v<T, bool>)
  {
    std::vector<std::uniform_int_distribution<int64>> dists;
    for(usize comp = 0; comp < numComp * 2; comp += 2)
    {
      dists.emplace_back((range.at(comp) ? 1 : 0), (range.at(comp + 1) ? 1 : 0));
    }
    ::RandomFill<T, Ranged, std::uniform_int_distribution<int64>>(dists, std::forward<ArgsT>(args)...);
    return;
  }
  if constexpr(!std::is_floating_point_v<T>)
  {
    std::vector<std::uniform_int_distribution<int64>> dists;
    for(usize comp = 0; comp < numComp * 2; comp += 2)
    {
      dists.emplace_back(range.at(comp), range.at(comp + 1));
    }
    ::RandomFill<T, Ranged, std::uniform_int_distribution<int64>>(dists, std::forward<ArgsT>(args)...);
  }
  if constexpr(std::is_floating_point_v<T>)
  {
    if constexpr(Ranged)
    {

      std::vector<std::uniform_real_distribution<float64>> dists;
      for(usize comp = 0; comp < numComp * 2; comp += 2)
      {
        dists.emplace_back(range.at(comp), range.at(comp + 1));
      }
      ::RandomFill<T, Ranged, std::uniform_real_distribution<float64>>(dists, std::forward<ArgsT>(args)...);
    }
    if constexpr(!Ranged)
    {
      std::vector<std::uniform_real_distribution<float64>> dists;
      for(usize comp = 0; comp < numComp * 2; comp += 2)
      {
        dists.emplace_back(0, 1);
      }
      ::RandomFill<T, Ranged, std::uniform_real_distribution<float64>>(dists, std::forward<ArgsT>(args)...);
    }
  }
}

std::vector<std::string> standardizeMultiComponent(const usize numComps, const std::vector<std::string>& componentValues)
{
  if(componentValues.size() == numComps)
  {
    return {componentValues};
  }
  else
  {
    std::vector<std::string> standardized(numComps);
    for(usize comp = 0; comp < numComps; comp++)
    {
      standardized[comp] = componentValues[0];
    }
    return standardized;
  }
}

struct FillArrayFunctor
{
  template <typename T>
  void operator()(IDataArray& iDataArray, const InitializeDataInputValues& inputValues)
  {
    auto& dataStore = iDataArray.template getIDataStoreRefAs<AbstractDataStore<T>>();
    usize numComp = dataStore.getNumberOfComponents(); // We checked that the values string is greater than max comps size so proceed check free

    switch(inputValues.initType)
    {
    case InitializeType::FillValue: {
      return ::ValueFill<T>(dataStore, standardizeMultiComponent(numComp, inputValues.stringValues));
    }
    case InitializeType::Incremental: {
      return ::FillIncForwarder<T>(inputValues.stepType, dataStore, standardizeMultiComponent(numComp, inputValues.startValues), standardizeMultiComponent(numComp, inputValues.stepValues));
    }
    case InitializeType::Random: {
      std::vector<T> range;
      if constexpr(!std::is_same_v<T, bool>)
      {
        for(usize comp = 0; comp < numComp; comp++)
        {
          range.push_back(std::numeric_limits<T>::min());
          range.push_back(std::numeric_limits<T>::max());
        }
      }
      if constexpr(std::is_same_v<T, bool>)
      {
        for(usize comp = 0; comp < numComp; comp++)
        {
          range.push_back(false);
          range.push_back(true);
        }
      }
      return ::FillRandomForwarder<T, false>(range, numComp, dataStore, inputValues.seed, inputValues.standardizeSeed);
    }
    case InitializeType::RangedRandom: {
      auto randBegin = standardizeMultiComponent(numComp, inputValues.randBegin);
      auto randEnd = standardizeMultiComponent(numComp, inputValues.randEnd);

      std::vector<T> range;
      for(usize comp = 0; comp < numComp; comp++)
      {
        Result<T> result = ConvertTo<T>::convert(randBegin[comp]);
        range.push_back(result.value());
        result = ConvertTo<T>::convert(randEnd[comp]);
        range.push_back(result.value());
      }
      return ::FillRandomForwarder<T, true>(range, numComp, dataStore, inputValues.seed, inputValues.standardizeSeed);
    }
    }
  }
};

} // namespace

// -----------------------------------------------------------------------------
InitializeData::InitializeData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, InitializeDataInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
InitializeData::~InitializeData() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& InitializeData::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> InitializeData::operator()()
{

  auto& iDataArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->InputArrayPath);

  ExecuteDataFunction(::FillArrayFunctor{}, iDataArray.getDataType(), iDataArray, *m_InputValues);

  return {};
}
