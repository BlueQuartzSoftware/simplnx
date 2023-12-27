#include "CombineStlFilesFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/CombineStlFiles.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateGeometry2DAction.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string CombineStlFilesFilter::name() const
{
  return FilterTraits<CombineStlFilesFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string CombineStlFilesFilter::className() const
{
  return FilterTraits<CombineStlFilesFilter>::className;
}

//------------------------------------------------------------------------------
Uuid CombineStlFilesFilter::uuid() const
{
  return FilterTraits<CombineStlFilesFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string CombineStlFilesFilter::humanName() const
{
  return "Combine STL Files";
}

//------------------------------------------------------------------------------
std::vector<std::string> CombineStlFilesFilter::defaultTags() const
{
  return {className(), "IO", "Input", "Read", "Import"};
}

//------------------------------------------------------------------------------
Parameters CombineStlFilesFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<FileSystemPathParameter>(k_StlFilesPath_Key, "Path to STL Files", "The path to the folder containing all the STL files to be combined", fs::path(""),
                                                          FileSystemPathParameter::ExtensionsType{}, FileSystemPathParameter::PathType::InputDir));
  params.insertSeparator(Parameters::Separator{"Created Data"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_TriangleDataContainerName_Key, "Triangle Geometry", "The path to the triangle geometry to be created from the combined STL files",
                                                             DataPath({"TriangleGeometry"})));
  params.insertSeparator(Parameters::Separator{"Created Face Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_FaceAttributeMatrixName_Key, "Face Attribute Matrix", "The name of the face level attribute matrix to be created with the geometry",
                                                          TriangleGeom::k_FaceDataName));
  params.insert(std::make_unique<DataObjectNameParameter>(k_FaceNormalsArrayName_Key, "Face Normals", "The name of the data array in which to store the face normals for the created triangle geometry",
                                                          "FaceNormals"));
  params.insertSeparator(Parameters::Separator{"Created Vertex Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_VertexAttributeMatrixName_Key, "Vertex Attribute Matrix", "The name of the vertex level attribute matrix to be created with the geometry",
                                                          TriangleGeom::k_VertexDataName));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer CombineStlFilesFilter::clone() const
{
  return std::make_unique<CombineStlFilesFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult CombineStlFilesFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                              const std::atomic_bool& shouldCancel) const
{
  auto pStlFilesPathValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_StlFilesPath_Key);
  auto pTriangleDataContainerNameValue = filterArgs.value<DataPath>(k_TriangleDataContainerName_Key);
  auto pFaceAttributeMatrixNameValue = filterArgs.value<std::string>(k_FaceAttributeMatrixName_Key);
  auto pFaceNormalsArrayNameValue = filterArgs.value<std::string>(k_FaceNormalsArrayName_Key);
  auto pVertexAttributeMatrixNameValue = filterArgs.value<std::string>(k_VertexAttributeMatrixName_Key);

  PreflightResult preflightResult;
  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  std::vector<fs::path> stlFiles;
  for(const auto& dirEntry : std::filesystem::directory_iterator{pStlFilesPathValue})
  {
    if(fs::is_regular_file(dirEntry.path()) && StringUtilities::toLower(dirEntry.path().extension().string()) == ".stl")
    {
      stlFiles.push_back(dirEntry.path());
    }
  }
  if(stlFiles.empty())
  {
    return MakePreflightErrorResult(-9370, fmt::format("No STL files were found in the selected directory '{}'", pStlFilesPathValue.string()));
  }

  auto createTriangleGeometryAction = std::make_unique<CreateTriangleGeometryAction>(pTriangleDataContainerNameValue, 1, 1, pVertexAttributeMatrixNameValue, pFaceAttributeMatrixNameValue,
                                                                                     CreateTriangleGeometryAction::k_DefaultVerticesName, CreateTriangleGeometryAction::k_DefaultFacesName);
  auto faceNormalsPath = createTriangleGeometryAction->getFaceDataPath().createChildPath(pFaceNormalsArrayNameValue);
  resultOutputActions.value().appendAction(std::move(createTriangleGeometryAction));

  auto createArrayAction = std::make_unique<CreateArrayAction>(nx::core::DataType::float64, std::vector<usize>{1}, std::vector<usize>{3}, faceNormalsPath);
  resultOutputActions.value().appendAction(std::move(createArrayAction));

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> CombineStlFilesFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                            const std::atomic_bool& shouldCancel) const
{
  CombineStlFilesInputValues inputValues;

  inputValues.StlFilesPath = filterArgs.value<FileSystemPathParameter::ValueType>(k_StlFilesPath_Key);
  inputValues.TriangleDataContainerName = filterArgs.value<DataPath>(k_TriangleDataContainerName_Key);
  inputValues.FaceAttributeMatrixName = inputValues.TriangleDataContainerName.createChildPath(filterArgs.value<std::string>(k_FaceAttributeMatrixName_Key));
  inputValues.FaceNormalsArrayName = inputValues.FaceAttributeMatrixName.createChildPath(filterArgs.value<std::string>(k_FaceNormalsArrayName_Key));

  return CombineStlFiles(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_StlFilesPathKey = "StlFilesPath";
constexpr StringLiteral k_TriangleDataContainerNameKey = "TriangleDataContainerName";
constexpr StringLiteral k_FaceAttributeMatrixNameKey = "FaceAttributeMatrixName";
constexpr StringLiteral k_FaceNormalsArrayNameKey = "FaceNormalsArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> CombineStlFilesFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = CombineStlFilesFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::InputFileFilterParameterConverter>(args, json, SIMPL::k_StlFilesPathKey, k_StlFilesPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringToDataPathFilterParameterConverter>(args, json, SIMPL::k_TriangleDataContainerNameKey, k_TriangleDataContainerName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_FaceAttributeMatrixNameKey, k_FaceAttributeMatrixName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_FaceNormalsArrayNameKey, k_FaceNormalsArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core