#include "ReadH5EspritDataFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/ReadH5EspritData.hpp"
#include "OrientationAnalysis/Parameters/OEMEbsdScanSelectionParameter.h"

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

#include "EbsdLib/IO/BrukerNano/EspritConstants.h"
#include "EbsdLib/IO/BrukerNano/EspritPhase.h"
#include "EbsdLib/IO/BrukerNano/H5EspritFields.h"
#include "EbsdLib/IO/BrukerNano/H5EspritReader.h"
#include "EbsdLib/IO/TSL/AngFields.h"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ReadH5EspritDataFilter::name() const
{
  return FilterTraits<ReadH5EspritDataFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ReadH5EspritDataFilter::className() const
{
  return FilterTraits<ReadH5EspritDataFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ReadH5EspritDataFilter::uuid() const
{
  return FilterTraits<ReadH5EspritDataFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ReadH5EspritDataFilter::humanName() const
{
  return "Read Bruker Nano Esprit Data (.h5)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ReadH5EspritDataFilter::defaultTags() const
{
  return {className(), "IO", "Input", "Read", "Import"};
}

//------------------------------------------------------------------------------
Parameters ReadH5EspritDataFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<OEMEbsdScanSelectionParameter>(k_SelectedScanNames_Key, "Scan Names", "The name of the scan in the .h5 file. Esprit can store multiple scans in a single file",
                                                                OEMEbsdScanSelectionParameter::ValueType{},
                                                                /* OEMEbsdScanSelectionParameter::AllowedManufacturers{EbsdLib::OEM::Bruker, EbsdLib::OEM::DREAM3D},*/
                                                                OEMEbsdScanSelectionParameter::EbsdReaderType::Esprit, OEMEbsdScanSelectionParameter::ExtensionsType{".h5", ".hdf5"}));
  params.insert(std::make_unique<Float32Parameter>(k_ZSpacing_Key, "Z Spacing (Microns)", "The spacing in microns between each layer.", 1.0f));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Origin_Key, "Origin", "The origin of the volume", std::vector<float32>{0.0F, 0.0F, 0.0F}, std::vector<std::string>{"x", "y", "z"}));
  params.insert(std::make_unique<BoolParameter>(k_DegreesToRadians_Key, "Convert Euler Angles to Radians", "Whether or not to convert the euler angles to radians", true));
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
IFilter::UniquePointer ReadH5EspritDataFilter::clone() const
{
  return std::make_unique<ReadH5EspritDataFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ReadH5EspritDataFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                               const std::atomic_bool& shouldCancel) const
{
  auto pSelectedScanNamesValue = filterArgs.value<OEMEbsdScanSelectionParameter::ValueType>(k_SelectedScanNames_Key);
  auto pZSpacingValue = filterArgs.value<float32>(k_ZSpacing_Key);
  auto pOriginValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Origin_Key);
  auto pDegreesToRadiansValue = filterArgs.value<bool>(k_DegreesToRadians_Key);
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
    return MakePreflightErrorResult(-9680, "The Z Spacing field contains a value that is non-positive.  The Z Spacing field must be set to a positive value.");
  }
  if(pSelectedScanNamesValue.scanNames.empty())
  {
    return MakePreflightErrorResult(-9681, "At least one scan must be chosen.  Please select a scan from the list.");
  }

  // read in the necessary info from the input h5 file
  H5EspritReader::Pointer reader = H5EspritReader::New();
  reader->setFileName(pSelectedScanNamesValue.inputFilePath.string());
  reader->setReadPatternData(pReadPatternDataValue);
  reader->setHDF5Path(pSelectedScanNamesValue.scanNames.front());
  if(const int err = reader->readHeaderOnly(); err < 0)
  {
    return MakePreflightErrorResult(-9682, fmt::format("An error occurred while reading the header data\n{} : {}", err, reader->getErrorMessage()));
  }

  // create the Image Geometry and it's attribute matrices
  const CreateImageGeometryAction::DimensionType dims = {static_cast<usize>(reader->getXDimension()), static_cast<usize>(reader->getYDimension()), pSelectedScanNamesValue.scanNames.size()};
  const std::vector<usize> tupleDims = {dims[2], dims[1], dims[0]};
  {
    CreateImageGeometryAction::SpacingType spacing = {static_cast<float32>(reader->getXStep()), static_cast<float32>(reader->getYStep()), pZSpacingValue};
    for(float& value : spacing)
    {
      value = (value == 0.0f ? 1.0f : value);
    }
    auto createDataGroupAction = std::make_unique<CreateImageGeometryAction>(pImageGeometryNameValue, dims, pOriginValue, spacing, pCellAttributeMatrixNameValue);
    resultOutputActions.value().appendAction(std::move(createDataGroupAction));
  }
  const auto phases = reader->getPhaseVector();
  std::vector<usize> ensembleTupleDims{phases.size() + 1};
  {
    auto createAttributeMatrixAction = std::make_unique<CreateAttributeMatrixAction>(cellEnsembleAMPath, ensembleTupleDims);
    resultOutputActions.value().appendAction(std::move(createAttributeMatrixAction));
  }

  // create the cell ensemble arrays : these arrays are purposely created using the AngFile constant names to match the corresponding Oim import filter!
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
  H5EspritFields espritFeatures;
  const auto names = espritFeatures.getFilterFeatures<std::vector<std::string>>();
  for(const auto& name : names)
  {
    if(name == EbsdLib::H5Esprit::phi1 || name == EbsdLib::H5Esprit::PHI || name == EbsdLib::H5Esprit::phi2)
    {
      continue;
    }

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
    auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::float32, tupleDims, std::vector<usize>{3}, cellAMPath.createChildPath(EbsdLib::Esprit::EulerAngles));
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }
  if(pReadPatternDataValue)
  {
    std::array<int32, 2> patternDims = {{0, 0}};
    reader->getPatternDims(patternDims);
    if(patternDims[0] == 0 || patternDims[1] == 0)
    {
      return MakePreflightErrorResult(-9683, fmt::format("The parameter 'Read Pattern Data' has been enabled but there does not seem to be any pattern data in the file for the scan name selected"));
    }
    auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::uint8, tupleDims, std::vector<usize>{static_cast<usize>(patternDims[0]), static_cast<usize>(patternDims[1])},
                                                                 cellAMPath.createChildPath(EbsdLib::H5Esprit::RawPatterns));
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ReadH5EspritDataFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                             const std::atomic_bool& shouldCancel) const
{
  ReadH5DataInputValues inputValues;
  ReadH5EspritDataInputValues espritInputValues;

  inputValues.SelectedScanNames = filterArgs.value<OEMEbsdScanSelectionParameter::ValueType>(k_SelectedScanNames_Key);
  inputValues.ReadPatternData = filterArgs.value<bool>(k_ReadPatternData_Key);
  inputValues.ImageGeometryPath = filterArgs.value<DataPath>(k_ImageGeometryName_Key);
  inputValues.CellEnsembleAttributeMatrixPath = inputValues.ImageGeometryPath.createChildPath(filterArgs.value<std::string>(k_CellEnsembleAttributeMatrixName_Key));
  inputValues.CellAttributeMatrixPath = inputValues.ImageGeometryPath.createChildPath(filterArgs.value<std::string>(k_CellAttributeMatrixName_Key));
  espritInputValues.DegreesToRadians = filterArgs.value<bool>(k_DegreesToRadians_Key);

  return ReadH5EspritData(dataStructure, messageHandler, shouldCancel, &inputValues, &espritInputValues)();
}
} // namespace complex
