#include "ReadAngDataFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/ReadAngData.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/CreateDataGroupAction.hpp"
#include "complex/Filter/Actions/CreateImageGeometryAction.hpp"
#include "complex/Filter/Actions/CreateStringArrayAction.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"

#include "EbsdLib/IO/TSL/AngFields.h"
#include "EbsdLib/IO/TSL/AngPhase.h"
#include "EbsdLib/IO/TSL/AngReader.h"

#include "complex/Utilities/SIMPLConversion.hpp"

#include <filesystem>

namespace fs = std::filesystem;

namespace complex
{
//------------------------------------------------------------------------------
std::string ReadAngDataFilter::name() const
{
  return FilterTraits<ReadAngDataFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ReadAngDataFilter::className() const
{
  return FilterTraits<ReadAngDataFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ReadAngDataFilter::uuid() const
{
  return FilterTraits<ReadAngDataFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ReadAngDataFilter::humanName() const
{
  return "Read EDAX EBSD Data (.ang)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ReadAngDataFilter::defaultTags() const
{
  return {className(), "IO", "Input", "Read", "Import", "EDAX", "ANG", "EBSD"};
}

//------------------------------------------------------------------------------
Parameters ReadAngDataFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<FileSystemPathParameter>(k_InputFile_Key, "Input File", "The input .ang file path", fs::path("input.ang"), FileSystemPathParameter::ExtensionsType{".ang"},
                                                          FileSystemPathParameter::PathType::InputFile));
  params.insertSeparator(Parameters::Separator{"Created Data Structure Objects"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_DataContainerName_Key, "Created Image Geometry", "The complete path to the Geometry being created.", DataPath({"DataContainer"})));
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_CellAttributeMatrixName_Key, "Created Cell Attribute Matrix", "The Attribute Matrix where the scan data is stored.", ImageGeom::k_CellDataName));
  params.insert(std::make_unique<DataObjectNameParameter>(k_CellEnsembleAttributeMatrixName_Key, "Created Cell Ensemble Attribute Matrix",
                                                          "The Attribute Matrix where the phase information is stored.", "CellEnsembleData"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ReadAngDataFilter::clone() const
{
  return std::make_unique<ReadAngDataFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ReadAngDataFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                          const std::atomic_bool& shouldCancel) const
{
  auto pInputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  auto pImageGeometryPath = filterArgs.value<DataPath>(k_DataContainerName_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<std::string>(k_CellAttributeMatrixName_Key);
  auto pCellEnsembleAttributeMatrixNameValue = filterArgs.value<std::string>(k_CellEnsembleAttributeMatrixName_Key);

  PreflightResult preflightResult;

  AngReader reader;
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

  ss << "Grid: " << reader.getGrid() << "\n"
     << "X Step: " << reader.getXStep() << "    Y Step: " << reader.getYStep() << "\n"
     << "Num Odd Cols: " << reader.getNumOddCols() << "    "
     << "Num Even Cols: " << reader.getNumEvenCols() << "    "
     << "Num Rows: " << reader.getNumRows() << "\n"
     << "Sample Physical Dimensions: " << (reader.getXStep() * reader.getNumOddCols()) << " (W) x " << (reader.getYStep() * reader.getNumRows()) << " (H) microns"
     << "\n";
  std::string fileInfo = ss.str();

  std::vector<PreflightValue> preflightUpdatedValues = {{"Ang File Information", fileInfo}};

  // Define a custom class that generates the changes to the DataStructure.
  auto createImageGeometryAction = std::make_unique<CreateImageGeometryAction>(pImageGeometryPath, CreateImageGeometryAction::DimensionType({imageGeomDims[0], imageGeomDims[1], imageGeomDims[2]}),
                                                                               origin, spacing, pCellAttributeMatrixNameValue, IGeometry::LengthUnit::Micrometer);

  // Assign the createImageGeometryAction to the Result<OutputActions>::actions vector via a push_back
  complex::Result<OutputActions> resultOutputActions;
  resultOutputActions.value().appendAction(std::move(createImageGeometryAction));

  DataPath cellAttributeMatrixPath = pImageGeometryPath.createChildPath(pCellAttributeMatrixNameValue);

  AngFields angFeatures;
  auto names = angFeatures.getFilterFeatures<std::vector<std::string>>();
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
    DataPath dataArrayPath = cellAttributeMatrixPath.createChildPath(EbsdLib::AngFile::Phases);
    auto action = std::make_unique<CreateArrayAction>(complex::DataType::int32, tupleDims, cDims, dataArrayPath);
    resultOutputActions.value().appendAction(std::move(action));
  }

  // Create the Cell Euler Angles Array
  {
    cDims[0] = 3;
    DataPath dataArrayPath = cellAttributeMatrixPath.createChildPath(EbsdLib::AngFile::EulerAngles);
    auto action = std::make_unique<CreateArrayAction>(complex::DataType::float32, tupleDims, cDims, dataArrayPath);
    resultOutputActions.value().appendAction(std::move(action));
  }

  // Create the Ensemble AttributeMatrix
  DataPath ensembleAttributeMatrixPath = pImageGeometryPath.createChildPath(pCellEnsembleAttributeMatrixNameValue);
  {
    auto createDataGroupAction = std::make_unique<CreateDataGroupAction>(ensembleAttributeMatrixPath);
    resultOutputActions.value().appendAction(std::move(createDataGroupAction));
  }

  std::vector<std::shared_ptr<AngPhase>> angPhases = reader.getPhaseVector();
  tupleDims = {angPhases.size() + 1}; // Always create 1 extra slot for the phases.
  // Create the Crystal Structures Array
  {
    cDims[0] = 1;
    DataPath dataArrayPath = ensembleAttributeMatrixPath.createChildPath(EbsdLib::AngFile::CrystalStructures);
    auto action = std::make_unique<CreateArrayAction>(complex::DataType::uint32, tupleDims, cDims, dataArrayPath);
    resultOutputActions.value().appendAction(std::move(action));
  }
  // Create the Lattice Constants Array
  {
    cDims[0] = 6;
    DataPath dataArrayPath = ensembleAttributeMatrixPath.createChildPath(EbsdLib::AngFile::LatticeConstants);
    auto action = std::make_unique<CreateArrayAction>(complex::DataType::float32, tupleDims, cDims, dataArrayPath);
    resultOutputActions.value().appendAction(std::move(action));
  }
  // Create the Material Names Array
  {
    DataPath dataArrayPath = ensembleAttributeMatrixPath.createChildPath(EbsdLib::AngFile::MaterialName);
    auto action = std::make_unique<CreateStringArrayAction>(tupleDims, dataArrayPath);
    resultOutputActions.value().appendAction(std::move(action));
  }

  if(reader.getGrid() == "HexGrid")
  {
    return {MakeErrorResult<OutputActions>(-19500, "Ang file has HexGrid. AngReader ONLY knows how to read SquareGrid"), std::move(preflightUpdatedValues)};
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ReadAngDataFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                        const std::atomic_bool& shouldCancel) const
{
  ReadAngDataInputValues inputValues;

  inputValues.InputFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  inputValues.DataContainerName = filterArgs.value<DataPath>(k_DataContainerName_Key);
  inputValues.CellAttributeMatrixName = filterArgs.value<std::string>(k_CellAttributeMatrixName_Key);
  inputValues.CellEnsembleAttributeMatrixName = filterArgs.value<std::string>(k_CellEnsembleAttributeMatrixName_Key);

  ReadAngData readAngData(dataStructure, messageHandler, shouldCancel, &inputValues);
  return readAngData();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_InputFileKey = "InputFile";
constexpr StringLiteral k_DataContainerNameKey = "DataContainerName";
constexpr StringLiteral k_CellAttributeMatrixNameKey = "CellAttributeMatrixName";
constexpr StringLiteral k_CellEnsembleAttributeMatrixNameKey = "CellEnsembleAttributeMatrixName";
} // namespace SIMPL
} // namespace

Result<Arguments> ReadAngDataFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ReadAngDataFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::InputFileFilterParameterConverter>(args, json, SIMPL::k_InputFileKey, k_InputFile_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerCreationFilterParameterConverter>(args, json, SIMPL::k_DataContainerNameKey, k_DataContainerName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_CellAttributeMatrixNameKey, k_CellAttributeMatrixName_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_CellEnsembleAttributeMatrixNameKey, k_CellEnsembleAttributeMatrixName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace complex
