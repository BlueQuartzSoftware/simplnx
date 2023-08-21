#include "StlFileReaderFilter.hpp"

#include "ComplexCore/Filters/Algorithms/StlFileReader.hpp"
#include "ComplexCore/utils/StlUtilities.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/CreateGeometry2DAction.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

#include <filesystem>
#include <tuple>

namespace fs = std::filesystem;
using namespace complex;

namespace complex
{

//------------------------------------------------------------------------------
std::string StlFileReaderFilter::name() const
{
  return FilterTraits<StlFileReaderFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string StlFileReaderFilter::className() const
{
  return FilterTraits<StlFileReaderFilter>::className;
}

//------------------------------------------------------------------------------
Uuid StlFileReaderFilter::uuid() const
{
  return FilterTraits<StlFileReaderFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string StlFileReaderFilter::humanName() const
{
  return "Import STL File";
}

//------------------------------------------------------------------------------
std::vector<std::string> StlFileReaderFilter::defaultTags() const
{
  return {className(), "IO", "Input", "Read", "Import"};
}

//------------------------------------------------------------------------------
Parameters StlFileReaderFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ScaleOutput, "Scale Output Geometry", "Scale the output Triangle Geometry by the Scaling Factor", false));
  params.insert(std::make_unique<Float32Parameter>(k_ScaleFactor, "Scale Factor", "The factor by which to scale the geometry", 1.0F));
  params.linkParameters(k_ScaleOutput, k_ScaleFactor, true);

  params.insert(std::make_unique<FileSystemPathParameter>(k_StlFilePath_Key, "STL File", "Input STL File", fs::path(""), FileSystemPathParameter::ExtensionsType{".stl"},
                                                          FileSystemPathParameter::PathType::InputFile));

  params.insertSeparator(Parameters::Separator{"Created Objects"});

  params.insert(std::make_unique<DataGroupCreationParameter>(k_GeometryDataPath_Key, "Geometry Name [Data Group]", "The complete path to the DataGroup containing the created Geometry data",
                                                             DataPath({"[Triangle Geometry]"})));

  params.insert(std::make_unique<StringParameter>(k_VertexMatrix_Key, "Vertex Matrix Name", "Name of the created Vertex Attribute Matrix", INodeGeometry0D::k_VertexDataName));
  params.insert(std::make_unique<StringParameter>(k_FaceMatrix_Key, "Face Matrix Name", "Name of the created Face Attribute Matrix", INodeGeometry2D::k_FaceDataName));

  params.insert(std::make_unique<StringParameter>(k_SharedVertexMatrix_Key, "Shared Vertex Matrix Name", "Name of the created Shared Vertex Attribute Matrix",
                                                  CreateTriangleGeometryAction::k_DefaultVerticesName));
  params.insert(
      std::make_unique<StringParameter>(k_SharedFaceMatrix_Key, "Shared Face Matrix Name", "Name of the created Shared Face Attribute Matrix", CreateTriangleGeometryAction::k_DefaultFacesName));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer StlFileReaderFilter::clone() const
{
  return std::make_unique<StlFileReaderFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult StlFileReaderFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                            const std::atomic_bool& shouldCancel) const
{
  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pStlFilePathValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_StlFilePath_Key);
  auto pTriangleGeometryPath = filterArgs.value<DataPath>(k_GeometryDataPath_Key);
  auto vertexMatrixName = filterArgs.value<std::string>(k_VertexMatrix_Key);
  auto faceMatrixName = filterArgs.value<std::string>(k_FaceMatrix_Key);
  auto sharedVertexMatrixName = filterArgs.value<std::string>(k_SharedVertexMatrix_Key);
  auto sharedFaceMatrixName = filterArgs.value<std::string>(k_SharedFaceMatrix_Key);

  PreflightResult preflightResult;

  complex::Result<OutputActions> resultOutputActions;

  std::vector<PreflightValue> preflightUpdatedValues;

  // If the filter needs to pass back some updated values via a key:value string:string set of values
  // you can declare and update that string here.

  // Collect all the errors
  std::vector<Error> errors;

  // Validate that the STL File is binary and readable.
  int32_t stlFileType = StlUtilities::DetermineStlFileType(pStlFilePathValue);
  if(stlFileType < 0)
  {
    Error result = {StlConstants::k_UnsupportedFileType,
                    fmt::format("The Input STL File is ASCII which is not currently supported. Please convert it to a binary STL file using another program.", pStlFilePathValue.string())};
    errors.push_back(result);
  }
  if(stlFileType > 0)
  {
    Error result = {StlConstants::k_ErrorOpeningFile, fmt::format("Error reading the STL file.", pStlFilePathValue.string())};
    errors.push_back(result);
  }

  // Now get the number of Triangles according to the STL Header
  int32_t numTriangles = StlUtilities::NumFacesFromHeader(pStlFilePathValue);
  if(numTriangles < 0)
  {
    Error result = {StlConstants::k_ErrorOpeningFile, fmt::format("Error reading the STL file.", pStlFilePathValue.string())};
    errors.push_back(result);
  }

  if(!errors.empty())
  {
    return {nonstd::make_unexpected(std::move(errors))};
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
  auto createTriangleGeometryAction =
      std::make_unique<CreateTriangleGeometryAction>(pTriangleGeometryPath, numTriangles, 1, vertexMatrixName, faceMatrixName, sharedVertexMatrixName, sharedFaceMatrixName);
  auto faceNormalsPath = createTriangleGeometryAction->getFaceDataPath().createChildPath(k_FaceNormals);
  resultOutputActions.value().appendAction(std::move(createTriangleGeometryAction));
  // Create the face Normals DataArray action and store it
  auto createArrayAction = std::make_unique<CreateArrayAction>(complex::DataType::float64, std::vector<usize>{static_cast<usize>(numTriangles)}, std::vector<usize>{3}, faceNormalsPath);
  resultOutputActions.value().appendAction(std::move(createArrayAction));

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods. (None to store for this filter... yet)

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> StlFileReaderFilter::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                          const std::atomic_bool& shouldCancel) const
{
  auto pStlFilePathValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_StlFilePath_Key);
  auto pTriangleGeometryPath = filterArgs.value<DataPath>(k_GeometryDataPath_Key);
  auto pFaceDataGroupPath = pTriangleGeometryPath.createChildPath(INodeGeometry2D::k_FaceDataName);
  auto pFaceNormalsPath = pFaceDataGroupPath.createChildPath(k_FaceNormals);

  auto scaleOutput = filterArgs.value<bool>(k_ScaleOutput);
  auto scaleFactor = filterArgs.value<float32>(k_ScaleFactor);
  // The actual STL File Reading is placed in a separate class `StlFileReader`
  Result<> result = StlFileReader(data, pStlFilePathValue, pTriangleGeometryPath, pFaceDataGroupPath, pFaceNormalsPath, scaleOutput, scaleFactor, shouldCancel)();
  return result;
}

} // namespace complex
