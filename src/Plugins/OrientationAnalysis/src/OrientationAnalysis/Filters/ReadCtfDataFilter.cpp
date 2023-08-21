#include "ReadCtfDataFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/ReadCtfData.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "complex/Filter/Actions/CreateImageGeometryAction.hpp"
#include "complex/Filter/Actions/CreateStringArrayAction.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"

#include "EbsdLib/IO/HKL/CtfFields.h"
#include "EbsdLib/IO/HKL/CtfPhase.h"
#include "EbsdLib/IO/HKL/CtfReader.h"

#include <filesystem>

namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ReadCtfDataFilter::name() const
{
  return FilterTraits<ReadCtfDataFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ReadCtfDataFilter::className() const
{
  return FilterTraits<ReadCtfDataFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ReadCtfDataFilter::uuid() const
{
  return FilterTraits<ReadCtfDataFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ReadCtfDataFilter::humanName() const
{
  return "Import Oxford Instr. EBSD Data (.ctf)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ReadCtfDataFilter::defaultTags() const
{
  return {className(), "IO", "Input", "Read", "Import", "Oxford", "CTF", "EBSD"};
}

//------------------------------------------------------------------------------
Parameters ReadCtfDataFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<FileSystemPathParameter>(k_InputFile_Key, "Input File", "The input .ctf file path", fs::path("input.ctf"), FileSystemPathParameter::ExtensionsType{".ctf"},
                                                          FileSystemPathParameter::PathType::InputFile));
  params.insert(std::make_unique<BoolParameter>(k_DegreesToRadians_Key, "Convert Euler Angles to Radians", "Whether or not to convert the Euler angles to Radians", false));
  params.insert(std::make_unique<BoolParameter>(k_EdaxHexagonalAlignment_Key, "Convert Hexagonal X-Axis to EDAX Standard",
                                                "Whether or not to convert a Hexagonal phase to the EDAX standard for x-axis alignment", false));

  params.insertSeparator(Parameters::Separator{"Created Data Structure Objects"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_DataContainerName_Key, "Created Image Geometry", "The complete path to the Geometry being created.", DataPath({"DataContainer"})));
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_CellAttributeMatrixName_Key, "Created Cell Attribute Matrix", "The Attribute Matrix where the scan data is stored.", ImageGeom::k_CellDataName));
  params.insert(std::make_unique<DataObjectNameParameter>(k_CellEnsembleAttributeMatrixName_Key, "Created Cell Ensemble Attribute Matrix",
                                                          "The Attribute Matrix where the phase information is stored.", "CellEnsembleData"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ReadCtfDataFilter::clone() const
{
  return std::make_unique<ReadCtfDataFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ReadCtfDataFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                          const std::atomic_bool& shouldCancel) const
{
  auto pInputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  auto pImageGeometryPath = filterArgs.value<DataPath>(k_DataContainerName_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<std::string>(k_CellAttributeMatrixName_Key);
  auto pCellEnsembleAttributeMatrixNameValue = filterArgs.value<std::string>(k_CellEnsembleAttributeMatrixName_Key);

  PreflightResult preflightResult;

  CtfReader reader;
  reader.setFileName(pInputFileValue.string());
  int32_t err = reader.readHeaderOnly();
  if(err < 0)
  {
    return {MakeErrorResult<OutputActions>(reader.getErrorCode(), reader.getErrorMessage())};
  }

  CreateImageGeometryAction::DimensionType imageGeomDims = {static_cast<size_t>(reader.getXDimension()), static_cast<size_t>(reader.getYDimension()), static_cast<size_t>(1)};
  std::vector<size_t> tupleDims = {imageGeomDims[2], imageGeomDims[1], imageGeomDims[0]};

  CreateImageGeometryAction::SpacingType spacing = {reader.getXStep(), reader.getYStep(), 1.0F};
  CreateImageGeometryAction::OriginType origin = {0.0F, 0.0F, 0.0F};

  // These variables should be updated with the latest data generated for each variable during preflight.
  // These will be returned through the preflightResult variable to the
  // user interface. You could make these member variables instead if needed.
  std::stringstream ss;
  std::array<float, 3> halfRes = {spacing[0] * 0.5F, spacing[1] * 0.5F, spacing[2] * 0.5F};

  ss << "X Step: " << reader.getXStep() << "    Y Step: " << reader.getYStep() << "\n"
     << "Num Cols: " << reader.getXCells() << "    "
     << "Num Rows: " << reader.getYCells() << "\n"
     << "Sample Physical Dimensions: " << (reader.getXStep() * reader.getXCells()) << " (W) x " << (reader.getYStep() * reader.getYCells()) << " (H) microns"
     << "\n";
  std::string fileInfo = ss.str();
  std::vector<PreflightValue> preflightUpdatedValues = {{"Ctf File Information", fileInfo}};

  // Define a custom class that generates the changes to the DataStructure.
  auto createImageGeometryAction = std::make_unique<CreateImageGeometryAction>(pImageGeometryPath, CreateImageGeometryAction::DimensionType({imageGeomDims[0], imageGeomDims[1], imageGeomDims[2]}),
                                                                               origin, spacing, pCellAttributeMatrixNameValue);

  // Assign the createImageGeometryAction to the Result<OutputActions>::actions vector via a push_back
  complex::Result<OutputActions> resultOutputActions;
  resultOutputActions.value().appendAction(std::move(createImageGeometryAction));

  DataPath cellAttributeMatrixPath = pImageGeometryPath.createChildPath(pCellAttributeMatrixNameValue);

  CtfFields ctfFeatures;
  const auto names = ctfFeatures.getFilterFeatures<std::vector<std::string>>();
  std::vector<size_t> cDims = {1ULL};

  for(const auto& name : names)
  {
    if(reader.getPointerType(name) == EbsdLib::NumericTypes::Type::Int32)
    {
      DataPath dataArrayPath = cellAttributeMatrixPath.createChildPath(name);
      auto action = std::make_unique<CreateArrayAction>(complex::DataType::int32, tupleDims, cDims, dataArrayPath);
      resultOutputActions.value().appendAction(std::move(action));
    }
    else if(reader.getPointerType(name) == EbsdLib::NumericTypes::Type::Float)
    {
      DataPath dataArrayPath = cellAttributeMatrixPath.createChildPath(name);
      auto action = std::make_unique<CreateArrayAction>(complex::DataType::float32, tupleDims, cDims, dataArrayPath);
      resultOutputActions.value().appendAction(std::move(action));
    }
  }

  // Create the Cell Phases Array
  {
    cDims[0] = 1;
    DataPath dataArrayPath = cellAttributeMatrixPath.createChildPath(EbsdLib::CtfFile::Phases);
    auto action = std::make_unique<CreateArrayAction>(complex::DataType::int32, tupleDims, cDims, dataArrayPath);
    resultOutputActions.value().appendAction(std::move(action));
  }

  // Create the Cell Euler Angles Array
  {
    cDims[0] = 3;
    DataPath dataArrayPath = cellAttributeMatrixPath.createChildPath(EbsdLib::CtfFile::EulerAngles);
    auto action = std::make_unique<CreateArrayAction>(complex::DataType::float32, tupleDims, cDims, dataArrayPath);
    resultOutputActions.value().appendAction(std::move(action));
  }

  // Create the Ensemble AttributeMatrix
  std::vector<std::shared_ptr<CtfPhase>> angPhases = reader.getPhaseVector();
  tupleDims = {angPhases.size() + 1}; // Always create 1 extra slot for the phases.
  DataPath ensembleAttributeMatrixPath = pImageGeometryPath.createChildPath(pCellEnsembleAttributeMatrixNameValue);
  {
    auto createAttributeMatrixAction = std::make_unique<CreateAttributeMatrixAction>(ensembleAttributeMatrixPath, tupleDims);
    resultOutputActions.value().appendAction(std::move(createAttributeMatrixAction));
  }

  // Create the Crystal Structures Array
  {
    cDims[0] = 1;
    DataPath dataArrayPath = ensembleAttributeMatrixPath.createChildPath(EbsdLib::CtfFile::CrystalStructures);
    auto action = std::make_unique<CreateArrayAction>(complex::DataType::uint32, tupleDims, cDims, dataArrayPath);
    resultOutputActions.value().appendAction(std::move(action));
  }
  // Create the Lattice Constants Array
  {
    cDims[0] = 6;
    DataPath dataArrayPath = ensembleAttributeMatrixPath.createChildPath(EbsdLib::CtfFile::LatticeConstants);
    auto action = std::make_unique<CreateArrayAction>(complex::DataType::float32, tupleDims, cDims, dataArrayPath);
    resultOutputActions.value().appendAction(std::move(action));
  }
  // Create the Material Names Array
  {
    DataPath dataArrayPath = ensembleAttributeMatrixPath.createChildPath(EbsdLib::CtfFile::MaterialName);
    auto action = std::make_unique<CreateStringArrayAction>(tupleDims, dataArrayPath);
    resultOutputActions.value().appendAction(std::move(action));
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ReadCtfDataFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                        const std::atomic_bool& shouldCancel) const
{
  ReadCtfDataInputValues inputValues;

  inputValues.InputFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  inputValues.DegreesToRadians = filterArgs.value<bool>(k_DegreesToRadians_Key);
  inputValues.EdaxHexagonalAlignment = filterArgs.value<bool>(k_EdaxHexagonalAlignment_Key);
  inputValues.DataContainerName = filterArgs.value<DataPath>(k_DataContainerName_Key);
  inputValues.CellAttributeMatrixName = filterArgs.value<std::string>(k_CellAttributeMatrixName_Key);
  inputValues.CellEnsembleAttributeMatrixName = filterArgs.value<std::string>(k_CellEnsembleAttributeMatrixName_Key);

  ReadCtfData readCtfData(dataStructure, messageHandler, shouldCancel, &inputValues);
  return readCtfData();
}
} // namespace complex
