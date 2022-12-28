#include "CreateFeatureArrayFromElementArray.hpp"

#include "complex/Common/TypesUtility.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStore.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"

using namespace complex;

namespace
{
template <typename T>
Result<> copyCellData(DataStructure& dataStructure, const DataPath& selectedCellArrayPathValue, const DataPath& featureIdsArrayPathValue, const DataPath& createdArrayNameValue,
                      const std::atomic_bool& shouldCancel)
{
  const DataArray<T>& selectedCellArray = dataStructure.getDataRefAs<DataArray<T>>(selectedCellArrayPathValue);
  const DataStore<T> selectedCellArrayStore = selectedCellArray.template getIDataStoreRefAs<DataStore<T>>();
  const Int32Array& featureIds = dataStructure.getDataRefAs<Int32Array>(featureIdsArrayPathValue);
  DataArray<T>& createdArray = dataStructure.getDataRefAs<DataArray<T>>(createdArrayNameValue);

  // Initialize the output array with a default value
  createdArray.fill(0);

  usize totalCellArrayComponents = selectedCellArray.getNumberOfComponents();

  std::map<int32, usize> featureMap;
  Result<> result;

  usize totalCellArrayTuples = selectedCellArray.getNumberOfTuples();
  for(usize cellTupleIdx = 0; cellTupleIdx < totalCellArrayTuples; cellTupleIdx++)
  {
    if(shouldCancel)
    {
      return {};
    }

    // Get the feature identifier (or what ever the user has selected as their "Feature" identifier
    int32 featureIdx = featureIds[cellTupleIdx];

    // Store the index of the first tuple with this feature identifier in the map
    if(featureMap.find(featureIdx) == featureMap.end())
    {
      featureMap[featureIdx] = totalCellArrayComponents * cellTupleIdx;
    }

    // Check that the values at the current index match the value at the first index
    usize firstInstanceCellTupleIdx = featureMap[featureIdx];
    for(usize cellCompIdx = 0; cellCompIdx < totalCellArrayComponents; cellCompIdx++)
    {
      T firstInstanceCellVal = selectedCellArray[firstInstanceCellTupleIdx + cellCompIdx];
      T currentCellVal = selectedCellArray[totalCellArrayComponents * cellTupleIdx + cellCompIdx];
      if(currentCellVal != firstInstanceCellVal && result.warnings().empty())
      {
        // The values are inconsistent with the first values for this feature identifier, so throw a warning
        result.warnings().push_back(Warning{-1000, fmt::format("Elements from Feature {} do not all have the same value. The last value copied into Feature {} will be used", featureIdx, featureIdx)});
      }

      createdArray[totalCellArrayComponents * featureIdx + cellCompIdx] = selectedCellArray[totalCellArrayComponents * cellTupleIdx + cellCompIdx];
    }
  }

  return result;
}
} // namespace

namespace complex
{
//------------------------------------------------------------------------------
std::string CreateFeatureArrayFromElementArray::name() const
{
  return FilterTraits<CreateFeatureArrayFromElementArray>::name.str();
}

//------------------------------------------------------------------------------
std::string CreateFeatureArrayFromElementArray::className() const
{
  return FilterTraits<CreateFeatureArrayFromElementArray>::className;
}

//------------------------------------------------------------------------------
Uuid CreateFeatureArrayFromElementArray::uuid() const
{
  return FilterTraits<CreateFeatureArrayFromElementArray>::uuid;
}

//------------------------------------------------------------------------------
std::string CreateFeatureArrayFromElementArray::humanName() const
{
  return "Create Feature Array from Element Array";
}

//------------------------------------------------------------------------------
std::vector<std::string> CreateFeatureArrayFromElementArray::defaultTags() const
{
  return {"#Core", "#Memory Management"};
}

//------------------------------------------------------------------------------
Parameters CreateFeatureArrayFromElementArray::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Element Data"});
  params.insert(
      std::make_unique<ArraySelectionParameter>(k_SelectedCellArrayPath_Key, "Element Data to Copy to Feature Data", "Element Data to Copy to Feature Data", DataPath{}, complex::GetAllDataTypes()));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellFeatureIdsArrayPath_Key, "Feature Ids", "Specifies to which Feature each Element belongs", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Feature Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_CreatedArrayName_Key, "Copied Attribute Array", "The path to the copied AttributeArray", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer CreateFeatureArrayFromElementArray::clone() const
{
  return std::make_unique<CreateFeatureArrayFromElementArray>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult CreateFeatureArrayFromElementArray::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                           const std::atomic_bool& shouldCancel) const
{
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  auto pCreatedArrayNameValue = filterArgs.value<DataPath>(k_CreatedArrayName_Key);

  const IDataArray& selectedCellArray = dataStructure.getDataRefAs<IDataArray>(pSelectedCellArrayPathValue);
  const IDataStore& selectedCellArrayStore = selectedCellArray.getIDataStoreRef();

  complex::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  DataType dataType = selectedCellArray.getDataType();

  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(dataType, std::vector<usize>{1}, selectedCellArrayStore.getComponentShape(), pCreatedArrayNameValue);
    resultOutputActions.value().actions.push_back(std::move(createArrayAction));
  }

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> CreateFeatureArrayFromElementArray::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                         const std::atomic_bool& shouldCancel) const
{
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  auto pCreatedArrayNameValue = filterArgs.value<DataPath>(k_CreatedArrayName_Key);

  const IDataArray& selectedCellArray = dataStructure.getDataRefAs<IDataArray>(pSelectedCellArrayPathValue);
  const Int32Array& featureIds = dataStructure.getDataRefAs<Int32Array>(pFeatureIdsArrayPathValue);
  IDataArray& createdArray = dataStructure.getDataRefAs<IDataArray>(pCreatedArrayNameValue);

  // Resize the created array to the proper size
  usize featureIdsMaxIdx = std::distance(featureIds.begin(), std::max_element(featureIds.cbegin(), featureIds.cend()));
  usize maxValue = featureIds[featureIdsMaxIdx];

  IDataStore& createdArrayStore = createdArray.getIDataStoreRefAs<IDataStore>();
  createdArrayStore.reshapeTuples(std::vector<usize>{maxValue + 1});

  switch(selectedCellArray.getDataType())
  {
  case DataType::int8:
    return copyCellData<int8>(dataStructure, pSelectedCellArrayPathValue, pFeatureIdsArrayPathValue, pCreatedArrayNameValue, shouldCancel);
  case DataType::uint8:
    return copyCellData<uint8>(dataStructure, pSelectedCellArrayPathValue, pFeatureIdsArrayPathValue, pCreatedArrayNameValue, shouldCancel);
  case DataType::int16:
    return copyCellData<int16>(dataStructure, pSelectedCellArrayPathValue, pFeatureIdsArrayPathValue, pCreatedArrayNameValue, shouldCancel);
  case DataType::uint16:
    return copyCellData<uint16>(dataStructure, pSelectedCellArrayPathValue, pFeatureIdsArrayPathValue, pCreatedArrayNameValue, shouldCancel);
  case DataType::int32:
    return copyCellData<int32>(dataStructure, pSelectedCellArrayPathValue, pFeatureIdsArrayPathValue, pCreatedArrayNameValue, shouldCancel);
  case DataType::uint32:
    return copyCellData<uint32>(dataStructure, pSelectedCellArrayPathValue, pFeatureIdsArrayPathValue, pCreatedArrayNameValue, shouldCancel);
  case DataType::int64:
    return copyCellData<int64>(dataStructure, pSelectedCellArrayPathValue, pFeatureIdsArrayPathValue, pCreatedArrayNameValue, shouldCancel);
  case DataType::uint64:
    return copyCellData<uint64>(dataStructure, pSelectedCellArrayPathValue, pFeatureIdsArrayPathValue, pCreatedArrayNameValue, shouldCancel);
  case DataType::float32:
    return copyCellData<float>(dataStructure, pSelectedCellArrayPathValue, pFeatureIdsArrayPathValue, pCreatedArrayNameValue, shouldCancel);
  case DataType::float64:
    return copyCellData<double>(dataStructure, pSelectedCellArrayPathValue, pFeatureIdsArrayPathValue, pCreatedArrayNameValue, shouldCancel);
  case DataType::boolean:
    return copyCellData<bool>(dataStructure, pSelectedCellArrayPathValue, pFeatureIdsArrayPathValue, pCreatedArrayNameValue, shouldCancel);
  default:
    return MakeErrorResult(-14000, fmt::format("The selected array was of unsupported type. The path is {}", pSelectedCellArrayPathValue.toString()));
  }
}
} // namespace complex
