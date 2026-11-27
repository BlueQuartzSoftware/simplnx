#include "EBSDSegmentFeaturesFilter.hpp"
#include "OrientationAnalysis/Filters/Algorithms/EBSDSegmentFeatures.hpp"

#include "simplnx/Common/Constants.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/IGridGeometry.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

using namespace nx::core;

namespace
{
DataPath FeatureIdsPath(const IGridGeometry& gridGeometry, const Arguments& filterArgs)
{
  return gridGeometry.getCellDataPath().createChildPath(filterArgs.value<std::string>(EBSDSegmentFeaturesFilter::k_FeatureIdsArrayName_Key));
}
} // namespace

namespace nx::core
{
//------------------------------------------------------------------------------
std::string EBSDSegmentFeaturesFilter::name() const
{
  return FilterTraits<EBSDSegmentFeaturesFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string EBSDSegmentFeaturesFilter::className() const
{
  return FilterTraits<EBSDSegmentFeaturesFilter>::className;
}

//------------------------------------------------------------------------------
Uuid EBSDSegmentFeaturesFilter::uuid() const
{
  return FilterTraits<EBSDSegmentFeaturesFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string EBSDSegmentFeaturesFilter::humanName() const
{
  return "Segment Features (Misorientation)";
}

//------------------------------------------------------------------------------
std::vector<std::string> EBSDSegmentFeaturesFilter::defaultTags() const
{
  return {className(), "Reconstruction", "Segmentation"};
}

//------------------------------------------------------------------------------
Parameters EBSDSegmentFeaturesFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<Float32ParameterType>(k_MisorientationTolerance_Key, "Misorientation Tolerance (Degrees)",
                                                       "Tolerance (in degrees) used to determine if neighboring Cells belong to the same Feature", 5.0f));
  params.insert(std::make_unique<BoolParameter>(k_RandomizeFeatureIds_Key, "Randomize Feature Ids", "Specifies if feature IDs should be randomized during calculations", false));
  params.insert(std::make_unique<ChoicesParameter>(k_NeighborScheme_Key, "Neighbor Scheme", "How many neighbors to use", segment_features::k_6NeighborIndex, segment_features::k_OperationChoices));

  params.insertSeparator(Parameters::Separator{"Optional Data Mask"});
  params.insertLinkableParameter(
      std::make_unique<BoolParameter>(k_UseMask_Key, "Use Mask Array", "Specifies whether to use a boolean array to exclude some Cells from the Feature identification process", false));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskArrayPath_Key, "Cell Mask Array", "Path to the data array that specifies if the Cell is to be counted in the algorithm", DataPath(),
                                                          GetDataArrayOnlyTypes(), ArraySelectionParameter::AllowedDataTypes{DataType::boolean, DataType::uint8},
                                                          ArraySelectionParameter::AllowedComponentShapesType{{1}}));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometryPath_Key, "Input Grid Geometry", "DataPath to input Grid Geometry", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypeSetType{IGeometry::Type::Image, IGeometry::Type::RectGrid}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_QuatsArrayPath_Key, "Cell Quaternions", "Specifies the orientation of the Cell in quaternion representation", DataPath{},
                                                          GetDataArrayOnlyTypes(), ArraySelectionParameter::AllowedDataTypes{nx::core::DataType::float32},
                                                          ArraySelectionParameter::AllowedComponentShapesType{{4}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Cell Phases", "Specifies to which Ensemble each cell belongs", DataPath{}, GetDataArrayOnlyTypes(),
                                                          ArraySelectionParameter::AllowedDataTypes{nx::core::DataType::int32}, ArraySelectionParameter::AllowedComponentShapesType{{1}}));
  params.insertSeparator(Parameters::Separator{"Input Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "Enumeration representing the crystal structure for each Ensemble",
                                                          DataPath({"Ensemble Data", "CrystalStructures"}), GetDataArrayOnlyTypes(),
                                                          ArraySelectionParameter::AllowedDataTypes{nx::core::DataType::uint32}, ArraySelectionParameter::AllowedComponentShapesType{{1}}));

  params.insertSeparator(Parameters::Separator{"Output Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_FeatureIdsArrayName_Key, "Cell Feature Ids", "Specifies to which feature each cell belongs.", "FeatureIds"));

  params.insertSeparator(Parameters::Separator{"Output Feature Data"});
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_CellFeatureAttributeMatrixName_Key, "Feature Attribute Matrix", "The name of the created cell feature attribute matrix", "Cell Feature Data"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_ActiveArrayName_Key, "Active",
                                                          "The name of the array which specifies if the Feature is still in the sample (true if the Feature is in the sample and false if it is not). "
                                                          "At the end of the Filter, all Features will be Active",
                                                          "Active"));

  params.insert(std::make_unique<BoolParameter>(k_IsPeriodic_Key, "Is Periodic", "Should segment features wrap around the image data", false));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_UseMask_Key, k_MaskArrayPath_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType EBSDSegmentFeaturesFilter::parametersVersion() const
{
  return 2;
}

//------------------------------------------------------------------------------
IFilter::UniquePointerType EBSDSegmentFeaturesFilter::clone() const
{
  return std::make_unique<EBSDSegmentFeaturesFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult EBSDSegmentFeaturesFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& /*messageHandler*/,
                                                                  const std::atomic_bool& /*shouldCancel*/, const ExecutionContext& /*executionContext*/) const
{
  auto pQuatsArrayPathValue = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);

  // Validate the tolerance != 0
  auto tolerance = filterArgs.value<float32>(k_MisorientationTolerance_Key);
  if(tolerance == 0.0F)
  {
    return {.output_actions = MakeErrorResult<OutputActions>(-655, fmt::format("Misorientation Tolerance cannot equal ZERO.", humanName()))};
  }

  // Validate the Grid Geometry
  auto gridGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key);
  const auto* inputGridGeomPtr = dataStructure.getDataAs<IGridGeometry>(gridGeomPath);
  auto featureIdsPath = FeatureIdsPath(*inputGridGeomPtr, filterArgs);
  auto pCellFeatureAttributeMatrixNameValue = gridGeomPath.createChildPath(filterArgs.value<std::string>(k_CellFeatureAttributeMatrixName_Key));
  auto activeArrayPath = pCellFeatureAttributeMatrixNameValue.createChildPath(filterArgs.value<std::string>(k_ActiveArrayName_Key));

  std::vector<DataPath> dataPaths;

  dataPaths.push_back(pQuatsArrayPathValue);
  dataPaths.push_back(pCellPhasesArrayPathValue);

  // Validate the GoodVoxels/Mask Array combination
  bool useGoodVoxels = filterArgs.value<bool>(k_UseMask_Key);
  if(useGoodVoxels)
  {
    dataPaths.push_back(filterArgs.value<DataPath>(k_MaskArrayPath_Key));
  }

  auto tupleValidityCheck = dataStructure.validateNumberOfTuples(dataPaths);
  if(!tupleValidityCheck)
  {
    return {.output_actions =
                MakeErrorResult<OutputActions>(-651, fmt::format("The following DataArrays all must have equal number of tuples but this was not satisfied.\n{}", tupleValidityCheck.error()))};
  }

  // Create the Cell Level FeatureIds array
  const auto& quats = dataStructure.getDataRefAs<Float32ArrayType>(pQuatsArrayPathValue);
  auto createFeatureIdsAction = std::make_unique<CreateArrayAction>(DataType::int32, quats.getIDataStore()->getTupleShape(), std::vector<usize>{1}, featureIdsPath);

  // Create the Feature Attribute Matrix
  auto createFeatureGroupAction = std::make_unique<CreateAttributeMatrixAction>(pCellFeatureAttributeMatrixNameValue, std::vector<usize>{1});
  auto createActiveAction = std::make_unique<CreateArrayAction>(DataType::uint8, std::vector<usize>{1}, std::vector<usize>{1}, activeArrayPath);

  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  resultOutputActions.value().appendAction(std::move(createFeatureGroupAction));
  resultOutputActions.value().appendAction(std::move(createActiveAction));
  resultOutputActions.value().appendAction(std::move(createFeatureIdsAction));

  return {.output_actions = resultOutputActions, .output_values = preflightUpdatedValues};
}

//------------------------------------------------------------------------------
Result<> EBSDSegmentFeaturesFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* /*pipelineNode*/, const MessageHandler& messageHandler,
                                                const std::atomic_bool& shouldCancel, const ExecutionContext& /*executionContext*/) const
{
  EBSDSegmentFeaturesInputValues inputValues;

  inputValues.image_geometry_path = filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key);
  inputValues.misorientation_tolerance = filterArgs.value<float32>(k_MisorientationTolerance_Key) * nx::core::constants::k_PiOver180F;
  inputValues.use_mask = filterArgs.value<bool>(k_UseMask_Key);
  inputValues.randomize_feature_ids = filterArgs.value<bool>(k_RandomizeFeatureIds_Key);
  inputValues.quats_array_path = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  inputValues.cell_phases_array_path = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  inputValues.mask_array_path = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  inputValues.crystal_structures_array_path = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  const auto& inputGridGeom = dataStructure.getDataRefAs<IGridGeometry>(inputValues.image_geometry_path);
  inputValues.feature_ids_array_path = FeatureIdsPath(inputGridGeom, filterArgs);
  inputValues.cell_feature_attribute_matrix_path = inputValues.image_geometry_path.createChildPath(filterArgs.value<std::string>(k_CellFeatureAttributeMatrixName_Key));
  inputValues.active_array_path = inputValues.cell_feature_attribute_matrix_path.createChildPath(filterArgs.value<std::string>(k_ActiveArrayName_Key));
  inputValues.is_periodic = filterArgs.value<bool>(k_IsPeriodic_Key);
  inputValues.neighbor_scheme = static_cast<SegmentFeatures::NeighborScheme>(filterArgs.value<ChoicesParameter::ValueType>(k_NeighborScheme_Key));

  return EBSDSegmentFeatures(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace nx::core

namespace
{
namespace simpl
{
constexpr StringLiteralType k_MisorientationToleranceKey = "MisorientationTolerance";
constexpr StringLiteralType k_UseGoodVoxelsKey = "UseGoodVoxels";
constexpr StringLiteralType k_RandomizeFeatureIdsKey = "RandomizeFeatureIds";
constexpr StringLiteralType k_QuatsArrayPathKey = "QuatsArrayPath";
constexpr StringLiteralType k_CellPhasesArrayPathKey = "CellPhasesArrayPath";
constexpr StringLiteralType k_GoodVoxelsArrayPathKey = "GoodVoxelsArrayPath";
constexpr StringLiteralType k_CrystalStructuresArrayPathKey = "CrystalStructuresArrayPath";
constexpr StringLiteralType k_FeatureIdsArrayNameKey = "FeatureIdsArrayName";
constexpr StringLiteralType k_CellFeatureAttributeMatrixNameKey = "CellFeatureAttributeMatrixName";
constexpr StringLiteralType k_ActiveArrayNameKey = "ActiveArrayName";
} // namespace simpl
} // namespace

Result<Arguments> EBSDSegmentFeaturesFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = EBSDSegmentFeaturesFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::FloatFilterParameterConverter<float32>>(args, json, simpl::k_MisorientationToleranceKey, k_MisorientationTolerance_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::LinkedBooleanFilterParameterConverter>(args, json, simpl::k_UseGoodVoxelsKey, k_UseMask_Key));
  Result<> randomizeResult = simpl_conversion::ConvertParameter<simpl_conversion::BooleanFilterParameterConverter>(args, json, simpl::k_RandomizeFeatureIdsKey, k_RandomizeFeatureIds_Key);
  if(randomizeResult.valid())
  {
    // This parameter does not appear in 6.5, thus we only include it in the output if it's valid
    results.push_back(std::move(randomizeResult));
  }
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_QuatsArrayPathKey, k_QuatsArrayPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionToGeometrySelectionFilterParameterConverter>(args, json, simpl::k_CellPhasesArrayPathKey,
                                                                                                                                        k_SelectedImageGeometryPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_CellPhasesArrayPathKey, k_CellPhasesArrayPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_GoodVoxelsArrayPathKey, k_MaskArrayPath_Key));
  results.push_back(
      simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_CrystalStructuresArrayPathKey, k_CrystalStructuresArrayPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::LinkedPathCreationFilterParameterConverter>(args, json, simpl::k_FeatureIdsArrayNameKey, k_FeatureIdsArrayName_Key));
  results.push_back(
      simpl_conversion::ConvertParameter<simpl_conversion::LinkedPathCreationFilterParameterConverter>(args, json, simpl::k_CellFeatureAttributeMatrixNameKey, k_CellFeatureAttributeMatrixName_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::LinkedPathCreationFilterParameterConverter>(args, json, simpl::k_ActiveArrayNameKey, k_ActiveArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
