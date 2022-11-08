#include "CopyFeatureArrayToElementArray.hpp"

#include "complex/Common/TypesUtility.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"

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
  params.insertSeparator(Parameters::Separator{"Comments"});
  params.insert(std::make_unique<CommentParameter>(k_FilterComment_Key, "Comments", "User notes/comments", ""));
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedFeatureArrayPath_Key, "Feature Data to Copy to Element Data", "", DataPath{}, complex::GetAllDataTypes()));

  params.insertSeparator(Parameters::Separator{"Input Cell Data Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellFeatureIdsArrayPath_Key, "Feature Ids", "Specifies to which Feature each Element belongs", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Created Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_CreatedArrayName_Key, "Created Cell Attribute Array", "The copied Attribute Array", ""));

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
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  auto createdArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_CreatedArrayName_Key);
  DataPath createdArrayPath = pFeatureIdsArrayPathValue.getParent().createChildPath(createdArrayName);

  const IDataArray& selectedFeatureArray = dataStructure.getDataRefAs<IDataArray>(pSelectedFeatureArrayPathValue);
  const IDataStore& selectedFeatureArrayStore = selectedFeatureArray.getIDataStoreRef();
  const IDataArray& featureIdsArray = dataStructure.getDataRefAs<IDataArray>(pFeatureIdsArrayPathValue);
  const IDataStore& featureIdsArrayStore = featureIdsArray.getIDataStoreRef();

  complex::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  DataType dataType = selectedFeatureArray.getDataType();

  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(dataType, featureIdsArrayStore.getTupleShape(), selectedFeatureArrayStore.getComponentShape(), createdArrayPath);
    resultOutputActions.value().actions.push_back(std::move(createArrayAction));
  }

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> CopyFeatureArrayToElementArray::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                     const std::atomic_bool& shouldCancel) const
{
  auto pSelectedFeatureArrayPathValue = filterArgs.value<DataPath>(k_SelectedFeatureArrayPath_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  auto createdArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_CreatedArrayName_Key);
  DataPath createdArrayPath = pFeatureIdsArrayPathValue.getParent().createChildPath(createdArrayName);

  const IDataArray& selectedFeatureArray = dataStructure.getDataRefAs<IDataArray>(pSelectedFeatureArrayPathValue);
  const Int32Array& featureIds = dataStructure.getDataRefAs<Int32Array>(pFeatureIdsArrayPathValue);

  auto results = ValidateNumFeaturesInArray(dataStructure, pSelectedFeatureArrayPathValue, featureIds);
  if(results.invalid())
  {
    return results;
  }

  switch(selectedFeatureArray.getDataType())
  {
  case DataType::int8:
    copyData<int8>(dataStructure, pSelectedFeatureArrayPathValue, pFeatureIdsArrayPathValue, createdArrayPath, shouldCancel);
    return {};
  case DataType::uint8:
    copyData<uint8>(dataStructure, pSelectedFeatureArrayPathValue, pFeatureIdsArrayPathValue, createdArrayPath, shouldCancel);
    return {};
  case DataType::int16:
    copyData<int16>(dataStructure, pSelectedFeatureArrayPathValue, pFeatureIdsArrayPathValue, createdArrayPath, shouldCancel);
    return {};
  case DataType::uint16:
    copyData<uint16>(dataStructure, pSelectedFeatureArrayPathValue, pFeatureIdsArrayPathValue, createdArrayPath, shouldCancel);
    return {};
  case DataType::int32:
    copyData<int32>(dataStructure, pSelectedFeatureArrayPathValue, pFeatureIdsArrayPathValue, createdArrayPath, shouldCancel);
    return {};
  case DataType::uint32:
    copyData<uint32>(dataStructure, pSelectedFeatureArrayPathValue, pFeatureIdsArrayPathValue, createdArrayPath, shouldCancel);
    return {};
  case DataType::int64:
    copyData<int64>(dataStructure, pSelectedFeatureArrayPathValue, pFeatureIdsArrayPathValue, createdArrayPath, shouldCancel);
    return {};
  case DataType::uint64:
    copyData<uint64>(dataStructure, pSelectedFeatureArrayPathValue, pFeatureIdsArrayPathValue, createdArrayPath, shouldCancel);
    return {};
  case DataType::float32:
    copyData<float>(dataStructure, pSelectedFeatureArrayPathValue, pFeatureIdsArrayPathValue, createdArrayPath, shouldCancel);
    return {};
  case DataType::float64:
    copyData<double>(dataStructure, pSelectedFeatureArrayPathValue, pFeatureIdsArrayPathValue, createdArrayPath, shouldCancel);
    return {};
  case DataType::boolean:
    copyData<bool>(dataStructure, pSelectedFeatureArrayPathValue, pFeatureIdsArrayPathValue, createdArrayPath, shouldCancel);
    return {};
  default:
    return MakeErrorResult(-14000, fmt::format("The selected array was of unsupported type. The path is {}", pSelectedFeatureArrayPathValue.toString()));
  }
}
} // namespace complex
