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

template <typename T>
std::string valuesToString(const std::vector<T>& values, const std::string& token = " x ")
{
  std::vector<std::string> shapeStrs;
  std::transform(values.cbegin(), values.cend(), std::back_inserter(shapeStrs), [](T val) { return std::to_string(val); });
  std::vector<std::string_view> shapeStrViews(shapeStrs.begin(), shapeStrs.end());
  return StringUtilities::join(shapeStrViews, token);
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
      displayPaths.push_back(fmt::format("{} ({})", paths[i], valuesToString(tupleShapes[i])));
    }
    displayPaths.emplace_back("...");
    for(usize i = totalPaths - tailCount; i < totalPaths; ++i)
    {
      displayPaths.push_back(fmt::format("{} ({})", paths[i], valuesToString(tupleShapes[i])));
    }
  }
  else
  {
    for(usize i = 0; i < paths.size(); ++i)
    {
      displayPaths.push_back(fmt::format("{} ({})", paths[i], valuesToString(tupleShapes[i])));
    }
  }

  return displayPaths;
}

Result<usize> commonMultiplier(const std::vector<usize>& dividend, const std::vector<usize>& divisor, usize dim)
{
  // 1) Basic sanity
  if(dividend.empty() || divisor.empty())
  {
    return MakeErrorResult<usize>(-1000, "Dividend vector is empty.");
  }
  if(divisor.empty())
  {
    return MakeErrorResult<usize>(-1001, "Divisor vector is empty.");
  }
  if(dividend.size() != divisor.size())
  {
    return MakeErrorResult<usize>(-1002, fmt::format("Size mismatch: dividend has {} entries, divisor has {}", dividend.size(), divisor.size()));
  }
  if(dim >= dividend.size())
  {
    return MakeErrorResult<usize>(-1003, fmt::format("Dimension {} out of range [0, {})", dim, dividend.size()));
  }

  for(usize i = 0; i < dividend.size(); ++i)
  {
    if(i == dim)
    {
      continue;
    }
    if(dividend[i] != divisor[i])
    {
      return MakeErrorResult<usize>(-1004, fmt::format("This filter splits tuples along a single dimension only, so all other dimensions must remain unchanged.\n"
                                                       "Input tuple shape    = {}\n"
                                                       "Requested tuple shape = {}\n"
                                                       "Chosen Split Dimension = {}\n"
                                                       "Mismatch at dimension {}: got {} but expected {}.",
                                                       valuesToString(dividend), valuesToString(divisor), dim, i, dividend[i], divisor[i]));
    }
  }

  const auto d = divisor[dim];
  if(d == 0)
  {
    return MakeErrorResult<usize>(-1005, fmt::format("Divisor at dimension {} is zero", dim));
  }

  const auto v = dividend[dim];
  if(v % d != 0)
  {
    return MakeErrorResult<usize>(-1006, fmt::format("Value {} at dimension {} is not divisible by {}", v, dim, d));
  }

  return {v / d};
}

Result<> preflightDataGroupOutput(SplitDataArrayByTuple::OutputContainer outputContainer, const DataPath& inputArrayPath, const DataPath& newDataGroupPath, const DataPath& existingDataGroupPath,
                                  const std::vector<usize>& inputArrayTupleShape, const std::vector<std::vector<float64>> splitDimensionCounts, usize splitDimension,
                                  std::vector<std::vector<usize>>& splitArrayTupleShapes, std::vector<DataPath>& arrayPaths, std::vector<IFilter::PreflightValue>& preflightUpdatedValues)
{
  if(splitDimension >= inputArrayTupleShape.size())
  {
    return {MakeErrorResult(to_underlying(SplitDataArrayByTuple::ErrorCodes::SplitDimOutOfRange),
                            fmt::format("The chosen split dimension ({}) is out of range of the input array's tuple shape rank (0-{}).  Please choose a dimension within this range.", splitDimension,
                                        inputArrayTupleShape.size() - 1))};
  }

  splitArrayTupleShapes = std::vector<std::vector<usize>>(splitDimensionCounts.size(), inputArrayTupleShape);
  std::vector<usize> splitDimensionCountsUSize;
  splitDimensionCountsUSize.reserve(splitDimensionCounts.size());

  usize splitCountsTotal = 0;
  std::string splitCountsStr;
  for(usize i = 0; i < splitDimensionCounts.size(); ++i)
  {
    const auto& row = splitDimensionCounts[i];
    if(row.size() > 1)
    {
      return {MakeErrorResult(
          to_underlying(SplitDataArrayByTuple::ErrorCodes::MultiDimensionalSplitCount),
          fmt::format("Split Array {} contains a multi-dimensional split dimension count ({}).  The split dimension count should be a single dimension.", i, valuesToString(row, "x")))};
    }

    const auto& splitDimensionCount = splitDimensionCounts[i][0];
    if(splitDimensionCount <= 0)
    {
      return {MakeErrorResult(to_underlying(SplitDataArrayByTuple::ErrorCodes::SplitCountLessThanZero),
                              fmt::format("Split Array {} contains \"{}\" at Tuple Dim {}.  All tuple shape values must be >= 1.", i, splitDimensionCount, splitDimension))};
    }
    auto splitCount = static_cast<usize>(splitDimensionCount);
    splitArrayTupleShapes[i][splitDimension] = splitCount;
    splitCountsTotal += splitCount;
    splitDimensionCountsUSize.push_back(splitCount);
  }

  if(splitCountsTotal != inputArrayTupleShape[splitDimension])
  {
    return {MakeErrorResult(to_underlying(SplitDataArrayByTuple::ErrorCodes::SplitCountSumNotEqual),
                            fmt::format("The sum of your Split Dimension Counts ({} = {}) does not equal the input array's tuple count for dimension {} ({}).",
                                        valuesToString(splitDimensionCountsUSize, "+"), splitCountsTotal, splitDimension, inputArrayTupleShape[splitDimension]))};
  }

  arrayPaths.reserve(splitArrayTupleShapes.size());
  ShapeType tupleShapeColSums(splitArrayTupleShapes[0].size(), 0);
  std::string tupleShapesOutputStr;
  for(usize i = 0; i < splitArrayTupleShapes.size(); i++)
  {
    const auto& splitArrayTupleShape = splitArrayTupleShapes[i];

    std::string arrayName = inputArrayPath.getTargetName() + "_" + StringUtilities::GenerateIndexString(static_cast<int32>(i) + 1, static_cast<int32>(splitArrayTupleShapes.size()));
    if(outputContainer == SplitDataArrayByTuple::OutputContainer::NewDataGroup)
    {
      arrayPaths.push_back(newDataGroupPath.createChildPath(arrayName));
    }
    else
    {
      arrayPaths.push_back(existingDataGroupPath.createChildPath(arrayName));
    }

    // Build up the tuple shape string and add it to the overall tuple shapes output string
    tupleShapesOutputStr += fmt::format("({})\n", valuesToString(splitArrayTupleShape));
  }
  tupleShapesOutputStr.pop_back(); // Remove unneeded newline character

  // Output array paths to preflight updated values
  auto arrayPathsStrs = dataPathsToStrings(arrayPaths);
  std::vector<std::string> displayPaths = createDisplayPaths(arrayPathsStrs, splitArrayTupleShapes);
  std::vector<std::string_view> displayPathsViews(displayPaths.begin(), displayPaths.end());
  preflightUpdatedValues.push_back({fmt::format("Created Split Arrays ({}): ", displayPaths.size()), fmt::format("{}", StringUtilities::join(displayPathsViews, "\n"))});
  return {};
}

Result<> preflightAttrMatrixOutput(SplitDataArrayByTuple::OutputContainer outputContainer, const DataPath& inputArrayPath, const DataPath& newAttrMatrixPath, const DataPath& existingAttrMatrixPath,
                                   const std::vector<usize>& inputArrayTupleShape, const std::vector<float64>& newAttrMatrixTupleShape, usize splitDimension, const DataStructure& dataStructure,
                                   std::vector<DataPath>& arrayPaths, std::vector<std::vector<usize>>& tupleShapes, std::vector<IFilter::PreflightValue>& preflightUpdatedValues)
{
  ShapeType tupleShape;
  if(outputContainer == SplitDataArrayByTuple::OutputContainer::NewAttrMatrix)
  {
    for(usize j = 0; j < newAttrMatrixTupleShape.size(); ++j)
    {
      if(newAttrMatrixTupleShape[j] <= 0)
      {
        return {MakeErrorResult(to_underlying(SplitDataArrayByTuple::ErrorCodes::AttrMatrixTupleShapeNegative),
                                fmt::format("Attribute matrix tuple shape contains \"{}\" at Tuple Dim {}.  All tuple shape values must be >= 1.", newAttrMatrixTupleShape[j], j))};
      }
    }
    tupleShape.resize(newAttrMatrixTupleShape.size());
    for(size_t i = 0; i < newAttrMatrixTupleShape.size(); i++)
    {
      tupleShape[i] = static_cast<usize>(newAttrMatrixTupleShape[i]);
    }
  }
  else
  {
    const auto& existingAttrMatrix = dataStructure.getDataRefAs<AttributeMatrix>(existingAttrMatrixPath);
    tupleShape = existingAttrMatrix.getShape();
  }

  auto result = commonMultiplier(inputArrayTupleShape, tupleShape, static_cast<usize>(splitDimension));
  if(result.invalid())
  {
    return {MakeErrorResult(to_underlying(SplitDataArrayByTuple::ErrorCodes::AttrMatrixTupleShapeNoCommonMultiplier),
                            fmt::format("The selected tuple shape ({0}) cannot cleanly split the input array '{1}' tuple shape ({2}) along dimension {3}.\n\n"
                                        "No integer multiplier applied to dimension {3} of the selected tuple shape ({4}) will produce the corresponding value in the input array's tuple shape ({5}).",
                                        valuesToString(tupleShape), inputArrayPath.toString(), valuesToString(inputArrayTupleShape), splitDimension, tupleShape[splitDimension],
                                        inputArrayTupleShape[splitDimension]))};
  }
  usize numOfAttrMatrixSplitArrays = result.value();

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

  params.insert(std::make_unique<NumberParameter<uint64>>(k_SplitDimension_Key, "Split Dimension", "The tuple shape dimension to split the arrays from the input data array.", 0));
  {
    DynamicTableInfo tableInfo;
    tableInfo.setRowsInfo(DynamicTableInfo::DynamicVectorInfo(2, "Split Data Array {}"));
    tableInfo.setColsInfo(DynamicTableInfo::StaticVectorInfo({"Split Dimension Count"}));
    params.insert(std::make_unique<DynamicTableParameter>(k_SplitDimensionCounts_Key, "Split Arrays: Split Dimension Counts",
                                                          "How many tuples each output array should contain along the split "
                                                          "dimension only.\n\n"
                                                          "• Enter one value per row; each row becomes a new split array.\n"
                                                          "• The sum of all rows **must equal** the number of tuples in the "
                                                          "input array’s split dimension.\n\n"
                                                          "Example: If the input array’s tuple shape is `(100 × 200 × 300)` and the "
                                                          "filter is splitting along the **first** dimension (100), then Split Dimension Counts "
                                                          "of `60 | 25 | 15` creates three output arrays whose tuple shapes are "
                                                          "`(60 × 200 × 300)`, `(25 × 200 × 300)`, and `(15 × 200 × 300)` "
                                                          "respectively.",
                                                          tableInfo));
  }

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
  params.linkParameters(k_OutputContainer, k_SplitDimensionCounts_Key, static_cast<ChoicesParameter::ValueType>(SplitDataArrayByTuple::OutputContainer::NewDataGroup));
  params.linkParameters(k_OutputContainer, k_ExistingDataGroupPath, static_cast<ChoicesParameter::ValueType>(SplitDataArrayByTuple::OutputContainer::ExistingDataGroup));
  params.linkParameters(k_OutputContainer, k_SplitDimensionCounts_Key, static_cast<ChoicesParameter::ValueType>(SplitDataArrayByTuple::OutputContainer::ExistingDataGroup));
  params.linkParameters(k_OutputContainer, k_NewAttributeMatrixPath, static_cast<ChoicesParameter::ValueType>(SplitDataArrayByTuple::OutputContainer::NewAttrMatrix));
  params.linkParameters(k_OutputContainer, k_AttrMatrixTupleShape_Key, static_cast<ChoicesParameter::ValueType>(SplitDataArrayByTuple::OutputContainer::NewAttrMatrix));
  params.linkParameters(k_OutputContainer, k_ExistingAttributeMatrixPath, static_cast<ChoicesParameter::ValueType>(SplitDataArrayByTuple::OutputContainer::ExistingAttrMatrix));

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
  auto pSplitDimension = filterArgs.value<NumberParameter<uint64>::ValueType>(k_SplitDimension_Key);
  auto pSplitDimensionCounts = filterArgs.value<DynamicTableParameter::ValueType>(k_SplitDimensionCounts_Key);
  auto pOutputContainer = static_cast<SplitDataArrayByTuple::OutputContainer>(filterArgs.value<ChoicesParameter::ValueType>(k_OutputContainer));
  auto pNewDataGroupPath = filterArgs.value<DataGroupCreationParameter::ValueType>(k_NewDataGroupPath);
  auto pExistingDataGroupPath = filterArgs.value<DataGroupSelectionParameter::ValueType>(k_ExistingDataGroupPath);
  auto pNewAttrMatrixPath = filterArgs.value<DataGroupCreationParameter::ValueType>(k_NewAttributeMatrixPath);
  auto pExistingAttrMatrixPath = filterArgs.value<AttributeMatrixSelectionParameter::ValueType>(k_ExistingAttributeMatrixPath);
  auto pNewAttrMatrixTupleShape = filterArgs.value<DynamicTableParameter::ValueType>(k_AttrMatrixTupleShape_Key);

  PreflightResult preflightResult;
  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  auto* inputArray = dataStructure.getDataAs<IArray>(pInputArrayPath);
  if(inputArray == nullptr)
  {
    return {MakeErrorResult<OutputActions>(to_underlying(SplitDataArrayByTuple::ErrorCodes::NoInputArray), fmt::format("Cannot find input array at path '{}'", pInputArrayPath.toString()))};
  }

  // Output input array's tuple shape to preflight updated values
  std::vector<std::string> displayPaths = createDisplayPaths({pInputArrayPath.toString()}, {inputArray->getTupleShape()});
  std::vector<std::string_view> displayPathsViews(displayPaths.begin(), displayPaths.end());
  preflightUpdatedValues.push_back({"Input Array", StringUtilities::join(displayPathsViews, "\n")});

  std::vector<DataPath> arrayPaths;
  std::vector<std::vector<usize>> tupleShapes;

  if(pSplitDimension < 0)
  {
    return {MakeErrorResult<OutputActions>(to_underlying(SplitDataArrayByTuple::ErrorCodes::SplitDimLessThanZero),
                                           fmt::format("The chosen split dimension ({}) is less than 0.  Please choose a non-negative number.", pSplitDimension))};
  }

  auto splitDimension = static_cast<usize>(pSplitDimension);

  if(pOutputContainer == SplitDataArrayByTuple::OutputContainer::NewDataGroup || pOutputContainer == SplitDataArrayByTuple::OutputContainer::ExistingDataGroup)
  {
    // Outputting to data group
    auto result = preflightDataGroupOutput(pOutputContainer, pInputArrayPath, pNewDataGroupPath, pExistingDataGroupPath, inputArray->getTupleShape(), pSplitDimensionCounts, splitDimension,
                                           tupleShapes, arrayPaths, preflightUpdatedValues);
    if(result.invalid())
    {
      return {ConvertResultTo<OutputActions>(std::move(result), {}), preflightUpdatedValues};
    }

    if(pOutputContainer == SplitDataArrayByTuple::OutputContainer::NewDataGroup)
    {
      resultOutputActions.value().appendAction(std::make_unique<CreateDataGroupAction>(pNewDataGroupPath));
    }
  }
  else
  {
    // Outputting to attribute matrix
    auto result = preflightAttrMatrixOutput(pOutputContainer, pInputArrayPath, pNewAttrMatrixPath, pExistingAttrMatrixPath, inputArray->getTupleShape(), pNewAttrMatrixTupleShape[0], splitDimension,
                                            dataStructure, arrayPaths, tupleShapes, preflightUpdatedValues);
    if(result.invalid())
    {
      return {ConvertResultTo<OutputActions>(std::move(result), {}), preflightUpdatedValues};
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
      ShapeType cDims = iInputDataArray->getComponentShape();
      resultOutputActions.value().appendAction(std::make_unique<CreateArrayAction>(iInputDataArray->getDataType(), tupleShapes[i], cDims, arrayPaths[i]));
      break;
    }
    case IArray::ArrayType::NeighborListArray: {
      auto iInputNeighborList = dynamic_cast<const INeighborList*>(inputArray);
      resultOutputActions.value().appendAction(std::make_unique<CreateNeighborListAction>(iInputNeighborList->getDataType(), tupleShapes[i], arrayPaths[i]));
      break;
    }
    case IArray::ArrayType::StringArray: {
      resultOutputActions.value().appendAction(std::make_unique<CreateStringArrayAction>(tupleShapes[i], arrayPaths[i]));
      break;
    }
    case IArray::ArrayType::Any: {
      return {MakeErrorResult<OutputActions>(to_underlying(SplitDataArrayByTuple::ErrorCodes::AnyArrayType),
                                             fmt::format("The input array '{}' has array type 'Any'.  This SHOULD NOT be possible, so please contact the developers.", pInputArrayPath.toString())),
              preflightUpdatedValues};
    }
    default: {
      return {MakeErrorResult<OutputActions>(
                  to_underlying(SplitDataArrayByTuple::ErrorCodes::UnsupportedArrayType),
                  fmt::format("The input array '{}' has an array type that is currently not supported by this filter, so please contact the developers.", pInputArrayPath.toString())),
              preflightUpdatedValues};
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
  inputValues.SplitDimension = filterArgs.value<NumberParameter<uint64>::ValueType>(k_SplitDimension_Key);
  return SplitDataArrayByTuple(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace nx::core