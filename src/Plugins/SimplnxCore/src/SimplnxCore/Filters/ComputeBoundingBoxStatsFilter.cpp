#include "ComputeBoundingBoxStatsFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/ComputeBoundingBoxStats.hpp"

#include "simplnx/Common/TypeTraits.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "simplnx/Filter/Actions/CreateNeighborListAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

using namespace nx::core;

namespace
{
struct IsIntegerType
{
  template <typename T>
  bool operator()()
  {
    return std::is_integral_v<T> && !std::is_same_v<T, bool>;
  }
};

void CreateCompatibleArrays(Result<OutputActions>& resultOutputActions, const DataStructure& dataStructure, const Arguments& filterArgs, ShapeType tupleDims, const DataPath& outputAMPath)
{
  auto calculateLength = filterArgs.value<bool>(ComputeBoundingBoxStatsFilter::k_CalculateLength_Key);
  auto calculateMin = filterArgs.value<bool>(ComputeBoundingBoxStatsFilter::k_CalculateMin_Key);
  auto calculateMax = filterArgs.value<bool>(ComputeBoundingBoxStatsFilter::k_CalculateMax_Key);
  auto calculateMean = filterArgs.value<bool>(ComputeBoundingBoxStatsFilter::k_CalculateMean_Key);
  auto calculateMedian = filterArgs.value<bool>(ComputeBoundingBoxStatsFilter::k_CalculateMedian_Key);
  auto calculateMode = filterArgs.value<bool>(ComputeBoundingBoxStatsFilter::k_CalculateMode_Key);
  auto calculateStdDeviation = filterArgs.value<bool>(ComputeBoundingBoxStatsFilter::k_CalculateStandardDeviation_Key);
  auto calculateSummation = filterArgs.value<bool>(ComputeBoundingBoxStatsFilter::k_CalculateSummation_Key);
  auto calculateNumUniqueValuesValue = filterArgs.value<bool>(ComputeBoundingBoxStatsFilter::k_CalculateUniqueValues_Key);

  auto* inputArray = dataStructure.getDataAs<IDataArray>(filterArgs.value<DataPath>(ComputeBoundingBoxStatsFilter::k_InputArrayPath_Key));
  DataType dataType = inputArray->getDataType();

  {
    auto arrayPath = filterArgs.value<std::string>(ComputeBoundingBoxStatsFilter::k_BoundsHasDataName_Key);
    auto action = std::make_unique<CreateArrayAction>(DataType::boolean, tupleDims, std::vector<usize>{1}, outputAMPath.createChildPath(arrayPath));
    resultOutputActions.value().appendAction(std::move(action));
  }

  if(calculateLength)
  {
    auto arrayPath = filterArgs.value<std::string>(ComputeBoundingBoxStatsFilter::k_LengthName_Key);
    auto action = std::make_unique<CreateArrayAction>(DataType::uint64, tupleDims, std::vector<usize>{1}, outputAMPath.createChildPath(arrayPath));
    resultOutputActions.value().appendAction(std::move(action));
  }
  if(calculateMin)
  {
    auto arrayPath = filterArgs.value<std::string>(ComputeBoundingBoxStatsFilter::k_MinName_Key);
    auto action = std::make_unique<CreateArrayAction>(dataType, tupleDims, std::vector<usize>{1}, outputAMPath.createChildPath(arrayPath));
    resultOutputActions.value().appendAction(std::move(action));
  }
  if(calculateMax)
  {
    auto arrayPath = filterArgs.value<std::string>(ComputeBoundingBoxStatsFilter::k_MaxName_Key);
    auto action = std::make_unique<CreateArrayAction>(dataType, tupleDims, std::vector<usize>{1}, outputAMPath.createChildPath(arrayPath));
    resultOutputActions.value().appendAction(std::move(action));
  }
  if(calculateMean)
  {
    auto arrayPath = filterArgs.value<std::string>(ComputeBoundingBoxStatsFilter::k_MeanName_Key);
    auto action = std::make_unique<CreateArrayAction>(DataType::float32, tupleDims, std::vector<usize>{1}, outputAMPath.createChildPath(arrayPath));
    resultOutputActions.value().appendAction(std::move(action));
  }
  if(calculateMedian)
  {
    auto arrayPath = filterArgs.value<std::string>(ComputeBoundingBoxStatsFilter::k_MedianName_Key);
    auto action = std::make_unique<CreateArrayAction>(DataType::float32, tupleDims, std::vector<usize>{1}, outputAMPath.createChildPath(arrayPath));
    resultOutputActions.value().appendAction(std::move(action));
  }
  if(calculateMode)
  {
    auto arrayPath = filterArgs.value<std::string>(ComputeBoundingBoxStatsFilter::k_ModeName_Key);
    auto action = std::make_unique<CreateNeighborListAction>(dataType, tupleDims, outputAMPath.createChildPath(arrayPath));
    resultOutputActions.value().appendAction(std::move(action));
  }
  if(calculateStdDeviation)
  {
    auto arrayPath = filterArgs.value<std::string>(ComputeBoundingBoxStatsFilter::k_StdDevName_Key);
    auto action = std::make_unique<CreateArrayAction>(DataType::float32, tupleDims, std::vector<usize>{1}, outputAMPath.createChildPath(arrayPath));
    resultOutputActions.value().appendAction(std::move(action));
  }
  if(calculateSummation)
  {
    auto arrayPath = filterArgs.value<std::string>(ComputeBoundingBoxStatsFilter::k_SummationName_Key);
    auto action = std::make_unique<CreateArrayAction>(DataType::float32, tupleDims, std::vector<usize>{1}, outputAMPath.createChildPath(arrayPath));
    resultOutputActions.value().appendAction(std::move(action));
  }
  if(calculateNumUniqueValuesValue)
  {
    auto arrayPath = filterArgs.value<std::string>(ComputeBoundingBoxStatsFilter::k_NumUniqueValuesName_Key);
    auto action = std::make_unique<CreateArrayAction>(DataType::int32, tupleDims, std::vector<usize>{1}, outputAMPath.createChildPath(arrayPath));
    resultOutputActions.value().appendAction(std::move(action));
  }
}
} // namespace

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ComputeBoundingBoxStatsFilter::name() const
{
  return FilterTraits<ComputeBoundingBoxStatsFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ComputeBoundingBoxStatsFilter::className() const
{
  return FilterTraits<ComputeBoundingBoxStatsFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ComputeBoundingBoxStatsFilter::uuid() const
{
  return FilterTraits<ComputeBoundingBoxStatsFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ComputeBoundingBoxStatsFilter::humanName() const
{
  return "Compute Statistics Within Bounding Boxes";
}

//------------------------------------------------------------------------------
std::vector<std::string> ComputeBoundingBoxStatsFilter::defaultTags() const
{
  return {className(), "Statistics"};
}

//------------------------------------------------------------------------------
Parameters ComputeBoundingBoxStatsFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_GeometryPath_Key, "Selected Image Geometry", "The DataPath to the Geometry that contains the points/edges/faces for the geometry",
                                                             DataPath{}, GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputArrayPath_Key, "Attribute Array to Compute Statistics", "Input Attribute Array for which to compute statistics", DataPath{},
                                                          nx::core::GetAllNumericTypes(), ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_UnifiedBoundsPath_Key, "Unified Bounding Boxes Array",
                                                          "The array containing the min and max point of the bounding box for each feature | tuple ordering {Min-X,Min-Y,Min-Z,Max-X,Max-Y,Max-Z}",
                                                          DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{6}}));

  params.insertSeparator(Parameters::Separator{"Output Destination"});
  params.insertLinkableParameter(
      std::make_unique<BoolParameter>(k_CreateNewAM_Key, "Create New AM For Statistics Arrays",
                                      "If true new Attribute Matrix will be created to store statistics, else you will be prompted to select existing AM matching unified bounds tuple count", false));
  params.insert(std::make_unique<AttributeMatrixSelectionParameter>(k_OutputAMPath_Key, "Output Statistics Attribute Matrix Path",
                                                                    "The output Attribute Matrix that the statistics arrays will be placed into. Must match unified bounds array tuple count.",
                                                                    DataPath{}));
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_NewAMName_Key, "Statistics Attribute Matrix Name", "The name of the created bounding box statistics Attribute Matrix", "Bounding Box Statistics"));

  params.insertSeparator(Parameters::Separator{"Output Arrays"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_BoundsHasDataName_Key, "Bounding-Box-Has-Data Array Name",
                                                          "The name of the boolean array that indicates whether or not each bounding box contains any data.  This array is especially useful to help "
                                                          "determine whether or not the outputted statistics are actually valid or not for a given bounding box.",
                                                          "BoundsHasData"));

  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_CalculateLength_Key, "Calculate Length", "Whether to compute the length of the input array", false));
  params.insert(std::make_unique<DataObjectNameParameter>(k_LengthName_Key, "Length Array Name", "The name of the length array", "Length"));

  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_CalculateMin_Key, "Calculate Minimum", "Whether to compute the minimum of the input array", false));
  params.insert(std::make_unique<DataObjectNameParameter>(k_MinName_Key, "Minimum Array Name", "The name of the minimum array", "Minimum"));

  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_CalculateMax_Key, "Calculate Maximum", "Whether to compute the maximum of the input array", false));
  params.insert(std::make_unique<DataObjectNameParameter>(k_MaxName_Key, "Maximum Array Name", "The name of the maximum array", "Maximum"));

  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_CalculateMean_Key, "Calculate Mean", "Whether to compute the arithmetic mean of the input array", false));
  params.insert(std::make_unique<DataObjectNameParameter>(k_MeanName_Key, "Mean Array Name", "The name of the mean array", "Mean"));

  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_CalculateMedian_Key, "Calculate Median", "Whether to compute the median of the input array", false));
  params.insert(std::make_unique<DataObjectNameParameter>(k_MedianName_Key, "Median Array Name", "The name of the median array", "Median"));

  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_CalculateMode_Key, "Calculate Mode", "Whether to compute the mode of the input array", false));
  params.insert(std::make_unique<DataObjectNameParameter>(k_ModeName_Key, "Mode Array Name", "The name of the mode array", "Mode"));

  params.insertLinkableParameter(
      std::make_unique<BoolParameter>(k_CalculateStandardDeviation_Key, "Calculate Standard Deviation", "Whether to compute the standard deviation of the input array", false));
  params.insert(std::make_unique<DataObjectNameParameter>(k_StdDevName_Key, "Standard Deviation Array Name", "The name of the standard deviation array", "StandardDeviation"));

  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_CalculateSummation_Key, "Calculate Summation", "Whether to compute the summation of the input array", false));
  params.insert(std::make_unique<DataObjectNameParameter>(k_SummationName_Key, "Summation Array Name", "The name of the summation array", "Summation"));

  params.insertLinkableParameter(
      std::make_unique<BoolParameter>(k_CalculateUniqueValues_Key, "Calculate Number of Unique Values", "Whether to compute the number of unique values in the input array", false));
  params.insert(std::make_unique<DataObjectNameParameter>(k_NumUniqueValuesName_Key, "Number of Unique Values Array Name", "The name of the array which stores the calculated number of unique values",
                                                          "Number of Unique Values Per Bound"));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_CreateNewAM_Key, k_NewAMName_Key, true);
  params.linkParameters(k_CreateNewAM_Key, k_OutputAMPath_Key, false);

  params.linkParameters(k_CalculateLength_Key, k_LengthName_Key, true);
  params.linkParameters(k_CalculateMin_Key, k_MinName_Key, true);
  params.linkParameters(k_CalculateMax_Key, k_MaxName_Key, true);
  params.linkParameters(k_CalculateMean_Key, k_MeanName_Key, true);
  params.linkParameters(k_CalculateMedian_Key, k_MedianName_Key, true);
  params.linkParameters(k_CalculateMode_Key, k_ModeName_Key, true);
  params.linkParameters(k_CalculateStandardDeviation_Key, k_StdDevName_Key, true);
  params.linkParameters(k_CalculateSummation_Key, k_SummationName_Key, true);
  params.linkParameters(k_CalculateUniqueValues_Key, k_NumUniqueValuesName_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ComputeBoundingBoxStatsFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ComputeBoundingBoxStatsFilter::clone() const
{
  return std::make_unique<ComputeBoundingBoxStatsFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ComputeBoundingBoxStatsFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                      const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto pCalculateLengthValue = filterArgs.value<bool>(k_CalculateLength_Key);
  auto pCalculateMinValue = filterArgs.value<bool>(k_CalculateMin_Key);
  auto pCalculateMaxValue = filterArgs.value<bool>(k_CalculateMax_Key);
  auto pCalculateMeanValue = filterArgs.value<bool>(k_CalculateMean_Key);
  auto pCalculateMedianValue = filterArgs.value<bool>(k_CalculateMedian_Key);
  auto pCalculateModeValue = filterArgs.value<bool>(k_CalculateMode_Key);
  auto pCalculateStdDeviationValue = filterArgs.value<bool>(k_CalculateStandardDeviation_Key);
  auto pCalculateSummationValue = filterArgs.value<bool>(k_CalculateSummation_Key);
  auto pCalculateNumUniqueValuesValue = filterArgs.value<bool>(k_CalculateUniqueValues_Key);

  auto pInputArrayPathValue = filterArgs.value<DataPath>(k_InputArrayPath_Key);
  auto pGeometryPathValue = filterArgs.value<DataPath>(k_GeometryPath_Key);
  auto pUnifiedBoundsPathValue = filterArgs.value<DataPath>(k_UnifiedBoundsPath_Key);

  auto pCreateNewAMValue = filterArgs.value<bool>(k_CreateNewAM_Key);

  FloatVec3 imageGeomSpacing = dataStructure.getDataAs<ImageGeom>(pGeometryPathValue)->getSpacing();

  if(std::count(imageGeomSpacing.begin(), imageGeomSpacing.end(), 0.0f) != 0)
  {
    return MakePreflightErrorResult(-59200, "Invalid Input: 0 detected in the image spacing array.");
  }

  Result<OutputActions> resultOutputActions;

  const auto* unifiedBoundsPtr = dataStructure.getDataAs<IDataArray>(pUnifiedBoundsPathValue);

  DataPath outputAMPath = {};

  if(pCreateNewAMValue)
  {
    outputAMPath = pGeometryPathValue.createChildPath(filterArgs.value<DataObjectNameParameter::ValueType>(k_NewAMName_Key));
    auto createAttributeMatrixAction = std::make_unique<CreateAttributeMatrixAction>(outputAMPath, unifiedBoundsPtr->getTupleShape());
    resultOutputActions.value().appendAction(std::move(createAttributeMatrixAction));
  }
  else
  {
    outputAMPath = filterArgs.value<DataPath>(k_OutputAMPath_Key);
    const auto* amPtr = dataStructure.getDataAs<AttributeMatrix>(outputAMPath);
    if(unifiedBoundsPtr->getNumberOfTuples() != amPtr->getNumberOfTuples())
    {
      return MakePreflightErrorResult(-59201, fmt::format("The number of tuples in the Unified Bounds Array ({}) does not match supplied Attribute Matrix tuple count ({})",
                                                          unifiedBoundsPtr->getNumberOfTuples(), amPtr->getNumberOfTuples()));
    }
  }

  if(!pCalculateMinValue && !pCalculateMaxValue && !pCalculateMeanValue && !pCalculateMedianValue && !pCalculateModeValue && !pCalculateStdDeviationValue && !pCalculateSummationValue &&
     !pCalculateLengthValue && !pCalculateNumUniqueValuesValue)
  {
    return MakePreflightErrorResult(-59202, "No statistics have been selected, so this filter will perform no operations");
  }

  const auto* inputArrayPtr = dataStructure.getDataAs<IDataArray>(pInputArrayPathValue);

  if(pCalculateModeValue && !ExecuteDataFunction(IsIntegerType{}, inputArrayPtr->getDataType()))
  {
    return MakePreflightErrorResult(-59203, "Calculating the mode requires selecting an input array with an integer data type (int8, uint8, int16, uint16, int32, uint32, int64, uint64).");
  }

  CreateCompatibleArrays(resultOutputActions, dataStructure, filterArgs, unifiedBoundsPtr->getTupleShape(), outputAMPath);

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ComputeBoundingBoxStatsFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                    const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  ComputeBoundingBoxStatsInputValues inputValues;

  inputValues.CalculateLength = filterArgs.value<bool>(k_CalculateLength_Key);
  inputValues.CalculateMin = filterArgs.value<bool>(k_CalculateMin_Key);
  inputValues.CalculateMax = filterArgs.value<bool>(k_CalculateMax_Key);
  inputValues.CalculateSummation = filterArgs.value<bool>(k_CalculateSummation_Key);
  inputValues.CalculateMean = filterArgs.value<bool>(k_CalculateMean_Key);
  inputValues.CalculateMedian = filterArgs.value<bool>(k_CalculateMedian_Key);
  inputValues.CalculateMode = filterArgs.value<bool>(k_CalculateMode_Key);
  inputValues.CalculateNumUniqueValues = filterArgs.value<bool>(k_CalculateUniqueValues_Key);
  inputValues.CalculateStdDev = filterArgs.value<bool>(k_CalculateStandardDeviation_Key);

  inputValues.GeometryPath = filterArgs.value<DataPath>(k_GeometryPath_Key);
  inputValues.UnifiedPath = filterArgs.value<DataPath>(k_UnifiedBoundsPath_Key);
  inputValues.InputPath = filterArgs.value<DataPath>(k_InputArrayPath_Key);

  DataPath outputAMPath = {};

  if(filterArgs.value<bool>(k_CreateNewAM_Key))
  {
    outputAMPath = inputValues.GeometryPath.createChildPath(filterArgs.value<DataObjectNameParameter::ValueType>(k_NewAMName_Key));
  }
  else
  {
    outputAMPath = filterArgs.value<DataPath>(k_OutputAMPath_Key);
  }

  inputValues.BoundsHasDataPath = outputAMPath.createChildPath(filterArgs.value<DataObjectNameParameter::ValueType>(k_BoundsHasDataName_Key));
  inputValues.LengthPath = outputAMPath.createChildPath(filterArgs.value<DataObjectNameParameter::ValueType>(k_LengthName_Key));
  inputValues.MinPath = outputAMPath.createChildPath(filterArgs.value<DataObjectNameParameter::ValueType>(k_MinName_Key));
  inputValues.MaxPath = outputAMPath.createChildPath(filterArgs.value<DataObjectNameParameter::ValueType>(k_MaxName_Key));
  inputValues.SummationPath = outputAMPath.createChildPath(filterArgs.value<DataObjectNameParameter::ValueType>(k_SummationName_Key));
  inputValues.MeanPath = outputAMPath.createChildPath(filterArgs.value<DataObjectNameParameter::ValueType>(k_MeanName_Key));
  inputValues.MedianPath = outputAMPath.createChildPath(filterArgs.value<DataObjectNameParameter::ValueType>(k_MedianName_Key));
  inputValues.ModePath = outputAMPath.createChildPath(filterArgs.value<DataObjectNameParameter::ValueType>(k_ModeName_Key));
  inputValues.NumUniqueValuesPath = outputAMPath.createChildPath(filterArgs.value<DataObjectNameParameter::ValueType>(k_NumUniqueValuesName_Key));
  inputValues.StdDevPath = outputAMPath.createChildPath(filterArgs.value<DataObjectNameParameter::ValueType>(k_StdDevName_Key));

  return ComputeBoundingBoxStats(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace nx::core
