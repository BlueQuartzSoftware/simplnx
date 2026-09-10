#include "PottsModelFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/PottsModel.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <algorithm>
#include <chrono>
#include <random>

using namespace nx::core;

namespace
{
constexpr int32 k_InvalidIterationsError = -72000;
constexpr int32 k_InvalidTemperatureError = -72001;
constexpr int32 k_InvalidFeatureIdsLocationError = -72002;
constexpr int32 k_InvalidMaskLocationError = -72003;
constexpr int32 k_MaskTupleCountMismatchError = -72004;
constexpr int32 k_InvalidDimensionalityError = -72006;
} // namespace

namespace nx::core
{
//------------------------------------------------------------------------------
std::string PottsModelFilter::name() const
{
  return FilterTraits<PottsModelFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string PottsModelFilter::className() const
{
  return FilterTraits<PottsModelFilter>::className;
}

//------------------------------------------------------------------------------
Uuid PottsModelFilter::uuid() const
{
  return FilterTraits<PottsModelFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string PottsModelFilter::humanName() const
{
  return "Potts Model";
}

//------------------------------------------------------------------------------
std::vector<std::string> PottsModelFilter::defaultTags() const
{
  return {className(), "Coarsening", "Monte Carlo", "Grain Growth", "Synthetic"};
}

//------------------------------------------------------------------------------
Parameters PottsModelFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<Int32Parameter>(k_Iterations_Key, "Iterations", "Number of Monte Carlo iterations. The value must be greater than zero.", 100));
  params.insert(std::make_unique<Float64Parameter>(k_Temperature_Key, "Temperature", "Simulation temperature in kelvin. The value must be greater than zero.", 273.0));
  params.insert(std::make_unique<BoolParameter>(k_PeriodicBoundaries_Key, "Periodic Boundaries", "Whether the image boundaries wrap to the opposite side.", false));

  params.insertSeparator(Parameters::Separator{"Random Number Seed Parameters"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(
      k_UseSeed_Key, "Use Seed for Random Generation",
      "When true, the supplied seed initializes the random generator. A fixed seed makes the run reproducible on the same platform but does not bit-match legacy DREAM3D output.", false));
  params.insert(std::make_unique<NumberParameter<uint64>>(k_SeedValue_Key, "Seed Value", "Seed that initializes the random generator.", std::mt19937::default_seed));
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_SeedArrayName_Key, "Stored Seed Value Array Name", "Name of the array that stores the seed used by this execution.", "PottsModel SeedValue"));

  params.insertSeparator(Parameters::Separator{"Optional Parameters"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseMask_Key, "Use Mask", "Whether to restrict site selection and neighborhoods to mask-true cells.", false));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskArrayPath_Key, "Mask", "Boolean or uint8 mask that selects participating cells.", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::boolean, DataType::uint8}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature IDs", "Int32 cell array that contains the spin IDs. The filter modifies this array in place.", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.linkParameters(k_UseSeed_Key, k_SeedValue_Key, true);
  params.linkParameters(k_UseMask_Key, k_MaskArrayPath_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType PottsModelFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer PottsModelFilter::clone() const
{
  return std::make_unique<PottsModelFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult PottsModelFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                                         const ExecutionContext& executionContext) const
{
  const auto iterations = filterArgs.value<int32>(k_Iterations_Key);
  const auto temperature = filterArgs.value<float64>(k_Temperature_Key);
  const auto useMask = filterArgs.value<bool>(k_UseMask_Key);
  const auto maskArrayPath = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  const auto featureIdsArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  const auto seedArrayName = filterArgs.value<std::string>(k_SeedArrayName_Key);

  if(iterations <= 0)
  {
    return MakePreflightErrorResult(k_InvalidIterationsError, fmt::format("The 'Iterations' parameter must be greater than zero. The current value is {}.", iterations));
  }
  if(temperature <= 0.0)
  {
    return MakePreflightErrorResult(k_InvalidTemperatureError, fmt::format("The 'Temperature' parameter must be greater than zero kelvin. The current value is {}.", temperature));
  }

  const DataPath cellAttributeMatrixPath = featureIdsArrayPath.getParent();
  const DataPath imageGeometryPath = cellAttributeMatrixPath.getParent();
  const auto* imageGeometry = dataStructure.getDataAs<ImageGeom>(imageGeometryPath);
  if(imageGeometry == nullptr || imageGeometry->getCellDataPath() != cellAttributeMatrixPath)
  {
    return MakePreflightErrorResult(k_InvalidFeatureIdsLocationError,
                                    fmt::format("The 'Feature IDs' array at path '{}' must be in the cell attribute matrix of an image geometry.", featureIdsArrayPath.toString()));
  }

  const auto imageDimensions = imageGeometry->getDimensions().toArray();
  const usize singletonDimensionCount = std::count(imageDimensions.cbegin(), imageDimensions.cend(), 1);
  if(singletonDimensionCount > 1)
  {
    return MakePreflightErrorResult(k_InvalidDimensionalityError,
                                    fmt::format("The image geometry for the 'Feature IDs' array at path '{}' must be two-dimensional or three-dimensional. Its dimensions are [{}, {}, {}].",
                                                featureIdsArrayPath.toString(), imageDimensions[0], imageDimensions[1], imageDimensions[2]));
  }

  const auto& featureIds = dataStructure.getDataRefAs<Int32Array>(featureIdsArrayPath);
  if(useMask)
  {
    const DataPath maskAttributeMatrixPath = maskArrayPath.getParent();
    const auto* maskImageGeometry = dataStructure.getDataAs<ImageGeom>(maskAttributeMatrixPath.getParent());
    if(maskImageGeometry == nullptr || maskImageGeometry->getCellDataPath() != maskAttributeMatrixPath)
    {
      return MakePreflightErrorResult(k_InvalidMaskLocationError, fmt::format("The 'Mask' array at path '{}' must be in the cell attribute matrix of an image geometry.", maskArrayPath.toString()));
    }

    const auto& mask = dataStructure.getDataRefAs<IDataArray>(maskArrayPath);
    if(mask.getNumberOfTuples() != featureIds.getNumberOfTuples())
    {
      return MakePreflightErrorResult(k_MaskTupleCountMismatchError,
                                      fmt::format("The 'Mask' array at path '{}' has {} tuples, but the 'Feature IDs' array at path '{}' has {} tuples. The tuple counts must match.",
                                                  maskArrayPath.toString(), mask.getNumberOfTuples(), featureIdsArrayPath.toString(), featureIds.getNumberOfTuples()));
    }
  }

  Result<OutputActions> resultOutputActions;
  resultOutputActions.value().appendAction(std::make_unique<CreateArrayAction>(DataType::uint64, std::vector<usize>{1}, std::vector<usize>{1}, DataPath({seedArrayName})));
  MarkDataPathModified(dataStructure, resultOutputActions, featureIdsArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> PottsModelFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                       const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto seed = filterArgs.value<uint64>(k_SeedValue_Key);
  if(!filterArgs.value<bool>(k_UseSeed_Key))
  {
    seed = static_cast<std::mt19937_64::result_type>(std::chrono::steady_clock::now().time_since_epoch().count());
  }

  dataStructure.getDataRefAs<UInt64Array>(DataPath({filterArgs.value<std::string>(k_SeedArrayName_Key)}))[0] = seed;

  PottsModelInputValues inputValues;
  inputValues.Iterations = filterArgs.value<int32>(k_Iterations_Key);
  inputValues.Temperature = filterArgs.value<float64>(k_Temperature_Key);
  inputValues.PeriodicBoundaries = filterArgs.value<bool>(k_PeriodicBoundaries_Key);
  inputValues.UseMask = filterArgs.value<bool>(k_UseMask_Key);
  inputValues.MaskArrayPath = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.SeedValue = seed;

  return PottsModel(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_IterationsKey = "Iterations";
constexpr StringLiteral k_TemperatureKey = "Temperature";
constexpr StringLiteral k_PeriodicBoundariesKey = "PeriodicBoundaries";
constexpr StringLiteral k_UseMaskKey = "UseMask";
constexpr StringLiteral k_MaskArrayPathKey = "MaskArrayPath";
constexpr StringLiteral k_FeatureIdsArrayPathKey = "FeatureIdsArrayPath";
} // namespace SIMPL
} // namespace

Result<Arguments> PottsModelFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = PottsModelFilter().getDefaultArguments();

  std::vector<Result<>> results;
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::IntFilterParameterConverter<int32>>(args, json, SIMPL::k_IterationsKey, k_Iterations_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DoubleFilterParameterConverter>(args, json, SIMPL::k_TemperatureKey, k_Temperature_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_PeriodicBoundariesKey, k_PeriodicBoundaries_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedBooleanFilterParameterConverter>(args, json, SIMPL::k_UseMaskKey, k_UseMask_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_MaskArrayPathKey, k_MaskArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_FeatureIdsArrayPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));
  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
