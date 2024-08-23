#include "ReadStlFileFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/ReadStlFile.hpp"
#include "SimplnxCore/utils/StlUtilities.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateGeometry2DAction.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <filesystem>
#include <tuple>

namespace fs = std::filesystem;
using namespace nx::core;

namespace nx::core
{

//------------------------------------------------------------------------------
std::string ReadStlFileFilter::name() const
{
  return FilterTraits<ReadStlFileFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ReadStlFileFilter::className() const
{
  return FilterTraits<ReadStlFileFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ReadStlFileFilter::uuid() const
{
  return FilterTraits<ReadStlFileFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ReadStlFileFilter::humanName() const
{
  return "Read STL File";
}

//------------------------------------------------------------------------------
std::vector<std::string> ReadStlFileFilter::defaultTags() const
{
  return {className(), "IO", "Input", "Read", "Import"};
}

//------------------------------------------------------------------------------
Parameters ReadStlFileFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ScaleOutput, "Scale Output Geometry", "Scale the output Triangle Geometry by the Scaling Factor", false));
  params.insert(std::make_unique<Float32Parameter>(k_ScaleFactor, "Scale Factor", "The factor by which to scale the geometry", 1.0F));
  params.linkParameters(k_ScaleOutput, k_ScaleFactor, true);

  params.insert(std::make_unique<FileSystemPathParameter>(k_StlFilePath_Key, "STL File", "Input STL File", fs::path(""), FileSystemPathParameter::ExtensionsType{".stl"},
                                                          FileSystemPathParameter::PathType::InputFile));

  params.insertSeparator(Parameters::Separator{"Output Triangle Geometry"});
  params.insert(
      std::make_unique<DataGroupCreationParameter>(k_CreatedTriangleGeometryPath_Key, "Created Triangle Geometry", "The name of the created Triangle Geometry", DataPath({"TriangleDataContainer"})));

  params.insertSeparator(Parameters::Separator{"Output Vertex Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_VertexAttributeMatrixName_Key, "Vertex Data [AttributeMatrix]",
                                                          "The name of the AttributeMatrix where the Vertex Data of the Triangle Geometry will be created", INodeGeometry0D::k_VertexDataName));

  params.insertSeparator(Parameters::Separator{"Output Face Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_FaceAttributeMatrixName_Key, "Face Data [AttributeMatrix]",
                                                          "The name of the AttributeMatrix where the Face Data of the Triangle Geometry will be created", INodeGeometry2D::k_FaceDataName));
  params.insert(std::make_unique<DataObjectNameParameter>(k_FaceNormalsName_Key, "Face Labels", "The name of the triangle normals data array", "Face Normals"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ReadStlFileFilter::clone() const
{
  return std::make_unique<ReadStlFileFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ReadStlFileFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                          const std::atomic_bool& shouldCancel) const
{
  auto pStlFilePathValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_StlFilePath_Key);
  auto pTriangleGeometryPath = filterArgs.value<DataPath>(k_CreatedTriangleGeometryPath_Key);
  auto vertexMatrixName = filterArgs.value<std::string>(k_VertexAttributeMatrixName_Key);
  auto faceMatrixName = filterArgs.value<std::string>(k_FaceAttributeMatrixName_Key);
  auto faceNormalsName = filterArgs.value<std::string>(k_FaceNormalsName_Key);

  nx::core::Result<OutputActions> resultOutputActions;

  // Validate that the STL File is binary and readable.
  StlConstants::StlFileType stlFileType = StlUtilities::DetermineStlFileType(pStlFilePathValue);
  if(stlFileType == StlConstants::StlFileType::ASCI)
  {
    return MakePreflightErrorResult(
        StlConstants::k_UnsupportedFileType,
        fmt::format("The Input STL File is ASCII which is not currently supported. Please convert it to a binary STL file using another program.", pStlFilePathValue.string()));
  }
  if(stlFileType == StlConstants::StlFileType::FileOpenError)
  {
    return MakePreflightErrorResult(StlConstants::k_ErrorOpeningFile, fmt::format("Error opening the STL file.", pStlFilePathValue.string()));
  }
  if(stlFileType == StlConstants::StlFileType::HeaderParseError)
  {
    return MakePreflightErrorResult(StlConstants::k_ErrorOpeningFile, fmt::format("Error reading the header from STL file.", pStlFilePathValue.string()));
  }

  // Now get the number of Triangles according to the STL Header
  int32_t numTriangles = StlUtilities::NumFacesFromHeader(pStlFilePathValue);
  if(numTriangles < 0)
  {
    return MakePreflightErrorResult(numTriangles, fmt::format("Error extracting the number of triangles from the STL file.", pStlFilePathValue.string()));
  }

  // This can happen in a LOT of STL files. Just means the writer didn't go back and update the header.
  if(numTriangles == 0)
  {
    numTriangles = 1;
  }

  // Assign the outputAction to the Result<OutputActions>::actions vector via a push_back
  // Assuming this filter did make some structural changes to the DataStructure then store
  // the outputAction into the resultOutputActions object via a std::move().
  // NOTE: That using std::move() means that you can *NOT* use the outputAction variable
  // past this point, we are going to scope each section so that we don't accidentally introduce bugs

  // Create the Triangle Geometry action and store it
  auto createTriangleGeometryAction = std::make_unique<CreateTriangleGeometryAction>(pTriangleGeometryPath, numTriangles, 1, vertexMatrixName, faceMatrixName,
                                                                                     CreateTriangleGeometryAction::k_DefaultVerticesName, CreateTriangleGeometryAction::k_DefaultFacesName);
  auto faceNormalsPath = createTriangleGeometryAction->getFaceDataPath().createChildPath(faceNormalsName);
  resultOutputActions.value().appendAction(std::move(createTriangleGeometryAction));
  // Create the face Normals DataArray action and store it
  auto createArrayAction = std::make_unique<CreateArrayAction>(nx::core::DataType::float64, std::vector<usize>{static_cast<usize>(numTriangles)}, std::vector<usize>{3}, faceNormalsPath);
  resultOutputActions.value().appendAction(std::move(createArrayAction));

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods. (None to store for this filter... yet)

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ReadStlFileFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                        const std::atomic_bool& shouldCancel) const
{
  auto pStlFilePathValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_StlFilePath_Key);
  auto pTriangleGeometryPath = filterArgs.value<DataPath>(k_CreatedTriangleGeometryPath_Key);
  auto vertexMatrixName = filterArgs.value<std::string>(k_VertexAttributeMatrixName_Key);
  auto faceMatrixName = filterArgs.value<std::string>(k_FaceAttributeMatrixName_Key);
  auto faceNormalsName = filterArgs.value<std::string>(k_FaceNormalsName_Key);

  auto pFaceDataGroupPath = pTriangleGeometryPath.createChildPath(faceMatrixName);

  auto pFaceNormalsPath = pFaceDataGroupPath.createChildPath(faceNormalsName);

  auto scaleOutput = filterArgs.value<bool>(k_ScaleOutput);
  auto scaleFactor = filterArgs.value<float32>(k_ScaleFactor);

  // The actual STL File Reading is placed in a separate class `ReadStlFile`
  Result<> result = ReadStlFile(dataStructure, pStlFilePathValue, pTriangleGeometryPath, pFaceDataGroupPath, pFaceNormalsPath, scaleOutput, scaleFactor, shouldCancel, messageHandler)();
  return result;
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_ScaleOutputKey = "ScaleOutput";
constexpr StringLiteral k_ScaleFactorKey = "ScaleFactor";
constexpr StringLiteral k_StlFilePathKey = "StlFilePath";
constexpr StringLiteral k_SurfaceMeshDataContainerNameKey = "SurfaceMeshDataContainerName";
constexpr StringLiteral k_VertexAttributeMatrixNameKey = "VertexAttributeMatrixName";
constexpr StringLiteral k_FaceAttributeMatrixNameKey = "FaceAttributeMatrixName";
constexpr StringLiteral k_FaceNormalsArrayNameKey = "FaceNormalsArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> ReadStlFileFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ReadStlFileFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedBooleanFilterParameterConverter>(args, json, SIMPL::k_ScaleOutputKey, k_ScaleOutput));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::FloatFilterParameterConverter<float32>>(args, json, SIMPL::k_ScaleFactorKey, k_ScaleFactor));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::InputFileFilterParameterConverter>(args, json, SIMPL::k_StlFilePathKey, k_StlFilePath_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerCreationFilterParameterConverter>(args, json, SIMPL::k_SurfaceMeshDataContainerNameKey, k_CreatedTriangleGeometryPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_VertexAttributeMatrixNameKey, k_VertexAttributeMatrixName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_FaceAttributeMatrixNameKey, k_FaceAttributeMatrixName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_FaceNormalsArrayNameKey, k_FaceNormalsName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
