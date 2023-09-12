#include "CopyFeatureArrayToElementArray.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"
#include "complex/Utilities/ParallelAlgorithmUtilities.hpp"
#include "complex/Utilities/ParallelDataAlgorithm.hpp"

using namespace complex;

namespace
{

template <typename T>
class CopyFeatureArrayToElementArrayImpl
{
public:
  CopyFeatureArrayToElementArrayImpl(DataStructure& dataStructure, const DataPath& selectedFeatureArrayPath, const Int32Array& featureIdsArray, const DataPath& createdArrayPath,
                                     const std::atomic_bool& shouldCancel)
  : m_DataStructure(dataStructure)
  , m_SelectedFeatureArrayPath(selectedFeatureArrayPath)
  , m_FeatureIdsArray(featureIdsArray)
  , m_CreatedArrayPath(createdArrayPath)
  , m_ShouldCancel(shouldCancel)
  {
  }

  ~CopyFeatureArrayToElementArrayImpl() = default;
  CopyFeatureArrayToElementArrayImpl(const CopyFeatureArrayToElementArrayImpl&) = default;           // Copy Constructor Not Implemented
  CopyFeatureArrayToElementArrayImpl(CopyFeatureArrayToElementArrayImpl&&) = delete;                 // Move Constructor Not Implemented
  CopyFeatureArrayToElementArrayImpl& operator=(const CopyFeatureArrayToElementArrayImpl&) = delete; // Copy Assignment Not Implemented
  CopyFeatureArrayToElementArrayImpl& operator=(CopyFeatureArrayToElementArrayImpl&&) = delete;      // Move Assignment Not Implemented

  void operator()(const Range& range) const
  {
    const DataArray<T>& selectedFeatureArray = m_DataStructure.getDataRefAs<DataArray<T>>(m_SelectedFeatureArrayPath);
    auto& createdArray = m_DataStructure.getDataRefAs<DataArray<T>>(m_CreatedArrayPath);
    const usize totalFeatureArrayComponents = selectedFeatureArray.getNumberOfComponents();

    for(usize i = range.min(); i < range.max(); ++i)
    {
      if(m_ShouldCancel)
      {
        return;
      }

      // Get the feature identifier (or what ever the user has selected as their "Feature" identifier
      const int32 featureIdx = m_FeatureIdsArray[i];

      for(usize faComp = 0; faComp < totalFeatureArrayComponents; faComp++)
      {
        createdArray[totalFeatureArrayComponents * i + faComp] = selectedFeatureArray[totalFeatureArrayComponents * featureIdx + faComp];
      }
    }
  }

private:
  DataStructure& m_DataStructure;
  const DataPath& m_SelectedFeatureArrayPath;
  const Int32Array& m_FeatureIdsArray;
  const DataPath& m_CreatedArrayPath;
  const std::atomic_bool& m_ShouldCancel;
};
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
  return {className(), "Core", "Memory Management"};
}

//------------------------------------------------------------------------------
Parameters CopyFeatureArrayToElementArray::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Feature Data"});
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_SelectedFeatureArrayPath_Key, "Feature Data to Copy to Element Data",
                                                               "The DataPath to the feature data that should be copied to the cell level", MultiArraySelectionParameter::ValueType{},
                                                               MultiArraySelectionParameter::AllowedTypes{IArray::ArrayType::Any}, complex::GetAllDataTypes()));

  params.insertSeparator(Parameters::Separator{"Input Cell Data Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellFeatureIdsArrayPath_Key, "Feature Ids", "Specifies to which Feature each Element belongs", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Created Cell Data"});
  params.insert(std::make_unique<StringParameter>(k_CreatedArraySuffix_Key, "Created Array Suffix", "The suffix to add to the input attribute array name when creating the copied array", ""));

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
  const auto pSelectedFeatureArrayPathsValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedFeatureArrayPath_Key);
  const auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  const auto createdArraySuffix = filterArgs.value<StringParameter::ValueType>(k_CreatedArraySuffix_Key);

  complex::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  if(pSelectedFeatureArrayPathsValue.empty())
  {
    return MakePreflightErrorResult(complex::FilterParameter::Constants::k_Validate_Empty_Value, "You must select at least one feature data array to copy to an element data array.");
  }

  if(!dataStructure.validateNumberOfTuples(pSelectedFeatureArrayPathsValue))
  {
    return MakePreflightErrorResult(-3020, "One or more of the selected feature arrays do not have a matching number of tuples. Make sure to select arrays from the feature level attribute matrix.");
  }

  const auto& featureIdsArray = dataStructure.getDataRefAs<IDataArray>(pFeatureIdsArrayPathValue);
  const IDataStore& featureIdsArrayStore = featureIdsArray.getIDataStoreRef();
  const std::vector<usize>& tDims = featureIdsArrayStore.getTupleShape();

  for(const auto& selectedFeatureArrayPath : pSelectedFeatureArrayPathsValue)
  {
    DataPath createdArrayPath = pFeatureIdsArrayPathValue.getParent().createChildPath(selectedFeatureArrayPath.getTargetName() + createdArraySuffix);
    const auto& selectedFeatureArray = dataStructure.getDataRefAs<IDataArray>(selectedFeatureArrayPath);
    DataType dataType = selectedFeatureArray.getDataType();
    auto createArrayAction = std::make_unique<CreateArrayAction>(dataType, tDims, selectedFeatureArray.getComponentShape(), createdArrayPath);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> CopyFeatureArrayToElementArray::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                     const std::atomic_bool& shouldCancel) const
{
  const auto pSelectedFeatureArrayPathsValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedFeatureArrayPath_Key);
  const auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  const auto createdArraySuffix = filterArgs.value<StringParameter::ValueType>(k_CreatedArraySuffix_Key);

  const Int32Array& featureIds = dataStructure.getDataRefAs<Int32Array>(pFeatureIdsArrayPathValue);
  for(const auto& selectedFeatureArrayPath : pSelectedFeatureArrayPathsValue)
  {
    DataPath createdArrayPath = pFeatureIdsArrayPathValue.getParent().createChildPath(selectedFeatureArrayPath.getTargetName() + createdArraySuffix);
    const IDataArray& selectedFeatureArray = dataStructure.getDataRefAs<IDataArray>(selectedFeatureArrayPath);

    messageHandler(IFilter::ProgressMessage{IFilter::ProgressMessage::Type::Info, fmt::format("Validating number of featureIds in input array '{}'...", selectedFeatureArrayPath.toString())});
    auto results = ValidateNumFeaturesInArray(dataStructure, selectedFeatureArrayPath, featureIds);
    if(results.invalid())
    {
      return results;
    }

    messageHandler(IFilter::ProgressMessage{IFilter::ProgressMessage::Type::Info, fmt::format("Copying data into target array '{}'...", createdArrayPath.toString())});
    ParallelDataAlgorithm dataAlg;
    dataAlg.setRange(0, featureIds.getNumberOfTuples());
    ExecuteParallelFunction<::CopyFeatureArrayToElementArrayImpl>(selectedFeatureArray.getDataType(), dataAlg, dataStructure, selectedFeatureArrayPath, featureIds, createdArrayPath, shouldCancel);
  }

  return {};
}
} // namespace complex
