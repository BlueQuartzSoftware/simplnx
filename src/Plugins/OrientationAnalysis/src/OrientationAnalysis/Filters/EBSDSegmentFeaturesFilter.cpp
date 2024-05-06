#include "EBSDSegmentFeaturesFilter.hpp"
#include "OrientationAnalysis/Filters/Algorithms/EBSDSegmentFeatures.hpp"

#include "simplnx/Common/Numbers.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/IGridGeometry.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include "simplnx/Parameters/NumberParameter.hpp"

using namespace nx::core;

namespace
{
inline constexpr int32 k_MissingGeomError = -440;
inline constexpr int32 k_IncorrectInputArray = -600;
inline constexpr int32 k_MissingInputArray = -601;
inline constexpr int32 k_MissingOrIncorrectGoodVoxelsArray = -602;
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
  params.insert(std::make_unique<Float32Parameter>(k_MisorientationTolerance_Key, "Misorientation Tolerance (Degrees)",
                                                   "Tolerance (in degrees) used to determine if neighboring Cells belong to the same Feature", 5.0f));
  params.insert(std::make_unique<BoolParameter>(k_RandomizeFeatures_Key, "Randomize Feature IDs", "Specifies if feature IDs should be randomized during calculations", false));
  params.insertLinkableParameter(
      std::make_unique<BoolParameter>(k_UseMask_Key, "Use Mask Array", "Specifies whether to use a boolean array to exclude some Cells from the Feature identification process", false));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskArrayPath_Key, "Cell Mask Array", "Path to the data array that specifies if the Cell is to be counted in the algorithm", DataPath(),
                                                          ArraySelectionParameter::AllowedTypes{DataType::boolean, DataType::uint8}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_UseMask_Key, k_MaskArrayPath_Key, true);

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometryPath_Key, "Input Grid Geometry", "DataPath to input Grid Geometry", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image, IGeometry::Type::RectGrid}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_QuatsArrayPath_Key, "Cell Quaternions", "Specifies the orientation of the Cell in quaternion representation", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{nx::core::DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{4}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Cell Phases", "Specifies to which Ensemble each cell belongs", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{nx::core::DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Input Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "Enumeration representing the crystal structure for each Ensemble",
                                                          DataPath({"Ensemble Data", "CrystalStructures"}), ArraySelectionParameter::AllowedTypes{nx::core::DataType::uint32},
                                                          ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Output Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_FeatureIdsArrayName_Key, "Cell Feature Ids", "Specifies to which Feature each Cell belongs.", "FeatureIds"));
  params.insertSeparator(Parameters::Separator{"Output Feature Data"});
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_CellFeatureAttributeMatrixName_Key, "Feature Attribute Matrix", "The name of the created cell feature attribute matrix", "CellFeatureData"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_ActiveArrayName_Key, "Active",
                                                          "The name of the array which specifies if the Feature is still in the sample (true if the Feature is in the sample and false if it is not). "
                                                          "At the end of the Filter, all Features will be Active",
                                                          "Active"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer EBSDSegmentFeaturesFilter::clone() const
{
  return std::make_unique<EBSDSegmentFeaturesFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult EBSDSegmentFeaturesFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& args, const MessageHandler& messageHandler,
                                                                  const std::atomic_bool& shouldCancel) const
{
  auto pQuatsArrayPathValue = args.value<DataPath>(k_QuatsArrayPath_Key);
  auto pCellPhasesArrayPathValue = args.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = args.value<DataPath>(k_CrystalStructuresArrayPath_Key);

  // Validate the tolerance != 0
  auto tolerance = args.value<float32>(k_MisorientationTolerance_Key);
  if(tolerance == 0.0F)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{-655, fmt::format("Misorientation Tolerance cannot equal ZERO.", humanName())}})};
  }

  // Validate the Crystal Structures array
  const auto& crystalStructures = dataStructure.getDataRefAs<UInt32Array>(pCrystalStructuresArrayPathValue);
  if(crystalStructures.getNumberOfComponents() != 1)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_IncorrectInputArray, "Crystal Structures Input Array must be a 1 component Int32 array"}})};
  }

  // Validate the Grid Geometry
  auto gridGeomPath = args.value<DataPath>(k_SelectedImageGeometryPath_Key);
  const auto* inputGridGeom = dataStructure.getDataAs<IGridGeometry>(gridGeomPath);
  DataPath inputCellDataPath = inputGridGeom->getCellDataPath();
  auto featureIdsPath = inputCellDataPath.createChildPath(args.value<std::string>(k_FeatureIdsArrayName_Key));
  auto pCellFeatureAttributeMatrixNameValue = gridGeomPath.createChildPath(args.value<std::string>(k_CellFeatureAttributeMatrixName_Key));
  auto activeArrayPath = pCellFeatureAttributeMatrixNameValue.createChildPath(args.value<std::string>(k_ActiveArrayName_Key));

  std::vector<DataPath> dataPaths;

  // Validate the Quats array
  const auto& quats = dataStructure.getDataRefAs<Float32Array>(pQuatsArrayPathValue);
  if(quats.getNumberOfComponents() != 4)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_IncorrectInputArray, "Quaternion Input Array must be a 4 component Float32 array"}})};
  }
  dataPaths.push_back(pQuatsArrayPathValue);

  // Validate the Phases array
  const auto& phases = dataStructure.getDataRefAs<Int32Array>(pCellPhasesArrayPathValue);
  if(phases.getNumberOfComponents() != 1)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_IncorrectInputArray, "Phases Input Array must be a 1 component Int32 array"}})};
  }
  dataPaths.push_back(pCellPhasesArrayPathValue);

  // Validate the GoodVoxels/Mask Array combination
  bool useGoodVoxels = args.value<bool>(k_UseMask_Key);
  DataPath goodVoxelsPath;
  if(useGoodVoxels)
  {
    goodVoxelsPath = args.value<DataPath>(k_MaskArrayPath_Key);

    const auto* goodVoxelsArray = dataStructure.getDataAs<IDataArray>(goodVoxelsPath);
    if(nullptr == goodVoxelsArray)
    {
      return {nonstd::make_unexpected(std::vector<Error>{Error{k_MissingOrIncorrectGoodVoxelsArray, fmt::format("Mask array is not located at path: '{}'", goodVoxelsPath.toString())}})};
    }
    if(goodVoxelsArray->getDataType() != DataType::boolean && goodVoxelsArray->getDataType() != DataType::uint8)
    {
      return {nonstd::make_unexpected(
          std::vector<Error>{Error{k_MissingOrIncorrectGoodVoxelsArray, fmt::format("Mask array at path '{}' is not of the correct type. It must be Bool or UInt8.", goodVoxelsPath.toString())}})};
    }
    dataPaths.push_back(goodVoxelsPath);
  }

  auto tupleValidityCheck = dataStructure.validateNumberOfTuples(dataPaths);
  if(!tupleValidityCheck)
  {
    return {MakeErrorResult<OutputActions>(-651, fmt::format("The following DataArrays all must have equal number of tuples but this was not satisfied.\n{}", tupleValidityCheck.error()))};
  }

  // Create output DataStructure Items
  auto createFeatureGroupAction = std::make_unique<CreateAttributeMatrixAction>(pCellFeatureAttributeMatrixNameValue, std::vector<usize>{1});
  auto createActiveAction = std::make_unique<CreateArrayAction>(DataType::uint8, std::vector<usize>{1}, std::vector<usize>{1}, activeArrayPath);
  auto createFeatureIdsAction = std::make_unique<CreateArrayAction>(DataType::int32, quats.getIDataStore()->getTupleShape(), std::vector<usize>{1}, featureIdsPath);

  OutputActions actions;
  actions.appendAction(std::move(createFeatureGroupAction));
  actions.appendAction(std::move(createActiveAction));
  actions.appendAction(std::move(createFeatureIdsAction));

  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> EBSDSegmentFeaturesFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                const std::atomic_bool& shouldCancel) const
{
  EBSDSegmentFeaturesInputValues inputValues;

  // Store the misorientation tolerance as radians
  inputValues.misorientationTolerance = filterArgs.value<float32>(k_MisorientationTolerance_Key) * static_cast<float>(nx::core::numbers::pi / 180.0f);
  inputValues.useGoodVoxels = filterArgs.value<bool>(k_UseMask_Key);
  inputValues.shouldRandomizeFeatureIds = filterArgs.value<bool>(k_RandomizeFeatures_Key);
  inputValues.gridGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key);
  inputValues.quatsArrayPath = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  inputValues.cellPhasesArrayPath = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  inputValues.goodVoxelsArrayPath = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  inputValues.crystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto cellDataAMPath = dataStructure.getDataAs<IGridGeometry>(inputValues.gridGeomPath)->getCellDataPath();
  inputValues.featureIdsArrayPath = cellDataAMPath.createChildPath(filterArgs.value<std::string>(k_FeatureIdsArrayName_Key));
  inputValues.cellFeatureAttributeMatrixPath = inputValues.gridGeomPath.createChildPath(filterArgs.value<std::string>(k_CellFeatureAttributeMatrixName_Key));
  inputValues.activeArrayPath = inputValues.cellFeatureAttributeMatrixPath.createChildPath(filterArgs.value<std::string>(k_ActiveArrayName_Key));

  // Let the Algorithm instance do the work
  return EBSDSegmentFeatures(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace nx::core

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_MisorientationToleranceKey = "MisorientationTolerance";
constexpr StringLiteral k_UseGoodVoxelsKey = "UseGoodVoxels";
constexpr StringLiteral k_RandomizeFeatureIdsKey = "RandomizeFeatureIds";
constexpr StringLiteral k_QuatsArrayPathKey = "QuatsArrayPath";
constexpr StringLiteral k_CellPhasesArrayPathKey = "CellPhasesArrayPath";
constexpr StringLiteral k_GoodVoxelsArrayPathKey = "GoodVoxelsArrayPath";
constexpr StringLiteral k_CrystalStructuresArrayPathKey = "CrystalStructuresArrayPath";
constexpr StringLiteral k_FeatureIdsArrayNameKey = "FeatureIdsArrayName";
constexpr StringLiteral k_CellFeatureAttributeMatrixNameKey = "CellFeatureAttributeMatrixName";
constexpr StringLiteral k_ActiveArrayNameKey = "ActiveArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> EBSDSegmentFeaturesFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = EBSDSegmentFeaturesFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::FloatFilterParameterConverter<float32>>(args, json, SIMPL::k_MisorientationToleranceKey, k_MisorientationTolerance_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedBooleanFilterParameterConverter>(args, json, SIMPL::k_UseGoodVoxelsKey, k_UseMask_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_RandomizeFeatureIdsKey, k_RandomizeFeatures_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_QuatsArrayPathKey, k_QuatsArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_CellPhasesArrayPathKey, k_CellPhasesArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_GoodVoxelsArrayPathKey, k_MaskArrayPath_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_CrystalStructuresArrayPathKey, k_CrystalStructuresArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayNameKey, k_FeatureIdsArrayName_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_CellFeatureAttributeMatrixNameKey, k_CellFeatureAttributeMatrixName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_ActiveArrayNameKey, k_ActiveArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
