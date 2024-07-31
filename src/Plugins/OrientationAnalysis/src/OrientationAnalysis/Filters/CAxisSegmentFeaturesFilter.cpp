#include "CAxisSegmentFeaturesFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/CAxisSegmentFeatures.hpp"

#include "simplnx/Common/Constants.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
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
constexpr int32 k_MissingGeomError = -440;
constexpr int32 k_IncorrectInputArray = -600;
constexpr int32 k_MissingInputArray = -601;
constexpr int32 k_MissingOrIncorrectGoodVoxelsArray = -602;
} // namespace

namespace nx::core
{
//------------------------------------------------------------------------------
std::string CAxisSegmentFeaturesFilter::name() const
{
  return FilterTraits<CAxisSegmentFeaturesFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string CAxisSegmentFeaturesFilter::className() const
{
  return FilterTraits<CAxisSegmentFeaturesFilter>::className;
}

//------------------------------------------------------------------------------
Uuid CAxisSegmentFeaturesFilter::uuid() const
{
  return FilterTraits<CAxisSegmentFeaturesFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string CAxisSegmentFeaturesFilter::humanName() const
{
  return "Segment Features (C-Axis Misalignment)";
}

//------------------------------------------------------------------------------
std::vector<std::string> CAxisSegmentFeaturesFilter::defaultTags() const
{
  return {className(), "Reconstruction", "Segmentation"};
}

//------------------------------------------------------------------------------
Parameters CAxisSegmentFeaturesFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});

  params.insert(std::make_unique<Float32Parameter>(k_MisorientationTolerance_Key, "C-Axis Misorientation Tolerance (Degrees)",
                                                   "Tolerance (in degrees) used to determine if neighboring Cells belong to the same Feature", 5.0f));
  params.insert(std::make_unique<BoolParameter>(k_RandomizeFeatureIds_Key, "Randomize Feature Ids", "Specifies whether to randomize the feature ids", false));

  params.insertSeparator(Parameters::Separator{"Optional Data Mask"});
  params.insertLinkableParameter(
      std::make_unique<BoolParameter>(k_UseMask_Key, "Use Mask Array", "Specifies whether to use a boolean array to exclude some Cells from the Feature identification process", true));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskArrayPath_Key, "Cell Mask Array", "Specifies if the Cell is to be counted in the algorithm. Only required if Use Mask Array is checked",
                                                          DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::boolean, DataType::uint8}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometryPath_Key, "Input Grid Geometry", "DataPath to input Grid Geometry", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image, IGeometry::Type::RectGrid}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_QuatsArrayPath_Key, "Cell Quaternions", "Specifies the orientation of the Cell in quaternion representation", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{4}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Cell Phases", "Specifies to which Ensemble each Cell belongs", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Input Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "Enumeration representing the crystal structure for each Ensemble", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::uint32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Output Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_FeatureIdsArrayName_Key, "Cell Feature Ids", "Specifies to which Feature each Cell belongs", "FeatureIds"));

  params.insertSeparator(Parameters::Separator{"Output Feature Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_CellFeatureAttributeMatrixName_Key, "Feature Attribute Matrix", "The name of the created feature attribute matrix", "Cell Feature Data"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_ActiveArrayName_Key, "Active",
                                                          "The name of the array which specifies if the Feature is still in the sample (true if the Feature is in the sample and false if it is not). "
                                                          "At the end of the Filter, all Features will be Active",
                                                          "Active"));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_UseMask_Key, k_MaskArrayPath_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer CAxisSegmentFeaturesFilter::clone() const
{
  return std::make_unique<CAxisSegmentFeaturesFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult CAxisSegmentFeaturesFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& args, const MessageHandler& messageHandler,
                                                                   const std::atomic_bool& shouldCancel) const
{
  auto pQuatsArrayPathValue = args.value<DataPath>(k_QuatsArrayPath_Key);
  auto pCellPhasesArrayPathValue = args.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = args.value<DataPath>(k_CrystalStructuresArrayPath_Key);

  // Validate the tolerance != 0
  auto tolerance = args.value<float32>(k_MisorientationTolerance_Key);
  if(tolerance == 0.0F)
  {
    return {MakeErrorResult<OutputActions>(-655, fmt::format("Misorientation Tolerance cannot equal ZERO.", humanName()))};
  }

  // Validate the Grid Geometry
  auto gridGeomPath = args.value<DataPath>(k_SelectedImageGeometryPath_Key);
  const auto* inputGridGeom = dataStructure.getDataAs<IGridGeometry>(gridGeomPath);
  DataPath inputCellDataPath = inputGridGeom->getCellDataPath();
  auto featureIdsPath = inputCellDataPath.createChildPath(args.value<std::string>(k_FeatureIdsArrayName_Key));
  auto pCellFeatureAttributeMatrixNameValue = gridGeomPath.createChildPath(args.value<std::string>(k_CellFeatureAttributeMatrixName_Key));
  auto activeArrayPath = pCellFeatureAttributeMatrixNameValue.createChildPath(args.value<std::string>(k_ActiveArrayName_Key));

  std::vector<DataPath> dataPaths;

  dataPaths.push_back(pQuatsArrayPathValue);
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
      return {MakeErrorResult<OutputActions>(k_MissingOrIncorrectGoodVoxelsArray, fmt::format("Mask array is not located at path: '{}'", goodVoxelsPath.toString()))};
    }
    if(goodVoxelsArray->getDataType() != DataType::boolean && goodVoxelsArray->getDataType() != DataType::uint8)
    {
      return {
          MakeErrorResult<OutputActions>(k_MissingOrIncorrectGoodVoxelsArray, fmt::format("Mask array at path '{}' is not of the correct type. It must be Bool or UInt8.", goodVoxelsPath.toString()))};
    }
    dataPaths.push_back(goodVoxelsPath);
  }

  auto tupleValidityCheck = dataStructure.validateNumberOfTuples(dataPaths);
  if(!tupleValidityCheck)
  {
    return {MakeErrorResult<OutputActions>(-651, fmt::format("The following DataArrays all must have equal number of tuples but this was not satisfied.\n{}", tupleValidityCheck.error()))};
  }

  // Create the Cell Level FeatureIds array
  const auto& quats = dataStructure.getDataRefAs<Float32Array>(pQuatsArrayPathValue);
  auto createFeatureIdsAction = std::make_unique<CreateArrayAction>(DataType::int32, quats.getIDataStore()->getTupleShape(), std::vector<usize>{1}, featureIdsPath);

  // Create the Feature Attribute Matrix
  auto createFeatureGroupAction = std::make_unique<CreateAttributeMatrixAction>(pCellFeatureAttributeMatrixNameValue, std::vector<usize>{1});
  auto createActiveAction = std::make_unique<CreateArrayAction>(DataType::uint8, std::vector<usize>{1}, std::vector<usize>{1}, activeArrayPath);

  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  resultOutputActions.value().appendAction(std::move(createFeatureGroupAction));
  resultOutputActions.value().appendAction(std::move(createActiveAction));
  resultOutputActions.value().appendAction(std::move(createFeatureIdsAction));

  resultOutputActions.warnings().push_back(
      {-8361, "Segmenting features via c-axis mis-orientation requires Hexagonal-Low 6/m or Hexagonal-High 6/mmm type crystal structures. Make sure your data is of one of these two types."});

  return {resultOutputActions, preflightUpdatedValues};
}

//------------------------------------------------------------------------------
Result<> CAxisSegmentFeaturesFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                 const std::atomic_bool& shouldCancel) const
{
  CAxisSegmentFeaturesInputValues inputValues;

  inputValues.ImageGeometryPath = filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key);
  inputValues.MisorientationTolerance = filterArgs.value<float32>(k_MisorientationTolerance_Key) * Constants::k_PiOver180F;
  inputValues.UseMask = filterArgs.value<bool>(k_UseMask_Key);
  inputValues.RandomizeFeatureIds = filterArgs.value<bool>(k_RandomizeFeatureIds_Key);
  inputValues.QuatsArrayPath = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  inputValues.CellPhasesArrayPath = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  inputValues.MaskArrayPath = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  inputValues.CrystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  inputValues.FeatureIdsArrayPath = inputValues.QuatsArrayPath.replaceName(filterArgs.value<std::string>(k_FeatureIdsArrayName_Key));
  inputValues.CellFeatureAttributeMatrixName = inputValues.ImageGeometryPath.createChildPath(filterArgs.value<std::string>(k_CellFeatureAttributeMatrixName_Key));
  inputValues.ActiveArrayPath = inputValues.CellFeatureAttributeMatrixName.createChildPath(filterArgs.value<std::string>(k_ActiveArrayName_Key));

  return CAxisSegmentFeatures(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

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

Result<Arguments> CAxisSegmentFeaturesFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = CAxisSegmentFeaturesFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_QuatsArrayPathKey, k_SelectedImageGeometryPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::FloatFilterParameterConverter<float32>>(args, json, SIMPL::k_MisorientationToleranceKey, k_MisorientationTolerance_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedBooleanFilterParameterConverter>(args, json, SIMPL::k_UseGoodVoxelsKey, k_UseMask_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_RandomizeFeatureIdsKey, k_RandomizeFeatureIds_Key));
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
} // namespace nx::core
