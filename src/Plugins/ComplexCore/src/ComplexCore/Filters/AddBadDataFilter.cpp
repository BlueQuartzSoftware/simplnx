#include "AddBadDataFilter.hpp"

#include "ComplexCore/Filters/Algorithms/AddBadData.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include <random>

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string AddBadDataFilter::name() const
{
  return FilterTraits<AddBadDataFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string AddBadDataFilter::className() const
{
  return FilterTraits<AddBadDataFilter>::className;
}

//------------------------------------------------------------------------------
Uuid AddBadDataFilter::uuid() const
{
  return FilterTraits<AddBadDataFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string AddBadDataFilter::humanName() const
{
  return "Add Bad Data";
}

//------------------------------------------------------------------------------
std::vector<std::string> AddBadDataFilter::defaultTags() const
{
  return {className(), "Synthetic Building", "Misc"};
}

//------------------------------------------------------------------------------
Parameters AddBadDataFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Seeded Randomness"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseSeed_Key, "Use Seed for Random Generation", "When true the user will be able to put in a seed for random generation", false));
  params.insert(std::make_unique<NumberParameter<uint64>>(k_SeedValue_Key, "Seed Value", "The seed fed into the random generator", std::mt19937::default_seed));
  params.insert(std::make_unique<DataObjectNameParameter>(k_SeedArrayName_Key, "Stored Seed Value Array Name", "Name of array holding the seed value", "AddBadData SeedValue"));

  params.insertSeparator(Parameters::Separator{"Optional Variables"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_PoissonNoise_Key, "Add Random Noise", "If true the user may set the poisson volume fraction", false));
  params.insert(std::make_unique<Float32Parameter>(k_PoissonVolFraction_Key, "Volume Fraction of Random Noise", "A value between 0 and 1 inclusive that is compared against random generation", 0.0f));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_BoundaryNoise_Key, "Add Boundary Noise", "If true the user may set the boundary volume fraction", false));
  params.insert(
      std::make_unique<Float32Parameter>(k_BoundaryVolFraction_Key, "Volume Fraction of Boundary Noise", "A value between 0 and 1 inclusive that is compared against random generation", 0.0f));

  params.insertSeparator(Parameters::Separator{"Required Objects"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_ImageGeometryPath_Key, "Image Geometry", "The selected image geometry", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_GBEuclideanDistancesArrayPath_Key, "Boundary Euclidean Distances", "This is the GB Manhattan Distances array", DataPath{},
                                                          std::set<DataType>{DataType::int32}));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_PoissonNoise_Key, k_PoissonVolFraction_Key, true);
  params.linkParameters(k_BoundaryNoise_Key, k_BoundaryVolFraction_Key, true);
  params.linkParameters(k_UseSeed_Key, k_SeedValue_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer AddBadDataFilter::clone() const
{
  return std::make_unique<AddBadDataFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult AddBadDataFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                         const std::atomic_bool& shouldCancel) const
{
  auto pPoissonNoiseValue = filterArgs.value<bool>(k_PoissonNoise_Key);
  auto pPoissonVolFractionValue = filterArgs.value<float32>(k_PoissonVolFraction_Key);
  auto pBoundaryNoiseValue = filterArgs.value<bool>(k_BoundaryNoise_Key);
  auto pBoundaryVolFractionValue = filterArgs.value<float32>(k_BoundaryVolFraction_Key);
  auto pGBEuclideanDistancesArrayPathValue = filterArgs.value<DataPath>(k_GBEuclideanDistancesArrayPath_Key);
  auto pImageGeometryPathValue = filterArgs.value<DataPath>(k_ImageGeometryPath_Key);
  auto pSeedArrayNameValue = filterArgs.value<std::string>(k_SeedArrayName_Key);

  PreflightResult preflightResult;
  complex::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  if(!pBoundaryNoiseValue && !pPoissonNoiseValue)
  {
    return {MakeErrorResult<OutputActions>(-76234, fmt::format("At least one type of noise must be selected"))};
  }

  if(pPoissonVolFractionValue > 1.0 && pPoissonVolFractionValue < 0.0)
  {
    return {MakeErrorResult<OutputActions>(-76235, fmt::format("Value must be between 0-1 inclusive. You selected: {}", pPoissonVolFractionValue))};
  }

  if(pBoundaryVolFractionValue > 1.0 && pBoundaryVolFractionValue < 0.0)
  {
    return {MakeErrorResult<OutputActions>(-76236, fmt::format("Value must be between 0-1 inclusive. You selected: {}", pBoundaryVolFractionValue))};
  }

  auto* imgGeomPtr = dataStructure.getDataAs<ImageGeom>(pImageGeometryPathValue);
  auto* cellAM = imgGeomPtr->getCellData();
  if(cellAM == nullptr)
  {
    return {MakeErrorResult<OutputActions>(-76237, fmt::format("Image geometry must have a valid cell Attribute Matrix"))};
  }

  {
    auto createAction = std::make_unique<CreateArrayAction>(DataType::uint64, std::vector<usize>{1}, std::vector<usize>{1}, DataPath({pSeedArrayNameValue}));
    resultOutputActions.value().appendAction(std::move(createAction));
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> AddBadDataFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                       const std::atomic_bool& shouldCancel) const
{
  auto seed = filterArgs.value<uint64>(k_SeedValue_Key);
  if(!filterArgs.value<bool>(k_UseSeed_Key))
  {
    seed = static_cast<std::mt19937_64::result_type>(std::chrono::steady_clock::now().time_since_epoch().count());
  }

  // Store Seed Value in Top Level Array
  dataStructure.getDataRefAs<UInt64Array>(DataPath({filterArgs.value<std::string>(k_SeedArrayName_Key)}))[0] = seed;

  AddBadDataInputValues inputValues;

  inputValues.PoissonNoise = filterArgs.value<bool>(k_PoissonNoise_Key);
  inputValues.PoissonVolFraction = filterArgs.value<float32>(k_PoissonVolFraction_Key);
  inputValues.BoundaryNoise = filterArgs.value<bool>(k_BoundaryNoise_Key);
  inputValues.BoundaryVolFraction = filterArgs.value<float32>(k_BoundaryVolFraction_Key);
  inputValues.GBEuclideanDistancesArrayPath = filterArgs.value<DataPath>(k_GBEuclideanDistancesArrayPath_Key);
  inputValues.ImageGeometryPath = filterArgs.value<DataPath>(k_ImageGeometryPath_Key);
  inputValues.SeedValue = filterArgs.value<uint64>(k_SeedValue_Key);

  return AddBadData(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
