#include "FindSurfaceAreaToVolumeFilter.hpp"

#include "ComplexCore/Filters/Algorithms/FindSurfaceAreaToVolume.hpp"

#include "complex/DataStructure/AttributeMatrix.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FindSurfaceAreaToVolumeFilter::name() const
{
  return FilterTraits<FindSurfaceAreaToVolumeFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string FindSurfaceAreaToVolumeFilter::className() const
{
  return FilterTraits<FindSurfaceAreaToVolumeFilter>::className;
}

//------------------------------------------------------------------------------
Uuid FindSurfaceAreaToVolumeFilter::uuid() const
{
  return FilterTraits<FindSurfaceAreaToVolumeFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string FindSurfaceAreaToVolumeFilter::humanName() const
{
  return "Find Surface Area to Volume & Sphericity";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindSurfaceAreaToVolumeFilter::defaultTags() const
{
  return {className(), "Statistics", "Morphological", "Volumes", "Surface Area"};
}

//------------------------------------------------------------------------------
Parameters FindSurfaceAreaToVolumeFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_CalculateSphericity_Key, "Calculate Sphericity", "Whether or not to calculate the sphericity of each Feature", false));

  params.insertSeparator(Parameters::Separator{"Required Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometry_Key, "Selected Image Geometry", "The target geometry", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellFeatureIdsArrayPath_Key, "Cell Feature Ids", "Specifies to which Feature each cell belongs", DataPath({"CellData", "FeatureIds"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Required Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(
      k_NumCellsArrayPath_Key, "Number of Cells", "Number of Cells that are owned by the Feature. This value does not place any distinction between Cells that may be of a different size",
      DataPath({"CellFeatureData", "NumElements"}), ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Created Feature Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_SurfaceAreaVolumeRatioArrayName_Key, "Surface Area to Volume Ratio",
                                                          "Ratio of surface area to volume for each Feature. The units are inverse length", "SurfaceAreaVolumeRatio"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_SphericityArrayName_Key, "Sphericity Array Name", "The sphericity of each feature", "Sphericity"));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_CalculateSphericity_Key, k_SphericityArrayName_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindSurfaceAreaToVolumeFilter::clone() const
{
  return std::make_unique<FindSurfaceAreaToVolumeFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FindSurfaceAreaToVolumeFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                      const std::atomic_bool& shouldCancel) const
{
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  auto pNumCellsArrayPathValue = filterArgs.value<DataPath>(k_NumCellsArrayPath_Key);
  auto pCalculateSphericityValue = filterArgs.value<bool>(k_CalculateSphericity_Key);

  PreflightResult preflightResult;

  complex::Result<OutputActions> resultOutputActions;

  // Get the Cell Data Array so we get the tuple shape correct
  const auto* featureDataArray = dataStructure.getDataAs<Int32Array>(pNumCellsArrayPathValue);
  if(nullptr == featureDataArray)
  {
    return MakePreflightErrorResult(-12801, fmt::format("{} Data Array is not of the correct type. Select a DataArray object.", pNumCellsArrayPathValue.toString()));
  }
  const auto* cellFeatureData = dataStructure.getDataAs<AttributeMatrix>(pNumCellsArrayPathValue.getParent());
  if(cellFeatureData == nullptr)
  {
    return MakePreflightErrorResult(-12802, fmt::format("The selected number of cells array {} is not located in an attribute matrix. Make sure you have selected the number of cells array "
                                                        "located in the cell feature attribute matrix of the selected geometry",
                                                        pNumCellsArrayPathValue.toString()));
  }
  IDataStore::ShapeType tupleShape = cellFeatureData->getShape();
  // Create the SurfaceAreaVolumeRatio
  {
    auto arrayPath = pNumCellsArrayPathValue.getParent().createChildPath(filterArgs.value<std::string>(k_SurfaceAreaVolumeRatioArrayName_Key));
    auto action = std::make_unique<CreateArrayAction>(DataType::float32, tupleShape, std::vector<usize>{1ULL}, arrayPath);
    resultOutputActions.value().appendAction(std::move(action));
  }
  // Create the SphericityArray
  if(pCalculateSphericityValue)
  {
    auto arrayPath = pNumCellsArrayPathValue.getParent().createChildPath(filterArgs.value<std::string>(k_SphericityArrayName_Key));
    auto action = std::make_unique<CreateArrayAction>(DataType::float32, tupleShape, std::vector<usize>{1ULL}, arrayPath);
    resultOutputActions.value().appendAction(std::move(action));
  }

  std::vector<PreflightValue> preflightUpdatedValues;

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> FindSurfaceAreaToVolumeFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                    const std::atomic_bool& shouldCancel) const
{
  FindSurfaceAreaToVolumeInputValues inputValues;

  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  inputValues.NumCellsArrayPath = filterArgs.value<DataPath>(k_NumCellsArrayPath_Key);
  inputValues.SurfaceAreaVolumeRatioArrayName = inputValues.NumCellsArrayPath.getParent().createChildPath(filterArgs.value<std::string>(k_SurfaceAreaVolumeRatioArrayName_Key));
  inputValues.CalculateSphericity = filterArgs.value<bool>(k_CalculateSphericity_Key);
  inputValues.SphericityArrayName = inputValues.NumCellsArrayPath.getParent().createChildPath(filterArgs.value<std::string>(k_SphericityArrayName_Key));
  inputValues.InputImageGeometry = filterArgs.value<DataPath>(k_SelectedImageGeometry_Key);

  return FindSurfaceAreaToVolume(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
