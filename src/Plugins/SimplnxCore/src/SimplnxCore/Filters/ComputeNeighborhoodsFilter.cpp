#include "ComputeNeighborhoodsFilter.hpp"

#include "Algorithms/ComputeNeighborhoods.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateNeighborListAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"

#include "simplnx/Utilities/GeometryHelpers.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include "simplnx/Parameters/NumberParameter.hpp"

#include <algorithm>

using namespace nx::core;

namespace
{
const nx::core::ChoicesParameter::ValueType k_MultiplesOfAverageIndex = 0ULL;
const nx::core::ChoicesParameter::ValueType k_SearchRadiusIndex = 1ULL;
} // namespace

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ComputeNeighborhoodsFilter::name() const
{
  return FilterTraits<ComputeNeighborhoodsFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ComputeNeighborhoodsFilter::className() const
{
  return FilterTraits<ComputeNeighborhoodsFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ComputeNeighborhoodsFilter::uuid() const
{
  return FilterTraits<ComputeNeighborhoodsFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ComputeNeighborhoodsFilter::humanName() const
{
  return "Compute Feature Neighborhoods";
}

//------------------------------------------------------------------------------
std::vector<std::string> ComputeNeighborhoodsFilter::defaultTags() const
{
  return {className(), "Statistics", "Morphological", "Find", "Generate", "Calculate", "Determine"};
}

//------------------------------------------------------------------------------
Parameters ComputeNeighborhoodsFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});

  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(
      k_SearchRadiusType_Key, "Search Radius Type",
      "How the neighbor search radius is defined: (0) as a multiple of each feature's own Equivalent Sphere Diameter, or (1) as an absolute search radius in microns.", k_MultiplesOfAverageIndex,
      ChoicesParameter::Choices{"Multiples of Equivalent Diameter", "Search Radius (microns)"}));

  params.insert(std::make_unique<Float32Parameter>(
      k_MultiplesOfAverage_Key, "Multiples of Equivalent Diameter",
      "Each feature searches within a radius equal to its OWN Equivalent Sphere Diameter multiplied by this value (radius = equivalentDiameter[i] * multiples). Larger features therefore have larger "
      "neighborhoods, and the neighbor relationship can be asymmetric.",
      1.0F));
  params.insert(std::make_unique<Float32Parameter>(k_SearchRadius_Key, "Search Radius (microns)",
                                                   "The absolute radius (in microns) within which to search for 'neighboring' Features. A Feature is a neighbor if its centroid lies within this "
                                                   "distance of the target Feature's centroid.",
                                                   1.0F));

  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometryPath_Key, "Selected Image Geometry", "The target geometry", DataPath({"Data Container"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));

  params.insertSeparator(Parameters::Separator{"Input Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(
      k_EquivalentDiametersArrayPath_Key, "Equivalent Diameters",
      "Path to the array specifying the diameter of a sphere with the same volume as the Feature. Only required when 'Search Radius Type' is 'Multiples of Equivalent Diameter'.",
      DataPath({"Cell Feature Data", "EquivalentDiameters"}), ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CentroidsArrayPath_Key, "Centroids", "Path to the array specifying the X, Y, Z coordinates of Feature center of mass",
                                                          DataPath({"Cell Feature Data", "Centroids"}), ArraySelectionParameter::AllowedTypes{DataType::float32},
                                                          ArraySelectionParameter::AllowedComponentShapes{{3}}));
  params.insertSeparator(Parameters::Separator{"Output Feature Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_NeighborhoodsArrayName_Key, "Neighborhoods",
                                                          "Number of Features that have their centroid within the user specified multiple of equivalent sphere diameters from each Feature",
                                                          "Neighborhoods"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_NeighborhoodListArrayName_Key, "Neighborhood List",
                                                          "List of the Features whose centroids are within the user specified multiple of equivalent sphere diameter from each Feature",
                                                          "NeighborhoodList"));

  // Associate the linkable Search Radius Type parameter with the parameters it controls.
  // Equivalent Diameters is only needed to compute the average diameter for the "Multiples" mode.
  params.linkParameters(k_SearchRadiusType_Key, k_MultiplesOfAverage_Key, k_MultiplesOfAverageIndex);
  params.linkParameters(k_SearchRadiusType_Key, k_EquivalentDiametersArrayPath_Key, k_MultiplesOfAverageIndex);
  params.linkParameters(k_SearchRadiusType_Key, k_SearchRadius_Key, k_SearchRadiusIndex);

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ComputeNeighborhoodsFilter::parametersVersion() const
{
  return 2;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ComputeNeighborhoodsFilter::clone() const
{
  return std::make_unique<ComputeNeighborhoodsFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ComputeNeighborhoodsFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                   const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto pSelectedImageGeometryPathValue = filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key);
  auto pSearchRadiusTypeValue = filterArgs.value<ChoicesParameter::ValueType>(k_SearchRadiusType_Key);
  auto pMultiplesOfAverageValue = filterArgs.value<float32>(k_MultiplesOfAverage_Key);
  auto pSearchRadiusValue = filterArgs.value<float32>(k_SearchRadius_Key);
  auto pEquivalentDiametersArrayPathValue = filterArgs.value<DataPath>(k_EquivalentDiametersArrayPath_Key);
  auto pCentroidsArrayPathValue = filterArgs.value<DataPath>(k_CentroidsArrayPath_Key);
  auto pNeighborhoodsArrayNameValue = filterArgs.value<std::string>(k_NeighborhoodsArrayName_Key);
  auto pNeighborhoodListArrayNameValue = filterArgs.value<std::string>(k_NeighborhoodListArrayName_Key);

  PreflightResult preflightResult;

  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  // Report the input Image Geometry information so the user has spatial context for choosing a search radius
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(pSelectedImageGeometryPathValue);
  preflightUpdatedValues.push_back(
      {"Input Image Geometry Info", nx::core::GeometryHelpers::Description::GenerateGeometryInfo(imageGeom.getDimensions(), imageGeom.getSpacing(), imageGeom.getOrigin(), imageGeom.getUnits())});

  // Validate only the value parameter that is active for the selected search radius type. In the
  // "Multiples of Equivalent Diameter" mode the Equivalent Diameters array is required and must have the
  // same number of tuples as the Centroids; in the "Search Radius (microns)" mode it is not used.
  if(pSearchRadiusTypeValue == k_MultiplesOfAverageIndex)
  {
    if(pMultiplesOfAverageValue <= 0.0F)
    {
      return {MakeErrorResult<OutputActions>(-5732, "'Multiples of Equivalent Diameter' must be greater than zero.")};
    }
    auto tupleValidityCheck = dataStructure.validateNumberOfTuples({pEquivalentDiametersArrayPathValue, pCentroidsArrayPathValue});
    if(!tupleValidityCheck)
    {
      return {MakeErrorResult<OutputActions>(-5730, fmt::format("The following DataArrays all must have equal number of tuples but this was not satisfied.\n{}", tupleValidityCheck.error()))};
    }
  }
  else if(pSearchRadiusTypeValue == k_SearchRadiusIndex)
  {
    if(pSearchRadiusValue <= 0.0F)
    {
      return {MakeErrorResult<OutputActions>(-5733, "'Search Radius (microns)' must be greater than zero.")};
    }

    // Give the user context for the absolute search radius relative to the geometry's physical size
    const SizeVec3 dims = imageGeom.getDimensions();
    const FloatVec3 spacing = imageGeom.getSpacing();
    const FloatVec3 extents = {static_cast<float32>(dims[0]) * spacing[0], static_cast<float32>(dims[1]) * spacing[1], static_cast<float32>(dims[2]) * spacing[2]};
    const float32 minSpacing = std::min({spacing[0], spacing[1], spacing[2]});
    const float32 maxExtent = std::max({extents[0], extents[1], extents[2]});

    preflightUpdatedValues.push_back({"Search Radius Context", fmt::format("Search Radius of {} micron(s) spans approximately {} voxel(s) along X, {} along Y, {} along Z", pSearchRadiusValue,
                                                                           static_cast<int64>(pSearchRadiusValue / spacing[0]), static_cast<int64>(pSearchRadiusValue / spacing[1]),
                                                                           static_cast<int64>(pSearchRadiusValue / spacing[2]))});

    // Warn if the radius is smaller than a single voxel (likely finds no neighbors) or larger than the
    // whole geometry (likely makes every feature a neighbor of every other feature)
    if(pSearchRadiusValue < minSpacing)
    {
      resultOutputActions.warnings().push_back(
          {-5734, fmt::format("The Search Radius ({}) is smaller than a single voxel edge ({}). This may result in no neighbors being found.", pSearchRadiusValue, minSpacing)});
    }
    if(pSearchRadiusValue > maxExtent)
    {
      resultOutputActions.warnings().push_back({-5735, fmt::format("The Search Radius ({}) is larger than the largest dimension of the Image Geometry ({}). This may result in every feature being "
                                                                   "counted as a neighbor of every other Feature.",
                                                                   pSearchRadiusValue, maxExtent)});
    }
  }

  // Locate the Cell Feature Attribute Matrix (via the Centroids array, which is required in both modes) that
  // the output arrays will be created in
  const auto* cellFeatureData = dataStructure.getDataAs<AttributeMatrix>(pCentroidsArrayPathValue.getParent());
  if(cellFeatureData == nullptr)
  {
    return MakePreflightErrorResult(-5731, fmt::format("The selected Centroids array is not located in an attribute matrix. Make sure you have selected the input arrays located in the "
                                                       "cell feature attribute matrix of the selected geometry"));
  }

  // Create the Neighborhoods Array in the Feature Attribute Matrix
  {
    auto action = std::make_unique<CreateArrayAction>(DataType::int32, cellFeatureData->getShape(), std::vector<usize>{1ULL}, pCentroidsArrayPathValue.replaceName(pNeighborhoodsArrayNameValue));
    resultOutputActions.value().appendAction(std::move(action));
  }
  // Create the NeighborList Output NeighborList in the Feature Attribute Matrix
  {
    auto action = std::make_unique<CreateNeighborListAction>(DataType::int32, cellFeatureData->getShape(), pCentroidsArrayPathValue.replaceName(pNeighborhoodListArrayNameValue));
    resultOutputActions.value().appendAction(std::move(action));
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ComputeNeighborhoodsFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                 const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  ComputeNeighborhoodsInputValues inputValues;

  inputValues.SearchRadiusType = filterArgs.value<ChoicesParameter::ValueType>(k_SearchRadiusType_Key);
  inputValues.MultiplesOfAverage = filterArgs.value<float32>(k_MultiplesOfAverage_Key);
  inputValues.SearchRadius = filterArgs.value<float32>(k_SearchRadius_Key);
  inputValues.EquivalentDiametersArrayPath = filterArgs.value<DataPath>(k_EquivalentDiametersArrayPath_Key);
  inputValues.CentroidsArrayPath = filterArgs.value<DataPath>(k_CentroidsArrayPath_Key);
  inputValues.NeighborhoodsArrayName = inputValues.CentroidsArrayPath.replaceName(filterArgs.value<std::string>(k_NeighborhoodsArrayName_Key));
  inputValues.NeighborhoodListArrayName = inputValues.CentroidsArrayPath.replaceName(filterArgs.value<std::string>(k_NeighborhoodListArrayName_Key));
  inputValues.InputImageGeometry = filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key);

  return ComputeNeighborhoods(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_MultiplesOfAverageKey = "MultiplesOfAverage";
constexpr StringLiteral k_EquivalentDiametersArrayPathKey = "EquivalentDiametersArrayPath";
constexpr StringLiteral k_CentroidsArrayPathKey = "CentroidsArrayPath";
constexpr StringLiteral k_NeighborhoodsArrayNameKey = "NeighborhoodsArrayName";
constexpr StringLiteral k_NeighborhoodListArrayNameKey = "NeighborhoodListArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> ComputeNeighborhoodsFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ComputeNeighborhoodsFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionToGeometrySelectionFilterParameterConverter>(args, json, SIMPL::k_EquivalentDiametersArrayPathKey,
                                                                                                                                      k_SelectedImageGeometryPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::FloatFilterParameterConverter<float32>>(args, json, SIMPL::k_MultiplesOfAverageKey, k_MultiplesOfAverage_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_EquivalentDiametersArrayPathKey, k_EquivalentDiametersArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_CentroidsArrayPathKey, k_CentroidsArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_NeighborhoodsArrayNameKey, k_NeighborhoodsArrayName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_NeighborhoodListArrayNameKey, k_NeighborhoodListArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
