#include "VtkRectilinearGridWriterFilter.hpp"

#include "ComplexCore/Filters/Algorithms/VtkRectilinearGridWriter.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string VtkRectilinearGridWriterFilter::name() const
{
  return FilterTraits<VtkRectilinearGridWriterFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string VtkRectilinearGridWriterFilter::className() const
{
  return FilterTraits<VtkRectilinearGridWriterFilter>::className;
}

//------------------------------------------------------------------------------
Uuid VtkRectilinearGridWriterFilter::uuid() const
{
  return FilterTraits<VtkRectilinearGridWriterFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string VtkRectilinearGridWriterFilter::humanName() const
{
  return "Vtk Rectilinear Grid Exporter";
}

//------------------------------------------------------------------------------
std::vector<std::string> VtkRectilinearGridWriterFilter::defaultTags() const
{
  return {className(), "IO", "Output", "Write", "Export"};
}

//------------------------------------------------------------------------------
Parameters VtkRectilinearGridWriterFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<FileSystemPathParameter>(k_OutputFile_Key, "Output File", "The output vtk file in which the geometry data is written", fs::path("Data/Output/RectilinearGrid.vtk"),
                                                          FileSystemPathParameter::ExtensionsType{".vtk"}, FileSystemPathParameter::PathType::OutputFile));
  params.insert(std::make_unique<BoolParameter>(k_WriteBinaryFile_Key, "Write Binary File", "Whether or not to write the vtk file in binary", false));
  params.insert(std::make_unique<GeometrySelectionParameter>(k_ImageGeometryPath_Key, "Image Geometry", "The path to the image geometry in which to write out to the vtk file", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_SelectedDataArrayPaths_Key, "Cell Data Arrays to Write", "The paths to the cell data arrays to write out with the geometry",
                                                               MultiArraySelectionParameter::ValueType{}, MultiArraySelectionParameter::AllowedTypes{IArray::ArrayType::DataArray}, GetAllDataTypes()));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer VtkRectilinearGridWriterFilter::clone() const
{
  return std::make_unique<VtkRectilinearGridWriterFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult VtkRectilinearGridWriterFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                       const std::atomic_bool& shouldCancel) const
{
  auto pOutputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputFile_Key);
  auto pWriteBinaryFileValue = filterArgs.value<bool>(k_WriteBinaryFile_Key);
  auto pImageGeometryPathValue = filterArgs.value<DataPath>(k_ImageGeometryPath_Key);
  auto pSelectedDataArrayPathsValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedDataArrayPaths_Key);

  PreflightResult preflightResult;
  complex::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  if(pSelectedDataArrayPathsValue.empty())
  {
    return MakePreflightErrorResult(-2070, "No cell data arrays are selected. You must select at least one array.");
  }

  if(!dataStructure.validateNumberOfTuples(pSelectedDataArrayPathsValue))
  {
    return MakePreflightErrorResult(
        -2071, "One or more of the selected cell data arrays have mismatching numbers of tuples. Make sure that all of the selected arrays come from the same cell level attribute matrix.");
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

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> VtkRectilinearGridWriterFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                     const std::atomic_bool& shouldCancel) const
{
  VtkRectilinearGridWriterInputValues inputValues;

  inputValues.OutputFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputFile_Key);
  inputValues.WriteBinaryFile = filterArgs.value<bool>(k_WriteBinaryFile_Key);
  inputValues.ImageGeometryPath = filterArgs.value<DataPath>(k_ImageGeometryPath_Key);
  inputValues.SelectedDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedDataArrayPaths_Key);

  return VtkRectilinearGridWriter(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
