#include "WriteGBCDTriangleDataFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/WriteGBCDTriangleData.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"

#include "complex/Utilities/SIMPLConversion.hpp"

#include <filesystem>

namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string WriteGBCDTriangleDataFilter::name() const
{
  return FilterTraits<WriteGBCDTriangleDataFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string WriteGBCDTriangleDataFilter::className() const
{
  return FilterTraits<WriteGBCDTriangleDataFilter>::className;
}

//------------------------------------------------------------------------------
Uuid WriteGBCDTriangleDataFilter::uuid() const
{
  return FilterTraits<WriteGBCDTriangleDataFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string WriteGBCDTriangleDataFilter::humanName() const
{
  return "Write GBCD Triangles File";
}

//------------------------------------------------------------------------------
std::vector<std::string> WriteGBCDTriangleDataFilter::defaultTags() const
{
  return {className(), "IO", "Output", "Write", "Export"};
}

//------------------------------------------------------------------------------
Parameters WriteGBCDTriangleDataFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<FileSystemPathParameter>(k_OutputFile_Key, "Output File", "The output GBCD triangle file path", "", FileSystemPathParameter::ExtensionsType{".ph"},
                                                          FileSystemPathParameter::PathType::OutputFile));
  params.insertSeparator(Parameters::Separator{"Required Face Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SurfaceMeshFaceLabelsArrayPath_Key, "Face Labels", "Specifies which Features are on either side of each Face",
                                                          DataPath({"[Triangle Geometry]", "FaceData", "FaceLabels"}), ArraySelectionParameter::AllowedTypes{DataType::int32},
                                                          ArraySelectionParameter::AllowedComponentShapes{{2}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SurfaceMeshFaceNormalsArrayPath_Key, "Face Normals", "Specifies the normal of each Face",
                                                          DataPath({"[Triangle Geometry]", "FaceData", "FaceNormals"}), ArraySelectionParameter::AllowedTypes{DataType::float64},
                                                          ArraySelectionParameter::AllowedComponentShapes{{3}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SurfaceMeshFaceAreasArrayPath_Key, "Face Areas", "Specifies the area of each Face",
                                                          DataPath({"[Triangle Geometry]", "FaceData", "FaceAreas"}), ArraySelectionParameter::AllowedTypes{DataType::float64},
                                                          ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Required Cell Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(
      k_FeatureEulerAnglesArrayPath_Key, "Average Euler Angles", "Three angles defining the orientation of the Feature in Bunge convention (Z-X-Z).",
      DataPath({"[Image Geometry]", "CellFeatureData", "AvgEulerAngles"}), ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{3}}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer WriteGBCDTriangleDataFilter::clone() const
{
  return std::make_unique<WriteGBCDTriangleDataFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult WriteGBCDTriangleDataFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                    const std::atomic_bool& shouldCancel) const
{
  auto pOutputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputFile_Key);
  auto pSurfaceMeshFaceLabelsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  auto pSurfaceMeshFaceNormalsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFaceNormalsArrayPath_Key);
  auto pSurfaceMeshFaceAreasArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFaceAreasArrayPath_Key);
  auto pFeatureEulerAnglesArrayPathValue = filterArgs.value<DataPath>(k_FeatureEulerAnglesArrayPath_Key);

  PreflightResult preflightResult;
  complex::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  // make sure all the face data has same number of tuples (i.e. they should all be coming from the same Triangle Geometry)
  std::vector<DataPath> triangleArrayPaths = {pSurfaceMeshFaceLabelsArrayPathValue, pSurfaceMeshFaceNormalsArrayPathValue, pSurfaceMeshFaceAreasArrayPathValue};
  auto tupleValidityCheck = dataStructure.validateNumberOfTuples(triangleArrayPaths);
  if(!tupleValidityCheck)
  {
    return {MakeErrorResult<OutputActions>(-48320, fmt::format("The following DataArrays all must have equal number of tuples but this was not satisfied.\n{}", tupleValidityCheck.error()))};
  }

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> WriteGBCDTriangleDataFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                  const std::atomic_bool& shouldCancel) const
{
  WriteGBCDTriangleDataInputValues inputValues;

  inputValues.OutputFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputFile_Key);
  inputValues.SurfaceMeshFaceLabelsArrayPath = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  inputValues.SurfaceMeshFaceNormalsArrayPath = filterArgs.value<DataPath>(k_SurfaceMeshFaceNormalsArrayPath_Key);
  inputValues.SurfaceMeshFaceAreasArrayPath = filterArgs.value<DataPath>(k_SurfaceMeshFaceAreasArrayPath_Key);
  inputValues.FeatureEulerAnglesArrayPath = filterArgs.value<DataPath>(k_FeatureEulerAnglesArrayPath_Key);

  return WriteGBCDTriangleData(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_OutputFileKey = "OutputFile";
constexpr StringLiteral k_SurfaceMeshFaceLabelsArrayPathKey = "SurfaceMeshFaceLabelsArrayPath";
constexpr StringLiteral k_SurfaceMeshFaceNormalsArrayPathKey = "SurfaceMeshFaceNormalsArrayPath";
constexpr StringLiteral k_SurfaceMeshFaceAreasArrayPathKey = "SurfaceMeshFaceAreasArrayPath";
constexpr StringLiteral k_FeatureEulerAnglesArrayPathKey = "FeatureEulerAnglesArrayPath";
} // namespace SIMPL
} // namespace

Result<Arguments> ExportGBCDTriangleDataFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ExportGBCDTriangleDataFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::OutputFileFilterParameterConverter>(args, json, SIMPL::k_OutputFileKey, k_OutputFile_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SurfaceMeshFaceLabelsArrayPathKey, k_SurfaceMeshFaceLabelsArrayPath_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SurfaceMeshFaceNormalsArrayPathKey, k_SurfaceMeshFaceNormalsArrayPath_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SurfaceMeshFaceAreasArrayPathKey, k_SurfaceMeshFaceAreasArrayPath_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureEulerAnglesArrayPathKey, k_FeatureEulerAnglesArrayPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace complex
