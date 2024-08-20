#include "CopyFeatureArrayToElementArrayFilter.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/ParallelAlgorithmUtilities.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

using namespace nx::core;

namespace
{
template <typename T>
class CopyFeatureArrayToElementArrayImpl
{
public:
  using StoreType = AbstractDataStore<T>;

  CopyFeatureArrayToElementArrayImpl(const IDataArray* selectedFeatureArray, const Int32AbstractDataStore& featureIdsStore, IDataArray* createdArray, const std::atomic_bool& shouldCancel)
  : m_SelectedFeature(selectedFeatureArray->template getIDataStoreRefAs<StoreType>())
  , m_FeatureIdsStore(featureIdsStore)
  , m_CreatedStore(createdArray->template getIDataStoreRefAs<StoreType>())
  , m_ShouldCancel(shouldCancel)
  {
  }

  void operator()(const Range& range) const
  {
    const usize totalFeatureArrayComponents = m_SelectedFeature.getNumberOfComponents();

    for(usize i = range.min(); i < range.max(); ++i)
    {
      if(m_ShouldCancel)
      {
        return;
      }

      for(usize faComp = 0; faComp < totalFeatureArrayComponents; faComp++)
      {
        // Get the feature identifier (or what ever the user has selected as their "Feature" identifier
        m_CreatedStore[totalFeatureArrayComponents * i + faComp] = m_SelectedFeature[totalFeatureArrayComponents * m_FeatureIdsStore[i] + faComp];
      }
    }
  }

private:
  const StoreType& m_SelectedFeature;
  const Int32AbstractDataStore& m_FeatureIdsStore;
  StoreType& m_CreatedStore;
  const std::atomic_bool& m_ShouldCancel;
};
} // namespace

namespace nx::core
{

//------------------------------------------------------------------------------
std::string CopyFeatureArrayToElementArrayFilter::name() const
{
  return FilterTraits<CopyFeatureArrayToElementArrayFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string CopyFeatureArrayToElementArrayFilter::className() const
{
  return FilterTraits<CopyFeatureArrayToElementArrayFilter>::className;
}

//------------------------------------------------------------------------------
Uuid CopyFeatureArrayToElementArrayFilter::uuid() const
{
  return FilterTraits<CopyFeatureArrayToElementArrayFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string CopyFeatureArrayToElementArrayFilter::humanName() const
{
  return "Create Element Array from Feature Array";
}

//------------------------------------------------------------------------------
std::vector<std::string> CopyFeatureArrayToElementArrayFilter::defaultTags() const
{
  return {className(), "Core", "Memory Management"};
}

//------------------------------------------------------------------------------
Parameters CopyFeatureArrayToElementArrayFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Feature Data"});
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_SelectedFeatureArrayPath_Key, "Feature Data to Copy to Cell Data",
                                                               "The DataPath to the feature data that should be copied to the cell level", MultiArraySelectionParameter::ValueType{},
                                                               MultiArraySelectionParameter::AllowedTypes{IArray::ArrayType::Any}, nx::core::GetAllDataTypes()));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellFeatureIdsArrayPath_Key, "Cell Feature Ids", "Specifies to which Feature each cell belongs", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Output Cell Data"});
  params.insert(std::make_unique<StringParameter>(k_CreatedArraySuffix_Key, "Created Array Suffix", "The suffix to add to the input attribute array name when creating the copied array", ""));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer CopyFeatureArrayToElementArrayFilter::clone() const
{
  return std::make_unique<CopyFeatureArrayToElementArrayFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult CopyFeatureArrayToElementArrayFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                             const std::atomic_bool& shouldCancel) const
{
  const auto pSelectedFeatureArrayPathsValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedFeatureArrayPath_Key);
  const auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  const auto createdArraySuffix = filterArgs.value<StringParameter::ValueType>(k_CreatedArraySuffix_Key);

  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  if(pSelectedFeatureArrayPathsValue.empty())
  {
    return MakePreflightErrorResult(nx::core::FilterParameter::Constants::k_Validate_Empty_Value, "You must select at least one feature data array to copy to an element data array.");
  }

  auto tupleValidityCheck = dataStructure.validateNumberOfTuples(pSelectedFeatureArrayPathsValue);
  if(!tupleValidityCheck)
  {
    return {MakeErrorResult<OutputActions>(-3020, fmt::format("The following DataArrays all must have equal number of tuples but this was not satisfied.\n{}", tupleValidityCheck.error()))};
  }

  const auto& featureIdsArray = dataStructure.getDataRefAs<IDataArray>(pFeatureIdsArrayPathValue);
  const IDataStore& featureIdsArrayStore = featureIdsArray.getIDataStoreRef();
  const std::vector<usize>& tDims = featureIdsArrayStore.getTupleShape();

  for(const auto& selectedFeatureArrayPath : pSelectedFeatureArrayPathsValue)
  {
    DataPath createdArrayPath = pFeatureIdsArrayPathValue.replaceName(selectedFeatureArrayPath.getTargetName() + createdArraySuffix);
    const auto& selectedFeatureArray = dataStructure.getDataRefAs<IDataArray>(selectedFeatureArrayPath);
    DataType dataType = selectedFeatureArray.getDataType();
    auto createArrayAction = std::make_unique<CreateArrayAction>(dataType, tDims, selectedFeatureArray.getComponentShape(), createdArrayPath);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> CopyFeatureArrayToElementArrayFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                           const std::atomic_bool& shouldCancel) const
{
  const auto pSelectedFeatureArrayPathsValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedFeatureArrayPath_Key);
  const auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  const auto createdArraySuffix = filterArgs.value<StringParameter::ValueType>(k_CreatedArraySuffix_Key);

  const auto& featureIds = dataStructure.getDataRefAs<Int32Array>(pFeatureIdsArrayPathValue);
  for(const auto& selectedFeatureArrayPath : pSelectedFeatureArrayPathsValue)
  {
    DataPath createdArrayPath = pFeatureIdsArrayPathValue.replaceName(selectedFeatureArrayPath.getTargetName() + createdArraySuffix);
    const auto* selectedFeatureArray = dataStructure.getDataAs<IDataArray>(selectedFeatureArrayPath);

    messageHandler(IFilter::ProgressMessage{IFilter::ProgressMessage::Type::Info, fmt::format("Validating number of featureIds in input array '{}'...", selectedFeatureArrayPath.toString())});
    auto results = ValidateNumFeaturesInArray(dataStructure, selectedFeatureArrayPath, featureIds);
    if(results.invalid())
    {
      return results;
    }

    messageHandler(IFilter::ProgressMessage{IFilter::ProgressMessage::Type::Info, fmt::format("Copying data into target array '{}'...", createdArrayPath.toString())});
    ParallelDataAlgorithm dataAlg;
    dataAlg.setRange(0, featureIds.getNumberOfTuples());
    ExecuteParallelFunction<::CopyFeatureArrayToElementArrayImpl>(selectedFeatureArray->getDataType(), dataAlg, selectedFeatureArray, featureIds.getDataStoreRef(),
                                                                  dataStructure.getDataAs<IDataArray>(createdArrayPath), shouldCancel);
  }

  return {};
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_SelectedFeatureArrayPathKey = "SelectedFeatureArrayPath";
constexpr StringLiteral k_FeatureIdsArrayPathKey = "FeatureIdsArrayPath";
constexpr StringLiteral k_CreatedArrayNameKey = "CreatedArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> CopyFeatureArrayToElementArrayFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = CopyFeatureArrayToElementArrayFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::SingleToMultiDataPathSelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedFeatureArrayPathKey, k_SelectedFeatureArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_CellFeatureIdsArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_CreatedArrayNameKey, k_CreatedArraySuffix_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
