#include "ComputeFeatureNeighborsFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/ComputeFeatureNeighbors.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateNeighborListAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <memory>

namespace nx::core
{
namespace
{
constexpr int32 k_InvalidInputDimensions = -76770;
} // namespace

//------------------------------------------------------------------------------
std::string ComputeFeatureNeighborsFilter::name() const
{
  return FilterTraits<ComputeFeatureNeighborsFilter>::name;
}

//------------------------------------------------------------------------------
std::string ComputeFeatureNeighborsFilter::className() const
{
  return FilterTraits<ComputeFeatureNeighborsFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ComputeFeatureNeighborsFilter::uuid() const
{
  return FilterTraits<ComputeFeatureNeighborsFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ComputeFeatureNeighborsFilter::humanName() const
{
  return "Compute Feature Neighbors";
}

//------------------------------------------------------------------------------
std::vector<std::string> ComputeFeatureNeighborsFilter::defaultTags() const
{
  return {className(), "Statistics", "Neighbors", "Features", "Find"};
}

//------------------------------------------------------------------------------
Parameters ComputeFeatureNeighborsFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});

  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_StoreBoundary_Key, "Store Boundary Cells Array", "Whether to store the boundary Cells array", false));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_StoreSurface_Key, "Store Surface Features Array", "Whether to store the surface Features array", false));

  params.insertSeparator(Parameters::Separator{"Input Data Objects"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometryPath_Key, "Image Geometry", "The geometry in which to identify feature neighbors", DataPath({"DataContainer"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsPath_Key, "Cell Feature Ids", "Specifies to which feature each cell belongs.", DataPath({"Cell Data", "FeatureIds"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Input Feature Data"});
  params.insert(std::make_unique<AttributeMatrixSelectionParameter>(k_CellFeaturesPath_Key, "Feature Attribute Matrix", "Feature Attribute Matrix of the selected Feature Ids",
                                                                    DataPath({"DataContainer", "Cell Feature Data"})));

  params.insertSeparator(Parameters::Separator{"Output Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(
      k_BoundaryCellsName_Key, "Boundary Cells",
      "The number of neighboring Cells of a given Cell that belong to a different Feature than itself. Values will range from 0 to 6. Only created if Store Boundary Cells Array is checked",
      "BoundaryCells"));
  params.insertSeparator(Parameters::Separator{"Output Feature Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_NumNeighborsName_Key, "Number of Neighbors", "Number of contiguous neighboring Features for a given Feature", "NumNeighbors"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_NeighborListName_Key, "Neighbor List", "List of the contiguous neighboring Features for a given Feature", "NeighborList"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_SharedSurfaceAreaName_Key, "Shared Surface Area List",
                                                          "List of the shared surface area for each of the contiguous neighboring Features for a given Feature", "SharedSurfaceAreaList"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_SurfaceFeaturesName_Key, "Surface Features",
                                                          "The name of the surface features data array. Flag equal to 1 if the Feature touches an outer surface of the sample and equal to 0 if it "
                                                          "does not. Only created if Store Surface Features Array is checked",
                                                          "SurfaceFeatures"));

  params.linkParameters(k_StoreBoundary_Key, k_BoundaryCellsName_Key, std::make_any<bool>(true));
  params.linkParameters(k_StoreSurface_Key, k_SurfaceFeaturesName_Key, std::make_any<bool>(true));
  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ComputeFeatureNeighborsFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ComputeFeatureNeighborsFilter::clone() const
{
  return std::make_unique<ComputeFeatureNeighborsFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ComputeFeatureNeighborsFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                      const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto storeBoundaryCells = filterArgs.value<bool>(k_StoreBoundary_Key);
  auto storeSurfaceFeatures = filterArgs.value<bool>(k_StoreSurface_Key);
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key);
  auto featureIdsPath = filterArgs.value<DataPath>(k_FeatureIdsPath_Key);
  auto boundaryCellsName = filterArgs.value<std::string>(k_BoundaryCellsName_Key);
  auto numNeighborsName = filterArgs.value<std::string>(k_NumNeighborsName_Key);
  auto neighborListName = filterArgs.value<std::string>(k_NeighborListName_Key);
  auto sharedSurfaceAreaName = filterArgs.value<std::string>(k_SharedSurfaceAreaName_Key);
  auto surfaceFeaturesName = filterArgs.value<std::string>(k_SurfaceFeaturesName_Key);
  auto featureAttrMatrixPath = filterArgs.value<DataPath>(k_CellFeaturesPath_Key);

  DataPath boundaryCellsPath = featureIdsPath.replaceName(boundaryCellsName);
  DataPath numNeighborsPath = featureAttrMatrixPath.createChildPath(numNeighborsName);
  DataPath neighborListPath = featureAttrMatrixPath.createChildPath(neighborListName);
  DataPath sharedSurfaceAreaPath = featureAttrMatrixPath.createChildPath(sharedSurfaceAreaName);
  DataPath surfaceFeaturesPath = featureAttrMatrixPath.createChildPath(surfaceFeaturesName);

  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);

  usize emptyDimCount = 0;
  if(imageGeom.getNumXCells() < 2)
  {
    emptyDimCount++;
  }
  if(imageGeom.getNumYCells() < 2)
  {
    emptyDimCount++;
  }
  if(imageGeom.getNumZCells() < 2)
  {
    emptyDimCount++;
  }

  if(emptyDimCount > 1)
  {
    return MakePreflightErrorResult(k_InvalidInputDimensions, "This filter requires at least 2 valid dimensions in the image geom. Two or more 1's were found in the image's dimensions");
  }

  OutputActions actions;

  auto& featureIdsArray = dataStructure.getDataRefAs<Int32Array>(featureIdsPath);
  ShapeType tupleShape = featureIdsArray.getIDataStore()->getTupleShape();

  const ShapeType cDims{1};

  // Create output Cell Data Arrays (if the user requested it)
  if(storeBoundaryCells)
  {
    auto action = std::make_unique<CreateArrayAction>(DataType::int8, tupleShape, cDims, boundaryCellsPath);
    actions.appendAction(std::move(action));
  }

  // Feature Data:
  // Validating the Feature Attribute Matrix and trying to find a child of the Group
  // that is an IDataArray subclass, so we can get the proper tuple shape
  const auto& featureAttrMatrix = dataStructure.getDataRefAs<AttributeMatrix>(featureAttrMatrixPath);
  tupleShape = featureAttrMatrix.getShape();

  // Create the NumNeighbors Output Data Array in the Feature Attribute Matrix
  {
    auto action = std::make_unique<CreateArrayAction>(DataType::int32, tupleShape, cDims, numNeighborsPath);
    actions.appendAction(std::move(action));
  }
  // Create the NeighborList Output NeighborList in the Feature Attribute Matrix
  {
    auto action = std::make_unique<CreateNeighborListAction>(DataType::int32, tupleShape, neighborListPath);
    actions.appendAction(std::move(action));
  }
  // And we do the same for the SharedSurfaceArea list in the Feature Attribute Matrix
  {
    auto action = std::make_unique<CreateNeighborListAction>(DataType::float32, tupleShape, sharedSurfaceAreaPath);
    actions.appendAction(std::move(action));
  }
  // Create the SurfaceFeatures Output Data Array in the Feature Attribute Matrix
  if(storeSurfaceFeatures)
  {
    auto action = std::make_unique<CreateArrayAction>(DataType::boolean, tupleShape, cDims, surfaceFeaturesPath, CreateArrayAction::k_DefaultDataFormat, "false");
    actions.appendAction(std::move(action));
  }

  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> ComputeFeatureNeighborsFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                    const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  ComputeFeatureNeighborsInputValues inputValues;

  // Options
  inputValues.StoreBoundaryCells = filterArgs.value<BoolParameter::ValueType>(k_StoreBoundary_Key);
  inputValues.StoreSurfaceFeatures = filterArgs.value<BoolParameter::ValueType>(k_StoreSurface_Key);

  // Geometry
  inputValues.InputImageGeometryPath = filterArgs.value<GeometrySelectionParameter::ValueType>(k_SelectedImageGeometryPath_Key);

  // Cell Data
  inputValues.FeatureIdsPath = filterArgs.value<ArraySelectionParameter::ValueType>(k_FeatureIdsPath_Key);
  inputValues.BoundaryCellsPath = inputValues.FeatureIdsPath.replaceName(filterArgs.value<DataObjectNameParameter::ValueType>(k_BoundaryCellsName_Key));

  // Feature Data
  inputValues.CellFeatureArrayPath = filterArgs.value<AttributeMatrixSelectionParameter::ValueType>(k_CellFeaturesPath_Key);
  inputValues.NumberOfNeighborsPath = inputValues.CellFeatureArrayPath.createChildPath(filterArgs.value<DataObjectNameParameter::ValueType>(k_NumNeighborsName_Key));
  inputValues.NeighborListPath = inputValues.CellFeatureArrayPath.createChildPath(filterArgs.value<DataObjectNameParameter::ValueType>(k_NeighborListName_Key));
  inputValues.SharedSurfaceAreaListPath = inputValues.CellFeatureArrayPath.createChildPath(filterArgs.value<DataObjectNameParameter::ValueType>(k_SharedSurfaceAreaName_Key));
  inputValues.SurfaceFeaturesPath = inputValues.CellFeatureArrayPath.createChildPath(filterArgs.value<DataObjectNameParameter::ValueType>(k_SurfaceFeaturesName_Key));

  return ComputeFeatureNeighbors(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_StoreBoundaryCellsKey = "StoreBoundaryCells";
constexpr StringLiteral k_StoreSurfaceFeaturesKey = "StoreSurfaceFeatures";
constexpr StringLiteral k_FeatureIdsArrayPathKey = "FeatureIdsArrayPath";
constexpr StringLiteral k_CellFeatureAttributeMatrixPathKey = "CellFeatureAttributeMatrixPath";
constexpr StringLiteral k_BoundaryCellsArrayNameKey = "BoundaryCellsArrayName";
constexpr StringLiteral k_NumNeighborsArrayNameKey = "NumNeighborsArrayName";
constexpr StringLiteral k_NeighborListArrayNameKey = "NeighborListArrayName";
constexpr StringLiteral k_SharedSurfaceAreaListArrayNameKey = "SharedSurfaceAreaListArrayName";
constexpr StringLiteral k_SurfaceFeaturesArrayNameKey = "SurfaceFeaturesArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> ComputeFeatureNeighborsFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ComputeFeatureNeighborsFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedBooleanFilterParameterConverter>(args, json, SIMPL::k_StoreBoundaryCellsKey, k_StoreBoundary_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedBooleanFilterParameterConverter>(args, json, SIMPL::k_StoreSurfaceFeaturesKey, k_StoreSurface_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_SelectedImageGeometryPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_FeatureIdsPath_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::AttributeMatrixSelectionFilterParameterConverter>(args, json, SIMPL::k_CellFeatureAttributeMatrixPathKey, k_CellFeaturesPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_BoundaryCellsArrayNameKey, k_BoundaryCellsName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_NumNeighborsArrayNameKey, k_NumNeighborsName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_NeighborListArrayNameKey, k_NeighborListName_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_SharedSurfaceAreaListArrayNameKey, k_SharedSurfaceAreaName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_SurfaceFeaturesArrayNameKey, k_SurfaceFeaturesName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
