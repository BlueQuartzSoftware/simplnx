#include "ReadH5OinaDataFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/ReadH5OinaData.hpp"
#include "OrientationAnalysis/Parameters/OEMEbsdScanSelectionParameter.h"

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
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

#include "EbsdLib/IO/HKL/CtfFields.h"
#include "EbsdLib/IO/HKL/H5OINAReader.h"

#include <filesystem>
namespace fs = std::filesystem;

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ReadH5OinaDataFilter::name() const
{
  return FilterTraits<ReadH5OinaDataFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ReadH5OinaDataFilter::className() const
{
  return FilterTraits<ReadH5OinaDataFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ReadH5OinaDataFilter::uuid() const
{
  return FilterTraits<ReadH5OinaDataFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ReadH5OinaDataFilter::humanName() const
{
  return "Import Oxford Aztec Data (.h5oina)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ReadH5OinaDataFilter::defaultTags() const
{
  return {className(), "IO", "Input", "Read", "Import", "Oxford", "CTF", "H5OINA", "EBSD"};
}

//------------------------------------------------------------------------------
Parameters ReadH5OinaDataFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<OEMEbsdScanSelectionParameter>(k_SelectedScanNames_Key, "Scan Names", "The name of the scan in the .h5oina file. Oxford can store multiple scans in a single file",
                                                                OEMEbsdScanSelectionParameter::ValueType{},
                                                                /* OEMEbsdScanSelectionParameter::AllowedManufacturers{EbsdLib::OEM::EDAX}, */
                                                                OEMEbsdScanSelectionParameter::EbsdReaderType::H5Oina, OEMEbsdScanSelectionParameter::ExtensionsType{".h5oina"}));
  params.insert(std::make_unique<BoolParameter>(k_EdaxHexagonalAlignment_Key, "Convert Hexagonal X-Axis to EDAX Standard",
                                                "Whether or not to convert a Hexagonal phase to the EDAX standard for x-axis alignment", true));
  params.insert(std::make_unique<BoolParameter>(k_ConvertPhaseToInt32_Key, "Convert Phase Data to Int32", "Native Phases data value is uint8. Convert to Int32 for better filter compatibility", true));
  params.insert(std::make_unique<Float32Parameter>(k_ZSpacing_Key, "Z Spacing (Microns)", "The spacing in microns between each layer.", 1.0f));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Origin_Key, "Origin", "The origin of the volume", std::vector<float32>{0.0F, 0.0F, 0.0F}, std::vector<std::string>{"x", "y", "z"}));
  params.insert(std::make_unique<BoolParameter>(k_ReadPatternData_Key, "Import Pattern Data", "Whether or not to import the pattern data", false));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_ImageGeometryName_Key, "Image Geometry", "The path to the created Image Geometry", DataPath({ImageGeom::k_TypeName})));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_CellAttributeMatrixName_Key, "Cell Attribute Matrix", "The name of the cell data attribute matrix for the created Image Geometry",
                                                          ImageGeom::k_CellDataName));
  params.insertSeparator(Parameters::Separator{"Cell Ensemble Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_CellEnsembleAttributeMatrixName_Key, "Cell Ensemble Attribute Matrix",
                                                          "The name of the cell ensemble data attribute matrix for the created Image Geometry", "Cell Ensemble Data"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ReadH5OinaDataFilter::clone() const
{
  return std::make_unique<ReadH5OinaDataFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ReadH5OinaDataFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                             const std::atomic_bool& shouldCancel) const
{
  auto pSelectedScanNamesValue = filterArgs.value<OEMEbsdScanSelectionParameter::ValueType>(k_SelectedScanNames_Key);
  auto pZSpacingValue = filterArgs.value<float32>(k_ZSpacing_Key);
  auto pOriginValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Origin_Key);
  auto pReadPatternDataValue = filterArgs.value<bool>(k_ReadPatternData_Key);
  auto pImageGeometryNameValue = filterArgs.value<DataPath>(k_ImageGeometryName_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<std::string>(k_CellAttributeMatrixName_Key);
  auto pCellEnsembleAttributeMatrixNameValue = filterArgs.value<std::string>(k_CellEnsembleAttributeMatrixName_Key);
  auto pConvertPhaseData = filterArgs.value<bool>(k_ConvertPhaseToInt32_Key);

  DataPath cellEnsembleAMPath = pImageGeometryNameValue.createChildPath(pCellEnsembleAttributeMatrixNameValue);
  DataPath cellAMPath = pImageGeometryNameValue.createChildPath(pCellAttributeMatrixNameValue);

  PreflightResult preflightResult;
  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  if(pZSpacingValue <= 0)
  {
    return MakePreflightErrorResult(-9580, "The Z Spacing field contains a value that is non-positive.  The Z Spacing field must be set to a positive value.");
  }
  if(pSelectedScanNamesValue.scanNames.empty())
  {
    return MakePreflightErrorResult(-9581, "At least one scan must be chosen.  Please select a scan from the list.");
  }

  // read in the necessary info from the input h5 file
  H5OINAReader reader;
  reader.setFileName(pSelectedScanNamesValue.inputFilePath.string());
  reader.setReadPatternData(pReadPatternDataValue);
  reader.setHDF5Path(pSelectedScanNamesValue.scanNames.front());
  if(const int err = reader.readHeaderOnly(); err < 0)
  {
    return MakePreflightErrorResult(-9582, fmt::format("An error occurred while reading the header data\n{} : {}", err, reader.getErrorMessage()));
  }

  // create the Image Geometry and it's attribute matrices
  const CreateImageGeometryAction::DimensionType dims = {static_cast<usize>(reader.getXDimension()), static_cast<usize>(reader.getYDimension()), pSelectedScanNamesValue.scanNames.size()};
  const std::vector<usize> tupleDims = {dims[2], dims[1], dims[0]};
  {
    CreateImageGeometryAction::SpacingType spacing = {reader.getXStep(), reader.getYStep(), pZSpacingValue};

    auto createDataGroupAction = std::make_unique<CreateImageGeometryAction>(pImageGeometryNameValue, dims, pOriginValue, spacing, pCellAttributeMatrixNameValue);
    resultOutputActions.value().appendAction(std::move(createDataGroupAction));
  }
  const auto phases = reader.getPhaseVector();
  std::vector<usize> ensembleTupleDims{phases.size() + 1};
  {
    auto createAttributeMatrixAction = std::make_unique<CreateAttributeMatrixAction>(cellEnsembleAMPath, ensembleTupleDims);
    resultOutputActions.value().appendAction(std::move(createAttributeMatrixAction));
  }

  // create the cell ensemble arrays
  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::uint32, ensembleTupleDims, std::vector<usize>{1}, cellEnsembleAMPath.createChildPath(EbsdLib::CtfFile::CrystalStructures));
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }
  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::float32, ensembleTupleDims, std::vector<usize>{6}, cellEnsembleAMPath.createChildPath(EbsdLib::CtfFile::LatticeConstants));
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }
  {
    auto createArrayAction = std::make_unique<CreateStringArrayAction>(ensembleTupleDims, cellEnsembleAMPath.createChildPath(EbsdLib::CtfFile::MaterialName));
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  // create the cell data arrays
  resultOutputActions.value().appendAction(std::make_unique<CreateArrayAction>(DataType::uint8, tupleDims, std::vector<usize>{1}, cellAMPath.createChildPath(EbsdLib::H5OINA::BandContrast)));
  resultOutputActions.value().appendAction(std::make_unique<CreateArrayAction>(DataType::uint8, tupleDims, std::vector<usize>{1}, cellAMPath.createChildPath(EbsdLib::H5OINA::BandSlope)));
  resultOutputActions.value().appendAction(std::make_unique<CreateArrayAction>(DataType::uint8, tupleDims, std::vector<usize>{1}, cellAMPath.createChildPath(EbsdLib::H5OINA::Bands)));
  resultOutputActions.value().appendAction(std::make_unique<CreateArrayAction>(DataType::uint8, tupleDims, std::vector<usize>{1}, cellAMPath.createChildPath(EbsdLib::H5OINA::Error)));
  resultOutputActions.value().appendAction(std::make_unique<CreateArrayAction>(DataType::float32, tupleDims, std::vector<usize>{3}, cellAMPath.createChildPath(EbsdLib::H5OINA::Euler)));
  resultOutputActions.value().appendAction(std::make_unique<CreateArrayAction>(DataType::float32, tupleDims, std::vector<usize>{1}, cellAMPath.createChildPath(EbsdLib::H5OINA::MeanAngularDeviation)));
  if(pConvertPhaseData)
  {
    resultOutputActions.value().appendAction(std::make_unique<CreateArrayAction>(DataType::int32, tupleDims, std::vector<usize>{1}, cellAMPath.createChildPath(EbsdLib::H5OINA::Phase)));
  }
  else
  {
    resultOutputActions.value().appendAction(std::make_unique<CreateArrayAction>(DataType::uint8, tupleDims, std::vector<usize>{1}, cellAMPath.createChildPath(EbsdLib::H5OINA::Phase)));
  }
  resultOutputActions.value().appendAction(std::make_unique<CreateArrayAction>(DataType::float32, tupleDims, std::vector<usize>{1}, cellAMPath.createChildPath(EbsdLib::H5OINA::X)));
  resultOutputActions.value().appendAction(std::make_unique<CreateArrayAction>(DataType::float32, tupleDims, std::vector<usize>{1}, cellAMPath.createChildPath(EbsdLib::H5OINA::Y)));

  if(pReadPatternDataValue)
  {
    std::array<int32, 2> patternDims = {{0, 0}};
    reader.getPatternDims(patternDims);
    if(patternDims[0] == 0 || patternDims[1] == 0)
    {
      return MakePreflightErrorResult(-9583, fmt::format("The parameter 'Read Pattern Data' has been enabled but there does not seem to be any pattern data in the file for the scan name selected"));
    }
    auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::uint16, tupleDims, std::vector<usize>{static_cast<usize>(patternDims[0]), static_cast<usize>(patternDims[1])},
                                                                 cellAMPath.createChildPath(EbsdLib::H5OINA::UnprocessedPatterns));
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ReadH5OinaDataFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                           const std::atomic_bool& shouldCancel) const
{
  ReadH5DataInputValues inputValues;

  inputValues.SelectedScanNames = filterArgs.value<OEMEbsdScanSelectionParameter::ValueType>(k_SelectedScanNames_Key);
  inputValues.ReadPatternData = filterArgs.value<bool>(k_ReadPatternData_Key);
  inputValues.ImageGeometryPath = filterArgs.value<DataPath>(k_ImageGeometryName_Key);
  inputValues.CellEnsembleAttributeMatrixPath = inputValues.ImageGeometryPath.createChildPath(filterArgs.value<std::string>(k_CellEnsembleAttributeMatrixName_Key));
  inputValues.CellAttributeMatrixPath = inputValues.ImageGeometryPath.createChildPath(filterArgs.value<std::string>(k_CellAttributeMatrixName_Key));
  inputValues.ConvertPhaseToInt32 = filterArgs.value<bool>(k_ConvertPhaseToInt32_Key);
  inputValues.EdaxHexagonalAlignment = filterArgs.value<bool>(k_EdaxHexagonalAlignment_Key);

  return ReadH5OinaData(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace nx::core
