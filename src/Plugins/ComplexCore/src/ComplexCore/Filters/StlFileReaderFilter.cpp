#include "StlFileReaderFilter.hpp"

#include "ComplexCore/Filters/Algorithms/StlFileReader.hpp"
#include "ComplexCore/utils/StlUtilities.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/CreateDataGroupAction.hpp"
#include "complex/Filter/Actions/CreateGeometry2DAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

#include <cstdio>
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
  return {"#IO", "#Input", "#Read", "#Import"};
}

//------------------------------------------------------------------------------
Parameters StlFileReaderFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<FileSystemPathParameter>(k_StlFilePath_Key, "STL File", "Input STL File", fs::path("*.stl"), FileSystemPathParameter::PathType::InputFile));
  // params.insert(std::make_unique<DataGroupSelectionParameter>(k_ParentDataGroupPath_Key, "Parent DataGroup", "", DataPath{}));

  params.insertSeparator(Parameters::Separator{"Created Objects"});

  params.insert(std::make_unique<DataGroupCreationParameter>(k_GeometryDataPath_Key, "Geometry Name [Data Group]", "", DataPath({"[Triangle Geometry]"})));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_FaceGroupDataPath_Key, "Triangle Face Data [Data Group]", "", DataPath({"[Triangle Geometry]", "Triangle Face Data"})));
  params.insert(std::make_unique<ArrayCreationParameter>(k_FaceNormalsDataPath_Key, "Face Normals [Data Array]", "", DataPath({"[Triangle Geometry]", "Triangle Face Data", "Normals"})));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer StlFileReaderFilter::clone() const
{
  return std::make_unique<StlFileReaderFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult StlFileReaderFilter::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pStlFilePathValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_StlFilePath_Key);
  auto pTriangleGeometryPath = filterArgs.value<DataPath>(k_GeometryDataPath_Key);
  auto pFaceDataGroupName = filterArgs.value<DataPath>(k_FaceGroupDataPath_Key);
  auto pFaceNormalsPath = filterArgs.value<DataPath>(k_FaceNormalsDataPath_Key);

  // Declare the preflightResult variable that will be populated with the results
  // of the preflight. The PreflightResult type contains the output Actions and
  // any preflight updated values that you want to be displayed to the user, typically
  // through a user interface (UI).
  PreflightResult preflightResult;

  // If your filter is making structural changes to the DataStructure then the filter
  // is going to create OutputActions subclasses that need to be returned. This will
  // store those actions.
  complex::Result<OutputActions> resultOutputActions;

  // If your filter is going to pass back some `preflight updated values` then this is where you
  // would create the code to store those values in the appropriate object. Note that we
  // in line creating the pair (NOT a std::pair<>) of Key:Value that will get stored in
  // the std::vector<PreflightValue> object.
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
  {
    auto createTriangleGeometryAction = std::make_unique<CreateTriangleGeometryAction>(pTriangleGeometryPath, numTriangles, 1);
    resultOutputActions.value().actions.push_back(std::move(createTriangleGeometryAction));
  }
  // Create Triangle FaceData (for the Normals) action and store it
  {
    auto createDataGroupAction = std::make_unique<CreateDataGroupAction>(pFaceDataGroupName);
    resultOutputActions.value().actions.push_back(std::move(createDataGroupAction));
  }
  // Create the face Normals DataArray action and store it
  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(complex::NumericType::float64, std::vector<usize>{static_cast<usize>(numTriangles)}, 3, pFaceNormalsPath);
    resultOutputActions.value().actions.push_back(std::move(createArrayAction));
  }

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods. (None to store for this filter... yet)

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> StlFileReaderFilter::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  auto pStlFilePathValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_StlFilePath_Key);
  auto pTriangleGeometryPath = filterArgs.value<DataPath>(k_GeometryDataPath_Key);
  auto pFaceDataGroupPath = filterArgs.value<DataPath>(k_FaceGroupDataPath_Key);
  auto pFaceNormalsPath = filterArgs.value<DataPath>(k_FaceNormalsDataPath_Key);

  // The actual STL File Reading is placed in a separate class `StlFileReader`
  Result<> result = StlFileReader(data, pStlFilePathValue, pTriangleGeometryPath, pFaceDataGroupPath, pFaceNormalsPath, this)();
  return result;
}

} // namespace complex
