#include "WriteVtkStructuredPointsFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/WriteVtkStructuredPoints.hpp"

#include "simplnx/Common/AtomicFile.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string WriteVtkStructuredPointsFilter::name() const
{
  return FilterTraits<WriteVtkStructuredPointsFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string WriteVtkStructuredPointsFilter::className() const
{
  return FilterTraits<WriteVtkStructuredPointsFilter>::className;
}

//------------------------------------------------------------------------------
Uuid WriteVtkStructuredPointsFilter::uuid() const
{
  return FilterTraits<WriteVtkStructuredPointsFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string WriteVtkStructuredPointsFilter::humanName() const
{
  return "Write Vtk Structured Points";
}

//------------------------------------------------------------------------------
std::vector<std::string> WriteVtkStructuredPointsFilter::defaultTags() const
{
  return {className(), "IO", "Output", "Write", "Export", "Vtk", "Structured Points"};
}

//------------------------------------------------------------------------------
Parameters WriteVtkStructuredPointsFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<FileSystemPathParameter>(k_OutputFile_Key, "Output File", "The output vtk file in which the geometry data is written", fs::path("data.vtk"),
                                                          FileSystemPathParameter::ExtensionsType{".vtk"}, FileSystemPathParameter::PathType::OutputFile));
  params.insert(std::make_unique<BoolParameter>(k_WriteBinaryFile_Key, "Write Binary File", "Whether or not to write the vtk file in binary", false));
  params.insert(std::make_unique<GeometrySelectionParameter>(k_ImageGeometryPath_Key, "Image Geometry", "The path to the image geometry in which to write out to the vtk file", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_SelectedDataArrayPaths_Key, "Cell Data Arrays to Write", "The paths to the cell data arrays to write out with the geometry",
                                                               MultiArraySelectionParameter::ValueType{}, MultiArraySelectionParameter::AllowedTypes{IArray::ArrayType::DataArray}, GetAllDataTypes()));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer WriteVtkStructuredPointsFilter::clone() const
{
  return std::make_unique<WriteVtkStructuredPointsFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult WriteVtkStructuredPointsFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                       const std::atomic_bool& shouldCancel) const
{
  auto pOutputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputFile_Key);
  auto pImageGeometryPathValue = filterArgs.value<DataPath>(k_ImageGeometryPath_Key);
  auto pSelectedDataArrayPathsValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedDataArrayPaths_Key);

  if(pSelectedDataArrayPathsValue.empty())
  {
    return MakePreflightErrorResult(-2070, "No cell data arrays are selected. You must select at least one array.");
  }

  auto tupleValidityCheck = dataStructure.validateNumberOfTuples(pSelectedDataArrayPathsValue);
  if(!tupleValidityCheck)
  {
    return {MakeErrorResult<OutputActions>(-2071, fmt::format("The following DataArrays all must have equal number of tuples but this was not satisfied.\n{}", tupleValidityCheck.error()))};
  }

  usize numTuples = dataStructure.getDataRefAs<IDataArray>(pSelectedDataArrayPathsValue[0]).getNumberOfTuples();
  usize numCells = dataStructure.getDataRefAs<ImageGeom>(pImageGeometryPathValue).getNumberOfCells();
  if(numCells != numTuples)
  {
    return MakePreflightErrorResult(
        -2072,
        fmt::format("The selected data arrays do not have the same number of tuples as the number of cells in the selected geometry ({}). Make sure your selected arrays are cell level data arrays",
                    pImageGeometryPathValue.toString()));
  }

  return {};
}

//------------------------------------------------------------------------------
Result<> WriteVtkStructuredPointsFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                     const std::atomic_bool& shouldCancel) const
{
  auto atomicFileResult = AtomicFile::Create(filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputFile_Key));
  if(atomicFileResult.invalid())
  {
    return ConvertResult(std::move(atomicFileResult));
  }
  AtomicFile atomicFile = std::move(atomicFileResult.value());

  WriteVtkStructuredPointsInputValues inputValues;

  inputValues.OutputFile = atomicFile.tempFilePath();
  inputValues.WriteBinaryFile = filterArgs.value<bool>(k_WriteBinaryFile_Key);
  inputValues.ImageGeometryPath = filterArgs.value<DataPath>(k_ImageGeometryPath_Key);
  inputValues.SelectedDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedDataArrayPaths_Key);

  auto result = WriteVtkStructuredPoints(dataStructure, messageHandler, shouldCancel, &inputValues)();
  if(result.valid())
  {
    Result<> commitResult = atomicFile.commit();
    if(commitResult.invalid())
    {
      return commitResult;
    }
  }
  return result;
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_OutputFileKey = "OutputFile";
constexpr StringLiteral k_WriteBinaryFileKey = "WriteBinaryFile";
constexpr StringLiteral k_SelectedDataArrayPathsKey = "SelectedDataArrayPaths";
} // namespace SIMPL
} // namespace

Result<Arguments> WriteVtkStructuredPointsFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = WriteVtkStructuredPointsFilter().getDefaultArguments();
  return ConvertResultTo<Arguments>({}, std::move(args));
}
} // namespace nx::core
