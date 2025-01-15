#include "ReadChannel5DataFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/ReadChannel5Data.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "simplnx/Filter/Actions/CreateImageGeometryAction.hpp"
#include "simplnx/Filter/Actions/CreateStringArrayAction.hpp"
#include "simplnx/Filter/Actions/DeleteDataAction.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include "EbsdLib/IO/HKL/CprReader.h"
#include "EbsdLib/IO/HKL/CtfFields.h"
#include "EbsdLib/IO/HKL/CtfPhase.h"

#include <filesystem>

namespace fs = std::filesystem;

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ReadChannel5DataFilter::name() const
{
  return FilterTraits<ReadChannel5DataFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ReadChannel5DataFilter::className() const
{
  return FilterTraits<ReadChannel5DataFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ReadChannel5DataFilter::uuid() const
{
  return FilterTraits<ReadChannel5DataFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ReadChannel5DataFilter::humanName() const
{
  return "Read Oxford Instr. Channel 5 (.cpr/.crc)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ReadChannel5DataFilter::defaultTags() const
{
  return {className(), "IO", "Input", "Read", "Import", "Oxford", "cpr", "crc", "Channel 5", "EBSD"};
}

//------------------------------------------------------------------------------
Parameters ReadChannel5DataFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<FileSystemPathParameter>(k_InputFile_Key, "Input File (.cpr)", "The input .cpr file path. The .crc file must also exist.", fs::path("input.cpr"),
                                                          FileSystemPathParameter::ExtensionsType{".cpr"}, FileSystemPathParameter::PathType::InputFile));
  params.insert(std::make_unique<BoolParameter>(k_EdaxHexagonalAlignment_Key, "Convert Hexagonal X-Axis to EDAX Standard",
                                                "Whether or not to convert a Hexagonal phase to the EDAX standard for x-axis alignment", false));
  params.insert(std::make_unique<BoolParameter>(k_CreateCompatibleArrays_Key, "Convert Data Arrays to be more compatible with DREAM3D",
                                                "Some arrays will be converted from the type they are in the file to more compatible types.", true));

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
IFilter::VersionType ReadChannel5DataFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ReadChannel5DataFilter::clone() const
{
  return std::make_unique<ReadChannel5DataFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ReadChannel5DataFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                               const std::atomic_bool& shouldCancel) const
{
  auto pInputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  auto pImageGeometryPath = filterArgs.value<DataPath>(k_CreatedImageGeometryPath_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<std::string>(k_CellAttributeMatrixName_Key);
  auto pCellEnsembleAttributeMatrixNameValue = filterArgs.value<std::string>(k_CellEnsembleAttributeMatrixName_Key);
  auto pCreateCompatibleTypes = filterArgs.value<bool>(k_CreateCompatibleArrays_Key);

  PreflightResult preflightResult;

  // First validate that the matching .crc file exists
  fs::path crcPath = pInputFileValue;
  crcPath.replace_extension(".crc");
  try
  {
    if(!std::filesystem::exists(crcPath))
    {
      return {MakeErrorResult<OutputActions>(-66500, fmt::format("Matching .crc file does not exist. '{}' ", crcPath.string())), {}};
    }
  } catch(std::exception& e)
  {
    return {MakeErrorResult<OutputActions>(-66501, fmt::format("Attempting to figure out if '{}' exists threw a std::exception. Please report this to the developers.", crcPath.string())), {}};
  }

  // Read the CPR file to gather the Geometry information and what arrays are present in the file.
  CprReader reader;
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
  // user interface.
  std::stringstream ss;
  std::array<float, 3> halfRes = {spacing[0] * 0.5F, spacing[1] * 0.5F, spacing[2] * 0.5F};

  ss << "X Step: " << reader.getXStep() << "    Y Step: " << reader.getYStep() << "\n"
     << "Num Cols: " << reader.getXCells() << "    "
     << "Num Rows: " << reader.getYCells() << "\n"
     << "Sample Physical Dimensions: " << (reader.getXStep() * reader.getXCells()) << " (W) x " << (reader.getYStep() * reader.getYCells()) << " (H) microns"
     << "\n";
  std::string fileInfo = ss.str();
  std::vector<PreflightValue> preflightUpdatedValues = {{"Cpr File Information", fileInfo}};

  // Define an Action that makes changes to the DataStructure
  auto createImageGeometryAction = std::make_unique<CreateImageGeometryAction>(pImageGeometryPath, CreateImageGeometryAction::DimensionType({imageGeomDims[0], imageGeomDims[1], imageGeomDims[2]}),
                                                                               origin, spacing, pCellAttributeMatrixNameValue, IGeometry::LengthUnit::Micrometer);

  // Assign the createImageGeometryAction to the Result<OutputActions>::actions vector via an appendAction
  nx::core::Result<OutputActions> resultOutputActions;
  resultOutputActions.value().appendAction(std::move(createImageGeometryAction));

  DataPath cellAttributeMatrixPath = pImageGeometryPath.createChildPath(pCellAttributeMatrixNameValue);
  std::vector<size_t> cDims = {1ULL};

  auto fieldParsers = reader.createFieldParsers(pInputFileValue.string());
  for(const auto& parser : fieldParsers)
  {
    if(parser.FieldDefinition.FieldName.empty())
    {
      std::cout << "Field Name was empty?\n";
      continue;
    }
    DataPath dataArrayPath = cellAttributeMatrixPath.createChildPath(parser.FieldDefinition.FieldName);

    if(parser.FieldDefinition.numericType == EbsdLib::NumericTypes::Type::Int32)
    {
      auto action = std::make_unique<CreateArrayAction>(nx::core::DataType::int32, tupleDims, cDims, dataArrayPath);
      resultOutputActions.value().appendAction(std::move(action));
    }
    else if(parser.FieldDefinition.numericType == EbsdLib::NumericTypes::Type::Float)
    {
      auto action = std::make_unique<CreateArrayAction>(nx::core::DataType::float32, tupleDims, cDims, dataArrayPath);
      resultOutputActions.value().appendAction(std::move(action));
    }
    else if(parser.FieldDefinition.numericType == EbsdLib::NumericTypes::Type::UInt8)
    {
      auto action = std::make_unique<CreateArrayAction>(nx::core::DataType::uint8, tupleDims, cDims, dataArrayPath);
      resultOutputActions.value().appendAction(std::move(action));
    }
  }

  //  // Create the Cell Phases Array
  if(pCreateCompatibleTypes)
  {
    cDims[0] = 1;
    DataPath dataArrayPath = cellAttributeMatrixPath.createChildPath(EbsdLib::CtfFile::Phases);
    auto action = std::make_unique<CreateArrayAction>(nx::core::DataType::int32, tupleDims, cDims, dataArrayPath);
    resultOutputActions.value().appendAction(std::move(action));

    dataArrayPath = cellAttributeMatrixPath.createChildPath(EbsdLib::Ctf::Phase);
    ;
    auto removeTempArrayAction = std::make_unique<DeleteDataAction>(dataArrayPath);
    resultOutputActions.value().appendDeferredAction(std::move(removeTempArrayAction));
  }
  //
  //  // Create the Cell Euler Angles Array
  if(pCreateCompatibleTypes)
  {
    cDims[0] = 3;
    DataPath dataArrayPath = cellAttributeMatrixPath.createChildPath(EbsdLib::CtfFile::EulerAngles);
    auto action = std::make_unique<CreateArrayAction>(nx::core::DataType::float32, tupleDims, cDims, dataArrayPath);
    resultOutputActions.value().appendAction(std::move(action));

    dataArrayPath = cellAttributeMatrixPath.createChildPath(EbsdLib::Ctf::phi1);
    ;
    auto removeTempArrayAction = std::make_unique<DeleteDataAction>(dataArrayPath);
    resultOutputActions.value().appendDeferredAction(std::move(removeTempArrayAction));

    dataArrayPath = cellAttributeMatrixPath.createChildPath(EbsdLib::Ctf::Phi);
    ;
    removeTempArrayAction = std::make_unique<DeleteDataAction>(dataArrayPath);
    resultOutputActions.value().appendDeferredAction(std::move(removeTempArrayAction));

    dataArrayPath = cellAttributeMatrixPath.createChildPath(EbsdLib::Ctf::phi2);
    ;
    removeTempArrayAction = std::make_unique<DeleteDataAction>(dataArrayPath);
    resultOutputActions.value().appendDeferredAction(std::move(removeTempArrayAction));
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
    auto action = std::make_unique<CreateArrayAction>(nx::core::DataType::uint32, tupleDims, cDims, dataArrayPath);
    resultOutputActions.value().appendAction(std::move(action));
  }
  // Create the Lattice Constants Array
  {
    cDims[0] = 6;
    DataPath dataArrayPath = ensembleAttributeMatrixPath.createChildPath(EbsdLib::CtfFile::LatticeConstants);
    auto action = std::make_unique<CreateArrayAction>(nx::core::DataType::float32, tupleDims, cDims, dataArrayPath);
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
Result<> ReadChannel5DataFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                             const std::atomic_bool& shouldCancel) const
{
  ReadChannel5DataInputValues inputValues;

  inputValues.InputFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  inputValues.EdaxHexagonalAlignment = filterArgs.value<bool>(k_EdaxHexagonalAlignment_Key);
  inputValues.DataContainerName = filterArgs.value<DataPath>(k_CreatedImageGeometryPath_Key);
  inputValues.CellAttributeMatrixName = filterArgs.value<std::string>(k_CellAttributeMatrixName_Key);
  inputValues.CellEnsembleAttributeMatrixName = filterArgs.value<std::string>(k_CellEnsembleAttributeMatrixName_Key);
  inputValues.CreateCompatibleArrays = filterArgs.value<bool>(k_CreateCompatibleArrays_Key);
  return ReadChannel5Data(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

} // namespace nx::core
