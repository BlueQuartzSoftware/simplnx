#include "WriteVtkRectilinearGridFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/WriteVtkRectilinearGrid.hpp"

#include "simplnx/Common/AtomicFile.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
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
std::string WriteVtkRectilinearGridFilter::name() const
{
  return FilterTraits<WriteVtkRectilinearGridFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string WriteVtkRectilinearGridFilter::className() const
{
  return FilterTraits<WriteVtkRectilinearGridFilter>::className;
}

//------------------------------------------------------------------------------
Uuid WriteVtkRectilinearGridFilter::uuid() const
{
  return FilterTraits<WriteVtkRectilinearGridFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string WriteVtkRectilinearGridFilter::humanName() const
{
  return "Write Vtk Rectilinear Grid";
}

//------------------------------------------------------------------------------
std::vector<std::string> WriteVtkRectilinearGridFilter::defaultTags() const
{
  return {className(), "IO", "Output", "Write", "Export", "Vtk", "Rectilinear Grid"};
}

//------------------------------------------------------------------------------
Parameters WriteVtkRectilinearGridFilter::parameters() const
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
IFilter::UniquePointer WriteVtkRectilinearGridFilter::clone() const
{
  return std::make_unique<WriteVtkRectilinearGridFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult WriteVtkRectilinearGridFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
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
    return MakePreflightErrorResult(-2071, fmt::format("The following DataArrays all must have equal number of tuples but this was not satisfied.\n{}", tupleValidityCheck.error()));
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
Result<> WriteVtkRectilinearGridFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                    const std::atomic_bool& shouldCancel) const
{
  auto atomicFileResult = AtomicFile::Create(filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputFile_Key));
  if(atomicFileResult.invalid())
  {
    return ConvertResult(std::move(atomicFileResult));
  }
  AtomicFile atomicFile = std::move(atomicFileResult.value());

  WriteVtkRectilinearGridInputValues inputValues;

  inputValues.OutputFile = atomicFile.tempFilePath();
  inputValues.WriteBinaryFile = filterArgs.value<bool>(k_WriteBinaryFile_Key);
  inputValues.ImageGeometryPath = filterArgs.value<DataPath>(k_ImageGeometryPath_Key);
  inputValues.SelectedDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedDataArrayPaths_Key);

  auto result = WriteVtkRectilinearGrid(dataStructure, messageHandler, shouldCancel, &inputValues)();
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

Result<Arguments> WriteVtkRectilinearGridFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = WriteVtkRectilinearGridFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::OutputFileFilterParameterConverter>(args, json, SIMPL::k_OutputFileKey, k_OutputFile_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_WriteBinaryFileKey, k_WriteBinaryFile_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerFromMultiSelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedDataArrayPathsKey, k_ImageGeometryPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::MultiDataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedDataArrayPathsKey, k_SelectedDataArrayPaths_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
