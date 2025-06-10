#include "SplitDataArrayByTupleFilter.hpp"

#include <algorithm>
#include <optional>
#include <ranges>

#include "SimplnxCore/Filters/Algorithms/SplitDataArrayByTuple.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "simplnx/Filter/Actions/CreateDataGroupAction.hpp"
#include "simplnx/Filter/Actions/CreateNeighborListAction.hpp"
#include "simplnx/Filter/Actions/CreateStringArrayAction.hpp"
#include "simplnx/Filter/Actions/DeleteDataAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

using namespace nx::core;

namespace
{
struct ConvertHexGridToSquareGridFilterCache
{
  std::vector<DataPath> outputArrayPaths;
  std::vector<std::vector<usize>> outputTupleShapes;
};

std::atomic_int32_t s_InstanceId = 0;
std::map<int32, ConvertHexGridToSquareGridFilterCache> s_HeaderCache;

constexpr usize k_MaxDisplayPaths = 5; // (must be >= 3 to show ellipsis)

std::vector<std::string> dataPathsToStrings(const std::vector<DataPath>& dataPaths)
{
  std::vector<std::string> dataPathsStrs;
  std::transform(dataPaths.cbegin(), dataPaths.cend(), std::back_inserter(dataPathsStrs), [](const DataPath& dp) { return dp.toString(); });
  return dataPathsStrs;
}

std::string shapeToString(const std::vector<usize>& shape)
{
  std::vector<std::string> shapeStrs;
  std::transform(shape.cbegin(), shape.cend(), std::back_inserter(shapeStrs), [](usize val) { return std::to_string(val); });
  std::vector<std::string_view> shapeStrViews(shapeStrs.begin(), shapeStrs.end());
  return StringUtilities::join(shapeStrViews, " x ");
}

std::vector<std::string> createDisplayPaths(const std::vector<std::string>& paths, const std::vector<std::vector<usize>>& tupleShapes)
{
  std::vector<std::string> displayPaths;
  const usize totalPaths = paths.size();
  if(totalPaths > k_MaxDisplayPaths && k_MaxDisplayPaths >= 3)
  {
    const usize headCount = (k_MaxDisplayPaths - 1) / 2;
    const usize tailCount = k_MaxDisplayPaths - headCount - 1;
    displayPaths.reserve(k_MaxDisplayPaths);
    for(usize i = 0; i < headCount; ++i)
    {
      displayPaths.push_back(fmt::format("{} ({})", paths[i], shapeToString(tupleShapes[i])));
    }
    displayPaths.emplace_back("...");
    for(usize i = totalPaths - tailCount; i < totalPaths; ++i)
    {
      displayPaths.push_back(fmt::format("{} ({})", paths[i], shapeToString(tupleShapes[i])));
    }
  }
  else
  {
    for(usize i = 0; i < paths.size(); ++i)
    {
      displayPaths.push_back(fmt::format("{} ({})", paths[i], shapeToString(tupleShapes[i])));
    }
  }

  return displayPaths;
}

std::optional<usize> commonMultiplier(const std::vector<usize>& dividend, const std::vector<usize>& divisor)
{
  if(dividend.size() != divisor.size() || dividend.empty())
  {
    return {};
  }

  if(divisor[0] == 0 || dividend[0] % divisor[0] != 0)
  {
    return {};
  }
  usize commonMultiplier = dividend[0] / divisor[0];

  for(usize i = 1; i < dividend.size(); ++i)
  {
    if(divisor[i] == 0 || dividend[i] % divisor[i] != 0 || dividend[i] / divisor[i] != commonMultiplier)
    {
      return {};
    }
  }

  return commonMultiplier;
}

Result<> preflightDataGroupOutput(SplitDataArrayByTuple::OutputContainer outputContainer, const DataPath& inputArrayPath, const DataPath& newDataGroupPath, const DataPath& existingDataGroupPath,
                                  const std::vector<usize>& inputArrayTupleShape, const std::vector<std::vector<usize>>& splitArrayTupleShapes, const DataStructure& dataStructure,
                                  Result<OutputActions>& resultOutputActions, std::vector<DataPath>& arrayPaths, std::vector<IFilter::PreflightValue>& preflightUpdatedValues)
{
  arrayPaths.reserve(splitArrayTupleShapes.size());
  std::vector<usize> tupleShapeColSums(splitArrayTupleShapes[0].size(), 0);
  std::string tupleShapesOutputStr;
  for(usize i = 0; i < splitArrayTupleShapes.size(); i++)
  {
    auto splitArrayTupleShape = splitArrayTupleShapes[i];

    std::string arrayName = inputArrayPath.getTargetName() + "_" + StringUtilities::GenerateIndexString(static_cast<int32>(i) + 1, static_cast<int32>(splitArrayTupleShapes.size()));
    if(outputContainer == SplitDataArrayByTuple::OutputContainer::NewDataGroup)
    {
      arrayPaths.push_back(newDataGroupPath.createChildPath(arrayName));
    }
    else
    {
      arrayPaths.push_back(existingDataGroupPath.createChildPath(arrayName));
    }

    std::transform(tupleShapeColSums.begin(), tupleShapeColSums.end(), splitArrayTupleShape.begin(), tupleShapeColSums.begin(), std::plus<>{});

    // Build up the tuple shape string and add it to the overall tuple shapes output string
    tupleShapesOutputStr += fmt::format("({})\n", shapeToString(splitArrayTupleShape));
  }
  tupleShapesOutputStr.pop_back(); // Remove unneeded newline character

  if(tupleShapeColSums != inputArrayTupleShape)
  {
    return {MakeErrorResult(
        -65405, fmt::format("The element-wise sum of your chosen tuple shapes ({}) does not equal the input array '{}' tuple shape ({}).  Please check your split arrays tuple shapes:\n\n{}",
                            shapeToString(tupleShapeColSums), inputArrayPath.toString(), shapeToString(inputArrayTupleShape), tupleShapesOutputStr))};
  }

  // Output array paths to preflight updated values
  auto arrayPathsStrs = dataPathsToStrings(arrayPaths);
  std::vector<std::string> displayPaths = createDisplayPaths(arrayPathsStrs, splitArrayTupleShapes);
  std::vector<std::string_view> displayPathsViews(displayPaths.begin(), displayPaths.end());
  preflightUpdatedValues.push_back({fmt::format("Created Split Arrays ({}): ", displayPaths.size()), fmt::format("{}", StringUtilities::join(displayPathsViews, "\n"))});

  return {};
}

Result<> preflightAttrMatrixOutput(SplitDataArrayByTuple::OutputContainer outputContainer, const DataPath& inputArrayPath, const DataPath& newAttrMatrixPath, const DataPath& existingAttrMatrixPath,
                                   const std::vector<usize>& inputArrayTupleShape, const std::vector<usize>& newAttrMatrixTupleShape, const DataStructure& dataStructure,
                                   std::vector<DataPath>& arrayPaths, std::vector<std::vector<usize>>& tupleShapes, std::vector<IFilter::PreflightValue>& preflightUpdatedValues)
{
  std::vector<usize> tupleShape;
  if(outputContainer == SplitDataArrayByTuple::OutputContainer::NewAttrMatrix)
  {
    tupleShape = newAttrMatrixTupleShape;
  }
  else
  {
    const auto& existingAttrMatrix = dataStructure.getDataRefAs<AttributeMatrix>(existingAttrMatrixPath);
    tupleShape = existingAttrMatrix.getShape();
  }

  auto opt = commonMultiplier(inputArrayTupleShape, tupleShape);
  if(!opt.has_value())
  {
    return {MakeErrorResult(-65401, fmt::format("The selected tuple shape ({0}) cannot cleanly split the input array '{1}' tuple shape ({2}).\n\n"
                                                "No single integer multiplier applied element-wise to ({0}) will produce ({2}).",
                                                shapeToString(tupleShape), inputArrayPath.toString(), shapeToString(inputArrayTupleShape)))};
  }
  usize numOfAttrMatrixSplitArrays = opt.value();

  arrayPaths.reserve(numOfAttrMatrixSplitArrays);
  tupleShapes.reserve(numOfAttrMatrixSplitArrays);
  for(usize i = 0; i < numOfAttrMatrixSplitArrays; i++)
  {
    std::string arrayName = inputArrayPath.getTargetName() + "_" + StringUtilities::GenerateIndexString(static_cast<int32>(i) + 1, static_cast<int32>(numOfAttrMatrixSplitArrays));
    if(outputContainer == SplitDataArrayByTuple::OutputContainer::NewAttrMatrix)
    {
      arrayPaths.push_back(newAttrMatrixPath.createChildPath(arrayName));
    }
    else
    {
      arrayPaths.push_back(existingAttrMatrixPath.createChildPath(arrayName));
    }
  }

  tupleShapes = std::vector<std::vector<usize>>(numOfAttrMatrixSplitArrays, tupleShape);

  // Output array paths to preflight updated values
  auto arrayPathsStrs = dataPathsToStrings(arrayPaths);
  std::vector<std::string> displayPaths = createDisplayPaths(arrayPathsStrs, tupleShapes);
  std::vector<std::string_view> displayPathsViews(displayPaths.begin(), displayPaths.end());
  preflightUpdatedValues.push_back({fmt::format("Created Split Arrays ({}): ", displayPaths.size()), fmt::format("{}", StringUtilities::join(displayPathsViews, "\n"))});

  return {};
}
} // namespace

namespace nx::core
{
//------------------------------------------------------------------------------
SplitDataArrayByTupleFilter::SplitDataArrayByTupleFilter()
: m_InstanceId(s_InstanceId.fetch_add(1))
{
  s_HeaderCache[m_InstanceId] = {};
}

//------------------------------------------------------------------------------
SplitDataArrayByTupleFilter::~SplitDataArrayByTupleFilter() noexcept
{
  s_HeaderCache.erase(m_InstanceId);
}

//------------------------------------------------------------------------------
std::string SplitDataArrayByTupleFilter::name() const
{
  return FilterTraits<SplitDataArrayByTupleFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string SplitDataArrayByTupleFilter::className() const
{
  return FilterTraits<SplitDataArrayByTupleFilter>::className;
}

//------------------------------------------------------------------------------
Uuid SplitDataArrayByTupleFilter::uuid() const
{
  return FilterTraits<SplitDataArrayByTupleFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string SplitDataArrayByTupleFilter::humanName() const
{
  return "Split Data Array (By Tuple)";
}

//------------------------------------------------------------------------------
std::vector<std::string> SplitDataArrayByTupleFilter::defaultTags() const
{
  return {className(), "Core", "Split", "Data", "Tuple"};
}

//------------------------------------------------------------------------------
Parameters SplitDataArrayByTupleFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_DataArrayPath_Key, "Data Array", "The data array to split by tuples.", DataPath{}, GetAllDataTypes()));
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_OutputContainer, "Output Container", "Set the output container where the output split arrays will be stored.",
                                                                    static_cast<ChoicesParameter::ValueType>(SplitDataArrayByTuple::OutputContainer::NewDataGroup),
                                                                    ChoicesParameter::Choices{"New Data Group", "Existing Data Group", "New Attribute Matrix", "Existing Attribute Matrix"}));
  params.insert(std::make_unique<BoolParameter>(k_DeleteOriginal_Key, "Remove Original Array", "Whether or not to remove the original data array after splitting", false));

  {
    DynamicTableInfo tableInfo;
    tableInfo.setRowsInfo(DynamicTableInfo::DynamicVectorInfo(2, "Tuple Shape {}"));
    tableInfo.setColsInfo(DynamicTableInfo::DynamicVectorInfo(1, "Tuple Dim {}"));
    params.insert(std::make_unique<DynamicTableParameter>(k_SplitArraysTupleShapes_Key, "Split Arrays Tuple Shapes",
                                                          "The tuple shapes for each split array.  Each column MUST add up to the total tuple"
                                                          "count for its corresponding dimension of the input array's tuple shape.  For example,"
                                                          "if the input array has tuple shape (100x200x300), the first column should add up to 100,"
                                                          "the second column should add up to 200, and the third column should add up to 300.",
                                                          tableInfo));
  }
  params.insert(std::make_unique<NumberParameter<uint64>>(k_NumOfAttrMatrixSplitArrays_Key, "Number of Split Arrays", "The number of split arrays that will be stored in the attribute matrix.", 2));

  params.insertSeparator(Parameters::Separator{"Output Parameter(s)"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_NewDataGroupPath, "New Data Group", "The path to the newly created data group where the output split arrays will be stored.",
                                                             DataPath({"Split Arrays"})));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_ExistingDataGroupPath, "Existing Data Group", "The path to the existing data group where the output split arrays will be stored.",
                                                              DataPath{}, DataGroupSelectionParameter::AllowedTypes{BaseGroup::GroupType::DataGroup}));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_NewAttributeMatrixPath, "New Attribute Matrix",
                                                             "The path to the newly created attribute matrix where the output split arrays will be stored.", DataPath({"Split Arrays"})));
  params.insert(std::make_unique<AttributeMatrixSelectionParameter>(k_ExistingAttributeMatrixPath, "Existing Attribute Matrix",
                                                                    "The path to the existing attribute matrix where the output split arrays will be stored.", DataPath()));
  {
    DynamicTableInfo tableInfo;
    tableInfo.setRowsInfo(DynamicTableInfo::StaticVectorInfo(1));
    tableInfo.setColsInfo(DynamicTableInfo::DynamicVectorInfo(1, "Tuple Dim {}"));
    params.insert(std::make_unique<DynamicTableParameter>(k_AttrMatrixTupleShape_Key, "Output Attribute Matrix Tuple Shape", "The tuple shape for the output attribute matrix.", tableInfo));
  }

  params.linkParameters(k_OutputContainer, k_NewDataGroupPath, static_cast<ChoicesParameter::ValueType>(SplitDataArrayByTuple::OutputContainer::NewDataGroup));
  params.linkParameters(k_OutputContainer, k_SplitArraysTupleShapes_Key, static_cast<ChoicesParameter::ValueType>(SplitDataArrayByTuple::OutputContainer::NewDataGroup));
  params.linkParameters(k_OutputContainer, k_ExistingDataGroupPath, static_cast<ChoicesParameter::ValueType>(SplitDataArrayByTuple::OutputContainer::ExistingDataGroup));
  params.linkParameters(k_OutputContainer, k_SplitArraysTupleShapes_Key, static_cast<ChoicesParameter::ValueType>(SplitDataArrayByTuple::OutputContainer::ExistingDataGroup));
  params.linkParameters(k_OutputContainer, k_NewAttributeMatrixPath, static_cast<ChoicesParameter::ValueType>(SplitDataArrayByTuple::OutputContainer::NewAttrMatrix));
  //  params.linkParameters(k_OutputContainer, k_NumOfAttrMatrixSplitArrays_Key, static_cast<ChoicesParameter::ValueType>(SplitDataArrayByTuple::OutputContainer::NewAttrMatrix));
  params.linkParameters(k_OutputContainer, k_AttrMatrixTupleShape_Key, static_cast<ChoicesParameter::ValueType>(SplitDataArrayByTuple::OutputContainer::NewAttrMatrix));
  params.linkParameters(k_OutputContainer, k_ExistingAttributeMatrixPath, static_cast<ChoicesParameter::ValueType>(SplitDataArrayByTuple::OutputContainer::ExistingAttrMatrix));
  params.linkParameters(k_OutputContainer, k_NumOfAttrMatrixSplitArrays_Key, static_cast<ChoicesParameter::ValueType>(SplitDataArrayByTuple::OutputContainer::ExistingAttrMatrix));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType SplitDataArrayByTupleFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer SplitDataArrayByTupleFilter::clone() const
{
  return std::make_unique<SplitDataArrayByTupleFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult SplitDataArrayByTupleFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                    const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto pInputArrayPath = filterArgs.value<ArraySelectionParameter::ValueType>(k_DataArrayPath_Key);
  auto pRemoveOriginal = filterArgs.value<bool>(k_DeleteOriginal_Key);
  auto pSplitArraysTupleShapes = filterArgs.value<DynamicTableParameter::ValueType>(k_SplitArraysTupleShapes_Key);
  auto pOutputContainer = static_cast<SplitDataArrayByTuple::OutputContainer>(filterArgs.value<ChoicesParameter::ValueType>(k_OutputContainer));
  auto pNewDataGroupPath = filterArgs.value<DataGroupCreationParameter::ValueType>(k_NewDataGroupPath);
  auto pExistingDataGroupPath = filterArgs.value<DataGroupSelectionParameter::ValueType>(k_ExistingDataGroupPath);
  auto pNewAttrMatrixPath = filterArgs.value<DataGroupCreationParameter::ValueType>(k_NewAttributeMatrixPath);
  auto pExistingAttrMatrixPath = filterArgs.value<AttributeMatrixSelectionParameter::ValueType>(k_ExistingAttributeMatrixPath);
  auto pNumOfAttrMatrixSplitArrays = filterArgs.value<NumberParameter<uint64>::ValueType>(k_NumOfAttrMatrixSplitArrays_Key);
  auto pNewAttrMatrixTupleShape = filterArgs.value<DynamicTableParameter::ValueType>(k_AttrMatrixTupleShape_Key);

  PreflightResult preflightResult;
  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  auto* inputArray = dataStructure.getDataAs<IArray>(pInputArrayPath);
  if(inputArray == nullptr)
  {
    return {MakeErrorResult<OutputActions>(-65400, fmt::format("Cannot find input array at path '{}'", pInputArrayPath.toString()))};
  }

  std::vector<DataPath> arrayPaths;
  std::vector<std::vector<usize>> tupleShapes;

  if(pOutputContainer == SplitDataArrayByTuple::OutputContainer::NewDataGroup || pOutputContainer == SplitDataArrayByTuple::OutputContainer::ExistingDataGroup)
  {
    for(usize i = 0; i < pSplitArraysTupleShapes.size(); ++i)
    {
      const auto& splitArraysTupleShape = pSplitArraysTupleShapes[i];
      for(usize j = 0; j < splitArraysTupleShape.size(); ++j)
      {
        if(splitArraysTupleShape[j] <= 0)
        {
          return {MakeErrorResult<OutputActions>(-65403, fmt::format("Tuple Shape {} contains \"{}\" at Tuple Dim {}.  All tuple shape values must be >= 1.", i, splitArraysTupleShape[j], j))};
        }
      }
    }

    // Outputting to data group
    tupleShapes.reserve(pSplitArraysTupleShapes.size());
    std::transform(pSplitArraysTupleShapes.begin(), pSplitArraysTupleShapes.end(), std::back_inserter(tupleShapes), [](auto const& row) { return std::vector<usize>(row.begin(), row.end()); });
    auto result = preflightDataGroupOutput(pOutputContainer, pInputArrayPath, pNewDataGroupPath, pExistingDataGroupPath, inputArray->getTupleShape(), tupleShapes, dataStructure, resultOutputActions,
                                           arrayPaths, preflightUpdatedValues);
    if(result.invalid())
    {
      return {ConvertResultTo<OutputActions>(std::move(result), {})};
    }

    if(pOutputContainer == SplitDataArrayByTuple::OutputContainer::NewDataGroup)
    {
      resultOutputActions.value().appendAction(std::make_unique<CreateDataGroupAction>(pNewDataGroupPath));
    }
  }
  else
  {
    if(pOutputContainer == SplitDataArrayByTuple::OutputContainer::NewAttrMatrix)
    {
      const auto& newAttrMatrixTupleShape = pNewAttrMatrixTupleShape[0];
      for(usize j = 0; j < newAttrMatrixTupleShape.size(); ++j)
      {
        if(newAttrMatrixTupleShape[j] <= 0)
        {
          return {MakeErrorResult<OutputActions>(-65404,
                                                 fmt::format("Attribute matrix tuple shape contains \"{}\" at Tuple Dim {}.  All tuple shape values must be >= 1.", newAttrMatrixTupleShape[j], j))};
        }
      }
    }

    // Outputting to attribute matrix
    auto newAttrMatrixTupleShape = std::vector<usize>(pNewAttrMatrixTupleShape[0].begin(), pNewAttrMatrixTupleShape[0].end());
    auto result = preflightAttrMatrixOutput(pOutputContainer, pInputArrayPath, pNewAttrMatrixPath, pExistingAttrMatrixPath, inputArray->getTupleShape(), newAttrMatrixTupleShape, dataStructure,
                                            arrayPaths, tupleShapes, preflightUpdatedValues);
    if(result.invalid())
    {
      return {ConvertResultTo<OutputActions>(std::move(result), {})};
    }

    if(pOutputContainer == SplitDataArrayByTuple::OutputContainer::NewAttrMatrix)
    {
      resultOutputActions.value().appendAction(std::make_unique<CreateAttributeMatrixAction>(pNewAttrMatrixPath, tupleShapes[0]));
    }
  }

  // Create the split arrays
  for(usize i = 0; i < arrayPaths.size(); ++i)
  {
    switch(inputArray->getArrayType())
    {
    case IArray::ArrayType::DataArray: {
      auto iInputDataArray = dynamic_cast<const IDataArray*>(inputArray);
      std::vector<usize> cDims = iInputDataArray->getComponentShape();
      resultOutputActions.value().appendAction(std::make_unique<CreateArrayAction>(iInputDataArray->getDataType(), tupleShapes[i], cDims, arrayPaths[i]));
      break;
    }
    case IArray::ArrayType::NeighborListArray: {
      auto iInputNeighborList = dynamic_cast<const INeighborList*>(inputArray);
      usize tupleCount = std::accumulate(tupleShapes[i].begin(), tupleShapes[i].end(), 1, std::multiplies<>());
      resultOutputActions.value().appendAction(std::make_unique<CreateNeighborListAction>(iInputNeighborList->getDataType(), tupleCount, arrayPaths[i]));
      break;
    }
    case IArray::ArrayType::StringArray: {
      resultOutputActions.value().appendAction(std::make_unique<CreateStringArrayAction>(tupleShapes[i], arrayPaths[i]));
      break;
    }
    case IArray::ArrayType::Any: {
      return {MakeErrorResult<OutputActions>(-65408,
                                             fmt::format("The input array '{}' has array type 'Any'.  This SHOULD NOT be possible, so please contact the developers.", pInputArrayPath.toString()))};
    }
    default: {
      return {MakeErrorResult<OutputActions>(
          -65409, fmt::format("The input array '{}' has an array type that is currently not supported by this filter, so please contact the developers.", pInputArrayPath.toString()))};
    }
    }
  }

  // Remove the original input array
  if(pRemoveOriginal)
  {
    resultOutputActions.value().appendDeferredAction(std::make_unique<DeleteDataAction>(pInputArrayPath, DeleteDataAction::DeleteType::JustObject));
  }

  s_HeaderCache[m_InstanceId].outputArrayPaths = arrayPaths;
  s_HeaderCache[m_InstanceId].outputTupleShapes = tupleShapes;

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> SplitDataArrayByTupleFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                  const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  SplitDataArrayByTupleInputValues inputValues;
  inputValues.InputArrayPath = filterArgs.value<ArraySelectionParameter::ValueType>(k_DataArrayPath_Key);
  inputValues.OutputArrayPaths = s_HeaderCache[m_InstanceId].outputArrayPaths;
  return SplitDataArrayByTuple(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace nx::core