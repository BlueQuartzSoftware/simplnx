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
#include "simplnx/Utilities/VtkLegacyFileReader.hpp"

#include <chrono>
#include <iostream>
#include <iomanip> // For std::setw and std::setfill
#include <filesystem>
namespace fs = std::filesystem;

using namespace nx::core;

namespace {


class StopWatch {
public:
  void start() {
    start_time = std::chrono::steady_clock::now();
  }

  void stop() {
    end_time = std::chrono::steady_clock::now();
  }

  void print(std::ostream& os) const {
    auto elapsed = end_time - start_time;
    auto hours = std::chrono::duration_cast<std::chrono::hours>(elapsed);
    auto minutes = std::chrono::duration_cast<std::chrono::minutes>(elapsed % std::chrono::hours(1));
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(elapsed % std::chrono::minutes(1));
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed % std::chrono::seconds(1));

    os << std::setw(2) << std::setfill('0') << hours.count() << ":"
       << std::setw(2) << std::setfill('0') << minutes.count() << ":"
       << std::setw(2) << std::setfill('0') << seconds.count() << "."
       << std::setw(3) << std::setfill('0') << milliseconds.count();
  }

private:
  std::chrono::steady_clock::time_point start_time;
  std::chrono::steady_clock::time_point end_time;
};

//int main() {
//  StopWatch sw;
//  sw.start();
//  // Simulate some work by sleeping for a specific duration
//  std::this_thread::sleep_for(std::chrono::seconds(1));
//  sw.stop();
//  sw.print(std::cout); // This will print the elapsed time in the specified format
//  return 0;
//}

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
  return "VTK STRUCTURED_POINTS Importer";
}

//------------------------------------------------------------------------------
std::vector<std::string> ReadVtkStructuredPointsFilter::defaultTags() const
{
  return {className(), "IO", "Input", "Read", "Import"};
}

//------------------------------------------------------------------------------
Parameters ReadVtkStructuredPointsFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});

  params.insert(std::make_unique<FileSystemPathParameter>(k_InputFile_Key, "Input VTK File", "The path to the input file", fs::path("DefaultInputFileName"),
                                                          FileSystemPathParameter::ExtensionsType{".vtk"}, FileSystemPathParameter::PathType::InputFile));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ReadPointData_Key, "Read Point Data", "", false));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ReadCellData_Key, "Read Cell Data", "", false));

  params.insertSeparator(Parameters::Separator{"Created Point Data"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_VertexDataContainerName_Key, "Data Container [Point Data]", "", DataPath({"VTK Point Data"})));
  params.insert(std::make_unique<DataObjectNameParameter>(k_VertexAttributeMatrixName_Key, "Attribute Matrix Name [Point Data]", "", "Vertex Data"));

  params.insertSeparator(Parameters::Separator{"Created Cell Data"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_VolumeDataContainerName_Key, "Data Container [Cell Data]", "", DataPath({"VTK Cell Data"})));
  params.insert(std::make_unique<DataObjectNameParameter>(k_CellAttributeMatrixName_Key, "Attribute Matrix Name [Cell Data]", "", "Cell Data"));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_ReadPointData_Key, k_VertexDataContainerName_Key, true);
  params.linkParameters(k_ReadPointData_Key, k_VertexAttributeMatrixName_Key, true);
  params.linkParameters(k_ReadCellData_Key, k_VolumeDataContainerName_Key, true);
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
  auto pVertexGeometryPath = filterArgs.value<DataPath>(k_VertexDataContainerName_Key);
  auto pImageGeometryPath = filterArgs.value<DataPath>(k_VolumeDataContainerName_Key);
  auto pVertexAttributeMatrixNameValue = filterArgs.value<DataObjectNameParameter::ValueType>(k_VertexAttributeMatrixName_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<DataObjectNameParameter::ValueType>(k_CellAttributeMatrixName_Key);

  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  StopWatch sw;
  sw.start();
  VtkLegacyFileReader legacyReader(pInputFileValue);
  legacyReader.setPreflight(true);
  resultOutputActions = legacyReader.preflightFile(pReadPointDataValue, pReadCellDataValue, pVertexGeometryPath, pImageGeometryPath, pVertexAttributeMatrixNameValue, pCellAttributeMatrixNameValue);
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
  inputValues.PointGeometryPath = filterArgs.value<DataPath>(k_VertexDataContainerName_Key);
  inputValues.CellGeometryPath = filterArgs.value<DataPath>(k_VolumeDataContainerName_Key);
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
