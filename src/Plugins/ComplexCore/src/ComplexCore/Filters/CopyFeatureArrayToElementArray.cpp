#include "CopyFeatureArrayToElementArray.hpp"

#include "complex/Common/TypesUtility.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"

using namespace complex;

namespace
{
// -----------------------------------------------------------------------------
template <typename T>
void copyData(DataStructure& dataStructure, const DataPath& selectedFeatureArrayPath, const DataPath& featureIdsArrayPath, const DataPath& createdArrayPath, const std::atomic_bool& shouldCancel)
{
  const DataArray<T>& selectedFeatureArray = dataStructure.getDataRefAs<DataArray<T>>(selectedFeatureArrayPath);
  const Int32Array& featureIds = dataStructure.getDataRefAs<Int32Array>(featureIdsArrayPath);
  DataArray<T>& createdArray = dataStructure.getDataRefAs<DataArray<T>>(createdArrayPath);

  usize totalFeatureIdTuples = featureIds.getNumberOfTuples();
  usize totalFeatureArrayComponents = selectedFeatureArray.getNumberOfComponents();
  for(usize i = 0; i < totalFeatureIdTuples; ++i)
  {
    if(shouldCancel)
    {
      return;
    }

    // Get the feature id (or what ever the user has selected as their "Feature" identifier
    int32 featureIdx = featureIds[i];

    for(usize faComp = 0; faComp < totalFeatureArrayComponents; faComp++)
    {
      createdArray[totalFeatureArrayComponents * i + faComp] = selectedFeatureArray[totalFeatureArrayComponents * featureIdx + faComp];
    }
  }
}
} // namespace

namespace complex
{

//------------------------------------------------------------------------------
std::string CopyFeatureArrayToElementArray::name() const
{
  return FilterTraits<CopyFeatureArrayToElementArray>::name.str();
}

//------------------------------------------------------------------------------
std::string CopyFeatureArrayToElementArray::className() const
{
  return FilterTraits<CopyFeatureArrayToElementArray>::className;
}

//------------------------------------------------------------------------------
Uuid CopyFeatureArrayToElementArray::uuid() const
{
  return FilterTraits<CopyFeatureArrayToElementArray>::uuid;
}

//------------------------------------------------------------------------------
std::string CopyFeatureArrayToElementArray::humanName() const
{
  return "Create Element Array from Feature Array";
}

//------------------------------------------------------------------------------
std::vector<std::string> CopyFeatureArrayToElementArray::defaultTags() const
{
  return {"#Core", "#Memory Management"};
}

//------------------------------------------------------------------------------
Parameters CopyFeatureArrayToElementArray::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  //  params.insertSeparator(Parameters::Separator{"Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedFeatureArrayPath_Key, "Feature Data to Copy to Element Data", "", DataPath{}, complex::GetAllDataTypes()));
  //  params.insertSeparator(Parameters::Separator{"Element Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "", DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::int32}, false));
  //  params.insertSeparator(Parameters::Separator{"Element Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_CreatedArrayName_Key, "Copied Attribute Array", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer CopyFeatureArrayToElementArray::clone() const
{
  return std::make_unique<CopyFeatureArrayToElementArray>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult CopyFeatureArrayToElementArray::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                       const std::atomic_bool& shouldCancel) const
{
  auto pSelectedFeatureArrayPathValue = filterArgs.value<DataPath>(k_SelectedFeatureArrayPath_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pCreatedArrayNameValue = filterArgs.value<DataPath>(k_CreatedArrayName_Key);

  const IDataArray& selectedFeatureArray = dataStructure.getDataRefAs<IDataArray>(pSelectedFeatureArrayPathValue);
  const IDataStore& selectedFeatureArrayStore = selectedFeatureArray.getIDataStoreRef();
  const IDataArray& featureIdsArray = dataStructure.getDataRefAs<IDataArray>(pFeatureIdsArrayPathValue);
  const IDataStore& featureIdsArrayStore = featureIdsArray.getIDataStoreRef();

  complex::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  DataType dataType = selectedFeatureArray.getDataType();

  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(dataType, featureIdsArrayStore.getTupleShape(), selectedFeatureArrayStore.getComponentShape(), pCreatedArrayNameValue);
    resultOutputActions.value().actions.push_back(std::move(createArrayAction));
  }

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> CopyFeatureArrayToElementArray::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                     const std::atomic_bool& shouldCancel) const
{
  auto pSelectedFeatureArrayPathValue = filterArgs.value<DataPath>(k_SelectedFeatureArrayPath_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pCreatedArrayNameValue = filterArgs.value<DataPath>(k_CreatedArrayName_Key);

  const IDataArray& selectedFeatureArray = dataStructure.getDataRefAs<IDataArray>(pSelectedFeatureArrayPathValue);
  const Int32Array& featureIds = dataStructure.getDataRefAs<Int32Array>(pFeatureIdsArrayPathValue);

  usize numFeatures = selectedFeatureArray.getNumberOfTuples();
  bool mismatchedFeatures = false;
  usize largestFeature = 0;
  for(const int32& featureId : featureIds)
  {
    if(shouldCancel)
    {
      return {};
    }

    if(static_cast<usize>(featureId) > largestFeature)
    {
      largestFeature = featureId;
      if(largestFeature >= numFeatures)
      {
        mismatchedFeatures = true;
        break;
      }
    }
  }

  if(mismatchedFeatures)
  {
    return MakeErrorResult(-5555, fmt::format("The largest Feature Id {} in the FeatureIds array is larger than the number of Features in the Feature Data array {}", largestFeature, numFeatures));
  }

  if(largestFeature != (numFeatures - 1))
  {
    return MakeErrorResult(-5555, fmt::format("The number of Features in the Feature Data array {} does not match the largest Feature Id in the FeatureIds array", numFeatures));
  }

  switch(selectedFeatureArray.getDataType())
  {
  case DataType::int8:
    copyData<int8>(dataStructure, pSelectedFeatureArrayPathValue, pFeatureIdsArrayPathValue, pCreatedArrayNameValue, shouldCancel);
    return {};
  case DataType::uint8:
    copyData<uint8>(dataStructure, pSelectedFeatureArrayPathValue, pFeatureIdsArrayPathValue, pCreatedArrayNameValue, shouldCancel);
    return {};
  case DataType::int16:
    copyData<int16>(dataStructure, pSelectedFeatureArrayPathValue, pFeatureIdsArrayPathValue, pCreatedArrayNameValue, shouldCancel);
    return {};
  case DataType::uint16:
    copyData<uint16>(dataStructure, pSelectedFeatureArrayPathValue, pFeatureIdsArrayPathValue, pCreatedArrayNameValue, shouldCancel);
    return {};
  case DataType::int32:
    copyData<int32>(dataStructure, pSelectedFeatureArrayPathValue, pFeatureIdsArrayPathValue, pCreatedArrayNameValue, shouldCancel);
    return {};
  case DataType::uint32:
    copyData<uint32>(dataStructure, pSelectedFeatureArrayPathValue, pFeatureIdsArrayPathValue, pCreatedArrayNameValue, shouldCancel);
    return {};
  case DataType::int64:
    copyData<int64>(dataStructure, pSelectedFeatureArrayPathValue, pFeatureIdsArrayPathValue, pCreatedArrayNameValue, shouldCancel);
    return {};
  case DataType::uint64:
    copyData<uint64>(dataStructure, pSelectedFeatureArrayPathValue, pFeatureIdsArrayPathValue, pCreatedArrayNameValue, shouldCancel);
    return {};
  case DataType::float32:
    copyData<float>(dataStructure, pSelectedFeatureArrayPathValue, pFeatureIdsArrayPathValue, pCreatedArrayNameValue, shouldCancel);
    return {};
  case DataType::float64:
    copyData<double>(dataStructure, pSelectedFeatureArrayPathValue, pFeatureIdsArrayPathValue, pCreatedArrayNameValue, shouldCancel);
    return {};
  case DataType::boolean:
    copyData<bool>(dataStructure, pSelectedFeatureArrayPathValue, pFeatureIdsArrayPathValue, pCreatedArrayNameValue, shouldCancel);
    return {};
  default:
    return MakeErrorResult(-14000, fmt::format("The selected array was of unsupported type. The path is {}", pSelectedFeatureArrayPathValue.toString()));
  }
}
} // namespace complex
