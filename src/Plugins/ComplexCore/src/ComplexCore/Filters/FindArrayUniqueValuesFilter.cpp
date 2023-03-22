#include "FindArrayUniqueValuesFilter.hpp"

#include "complex/DataStructure/AttributeMatrix.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Utilities/ParallelDataAlgorithm.hpp"

using namespace complex;

namespace complex
{
OutputActions FindArrayUniqueValuesFilter::createCompatibleArrays(const DataStructure& data, const Arguments& args, usize numBins, std::vector<usize> tupleDims) const
{
  auto inputArrayPath = args.value<DataPath>(k_SelectedArrayPath_Key);
  auto* inputArray = data.getDataAs<IDataArray>(inputArrayPath);
  auto destinationAttributeMatrixValue = args.value<DataPath>(k_DestinationAttributeMatrix_Key);
  DataType dataType = inputArray->getDataType();

  OutputActions actions;

  auto amAction = std::make_unique<CreateAttributeMatrixAction>(destinationAttributeMatrixValue, tupleDims);
  actions.actions.push_back(std::move(amAction));

  return std::move(actions);
}

//------------------------------------------------------------------------------
std::string FindArrayUniqueValuesFilter::name() const
{
  return FilterTraits<FindArrayUniqueValuesFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string FindArrayUniqueValuesFilter::className() const
{
  return FilterTraits<FindArrayUniqueValuesFilter>::className;
}

//------------------------------------------------------------------------------
Uuid FindArrayUniqueValuesFilter::uuid() const
{
  return FilterTraits<FindArrayUniqueValuesFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string FindArrayUniqueValuesFilter::humanName() const
{
  return "Find Attribute Array Unique Values";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindArrayUniqueValuesFilter::defaultTags() const
{
  return {"ComplexCore", "Statistics"};
}

//------------------------------------------------------------------------------
Parameters FindArrayUniqueValuesFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Required Input Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedArrayPath_Key, "Attribute Array to Compute Statistics", "Input Attribute Array for which to compute statistics", DataPath{},
                                                          complex::GetAllDataTypes(), ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(
      std::make_unique<DataGroupCreationParameter>(k_DestinationAttributeMatrix_Key, "Destination Attribute Matrix", "Attribute Matrix in which to store the computed unique values", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Optional Data Mask"});
  params.insertLinkableParameter(
      std::make_unique<BoolParameter>(k_UseMask_Key, "Use Mask", "Whether to use a boolean mask array to ignore certain points flagged as false from the statistics", false));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskArrayPath_Key, "Mask", "The path to the data array that specifies if the point is to be counted in the statistics", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::boolean, DataType::uint8}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_UseMask_Key, k_MaskArrayPath_Key, true);
  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindArrayUniqueValuesFilter::clone() const
{
  return std::make_unique<FindArrayUniqueValuesFilter>();
}

template <typename T, typename V>
class FindUniqueValuesImpl
{
public:
  FindUniqueValuesImpl(DataStructure data, DataPath selectedArrayPath, DataPath maskArrayPath, bool useMask, std::map<V, int>& uniqueValuesMap)
  : m_Data(data)
  , m_SelectedArrayPath(selectedArrayPath)
  , m_MaskArrayPath(maskArrayPath)
  , m_UseMask(useMask)
  , m_UniqueValuesMap(uniqueValuesMap)
  {
  }

  virtual ~FindUniqueValuesImpl() = default;

  void compute(usize start, usize end) const
  {
    const T& selectedArray = m_Data.getDataRefAs<T>(m_SelectedArrayPath);

    for(int i = start; i < end; i++)
    {
      if(!m_UseMask)
      {
        if(m_UniqueValuesMap.find(selectedArray[i]) != m_UniqueValuesMap.end())
        {
          m_UniqueValuesMap[selectedArray[i]] = m_UniqueValuesMap[selectedArray[i]]++;
        }
        else
        {
          m_UniqueValuesMap[selectedArray[i]] = 1;
        }
      }
      else if(m_UseMask)
      {
        const BoolArray& maskArray = m_Data.getDataRefAs<BoolArray>(m_MaskArrayPath);

        if(maskArray[i])
        {
          if(m_UniqueValuesMap.find(selectedArray[i]) != m_UniqueValuesMap.end())
          {
            m_UniqueValuesMap[selectedArray[i]] = m_UniqueValuesMap[selectedArray[i]]++;
          }
          else
          {
            m_UniqueValuesMap[selectedArray[i]] = 1;
          }
        }
      }
    }
  }

  void operator()(const complex::Range& range) const
  {
    compute(range.min(), range.max());
  }

private:
  DataStructure m_Data;
  DataPath m_SelectedArrayPath;
  DataPath m_MaskArrayPath;
  bool m_UseMask;
  std::map<V, int>& m_UniqueValuesMap;
};

//------------------------------------------------------------------------------
template <typename T, typename V>
void findUniqueValues(DataStructure data, DataPath selectedArrayPath, DataPath maskArrayPath, DataPath destinationAttributeMatrixPath, bool useMask, std::string arrayName)
{
  auto& destinationAttributeMatrix = data.getDataRefAs<AttributeMatrix>(destinationAttributeMatrixPath);
  T& selectedArray = data.getDataRefAs<T>(selectedArrayPath);
  T& destinationArray = data.getDataRefAs<T>(destinationAttributeMatrixPath.createChildPath(arrayName));
  std::map<V, int> uniqueValuesMap;

  //#define TEST

#ifndef TEST

  ParallelDataAlgorithm dataAlg;
  dataAlg.setRange(0, selectedArray.getSize());

  dataAlg.execute(FindUniqueValuesImpl<T, V>(data, selectedArrayPath, maskArrayPath, useMask, uniqueValuesMap));

#endif // TEST

#ifdef TEST
  for(int i = 0; i < selectedArray.getSize(); i++)
  {
    if(!useMask)
    {
      if(uniqueValuesMap.find(selectedArray[i]) != uniqueValuesMap.end())
      {
        uniqueValuesMap[selectedArray[i]] = uniqueValuesMap[selectedArray[i]]++;
      }
      else
      {
        uniqueValuesMap[selectedArray[i]] = 1;
      }
    }
    else if(useMask)
    {
      const BoolArray& maskArray = data.getDataRefAs<BoolArray>(maskArrayPath);

      if(maskArray[i])
      {
        if(uniqueValuesMap.find(selectedArray[i]) != uniqueValuesMap.end())
        {
          uniqueValuesMap[selectedArray[i]] = uniqueValuesMap[selectedArray[i]]++;
        }
        else
        {
          uniqueValuesMap[selectedArray[i]] = 1;
        }
      }
    }
  }
#endif // TEST

  for(int j = 0; j < destinationArray.getSize(); j++)
  {
    destinationArray[j] = uniqueValuesMap.size();
  }
}

void findArrayType(DataStructure data, DataPath selectedArrayPath, DataPath maskArrayPath, DataPath destinationAttributeMatrixPath, bool useMask, std::string arrayName,
                   IFilter::MessageHandler messageHandler)
{
  const auto* selectedArray = data.getDataAs<IDataArray>(selectedArrayPath);

  if(selectedArray->getTypeName() == "DataArray<int8>")
  {
    findUniqueValues<Int8Array, int8>(data, selectedArrayPath, maskArrayPath, destinationAttributeMatrixPath, useMask, arrayName);
  }
  else if(selectedArray->getTypeName() == "DataArray<uint8>")
  {
    findUniqueValues<UInt8Array, uint8>(data, selectedArrayPath, maskArrayPath, destinationAttributeMatrixPath, useMask, arrayName);
  }
  else if(selectedArray->getTypeName() == "DataArray<int16>")
  {
    findUniqueValues<Int16Array, int16>(data, selectedArrayPath, maskArrayPath, destinationAttributeMatrixPath, useMask, arrayName);
  }
  else if(selectedArray->getTypeName() == "DataArray<uint16>")
  {
    findUniqueValues<UInt16Array, uint16>(data, selectedArrayPath, maskArrayPath, destinationAttributeMatrixPath, useMask, arrayName);
  }
  else if(selectedArray->getTypeName() == "DataArray<int32>")
  {
    findUniqueValues<Int32Array, int32>(data, selectedArrayPath, maskArrayPath, destinationAttributeMatrixPath, useMask, arrayName);
  }
  else if(selectedArray->getTypeName() == "DataArray<uint32>")
  {
    findUniqueValues<UInt32Array, uint32>(data, selectedArrayPath, maskArrayPath, destinationAttributeMatrixPath, useMask, arrayName);
  }
  else if(selectedArray->getTypeName() == "DataArray<int64>")
  {
    findUniqueValues<Int64Array, int64>(data, selectedArrayPath, maskArrayPath, destinationAttributeMatrixPath, useMask, arrayName);
  }
  else if(selectedArray->getTypeName() == "DataArray<uint64>")
  {
    findUniqueValues<UInt64Array, uint64>(data, selectedArrayPath, maskArrayPath, destinationAttributeMatrixPath, useMask, arrayName);
  }
  else if(selectedArray->getTypeName() == "DataArray<float32>")
  {
    findUniqueValues<Float32Array, float32>(data, selectedArrayPath, maskArrayPath, destinationAttributeMatrixPath, useMask, arrayName);
  }
  else if(selectedArray->getTypeName() == "DataArray<float64>")
  {
    findUniqueValues<Float64Array, float64>(data, selectedArrayPath, maskArrayPath, destinationAttributeMatrixPath, useMask, arrayName);
  }
  else if(selectedArray->getTypeName() == "DataArray<bool>")
  {
    findUniqueValues<BoolArray, bool>(data, selectedArrayPath, maskArrayPath, destinationAttributeMatrixPath, useMask, arrayName);
  }
  else
  {
    std::string err = "Invalid Data Array";
    messageHandler({IFilter::Message::Type::Error, err});
  }
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FindArrayUniqueValuesFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                    const std::atomic_bool& shouldCancel) const
{
  auto pSelectedArrayPathValue = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  auto pUseMaskValue = filterArgs.value<bool>(k_UseMask_Key);
  auto pMaskArrayPathValue = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  auto pDestinationAttributeMatrixValue = filterArgs.value<DataPath>(k_DestinationAttributeMatrix_Key);

  PreflightResult preflightResult;
  Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  std::vector<usize> tDims = {1};
  std::vector<usize> compDims = {1};

  const auto* inputArrayPtr = dataStructure.getDataAs<IDataArray>(pSelectedArrayPathValue);

  if(inputArrayPtr == nullptr)
  {
    return {MakeErrorResult<OutputActions>(-57202, fmt::format("Could not find selected input array at path '{}' ", pSelectedArrayPathValue.toString())), {}};
  }

  if(inputArrayPtr->getNumberOfComponents() != 1)
  {
    return {MakeErrorResult<OutputActions>(-57203, fmt::format("Input array must be a scalar array")), {}};
  }

  if(pUseMaskValue)
  {
    const auto* maskPtr = dataStructure.getDataAs<IDataArray>(pMaskArrayPathValue);
    if(maskPtr == nullptr)
    {
      return {MakeErrorResult<OutputActions>(-57207, fmt::format("Could not find mask array at path '{}' ", pMaskArrayPathValue.toString())), {}};
    }
    if(maskPtr->getDataType() != DataType::boolean && maskPtr->getDataType() != DataType::uint8)
    {
      return {MakeErrorResult<OutputActions>(-57207, fmt::format("Mask array must be of type Boolean or UInt8")), {}};
    }
  }

  auto createMatrix = std::make_unique<CreateAttributeMatrixAction>(pDestinationAttributeMatrixValue, tDims);
  resultOutputActions.value().actions.push_back(std::move(createMatrix));

  DataPath path = pDestinationAttributeMatrixValue.createChildPath("Unique Values");
  auto createArray = std::make_unique<CreateArrayAction>(DataType::int32, tDims, compDims, path);
  resultOutputActions.value().actions.push_back(std::move(createArray));

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> FindArrayUniqueValuesFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                  const std::atomic_bool& shouldCancel) const
{

  bool useMask = filterArgs.value<bool>(k_UseMask_Key);
  DataPath selectedArrayPath = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  DataPath maskArrayPath = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  DataPath destinationAttributeMatrix = filterArgs.value<DataPath>(k_DestinationAttributeMatrix_Key);

  findArrayType(dataStructure, selectedArrayPath, maskArrayPath, destinationAttributeMatrix, useMask, "Unique Values", messageHandler);
  return {};
}
} // namespace complex
