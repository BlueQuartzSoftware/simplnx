#include "CreateFeatureArrayFromElementArrayFilter.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Utilities/DataObjectUtilities.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include "simplnx/Utilities/FilterUtilities.hpp"

using namespace nx::core;

namespace
{
struct CopyCellDataFunctor
{
  template <typename T>
  Result<> operator()(DataStructure& dataStructure, const DataPath& selectedCellArrayPathValue, const DataPath& featureIdsArrayPathValue, const DataPath& createdArrayNameValue,
                      const std::atomic_bool& shouldCancel)
  {
    const DataArray<T>& selectedCellArray = dataStructure.getDataRefAs<DataArray<T>>(selectedCellArrayPathValue);
    const auto& selectedCellArrayStore = selectedCellArray.getDataStoreRef();
    const Int32Array& featureIdsArray = dataStructure.getDataRefAs<Int32Array>(featureIdsArrayPathValue);
    const Int32AbstractDataStore& featureIds = featureIdsArray.getDataStoreRef();
    auto& createdArray = dataStructure.getDataRefAs<DataArray<T>>(createdArrayNameValue);
    auto& createdDataStore = createdArray.getDataStoreRef();

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
        T firstInstanceCellVal = selectedCellArrayStore[firstInstanceCellTupleIdx + cellCompIdx];
        T currentCellVal = selectedCellArrayStore[totalCellArrayComponents * cellTupleIdx + cellCompIdx];
        if(currentCellVal != firstInstanceCellVal && result.warnings().empty())
        {
          // The values are inconsistent with the first values for this feature identifier, so throw a warning
          result.warnings().push_back(
              Warning{-1000, fmt::format("Elements from Feature {} do not all have the same value. The last value copied into Feature {} will be used", featureIdx, featureIdx)});
        }

        createdDataStore[totalCellArrayComponents * featureIdx + cellCompIdx] = selectedCellArrayStore[totalCellArrayComponents * cellTupleIdx + cellCompIdx];
      }
    }

    return result;
  }
};
} // namespace

namespace nx::core
{
//------------------------------------------------------------------------------
std::string CreateFeatureArrayFromElementArrayFilter::name() const
{
  return FilterTraits<CreateFeatureArrayFromElementArrayFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string CreateFeatureArrayFromElementArrayFilter::className() const
{
  return FilterTraits<CreateFeatureArrayFromElementArrayFilter>::className;
}

//------------------------------------------------------------------------------
Uuid CreateFeatureArrayFromElementArrayFilter::uuid() const
{
  return FilterTraits<CreateFeatureArrayFromElementArrayFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string CreateFeatureArrayFromElementArrayFilter::humanName() const
{
  return "Create Feature Array from Element Array";
}

//------------------------------------------------------------------------------
std::vector<std::string> CreateFeatureArrayFromElementArrayFilter::defaultTags() const
{
  return {className(), "Core", "Memory Management"};
}

//------------------------------------------------------------------------------
Parameters CreateFeatureArrayFromElementArrayFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Data"});
  params.insert(
      std::make_unique<ArraySelectionParameter>(k_SelectedCellArrayPath_Key, "Data to Copy to Feature Data", "Element Data to Copy to Feature Data", DataPath{}, nx::core::GetAllDataTypes()));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellFeatureIdsArrayPath_Key, "Cell Feature Ids", "Specifies to which Feature each Element belongs", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Input Feature Data"});
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_CellFeatureAttributeMatrixPath_Key, "Feature Attribute Matrix",
                                                              "The path to the cell feature attribute matrix where the converted output feature array will be stored",
                                                              DataPath({"DataContainer", "Cell Feature Data"}), DataGroupSelectionParameter::AllowedTypes{BaseGroup::GroupType::AttributeMatrix}));
  params.insertSeparator(Parameters::Separator{"Output Feature Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_CreatedArrayName_Key, "Created Feature Attribute Array", "The path to the copied AttributeArray", ""));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer CreateFeatureArrayFromElementArrayFilter::clone() const
{
  return std::make_unique<CreateFeatureArrayFromElementArrayFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult CreateFeatureArrayFromElementArrayFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                                 const std::atomic_bool& shouldCancel) const
{
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  auto pCellFeatureAttributeMatrixPathValue = filterArgs.value<DataPath>(k_CellFeatureAttributeMatrixPath_Key);
  auto pCreatedArrayNameValue = filterArgs.value<std::string>(k_CreatedArrayName_Key);

  const auto& selectedCellArray = dataStructure.getDataRefAs<IDataArray>(pSelectedCellArrayPathValue);
  const IDataStore& selectedCellArrayStore = selectedCellArray.getIDataStoreRef();

  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  // Get the target Attribute Matrix that the output array will be stored with
  // the proper tuple shape
  std::vector<usize> amTupleShape = {1ULL};
  // First try getting the amPath as an AttributeMatrix
  auto* featureAttributeMatrixPtr = dataStructure.getDataAs<AttributeMatrix>(pCellFeatureAttributeMatrixPathValue);
  if(featureAttributeMatrixPtr != nullptr)
  {
    amTupleShape = featureAttributeMatrixPtr->getShape();
  }

  {
    DataType dataType = selectedCellArray.getDataType();
    auto createArrayAction =
        std::make_unique<CreateArrayAction>(dataType, amTupleShape, selectedCellArrayStore.getComponentShape(), pCellFeatureAttributeMatrixPathValue.createChildPath(pCreatedArrayNameValue));
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> CreateFeatureArrayFromElementArrayFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                               const std::atomic_bool& shouldCancel) const
{
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  auto pCellFeatureAttributeMatrixPathValue = filterArgs.value<DataPath>(k_CellFeatureAttributeMatrixPath_Key);
  auto pCreatedArrayNameValue = filterArgs.value<std::string>(k_CreatedArrayName_Key);

  const DataPath createdArrayPath = pCellFeatureAttributeMatrixPathValue.createChildPath(pCreatedArrayNameValue);
  const IDataArray& selectedCellArray = dataStructure.getDataRefAs<IDataArray>(pSelectedCellArrayPathValue);
  const Int32Array& featureIdsArray = dataStructure.getDataRefAs<Int32Array>(pFeatureIdsArrayPathValue);
  const Int32AbstractDataStore& featureIds = featureIdsArray.getDataStoreRef();
  auto& createdArray = dataStructure.getDataRefAs<IDataArray>(createdArrayPath);

  // Resize the created array to the proper size
  usize featureIdsMaxIdx = std::distance(featureIds.begin(), std::max_element(featureIds.cbegin(), featureIds.cend()));
  usize maxValue = featureIds[featureIdsMaxIdx];
  auto& cellFeatureAttrMat = dataStructure.getDataRefAs<AttributeMatrix>(pCellFeatureAttributeMatrixPathValue);

  auto& createdArrayStore = createdArray.getIDataStoreRefAs<IDataStore>();
  createdArrayStore.resizeTuples(std::vector<usize>{maxValue + 1});
  cellFeatureAttrMat.resizeTuples(std::vector<usize>{maxValue + 1});

  return ExecuteDataFunction(CopyCellDataFunctor{}, selectedCellArray.getDataType(), dataStructure, pSelectedCellArrayPathValue, pFeatureIdsArrayPathValue, createdArrayPath, shouldCancel);
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_SelectedCellArrayPathKey = "SelectedCellArrayPath";
constexpr StringLiteral k_FeatureIdsArrayPathKey = "FeatureIdsArrayPath";
constexpr StringLiteral k_CellFeatureAttributeMatrixNameKey = "CellFeatureAttributeMatrixName";
constexpr StringLiteral k_CreatedArrayNameKey = "CreatedArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> CreateFeatureArrayFromElementArrayFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = CreateFeatureArrayFromElementArrayFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_SelectedCellArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_CellFeatureIdsArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::AttributeMatrixSelectionFilterParameterConverter>(args, json, SIMPL::k_CellFeatureAttributeMatrixNameKey,
                                                                                                                         k_CellFeatureAttributeMatrixPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_CreatedArrayNameKey, k_CreatedArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
