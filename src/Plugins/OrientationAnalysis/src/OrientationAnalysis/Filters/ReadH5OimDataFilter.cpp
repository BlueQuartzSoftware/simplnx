#include "ReadH5OimDataFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/ReadH5OimData.hpp"
#include "OrientationAnalysis/Parameters/OEMEbsdScanSelectionParameter.h"
#include "OrientationAnalysis/utilities/EbsdReaderUtilities.hpp"
#include "OrientationAnalysis/utilities/SIMPLConversion.hpp"

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

#include <EbsdLib/IO/TSL/AngFields.h>
#include <EbsdLib/IO/TSL/H5OIMReader.h>
#include <EbsdLib/LaueOps/LaueOps.h>

#include <filesystem>

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ReadH5OimDataFilter::name() const
{
  return FilterTraits<ReadH5OimDataFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ReadH5OimDataFilter::className() const
{
  return FilterTraits<ReadH5OimDataFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ReadH5OimDataFilter::uuid() const
{
  return FilterTraits<ReadH5OimDataFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ReadH5OimDataFilter::humanName() const
{
  return "Read EDAX OIMAnalysis Data (.h5)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ReadH5OimDataFilter::defaultTags() const
{
  return {className(), "IO", "Input", "Read", "Import"};
}

//------------------------------------------------------------------------------
Parameters ReadH5OimDataFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<OEMEbsdScanSelectionParameter>(k_SelectedScanNames_Key, "Scan Names", "The name of the scan in the .h5 file. EDAX can store multiple scans in a single file",
                                                                OEMEbsdScanSelectionParameter::ValueType{}, OEMEbsdScanSelectionParameter::EbsdReaderType::Oim,
                                                                OEMEbsdScanSelectionParameter::ExtensionsType{".h5"}));
  params.insert(std::make_unique<VectorFloat32ParameterType>(k_Origin_Key, "Origin", "The origin of the volume", std::vector<float32>{0.0F, 0.0F, 0.0F}, std::vector<std::string>{"x", "y", "z"}));
  params.insert(std::make_unique<Float32ParameterType>(k_ZSpacing_Key, "Z Spacing (Microns)", "The spacing in microns between each layer.", 1.0f));
  params.insert(std::make_unique<BoolParameter>(k_ReadPatternData_Key, "Import Pattern Data", "Whether or not to import the pattern data", false));
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
IFilter::VersionType ReadH5OimDataFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointerType ReadH5OimDataFilter::clone() const
{
  return std::make_unique<ReadH5OimDataFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ReadH5OimDataFilter::preflightImpl(const DataStructure& /*dataStructure*/, const Arguments& filterArgs, const MessageHandler& /*messageHandler*/,
                                                            const std::atomic_bool& /*shouldCancel*/, const ExecutionContext& /*executionContext*/) const
{
  auto pSelectedScanNamesValue = filterArgs.value<OEMEbsdScanSelectionParameter::ValueType>(k_SelectedScanNames_Key);
  auto pZSpacingValue = filterArgs.value<float32>(k_ZSpacing_Key);
  auto pOriginValue = filterArgs.value<VectorFloat32ParameterType::ValueType>(k_Origin_Key);
  auto pReadPatternDataValue = filterArgs.value<bool>(k_ReadPatternData_Key);
  auto pImageGeometryNameValue = filterArgs.value<DataPath>(k_CreatedImageGeometryPath_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<std::string>(k_CellAttributeMatrixName_Key);
  auto pCellEnsembleAttributeMatrixNameValue = filterArgs.value<std::string>(k_CellEnsembleAttributeMatrixName_Key);

  DataPath cellEnsembleAMPath = pImageGeometryNameValue.createChildPath(pCellEnsembleAttributeMatrixNameValue);
  DataPath cellAMPath = pImageGeometryNameValue.createChildPath(pCellAttributeMatrixNameValue);

  PreflightResult preflightResult;
  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  if(pZSpacingValue <= 0)
  {
    return MakePreflightErrorResult(-9580, fmt::format("The Z Spacing field contains a value ({}) that is non-positive.  The Z Spacing field must be set to a positive value.", pZSpacingValue));
  }
  if(pSelectedScanNamesValue.scan_names.empty())
  {
    return MakePreflightErrorResult(-9581, "At least one scan must be chosen.  Please select a scan from the list.");
  }

  // read in the necessary info from the input h5 file
  ebsdlib::H5OIMReader::Pointer reader = ebsdlib::H5OIMReader::New();
  reader->setFileName(pSelectedScanNamesValue.input_file_path.string());
  reader->setReadPatternData(pReadPatternDataValue);
  reader->setHDF5Path(pSelectedScanNamesValue.scan_names.front());
  if(const int err = reader->readHeaderOnly(); err < 0)
  {
    return MakePreflightErrorResult(-9582, fmt::format("An error occurred while reading the header data\n{} : {}", err, reader->getErrorMessage()));
  }

  // create the Image Geometry and it's attribute matrices
  const CreateImageGeometryAction::DimensionType dims = {static_cast<usize>(reader->getXDimension()), static_cast<usize>(reader->getYDimension()), pSelectedScanNamesValue.scan_names.size()};
  const ShapeType tupleDims = {dims[2], dims[1], dims[0]};
  {
    CreateImageGeometryAction::SpacingType spacing = {reader->getXStep(), reader->getYStep(), pZSpacingValue};

    auto createDataGroupAction = std::make_unique<CreateImageGeometryAction>(pImageGeometryNameValue, dims, pOriginValue, spacing, pCellAttributeMatrixNameValue);
    resultOutputActions.value().appendAction(std::move(createDataGroupAction));
  }

  ebsd_reader_utilities::GeneratePreflightScanInformation<ebsdlib::H5OIMReader>(*reader, preflightUpdatedValues);
  ebsd_reader_utilities::GeneratePreflightPhaseInformation<ebsdlib::H5OIMReader>(*reader, preflightUpdatedValues);

  auto phases = reader->getPhaseVector();
  std::vector<usize> ensembleTupleDims{phases.size() + 1};
  {
    auto createAttributeMatrixAction = std::make_unique<CreateAttributeMatrixAction>(cellEnsembleAMPath, ensembleTupleDims);
    resultOutputActions.value().appendAction(std::move(createAttributeMatrixAction));
  }

  // create the cell ensemble arrays
  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::uint32, ensembleTupleDims, std::vector<usize>{1}, cellEnsembleAMPath.createChildPath(ebsdlib::AngFile::CrystalStructures));
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }
  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::float32, ensembleTupleDims, std::vector<usize>{6}, cellEnsembleAMPath.createChildPath(ebsdlib::AngFile::LatticeConstants));
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }
  {
    auto createArrayAction = std::make_unique<CreateStringArrayAction>(ensembleTupleDims, cellEnsembleAMPath.createChildPath(ebsdlib::AngFile::MaterialName));
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  // create the cell data arrays

  ebsdlib::AngFields angFeatures;
  const auto names = angFeatures.getFilterFeatures<std::vector<std::string>>();
  for(const auto& name : names)
  {
    if(reader->getPointerType(name) == ebsdlib::NumericTypes::Type::Int32)
    {
      auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::int32, tupleDims, std::vector<usize>{1}, cellAMPath.createChildPath(name));
      resultOutputActions.value().appendAction(std::move(createArrayAction));
    }
    else if(reader->getPointerType(name) == ebsdlib::NumericTypes::Type::Float)
    {
      auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::float32, tupleDims, std::vector<usize>{1}, cellAMPath.createChildPath(name));
      resultOutputActions.value().appendAction(std::move(createArrayAction));
    }
  }
  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::float32, tupleDims, std::vector<usize>{3}, cellAMPath.createChildPath(ebsdlib::AngFile::EulerAngles));
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }
  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::int32, tupleDims, std::vector<usize>{1}, cellAMPath.createChildPath(ebsdlib::AngFile::Phases));
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }
  if(pReadPatternDataValue)
  {
    std::array<int32, 2> patternDims = {{0, 0}};
    reader->getPatternDims(patternDims);
    if(patternDims[0] == 0 || patternDims[1] == 0)
    {
      return MakePreflightErrorResult(-9583, fmt::format("The parameter 'Read Pattern Data' has been enabled but there does not seem to be any pattern data in the file for the scan name selected"));
    }
    auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::uint8, tupleDims, std::vector<usize>{static_cast<usize>(patternDims[0]), static_cast<usize>(patternDims[1])},
                                                                 cellAMPath.createChildPath(ebsdlib::Ang::PatternData));
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  return {.output_actions = std::move(resultOutputActions), .output_values = std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ReadH5OimDataFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* /*pipelineNode*/, const MessageHandler& messageHandler,
                                          const std::atomic_bool& shouldCancel, const ExecutionContext& /*executionContext*/) const
{
  ReadH5DataInputValues inputValues;

  inputValues.selected_scan_names = filterArgs.value<OEMEbsdScanSelectionParameter::ValueType>(k_SelectedScanNames_Key);
  inputValues.read_pattern_data = filterArgs.value<bool>(k_ReadPatternData_Key);
  inputValues.image_geometry_path = filterArgs.value<DataPath>(k_CreatedImageGeometryPath_Key);
  inputValues.cell_ensemble_attribute_matrix_path = inputValues.image_geometry_path.createChildPath(filterArgs.value<std::string>(k_CellEnsembleAttributeMatrixName_Key));
  inputValues.cell_attribute_matrix_path = inputValues.image_geometry_path.createChildPath(filterArgs.value<std::string>(k_CellAttributeMatrixName_Key));

  return ReadH5OimData(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace simpl
{
constexpr StringLiteralType k_InputFileKey = "InputFile";
constexpr StringLiteralType k_SelectedScanNamesKey = "SelectedScanNames";
constexpr StringLiteralType k_ZSpacingKey = "ZSpacing";
constexpr StringLiteralType k_OriginKey = "Origin";
constexpr StringLiteralType k_ReadPatternDataKey = "ReadPatternData";
constexpr StringLiteralType k_DataContainerNameKey = "DataContainerName";
constexpr StringLiteralType k_CellAttributeMatrixNameKey = "CellAttributeMatrixName";
constexpr StringLiteralType k_CellEnsembleAttributeMatrixNameKey = "CellEnsembleAttributeMatrixName";
} // namespace simpl
} // namespace

Result<Arguments> ReadH5OimDataFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ReadH5OimDataFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(
      simpl_conversion::Convert2Parameters<simpl_conversion::OEMEbsdScanSelectionFilterParameterConverter>(args, json, simpl::k_InputFileKey, simpl::k_SelectedScanNamesKey, k_SelectedScanNames_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::FloatFilterParameterConverter<float32>>(args, json, simpl::k_ZSpacingKey, k_ZSpacing_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::FloatVec3FilterParameterConverterType>(args, json, simpl::k_OriginKey, k_Origin_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::BooleanFilterParameterConverter>(args, json, simpl::k_ReadPatternDataKey, k_ReadPatternData_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DCPathBuilderFilterParameterConverter>(args, json, simpl::k_DataContainerNameKey, k_CreatedImageGeometryPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::LinkedPathCreationFilterParameterConverter>(args, json, simpl::k_CellAttributeMatrixNameKey, k_CellAttributeMatrixName_Key));
  results.push_back(
      simpl_conversion::ConvertParameter<simpl_conversion::LinkedPathCreationFilterParameterConverter>(args, json, simpl::k_CellEnsembleAttributeMatrixNameKey, k_CellEnsembleAttributeMatrixName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
