#include "ComputeMDFFilter.hpp"
#include "OrientationAnalysis/Filters/Algorithms/ComputeMDF.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "simplnx/Filter/Actions/CreateDataGroupAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ComputeMDFFilter::name() const
{
  return FilterTraits<ComputeMDFFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ComputeMDFFilter::className() const
{
  return FilterTraits<ComputeMDFFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ComputeMDFFilter::uuid() const
{
  return FilterTraits<ComputeMDFFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ComputeMDFFilter::humanName() const
{
  return "Compute MDF";
}

//------------------------------------------------------------------------------
std::vector<std::string> ComputeMDFFilter::defaultTags() const
{
  return {className(), "Statistics", "Crystallography", "Misorientation", "MDF"};
}

//------------------------------------------------------------------------------
Parameters ComputeMDFFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<Float32Parameter>(k_Halfwidth_Key, "Kernel Halfwidth (Degrees)",
                                                   "Halfwidth of the de la Vallee Poussin kernel used for the density estimate. MTEX default is 10 degrees.", 10.0f));
  params.insert(std::make_unique<Int32Parameter>(k_NumCurvePoints_Key, "Number of Curve Points", "Number of points in the output angle-distribution curves", 200));
  params.insert(
      std::make_unique<BoolParameter>(k_UseAreaWeights_Key, "Weight by Boundary Face Area", "Weight each boundary segment by its face area instead of equally (MTEX default is equal weights)", false));

  params.insertSeparator(Parameters::Separator{"Required Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_ImageGeometryPath_Key, "Image Geometry", "The geometry whose cells are scanned for feature boundaries", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellEulerAnglesArrayPath_Key, "Euler Angles", "Three angles defining the orientation of the Cell in Bunge convention (Z-X-Z)", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{3}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Phases", "Specifies to which Ensemble each Cell belongs", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "Specifies to which Feature each Cell belongs", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Required Input Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "Enumeration representing the crystal structure for each Ensemble", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::uint32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Output Data"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_OutputGroupPath_Key, "Output MDF Data Group", "Newly created DataGroup holding the per-phase MDF results", DataPath({"MDF Data"})));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ComputeMDFFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ComputeMDFFilter::clone() const
{
  return std::make_unique<ComputeMDFFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ComputeMDFFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                                         const ExecutionContext& executionContext) const
{
  auto pCellEulerAnglesArrayPathValue = filterArgs.value<DataPath>(k_CellEulerAnglesArrayPath_Key);
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto pHalfwidthValue = filterArgs.value<float32>(k_Halfwidth_Key);
  auto pNumCurvePointsValue = filterArgs.value<int32>(k_NumCurvePoints_Key);
  auto pOutputGroupPathValue = filterArgs.value<DataPath>(k_OutputGroupPath_Key);

  if(pHalfwidthValue <= 0.0f || pHalfwidthValue > 90.0f)
  {
    return {MakeErrorResult<OutputActions>(-96500, fmt::format("Kernel halfwidth must be in (0, 90] degrees, got {}", pHalfwidthValue))};
  }
  if(pNumCurvePointsValue < 2)
  {
    return {MakeErrorResult<OutputActions>(-96501, fmt::format("Number of curve points must be at least 2, got {}", pNumCurvePointsValue))};
  }

  const auto& crystalStructuresRef = dataStructure.getDataRefAs<UInt32Array>(pCrystalStructuresArrayPathValue);
  usize numEnsembles = crystalStructuresRef.getNumberOfTuples();
  if(numEnsembles < 2)
  {
    return {MakeErrorResult<OutputActions>(-96502, "Crystal Structures array must contain at least one ensemble beyond index 0")};
  }

  auto tupleValidityCheck = dataStructure.validateNumberOfTuples({pCellEulerAnglesArrayPathValue, pCellPhasesArrayPathValue, pFeatureIdsArrayPathValue});
  if(!tupleValidityCheck)
  {
    return {MakeErrorResult<OutputActions>(-96503, fmt::format("Cell input arrays must have equal tuple counts.\n{}", tupleValidityCheck.error()))};
  }

  Result<OutputActions> resultOutputActions;
  resultOutputActions.value().appendAction(std::make_unique<CreateDataGroupAction>(pOutputGroupPathValue));
  for(usize ensembleIndex = 1; ensembleIndex < numEnsembles; ensembleIndex++)
  {
    DataPath phaseGroupPath = pOutputGroupPathValue.createChildPath(PhaseGroupName(static_cast<int32>(ensembleIndex)));
    resultOutputActions.value().appendAction(std::make_unique<CreateDataGroupAction>(phaseGroupPath));

    // The MDF bin array's real tuple count depends on the crystal-structure VALUE (data, not
    // structure), so it cannot be known until execute time. Create it here with a placeholder
    // size; the algorithm resizes it once the actual Laue class is known.
    resultOutputActions.value().appendAction(std::make_unique<CreateArrayAction>(DataType::float64, std::vector<usize>{1}, std::vector<usize>{1}, phaseGroupPath.createChildPath(k_MDFArrayName)));

    DataPath angleAMPath = phaseGroupPath.createChildPath(k_AngleDistributionAMName);
    resultOutputActions.value().appendAction(std::make_unique<CreateAttributeMatrixAction>(angleAMPath, std::vector<usize>{static_cast<usize>(pNumCurvePointsValue)}));
    for(const auto& arrayName : {k_AnglesArrayName, k_MDFDensityArrayName, k_RandomDensityArrayName})
    {
      resultOutputActions.value().appendAction(
          std::make_unique<CreateArrayAction>(DataType::float64, std::vector<usize>{static_cast<usize>(pNumCurvePointsValue)}, std::vector<usize>{1}, angleAMPath.createChildPath(arrayName)));
    }
  }

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ComputeMDFFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                       const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  ComputeMDFInputValues inputValues;

  inputValues.ImageGeometryPath = filterArgs.value<DataPath>(k_ImageGeometryPath_Key);
  inputValues.CellEulerAnglesArrayPath = filterArgs.value<DataPath>(k_CellEulerAnglesArrayPath_Key);
  inputValues.CellPhasesArrayPath = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.CrystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  inputValues.OutputGroupPath = filterArgs.value<DataPath>(k_OutputGroupPath_Key);
  inputValues.HalfwidthDegrees = filterArgs.value<float32>(k_Halfwidth_Key);
  inputValues.NumCurvePoints = filterArgs.value<int32>(k_NumCurvePoints_Key);
  inputValues.UseAreaWeights = filterArgs.value<bool>(k_UseAreaWeights_Key);

  return ComputeMDF(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace nx::core
