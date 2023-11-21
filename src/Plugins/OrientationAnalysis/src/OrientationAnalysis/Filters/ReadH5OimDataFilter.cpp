#include "ReadH5OimDataFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/ReadH5OimData.hpp"
#include "OrientationAnalysis/Parameters/OEMEbsdScanSelectionParameter.h"
#include "OrientationAnalysis/utilities/SIMPLConversion.hpp"

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
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"
#include "complex/Utilities/SIMPLConversion.hpp"

#include "EbsdLib/IO/TSL/AngFields.h"
#include "EbsdLib/IO/TSL/H5OIMReader.h"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
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
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<OEMEbsdScanSelectionParameter>(k_SelectedScanNames_Key, "Scan Names", "The name of the scan in the .h5 file. EDAX can store multiple scans in a single file",
                                                                OEMEbsdScanSelectionParameter::ValueType{},
                                                                /* OEMEbsdScanSelectionParameter::AllowedManufacturers{EbsdLib::OEM::EDAX},*/
                                                                OEMEbsdScanSelectionParameter::EbsdReaderType::Oim, OEMEbsdScanSelectionParameter::ExtensionsType{".h5"}));
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
IFilter::UniquePointer ReadH5OimDataFilter::clone() const
{
  return std::make_unique<ReadH5OimDataFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ReadH5OimDataFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                            const std::atomic_bool& shouldCancel) const
{
  auto pSelectedScanNamesValue = filterArgs.value<OEMEbsdScanSelectionParameter::ValueType>(k_SelectedScanNames_Key);
  auto pZSpacingValue = filterArgs.value<float32>(k_ZSpacing_Key);
  auto pOriginValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Origin_Key);
  auto pReadPatternDataValue = filterArgs.value<bool>(k_ReadPatternData_Key);
  auto pImageGeometryNameValue = filterArgs.value<DataPath>(k_ImageGeometryName_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<std::string>(k_CellAttributeMatrixName_Key);
  auto pCellEnsembleAttributeMatrixNameValue = filterArgs.value<std::string>(k_CellEnsembleAttributeMatrixName_Key);

  DataPath cellEnsembleAMPath = pImageGeometryNameValue.createChildPath(pCellEnsembleAttributeMatrixNameValue);
  DataPath cellAMPath = pImageGeometryNameValue.createChildPath(pCellAttributeMatrixNameValue);

  PreflightResult preflightResult;
  complex::Result<OutputActions> resultOutputActions;
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
  H5OIMReader::Pointer reader = H5OIMReader::New();
  reader->setFileName(pSelectedScanNamesValue.inputFilePath.string());
  reader->setReadPatternData(pReadPatternDataValue);
  reader->setHDF5Path(pSelectedScanNamesValue.scanNames.front());
  if(const int err = reader->readHeaderOnly(); err < 0)
  {
    return MakePreflightErrorResult(-9582, fmt::format("An error occurred while reading the header data\n{} : {}", err, reader->getErrorMessage()));
  }

  // create the Image Geometry and it's attribute matrices
  const CreateImageGeometryAction::DimensionType dims = {static_cast<usize>(reader->getXDimension()), static_cast<usize>(reader->getYDimension()), pSelectedScanNamesValue.scanNames.size()};
  const std::vector<usize> tupleDims = {dims[2], dims[1], dims[0]};
  {
    CreateImageGeometryAction::SpacingType spacing = {reader->getXStep(), reader->getYStep(), pZSpacingValue};

    auto createDataGroupAction = std::make_unique<CreateImageGeometryAction>(pImageGeometryNameValue, dims, pOriginValue, spacing, pCellAttributeMatrixNameValue);
    resultOutputActions.value().appendAction(std::move(createDataGroupAction));
  }
  const auto phases = reader->getPhaseVector();
  std::vector<usize> ensembleTupleDims{phases.size() + 1};
  {
    auto createAttributeMatrixAction = std::make_unique<CreateAttributeMatrixAction>(cellEnsembleAMPath, ensembleTupleDims);
    resultOutputActions.value().appendAction(std::move(createAttributeMatrixAction));
  }

  // create the cell ensemble arrays
  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::uint32, ensembleTupleDims, std::vector<usize>{1}, cellEnsembleAMPath.createChildPath(EbsdLib::AngFile::CrystalStructures));
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }
  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::float32, ensembleTupleDims, std::vector<usize>{6}, cellEnsembleAMPath.createChildPath(EbsdLib::AngFile::LatticeConstants));
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }
  {
    auto createArrayAction = std::make_unique<CreateStringArrayAction>(ensembleTupleDims, cellEnsembleAMPath.createChildPath(EbsdLib::AngFile::MaterialName));
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  // create the cell data arrays

  AngFields angFeatures;
  const auto names = angFeatures.getFilterFeatures<std::vector<std::string>>();
  for(const auto& name : names)
  {
    if(reader->getPointerType(name) == EbsdLib::NumericTypes::Type::Int32)
    {
      auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::int32, tupleDims, std::vector<usize>{1}, cellAMPath.createChildPath(name));
      resultOutputActions.value().appendAction(std::move(createArrayAction));
    }
    else if(reader->getPointerType(name) == EbsdLib::NumericTypes::Type::Float)
    {
      auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::float32, tupleDims, std::vector<usize>{1}, cellAMPath.createChildPath(name));
      resultOutputActions.value().appendAction(std::move(createArrayAction));
    }
  }
  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::float32, tupleDims, std::vector<usize>{3}, cellAMPath.createChildPath(EbsdLib::AngFile::EulerAngles));
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }
  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::int32, tupleDims, std::vector<usize>{1}, cellAMPath.createChildPath(EbsdLib::AngFile::Phases));
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
                                                                 cellAMPath.createChildPath(EbsdLib::Ang::PatternData));
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ReadH5OimDataFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                          const std::atomic_bool& shouldCancel) const
{
  ReadH5DataInputValues inputValues;

  inputValues.SelectedScanNames = filterArgs.value<OEMEbsdScanSelectionParameter::ValueType>(k_SelectedScanNames_Key);
  inputValues.ReadPatternData = filterArgs.value<bool>(k_ReadPatternData_Key);
  inputValues.ImageGeometryPath = filterArgs.value<DataPath>(k_ImageGeometryName_Key);
  inputValues.CellEnsembleAttributeMatrixPath = inputValues.ImageGeometryPath.createChildPath(filterArgs.value<std::string>(k_CellEnsembleAttributeMatrixName_Key));
  inputValues.CellAttributeMatrixPath = inputValues.ImageGeometryPath.createChildPath(filterArgs.value<std::string>(k_CellAttributeMatrixName_Key));

  return ReadH5OimData(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_InputFileKey = "InputFile";
constexpr StringLiteral k_SelectedScanNamesKey = "SelectedScanNames";
constexpr StringLiteral k_ZSpacingKey = "ZSpacing";
constexpr StringLiteral k_OriginKey = "Origin";
constexpr StringLiteral k_ReadPatternDataKey = "ReadPatternData";
constexpr StringLiteral k_DataContainerNameKey = "DataContainerName";
constexpr StringLiteral k_CellAttributeMatrixNameKey = "CellAttributeMatrixName";
constexpr StringLiteral k_CellEnsembleAttributeMatrixNameKey = "CellEnsembleAttributeMatrixName";
} // namespace SIMPL
} // namespace

Result<Arguments> ReadH5OimDataFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ReadH5OimDataFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(
      SIMPLConversion::Convert2Parameters<SIMPLConversion::OEMEbsdScanSelectionFilterParameterConverter>(args, json, SIMPL::k_InputFileKey, SIMPL::k_SelectedScanNamesKey, k_SelectedScanNames_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::FloatFilterParameterConverter<float32>>(args, json, SIMPL::k_ZSpacingKey, k_ZSpacing_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::FloatVec3FilterParameterConverter>(args, json, SIMPL::k_OriginKey, k_Origin_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_ReadPatternDataKey, k_ReadPatternData_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerCreationFilterParameterConverter>(args, json, SIMPL::k_DataContainerNameKey, k_ImageGeometryName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_CellAttributeMatrixNameKey, k_CellAttributeMatrixName_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_CellEnsembleAttributeMatrixNameKey, k_CellEnsembleAttributeMatrixName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace complex
