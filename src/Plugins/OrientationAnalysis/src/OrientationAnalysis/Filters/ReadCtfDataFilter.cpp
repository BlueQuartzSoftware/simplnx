#include "ReadCtfDataFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/ReadCtfData.hpp"
#include "OrientationAnalysis/utilities/EbsdReaderUtilities.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "simplnx/Filter/Actions/CreateImageGeometryAction.hpp"
#include "simplnx/Filter/Actions/CreateStringArrayAction.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <EbsdLib/IO/HKL/CtfConstants.h>
#include <EbsdLib/IO/HKL/CtfFields.h>
#include <EbsdLib/IO/HKL/CtfReader.h>

#include <fmt/format.h>

#include <filesystem>

namespace fs = std::filesystem;

using namespace nx::core;

namespace nx::core
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
  return "Read Oxford Instr. EBSD Data (.ctf)";
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
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<FileSystemPathParameter>(k_InputFile_Key, "Input File", "The input .ctf file path", fs::path("input.ctf"), FileSystemPathParameter::ExtensionsType{".ctf"},
                                                          FileSystemPathParameter::PathType::InputFile));
  params.insert(std::make_unique<BoolParameter>(k_DegreesToRadians_Key, "Convert Euler Angles to Radians", "Whether or not to convert the Euler angles to Radians", true));
  params.insert(std::make_unique<BoolParameter>(k_EdaxHexagonalAlignment_Key, "Convert Hexagonal X-Axis to EDAX Standard",
                                                "Whether or not to convert a Hexagonal phase to the EDAX standard for x-axis alignment", true));

  params.insertSeparator(Parameters::Separator{"Output Image Geometry"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_CreatedImageGeometryPath_Key, "Image Geometry", "The path to the created Image Geometry", DataPath({ImageGeom::k_TypeName})));
  params.insertSeparator(Parameters::Separator{"Output Cell Attribute Matrix"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_CellAttributeMatrixName_Key, "Cell Attribute Matrix", "The name of the cell data attribute matrix for the created Image Geometry",
                                                          ImageGeom::k_CellAttributeMatrixName));
  params.insertSeparator(Parameters::Separator{"Output Ensemble Attribute Matrix"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_CellEnsembleAttributeMatrixName_Key, "Ensemble Attribute Matrix", "The Attribute Matrix where the phase information is stored.",
                                                          "Cell Ensemble Data"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ReadCtfDataFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ReadCtfDataFilter::clone() const
{
  return std::make_unique<ReadCtfDataFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ReadCtfDataFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                                          const ExecutionContext& executionContext) const
{
  auto pInputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  auto pImageGeometryPath = filterArgs.value<DataPath>(k_CreatedImageGeometryPath_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<std::string>(k_CellAttributeMatrixName_Key);
  auto pCellEnsembleAttributeMatrixNameValue = filterArgs.value<std::string>(k_CellEnsembleAttributeMatrixName_Key);

  std::vector<PreflightValue> preflightUpdatedValues;

  ebsdlib::CtfReader reader;
  reader.setFileName(pInputFileValue.string());
  const int32 err = reader.readHeaderOnly();
  if(err < 0)
  {
    // CtfReader does not set its error-code member on every failure path, so fall back to the returned code.
    const int32 errorCode = reader.getErrorCode() < 0 ? reader.getErrorCode() : err;
    return {MakeErrorResult<OutputActions>(errorCode, reader.getErrorMessage())};
  }

  // readHeaderOnly() can accept a header with zero dimensions.
  // Reject it before creating a zero-sized Image Geometry.
  if(reader.getXCells() < 1 || reader.getYCells() < 1)
  {
    return {MakeErrorResult<OutputActions>(-19604, fmt::format("The .ctf file header reports X Cells = {} and Y Cells = {}. Both must be at least 1. The file may be malformed or not a .ctf file.",
                                                               reader.getXCells(), reader.getYCells()))};
  }

  // .ctf files can carry more than one slice (ZCells > 1). A missing ZCells key parses as its
  // default of 1. A NEGATIVE value must be rejected here: CtfReader::readData() captures its slice
  // loop bound before clamping the value, so it would read zero data lines while sizing its buffers
  // for one slice — the copies would then see only sentinel bytes, never the file's data.
  if(reader.getZCells() < 0)
  {
    return {MakeErrorResult<OutputActions>(
        -19604, fmt::format("The .ctf file header reports Z Cells = {}. A negative slice count is not usable. The file may be malformed or not a .ctf file.", reader.getZCells()))};
  }
  // A ZCells of 0 slips through the header-only read; it is caught at execute by the -19603
  // reader/geometry cell-count guard.
  const size_t zCells = reader.getZCells() < 1 ? 1 : static_cast<size_t>(reader.getZCells());
  CreateImageGeometryAction::DimensionType imageGeomDims = {static_cast<size_t>(reader.getXCells()), static_cast<size_t>(reader.getYCells()), zCells};
  std::vector<size_t> tupleDims = {imageGeomDims[2], imageGeomDims[1], imageGeomDims[0]};

  // A 2D .ctf file has no ZStep key (parses as 0), in which case the slice thickness defaults to 1.
  // A negative ZStep is equally unusable and gets the same default.
  const float32 zStep = reader.getZStep() > 0.0F ? reader.getZStep() : 1.0F;
  CreateImageGeometryAction::SpacingType spacing = {reader.getXStep(), reader.getYStep(), zStep};
  CreateImageGeometryAction::OriginType origin = {0.0F, 0.0F, 0.0F};

  EbsdReaderUtilities::GeneratePreflightScanInformation<ebsdlib::CtfReader>(reader, preflightUpdatedValues);
  EbsdReaderUtilities::GeneratePreflightPhaseInformation<ebsdlib::CtfReader>(reader, preflightUpdatedValues);

  auto createImageGeometryAction = std::make_unique<CreateImageGeometryAction>(pImageGeometryPath, imageGeomDims, origin, spacing, pCellAttributeMatrixNameValue, IGeometry::LengthUnit::Micrometer);

  nx::core::Result<OutputActions> resultOutputActions;
  resultOutputActions.value().appendAction(std::move(createImageGeometryAction));

  DataPath cellAttributeMatrixPath = pImageGeometryPath.createChildPath(pCellAttributeMatrixNameValue);

  // These are the 7 pass-through columns; the copy loop in Algorithms/ReadCtfData.cpp
  // (passthroughColumns) must stay in lockstep with CtfFields::getFilterFeatures().
  ebsdlib::CtfFields ctfFeatures;
  const auto names = ctfFeatures.getFilterFeatures<std::vector<std::string>>();
  std::vector<size_t> cDims = {1ULL};

  for(const auto& name : names)
  {
    if(reader.getPointerType(name) == ebsdlib::NumericTypes::Type::Int32)
    {
      DataPath dataArrayPath = cellAttributeMatrixPath.createChildPath(name);
      auto action = std::make_unique<CreateArrayAction>(nx::core::DataType::int32, tupleDims, cDims, dataArrayPath);
      resultOutputActions.value().appendAction(std::move(action));
    }
    else if(reader.getPointerType(name) == ebsdlib::NumericTypes::Type::Float)
    {
      DataPath dataArrayPath = cellAttributeMatrixPath.createChildPath(name);
      auto action = std::make_unique<CreateArrayAction>(nx::core::DataType::float32, tupleDims, cDims, dataArrayPath);
      resultOutputActions.value().appendAction(std::move(action));
    }
  }

  // Create the Cell Phases Array
  {
    cDims[0] = 1;
    DataPath dataArrayPath = cellAttributeMatrixPath.createChildPath(ebsdlib::CtfFile::Phases);
    auto action = std::make_unique<CreateArrayAction>(nx::core::DataType::int32, tupleDims, cDims, dataArrayPath);
    resultOutputActions.value().appendAction(std::move(action));
  }

  // Create the Cell Euler Angles Array
  {
    cDims[0] = 3;
    DataPath dataArrayPath = cellAttributeMatrixPath.createChildPath(ebsdlib::CtfFile::EulerAngles);
    auto action = std::make_unique<CreateArrayAction>(nx::core::DataType::float32, tupleDims, cDims, dataArrayPath);
    resultOutputActions.value().appendAction(std::move(action));
  }

  // Slot 0 is reserved for the invalid phase.
  // CtfReader assigns later phases sequentially, so tuple count is phase count plus one.
  tupleDims = {reader.getPhaseVector().size() + 1};
  DataPath ensembleAttributeMatrixPath = pImageGeometryPath.createChildPath(pCellEnsembleAttributeMatrixNameValue);
  {
    auto createAttributeMatrixAction = std::make_unique<CreateAttributeMatrixAction>(ensembleAttributeMatrixPath, tupleDims);
    resultOutputActions.value().appendAction(std::move(createAttributeMatrixAction));
  }

  // Create the Crystal Structures Array
  {
    cDims[0] = 1;
    DataPath dataArrayPath = ensembleAttributeMatrixPath.createChildPath(ebsdlib::CtfFile::CrystalStructures);
    auto action = std::make_unique<CreateArrayAction>(nx::core::DataType::uint32, tupleDims, cDims, dataArrayPath);
    resultOutputActions.value().appendAction(std::move(action));
  }
  // Create the Lattice Constants Array
  {
    cDims[0] = 6;
    DataPath dataArrayPath = ensembleAttributeMatrixPath.createChildPath(ebsdlib::CtfFile::LatticeConstants);
    auto action = std::make_unique<CreateArrayAction>(nx::core::DataType::float32, tupleDims, cDims, dataArrayPath);
    resultOutputActions.value().appendAction(std::move(action));
  }
  // Create the Material Names Array
  {
    DataPath dataArrayPath = ensembleAttributeMatrixPath.createChildPath(ebsdlib::CtfFile::MaterialName);
    auto action = std::make_unique<CreateStringArrayAction>(tupleDims, dataArrayPath);
    resultOutputActions.value().appendAction(std::move(action));
  }

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ReadCtfDataFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                        const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  ReadCtfDataInputValues inputValues;

  inputValues.InputFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  inputValues.DegreesToRadians = filterArgs.value<bool>(k_DegreesToRadians_Key);
  inputValues.EdaxHexagonalAlignment = filterArgs.value<bool>(k_EdaxHexagonalAlignment_Key);
  inputValues.DataContainerName = filterArgs.value<DataPath>(k_CreatedImageGeometryPath_Key);
  inputValues.CellAttributeMatrixName = filterArgs.value<std::string>(k_CellAttributeMatrixName_Key);
  inputValues.CellEnsembleAttributeMatrixName = filterArgs.value<std::string>(k_CellEnsembleAttributeMatrixName_Key);

  ReadCtfData readCtfData(dataStructure, messageHandler, shouldCancel, &inputValues);
  return readCtfData();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_InputFileKey = "InputFile";
constexpr StringLiteral k_DegreesToRadiansKey = "DegreesToRadians";
constexpr StringLiteral k_EdaxHexagonalAlignmentKey = "EdaxHexagonalAlignment";
constexpr StringLiteral k_DataContainerNameKey = "DataContainerName";
constexpr StringLiteral k_CellAttributeMatrixNameKey = "CellAttributeMatrixName";
constexpr StringLiteral k_CellEnsembleAttributeMatrixNameKey = "CellEnsembleAttributeMatrixName";
} // namespace SIMPL
} // namespace

Result<Arguments> ReadCtfDataFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ReadCtfDataFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::InputFileFilterParameterConverter>(args, json, SIMPL::k_InputFileKey, k_InputFile_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_DegreesToRadiansKey, k_DegreesToRadians_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_EdaxHexagonalAlignmentKey, k_EdaxHexagonalAlignment_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DCPathBuilderFilterParameterConverter>(args, json, SIMPL::k_DataContainerNameKey, k_CreatedImageGeometryPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_CellAttributeMatrixNameKey, k_CellAttributeMatrixName_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_CellEnsembleAttributeMatrixNameKey, k_CellEnsembleAttributeMatrixName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
