#include "ReadVtkStructuredPointsFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/ReadVtkStructuredPoints.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/CreateDataGroupAction.hpp"
#include "simplnx/Filter/Actions/EmptyAction.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Utilities/TimeUtilities.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace nx::core;

namespace
{

}

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ReadVtkStructuredPointsFilter::name() const
{
  return FilterTraits<ReadVtkStructuredPointsFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ReadVtkStructuredPointsFilter::className() const
{
  return FilterTraits<ReadVtkStructuredPointsFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ReadVtkStructuredPointsFilter::uuid() const
{
  return FilterTraits<ReadVtkStructuredPointsFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ReadVtkStructuredPointsFilter::humanName() const
{
  return "Read Vtk Structured Points File";
}

//------------------------------------------------------------------------------
std::vector<std::string> ReadVtkStructuredPointsFilter::defaultTags() const
{
  return {className(), "IO", "Input", "Read", "Import", "Vtk", "Structured Points"};
}

//------------------------------------------------------------------------------
Parameters ReadVtkStructuredPointsFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});

  params.insert(std::make_unique<FileSystemPathParameter>(k_InputFile_Key, "Input VTK File", "The path to the input file", fs::path("data.vtk"), FileSystemPathParameter::ExtensionsType{".vtk"},
                                                          FileSystemPathParameter::PathType::InputFile));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ReadPointData_Key, "Read Point Data", "Determines whether or not the Point Data will be read.", false));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ReadCellData_Key, "Read Cell Data", "Determines whether or not the Cell Data will be read.", false));

  params.insertSeparator(Parameters::Separator{"Created Point Data"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_CreatedVertexGeometryPath_Key, "Data Container [Point Data]", "The data container for the Point Data.", DataPath({"VTK Point Data"})));
  params.insert(std::make_unique<DataObjectNameParameter>(k_VertexAttributeMatrixName_Key, "Attribute Matrix Name [Point Data]", "The attribute matrix for the Point Data.", "Vertex Data"));

  params.insertSeparator(Parameters::Separator{"Created Cell Data"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_CreatedImageGeometryPath_Key, "Data Container [Cell Data]", "The data container for the Cell Data.", DataPath({"VTK Cell Data"})));
  params.insert(std::make_unique<DataObjectNameParameter>(k_CellAttributeMatrixName_Key, "Attribute Matrix Name [Cell Data]", "The attribute matrix for the Cell Data.", "Cell Data"));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_ReadPointData_Key, k_CreatedVertexGeometryPath_Key, true);
  params.linkParameters(k_ReadPointData_Key, k_VertexAttributeMatrixName_Key, true);
  params.linkParameters(k_ReadCellData_Key, k_CreatedImageGeometryPath_Key, true);
  params.linkParameters(k_ReadCellData_Key, k_CellAttributeMatrixName_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ReadVtkStructuredPointsFilter::clone() const
{
  return std::make_unique<ReadVtkStructuredPointsFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ReadVtkStructuredPointsFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                      const std::atomic_bool& shouldCancel) const
{
  auto pInputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  auto pReadPointDataValue = filterArgs.value<bool>(k_ReadPointData_Key);
  auto pReadCellDataValue = filterArgs.value<bool>(k_ReadCellData_Key);
  auto pVertexGeometryPath = filterArgs.value<DataPath>(k_CreatedVertexGeometryPath_Key);
  auto pImageGeometryPath = filterArgs.value<DataPath>(k_CreatedImageGeometryPath_Key);
  auto pVertexAttributeMatrixNameValue = filterArgs.value<DataObjectNameParameter::ValueType>(k_VertexAttributeMatrixName_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<DataObjectNameParameter::ValueType>(k_CellAttributeMatrixName_Key);

  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  nx::core::StopWatch sw;
  sw.start();

  ReadVtkStructuredPointsInputValues inputValues;

  inputValues.InputFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  inputValues.ReadPointData = filterArgs.value<bool>(k_ReadPointData_Key);
  inputValues.ReadCellData = filterArgs.value<bool>(k_ReadCellData_Key);
  inputValues.PointGeomPath = filterArgs.value<DataPath>(k_CreatedVertexGeometryPath_Key);
  inputValues.ImageGeomPath = filterArgs.value<DataPath>(k_CreatedImageGeometryPath_Key);
  inputValues.PointAttributeMatrixName = filterArgs.value<DataObjectNameParameter::ValueType>(k_VertexAttributeMatrixName_Key);
  inputValues.CellAttributeMatrixName = filterArgs.value<DataObjectNameParameter::ValueType>(k_CellAttributeMatrixName_Key);

  resultOutputActions = ReadVtkStructuredPoints::PreflightFile(inputValues);

  sw.stop();
  sw.print(std::cout);
  std::cout << "\n";

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ReadVtkStructuredPointsFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                    const std::atomic_bool& shouldCancel) const
{
  ReadVtkStructuredPointsInputValues inputValues;

  inputValues.InputFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  inputValues.ReadPointData = filterArgs.value<bool>(k_ReadPointData_Key);
  inputValues.ReadCellData = filterArgs.value<bool>(k_ReadCellData_Key);
  inputValues.PointGeomPath = filterArgs.value<DataPath>(k_CreatedVertexGeometryPath_Key);
  inputValues.ImageGeomPath = filterArgs.value<DataPath>(k_CreatedImageGeometryPath_Key);
  inputValues.PointAttributeMatrixName = filterArgs.value<DataObjectNameParameter::ValueType>(k_VertexAttributeMatrixName_Key);
  inputValues.CellAttributeMatrixName = filterArgs.value<DataObjectNameParameter::ValueType>(k_CellAttributeMatrixName_Key);

  return ReadVtkStructuredPoints(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
// @PARAMETER_JSON_CONSTANTS@
} // namespace SIMPL
} // namespace

//------------------------------------------------------------------------------
Result<Arguments> ReadVtkStructuredPointsFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ReadVtkStructuredPointsFilter().getDefaultArguments();

  std::vector<Result<>> results;

  // @PARAMETER_JSON_CONVERSION@

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}

} // namespace nx::core
