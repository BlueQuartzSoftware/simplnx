#include "FindFeatureReferenceCAxisMisorientationsFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/FindFeatureReferenceCAxisMisorientations.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FindFeatureReferenceCAxisMisorientationsFilter::name() const
{
  return FilterTraits<FindFeatureReferenceCAxisMisorientationsFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string FindFeatureReferenceCAxisMisorientationsFilter::className() const
{
  return FilterTraits<FindFeatureReferenceCAxisMisorientationsFilter>::className;
}

//------------------------------------------------------------------------------
Uuid FindFeatureReferenceCAxisMisorientationsFilter::uuid() const
{
  return FilterTraits<FindFeatureReferenceCAxisMisorientationsFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string FindFeatureReferenceCAxisMisorientationsFilter::humanName() const
{
  return "Find Feature Reference C-Axis Misalignments";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindFeatureReferenceCAxisMisorientationsFilter::defaultTags() const
{
  return {className(), "Statistics", "Crystallography"};
}

//------------------------------------------------------------------------------
Parameters FindFeatureReferenceCAxisMisorientationsFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_ImageGeometryPath_Key, "Image Geometry", "The path to the input image geometry", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insertSeparator(Parameters::Separator{"Required Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "Specifies to which Feature each Cell belongs", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Phases", "Specifies to which Ensemble each Cell belongs", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_QuatsArrayPath_Key, "Quaternions", "Specifies the orientation of the Cell in quaternion representation", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{4}}));
  params.insertSeparator(Parameters::Separator{"Required Cell Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_AvgCAxesArrayPath_Key, "Average C-Axes", "The direction of the Feature's C-axis in the sample reference frame", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{3}}));
  params.insertSeparator(Parameters::Separator{"Required Cell Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "Enumeration representing the crystal structure for each Ensemble", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::uint32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Created Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_FeatureReferenceCAxisMisorientationsArrayName_Key, "Feature Reference C-Axis Misorientations",
                                                          "Misorientation angle (in degrees) between Cell's C-axis and the C-axis of the Feature that owns that Cell",
                                                          "FeatureRefCAxisMisorientation"));
  params.insertSeparator(Parameters::Separator{"Created Cell Feature Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_FeatureAvgCAxisMisorientationsArrayName_Key, "Average C-Axis Misorientations",
                                                          "Average of the Feature Reference CAxis Misorientation values for all of the Cells that belong to the Feature", "AvgCAxisMisorientation"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_FeatureStdevCAxisMisorientationsArrayName_Key, "Feature Stdev C-Axis Misorientations",
                                                          "Standard deviation of the Feature Reference CAxis Misorientation values for all of the Cells that belong to the Feature",
                                                          "StdevCAxisMisorientation"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindFeatureReferenceCAxisMisorientationsFilter::clone() const
{
  return std::make_unique<FindFeatureReferenceCAxisMisorientationsFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FindFeatureReferenceCAxisMisorientationsFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                                       const std::atomic_bool& shouldCancel) const
{
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pQuatsArrayPathValue = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  auto pAvgCAxesArrayPathValue = filterArgs.value<DataPath>(k_AvgCAxesArrayPath_Key);
  auto pFeatureAvgCAxisMisorientationsArrayNameValue = filterArgs.value<std::string>(k_FeatureAvgCAxisMisorientationsArrayName_Key);
  auto pFeatureStDevCAxisMisorientationsArrayNameValue = filterArgs.value<std::string>(k_FeatureStdevCAxisMisorientationsArrayName_Key);
  auto pFeatureReferenceCAxisMisorientationsArrayNameValue = filterArgs.value<std::string>(k_FeatureReferenceCAxisMisorientationsArrayName_Key);

  PreflightResult preflightResult;
  complex::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  if(!dataStructure.validateNumberOfTuples({pFeatureIdsArrayPathValue, pCellPhasesArrayPathValue, pQuatsArrayPathValue}))
  {
    return MakePreflightErrorResult(-9800, fmt::format("The quaternions cell data array '{}', feature ids cell data array '{}' and cell phases data array '{}' have mismatching number of tuples. Make "
                                                       "sure these arrays are all located in the cell data attribute matrix for the selected geometry.",
                                                       pQuatsArrayPathValue.toString(), pFeatureIdsArrayPathValue.toString(), pCellPhasesArrayPathValue.toString()));
  }

  std::vector<usize> tupleShape = dataStructure.getDataRefAs<Int32Array>(pFeatureIdsArrayPathValue).getTupleShape();
  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::float32, tupleShape, std::vector<usize>{1},
                                                                 pFeatureIdsArrayPathValue.getParent().createChildPath(pFeatureReferenceCAxisMisorientationsArrayNameValue));
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }
  tupleShape = dataStructure.getDataRefAs<Float32Array>(pAvgCAxesArrayPathValue).getTupleShape();
  {
    auto createArrayAction =
        std::make_unique<CreateArrayAction>(DataType::float32, tupleShape, std::vector<usize>{1}, pAvgCAxesArrayPathValue.getParent().createChildPath(pFeatureAvgCAxisMisorientationsArrayNameValue));
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }
  {
    auto createArrayAction =
        std::make_unique<CreateArrayAction>(DataType::float32, tupleShape, std::vector<usize>{1}, pAvgCAxesArrayPathValue.getParent().createChildPath(pFeatureStDevCAxisMisorientationsArrayNameValue));
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  resultOutputActions.warnings().push_back(
      {-9801, "Finding the feature reference c-axis mis orientation requires Hexagonal-Low 6/m or Hexagonal-High 6/mmm type crystal structures. Make sure your data is of one of these two types."});

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> FindFeatureReferenceCAxisMisorientationsFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode,
                                                                     const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  FindFeatureReferenceCAxisMisorientationsInputValues inputValues;

  inputValues.ImageGeometryPath = filterArgs.value<DataPath>(k_ImageGeometryPath_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.CellPhasesArrayPath = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  inputValues.QuatsArrayPath = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  inputValues.AvgCAxesArrayPath = filterArgs.value<DataPath>(k_AvgCAxesArrayPath_Key);
  inputValues.CrystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  inputValues.FeatureAvgCAxisMisorientationsArrayName = inputValues.AvgCAxesArrayPath.getParent().createChildPath(filterArgs.value<std::string>(k_FeatureAvgCAxisMisorientationsArrayName_Key));
  inputValues.FeatureStdevCAxisMisorientationsArrayName = inputValues.AvgCAxesArrayPath.getParent().createChildPath(filterArgs.value<std::string>(k_FeatureStdevCAxisMisorientationsArrayName_Key));
  inputValues.FeatureReferenceCAxisMisorientationsArrayName =
      inputValues.FeatureIdsArrayPath.getParent().createChildPath(filterArgs.value<std::string>(k_FeatureReferenceCAxisMisorientationsArrayName_Key));

  return FindFeatureReferenceCAxisMisorientations(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
