#include "FindGBCDFilter.hpp"
#include "OrientationAnalysis/Filters/Algorithms/FindGBCD.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FindGBCDFilter::name() const
{
  return FilterTraits<FindGBCDFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string FindGBCDFilter::className() const
{
  return FilterTraits<FindGBCDFilter>::className;
}

//------------------------------------------------------------------------------
Uuid FindGBCDFilter::uuid() const
{
  return FilterTraits<FindGBCDFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string FindGBCDFilter::humanName() const
{
  return "Find GBCD";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindGBCDFilter::defaultTags() const
{
  return {className(), "Statistics", "Crystallography"};
}

//------------------------------------------------------------------------------
Parameters FindGBCDFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<Float32Parameter>(k_GBCDRes_Key, "GBCD Spacing (Degrees)", "The resolution in degrees for the GBCD calculation", 9.0f));
  params.insertSeparator(Parameters::Separator{"Face Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_TriangleGeometry_Key, "Triangle Geometry", "Path to the triangle geometry for which to calculate the GBCD", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Triangle}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SurfaceMeshFaceLabelsArrayPath_Key, "Face Labels", "Array specifying which Features are on either side of each Face", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{2}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SurfaceMeshFaceNormalsArrayPath_Key, "Face Normals", "Array specifying the normal of each Face", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::float64}, ArraySelectionParameter::AllowedComponentShapes{{3}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SurfaceMeshFaceAreasArrayPath_Key, "Face Areas", "Array specifying the area of each Face", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::float64}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Cell Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureEulerAnglesArrayPath_Key, "Average Euler Angles",
                                                          "Array specifying three angles defining the orientation of the Feature in Bunge convention (Z-X-Z)", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{3}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeaturePhasesArrayPath_Key, "Phases", "Specifies to which phase each Feature belongs", DataPath({"CellFeatureData", "Phases"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Cell Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "Enumeration representing the crystal structure for each Ensemble",
                                                          DataPath({"Ensemble Data", "CrystalStructures"}), ArraySelectionParameter::AllowedTypes{DataType::uint32},
                                                          ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Face Ensemble Data"});
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_FaceEnsembleAttributeMatrixName_Key, "Face Ensemble Attribute Matrix", "The name of the created face ensemble attribute matrix", "FaceEnsembleData"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_GBCDArrayName_Key, "GBCD",
                                                          "5 parameter GBCD data. The 6th component is used internally to track the northern vs. southern hemisphere of the Lambert sphere", "GBCD"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindGBCDFilter::clone() const
{
  return std::make_unique<FindGBCDFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FindGBCDFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                       const std::atomic_bool& shouldCancel) const
{
  auto pGBCDResValue = filterArgs.value<float32>(k_GBCDRes_Key);
  auto pTriangleGeometryPathValue = filterArgs.value<DataPath>(k_TriangleGeometry_Key);
  auto pSurfaceMeshFaceLabelsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  auto pSurfaceMeshFaceNormalsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFaceNormalsArrayPath_Key);
  auto pSurfaceMeshFaceAreasArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFaceAreasArrayPath_Key);
  auto pFeatureEulerAnglesArrayPathValue = filterArgs.value<DataPath>(k_FeatureEulerAnglesArrayPath_Key);
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto pFaceEnsembleAttributeMatrixNameValue = filterArgs.value<std::string>(k_FaceEnsembleAttributeMatrixName_Key);
  auto pGBCDArrayNameValue = filterArgs.value<std::string>(k_GBCDArrayName_Key);

  DataPath faceEnsembleAttributeMatrixPath = pTriangleGeometryPathValue.createChildPath(pFaceEnsembleAttributeMatrixNameValue);

  PreflightResult preflightResult;
  complex::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  if(dataStructure.getDataAs<Float32Array>(pFeatureEulerAnglesArrayPathValue) == nullptr)
  {
    return {MakeErrorResult<OutputActions>(-74350, fmt::format("Could not find euler angles array at path '{}'", pFeatureEulerAnglesArrayPathValue.toString()))};
  }
  if(dataStructure.getDataAs<Int32Array>(pFeaturePhasesArrayPathValue) == nullptr)
  {
    return {MakeErrorResult<OutputActions>(-74351, fmt::format("Could not find phases array at path '{}'", pFeaturePhasesArrayPathValue.toString()))};
  }
  const auto* crystalStructures = dataStructure.getDataAs<UInt32Array>(pCrystalStructuresArrayPathValue);
  if(crystalStructures == nullptr)
  {
    return {MakeErrorResult<OutputActions>(-74352, fmt::format("Could not find crystal structures array at path '{}'", pCrystalStructuresArrayPathValue.toString()))};
  }

  // order here matters...because we are going to use the size of the crystal structures to size the face AttributeMatrix
  if(dataStructure.getDataAs<TriangleGeom>(pTriangleGeometryPathValue) == nullptr)
  {
    return {MakeErrorResult<OutputActions>(-74353, fmt::format("Could not find triangle geometry array at path '{}'", pTriangleGeometryPathValue.toString()))};
  }

  std::vector<usize> tupleShape(1, crystalStructures->getNumberOfTuples());
  auto createAttributeMatrixAction = std::make_unique<CreateAttributeMatrixAction>(faceEnsembleAttributeMatrixPath, tupleShape);
  resultOutputActions.value().appendAction(std::move(createAttributeMatrixAction));

  if(dataStructure.getDataAs<Int32Array>(pSurfaceMeshFaceLabelsArrayPathValue) == nullptr)
  {
    return {MakeErrorResult<OutputActions>(-74354, fmt::format("Could not find face labels array at path '{}'", pSurfaceMeshFaceLabelsArrayPathValue.toString()))};
  }
  if(dataStructure.getDataAs<Float64Array>(pSurfaceMeshFaceNormalsArrayPathValue) == nullptr)
  {
    return {MakeErrorResult<OutputActions>(-74355, fmt::format("Could not find face normals array at path '{}'", pSurfaceMeshFaceNormalsArrayPathValue.toString()))};
  }
  if(dataStructure.getDataAs<Float64Array>(pSurfaceMeshFaceAreasArrayPathValue) == nullptr)
  {
    return {MakeErrorResult<OutputActions>(-74356, fmt::format("Could not find face areas array at path '{}'", pSurfaceMeshFaceAreasArrayPathValue.toString()))};
  }

  // call the sizeGBCD function to get the GBCD ranges, dimensions, etc.  Note that the input parameters do not affect the size and can be dummy values here;
  SizeGBCD sizeGbcd(0, 0, pGBCDResValue);
  std::vector<usize> componentShape(6);
  componentShape[0] = sizeGbcd.m_GbcdSizes[0];
  componentShape[1] = sizeGbcd.m_GbcdSizes[1];
  componentShape[2] = sizeGbcd.m_GbcdSizes[2];
  componentShape[3] = sizeGbcd.m_GbcdSizes[3];
  componentShape[4] = sizeGbcd.m_GbcdSizes[4];
  componentShape[5] = 2;

  auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::float64, tupleShape, componentShape, faceEnsembleAttributeMatrixPath.createChildPath(pGBCDArrayNameValue));
  resultOutputActions.value().appendAction(std::move(createArrayAction));

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> FindGBCDFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                     const std::atomic_bool& shouldCancel) const
{
  FindGBCDInputValues inputValues;
  inputValues.GBCDRes = filterArgs.value<float32>(k_GBCDRes_Key);
  inputValues.TriangleGeometryPath = filterArgs.value<DataPath>(k_TriangleGeometry_Key);
  inputValues.SurfaceMeshFaceLabelsArrayPath = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  inputValues.SurfaceMeshFaceNormalsArrayPath = filterArgs.value<DataPath>(k_SurfaceMeshFaceNormalsArrayPath_Key);
  inputValues.SurfaceMeshFaceAreasArrayPath = filterArgs.value<DataPath>(k_SurfaceMeshFaceAreasArrayPath_Key);
  inputValues.FeatureEulerAnglesArrayPath = filterArgs.value<DataPath>(k_FeatureEulerAnglesArrayPath_Key);
  inputValues.FeaturePhasesArrayPath = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  inputValues.CrystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  inputValues.FaceEnsembleAttributeMatrixName = inputValues.TriangleGeometryPath.createChildPath(filterArgs.value<std::string>(k_FaceEnsembleAttributeMatrixName_Key));
  inputValues.GBCDArrayName = inputValues.FaceEnsembleAttributeMatrixName.createChildPath(filterArgs.value<std::string>(k_GBCDArrayName_Key));

  return FindGBCD(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
