#include "CopyFeatureArrayToElementArray.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
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
  CopyFeatureArrayToElementArrayImpl(DataStructure& dataStructure, const DataPath& selectedFeatureArrayPath, const DataPath& featureIdsArrayPath, const DataPath& createdArrayPath,
                                     const std::atomic_bool& shouldCancel)
  : m_DataStructure(dataStructure)
  , m_SelectedFeatureArrayPath(selectedFeatureArrayPath)
  , m_FeatureIdsArrayPath(featureIdsArrayPath)
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
    using DataArrayType = DataArray<T>;
    const DataArrayType& selectedFeatureArray = m_DataStructure.getDataRefAs<DataArrayType>(m_SelectedFeatureArrayPath);
    const Int32Array& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_FeatureIdsArrayPath);
    auto& createdArray = m_DataStructure.getDataRefAs<DataArray<T>>(m_CreatedArrayPath);

    usize totalFeatureArrayComponents = selectedFeatureArray.getNumberOfComponents();

    for(usize i = range.min(); i < range.max(); ++i)
    {
      if(m_ShouldCancel)
      {
        return;
      }

      // Get the feature identifier (or what ever the user has selected as their "Feature" identifier
      int32 featureIdx = featureIds[i];

      for(usize faComp = 0; faComp < totalFeatureArrayComponents; faComp++)
      {
        createdArray[totalFeatureArrayComponents * i + faComp] = selectedFeatureArray[totalFeatureArrayComponents * featureIdx + faComp];
      }
    }
  }

private:
  DataStructure& m_DataStructure;
  const DataPath& m_SelectedFeatureArrayPath;
  const DataPath& m_FeatureIdsArrayPath;
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
  return {"Core", "Memory Management"};
}

//------------------------------------------------------------------------------
Parameters CopyFeatureArrayToElementArray::parameters() const
{
  Parameters params;

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

  const auto& selectedFeatureArray = dataStructure.getDataRefAs<IDataArray>(pSelectedFeatureArrayPathValue);
  const IDataStore& selectedFeatureArrayStore = selectedFeatureArray.getIDataStoreRef();
  const auto& featureIdsArray = dataStructure.getDataRefAs<IDataArray>(pFeatureIdsArrayPathValue);
  const IDataStore& featureIdsArrayStore = featureIdsArray.getIDataStoreRef();

  complex::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  DataType dataType = selectedFeatureArray.getDataType();

  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(dataType, featureIdsArrayStore.getTupleShape(), selectedFeatureArrayStore.getComponentShape(), createdArrayPath);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
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

  messageHandler(IFilter::ProgressMessage{IFilter::ProgressMessage::Type::Info, "Validating number of featureIds in input array..."});
  auto results = ValidateNumFeaturesInArray(dataStructure, pSelectedFeatureArrayPathValue, featureIds);
  if(results.invalid())
  {
    return results;
  }

  messageHandler(IFilter::ProgressMessage{IFilter::ProgressMessage::Type::Info, "Copying data into target array"});
  ParallelDataAlgorithm dataAlg;
  dataAlg.setRange(0, featureIds.getNumberOfTuples());
  ExecuteParallelFunction<::CopyFeatureArrayToElementArrayImpl>(selectedFeatureArray.getDataType(), dataAlg, dataStructure, pSelectedFeatureArrayPathValue, pFeatureIdsArrayPathValue, createdArrayPath,
                                                                shouldCancel);

  return {};
}
} // namespace complex
