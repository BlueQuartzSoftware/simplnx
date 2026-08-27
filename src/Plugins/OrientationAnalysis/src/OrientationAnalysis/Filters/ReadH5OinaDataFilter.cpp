#include "ReadH5OinaDataFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/ReadH5OinaData.hpp"
#include "OrientationAnalysis/Parameters/OEMEbsdScanSelectionParameter.h"
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
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

#include <EbsdLib/IO/HKL/CtfConstants.h>
#include <EbsdLib/IO/HKL/CtfFields.h>
#include <EbsdLib/IO/HKL/H5OINAReader.h>
#include <EbsdLib/LaueOps/LaueOps.h>

#include <algorithm>
#include <cmath>
#include <list>
#include <string>

#include <fmt/ranges.h>

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
  return "Read Oxford Aztec Data (.h5oina)";
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
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<OEMEbsdScanSelectionParameter>(k_SelectedScanNames_Key, "Scan Names", "The name of the scan in the .h5oina file. Oxford can store multiple scans in a single file",
                                                                OEMEbsdScanSelectionParameter::ValueType{}, OEMEbsdScanSelectionParameter::EbsdReaderType::H5Oina,
                                                                OEMEbsdScanSelectionParameter::ExtensionsType{".h5oina"}));
  params.insert(std::make_unique<BoolParameter>(k_EdaxHexagonalAlignment_Key, "Convert Hexagonal X-Axis to EDAX Standard",
                                                "Whether or not to convert a Hexagonal phase to the EDAX standard for x-axis alignment", true));
  params.insert(std::make_unique<BoolParameter>(k_ConvertPhaseToInt32_Key, "Convert Phase Data to Int32", "Native Phases data value is uint8. Convert to Int32 for better filter compatibility", true));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Origin_Key, "Origin", "The origin of the volume", std::vector<float32>{0.0F, 0.0F, 0.0F}, std::vector<std::string>{"x", "y", "z"}));
  params.insert(std::make_unique<Float32Parameter>(k_ZSpacing_Key, "Z Spacing (Microns)", "The spacing in microns between each layer.", 1.0f));
  params.insert(std::make_unique<BoolParameter>(k_ReadPatternData_Key, "Import Pattern Data",
                                                "Whether or not to import the diffraction pattern data. Pattern import is not yet supported for H5OINA files, so turning this on stops the filter "
                                                "with an error.",
                                                false));
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
IFilter::VersionType ReadH5OinaDataFilter::parametersVersion() const
{
  // Version 2: Pattern import is explicitly unsupported, and multi-scan inputs
  // must have compatible geometry and identical phase definitions.
  return 2;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ReadH5OinaDataFilter::clone() const
{
  return std::make_unique<ReadH5OinaDataFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ReadH5OinaDataFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                             const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto pSelectedScanNamesValue = filterArgs.value<OEMEbsdScanSelectionParameter::ValueType>(k_SelectedScanNames_Key);
  auto pZSpacingValue = filterArgs.value<float32>(k_ZSpacing_Key);
  auto pOriginValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Origin_Key);
  auto pReadPatternDataValue = filterArgs.value<bool>(k_ReadPatternData_Key);
  auto pImageGeometryNameValue = filterArgs.value<DataPath>(k_CreatedImageGeometryPath_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<std::string>(k_CellAttributeMatrixName_Key);
  auto pCellEnsembleAttributeMatrixNameValue = filterArgs.value<std::string>(k_CellEnsembleAttributeMatrixName_Key);
  auto pConvertPhaseData = filterArgs.value<bool>(k_ConvertPhaseToInt32_Key);

  DataPath cellEnsembleAMPath = pImageGeometryNameValue.createChildPath(pCellEnsembleAttributeMatrixNameValue);
  DataPath cellAMPath = pImageGeometryNameValue.createChildPath(pCellAttributeMatrixNameValue);

  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  const std::string inputFilePath = pSelectedScanNamesValue.inputFilePath.string();

  if(!std::isfinite(pZSpacingValue) || pZSpacingValue <= 0)
  {
    return MakePreflightErrorResult(-9580, fmt::format("The Z Spacing value ({}) must be finite and positive.", pZSpacingValue));
  }
  if(pSelectedScanNamesValue.scanNames.empty())
  {
    return MakePreflightErrorResult(-9581, "At least one scan must be chosen. Please select a scan from the list.");
  }
  if(pReadPatternDataValue)
  {
    return MakePreflightErrorResult(-9583, fmt::format("Pattern import is not yet supported for H5OINA files, so 'Import Pattern Data' must be turned off to read '{}'. The diffraction "
                                                       "patterns a file does contain can be read with the 'Read HDF5 Dataset' filter.",
                                                       inputFilePath));
  }

  // read in the necessary info from the input h5 file
  ebsdlib::H5OINAReader reader;
  reader.setFileName(inputFilePath);
  reader.setReadPatternData(false);

  // Every selected scan must be present in the file. Checking only the first one
  // leaves a bad later name to fail part way through execute, with the scans that
  // were already imported left in the output arrays.
  std::list<std::string> availableScanNames;
  if(const int err = reader.readScanNames(availableScanNames); err < 0)
  {
    return MakePreflightErrorResult(-9582, fmt::format("An error occurred while listing the scans in '{}'.\n  Error Code: {}\n  Message: {}", inputFilePath, err, reader.getErrorMessage()));
  }
  for(const std::string& scanName : pSelectedScanNamesValue.scanNames)
  {
    if(std::find(availableScanNames.cbegin(), availableScanNames.cend(), scanName) == availableScanNames.cend())
    {
      std::string availableList;
      for(const std::string& availableScanName : availableScanNames)
      {
        availableList += (availableList.empty() ? "" : ", ") + availableScanName;
      }
      return MakePreflightErrorResult(-9586, fmt::format("The selected scan '{}' is not present in '{}'. The scans available in this file are: {}", scanName, inputFilePath,
                                                         availableList.empty() ? std::string("<none>") : availableList));
    }
  }

  const std::string& firstScanName = pSelectedScanNamesValue.scanNames.front();
  reader.setHDF5Path(firstScanName);
  if(const int err = reader.readHeaderOnly(); err < 0)
  {
    return MakePreflightErrorResult(
        -9582, fmt::format("An error occurred while reading the header of scan '{}' in '{}'.\n  Error Code: {}\n  Message: {}", firstScanName, inputFilePath, err, reader.getErrorMessage()));
  }

  // The geometry is sized from these two values, so a count below 1 has to be
  // rejected here rather than producing an empty or absurdly large geometry.
  if(reader.getXDimension() < 1 || reader.getYDimension() < 1)
  {
    return MakePreflightErrorResult(-9584, fmt::format("The header of scan '{}' in '{}' reports X Cells = {} and Y Cells = {}. Both must be at least 1. The file may be malformed or may not be an "
                                                       "H5OINA file.",
                                                       firstScanName, inputFilePath, reader.getXDimension(), reader.getYDimension()));
  }

  const auto validateSpacing = [&](const std::string& scanName, ebsdlib::H5OINAReader& scanReader) -> Result<> {
    const float32 xStep = scanReader.getXStep();
    const float32 yStep = scanReader.getYStep();
    if(!std::isfinite(xStep) || !std::isfinite(yStep) || xStep <= 0.0F || yStep <= 0.0F)
    {
      return MakeErrorResult(-9591, fmt::format("Scan '{}' in '{}' reports X Step = {} and Y Step = {}. Both values must be finite and positive.", scanName, inputFilePath, xStep, yStep));
    }
    return {};
  };

  if(Result<> spacingCheck = validateSpacing(firstScanName, reader); spacingCheck.invalid())
  {
    return MakePreflightErrorResult(spacingCheck.errors().front().code, spacingCheck.errors().front().message);
  }

  // The Ensemble Attribute Matrix is sized from the number of phase groups in the
  // FIRST selected scan, but the shared ensemble fill in IEbsdOemReader::readData runs
  // once per selected scan and places each phase at the index carried by its HDF5 group
  // name. Both properties below therefore have to hold for EVERY selected scan, not just
  // the first: a later scan with a group named outside 1..N, or with more phase groups
  // than the first scan, writes past the end of the ensemble arrays at execute.
  const auto phases = reader.getPhaseVector();
  const usize ensemblePhaseCount = phases.size();
  const auto validatePhaseGroups = [&](const std::string& scanName, const auto& scanPhases, bool comparePhaseDefinitions) -> Result<> {
    if(scanPhases.size() != ensemblePhaseCount)
    {
      return MakeErrorResult(-9589, fmt::format("Scan '{}' in '{}' declares {} phase group(s), but scan '{}' declares {}. Every selected scan must declare the same phase groups, because the single "
                                                "Ensemble Attribute Matrix that all of the stacked scans share is sized and filled from those groups. Import scans with differing phase lists "
                                                "separately.",
                                                scanName, inputFilePath, scanPhases.size(), firstScanName, ensemblePhaseCount));
    }
    for(const auto& phase : scanPhases)
    {
      const int32 phaseIndex = phase->getPhaseIndex();
      if(phaseIndex < 1 || static_cast<usize>(phaseIndex) > ensemblePhaseCount)
      {
        return MakeErrorResult(-9587, fmt::format("Scan '{}' in '{}' declares {} phase(s), but one of them carries index {}. The phase groups of an H5OINA file must be named 1 through {}.", scanName,
                                                  inputFilePath, ensemblePhaseCount, phaseIndex, ensemblePhaseCount));
      }

      if(comparePhaseDefinitions)
      {
        const auto referencePhaseIter = std::find_if(phases.cbegin(), phases.cend(), [phaseIndex](const auto& referencePhase) { return referencePhase->getPhaseIndex() == phaseIndex; });
        if(referencePhaseIter == phases.cend())
        {
          return MakeErrorResult(-9587, fmt::format("Scan '{}' in '{}' declares phase index {}, but scan '{}' does not. Every selected scan must use the same phase group names.", scanName,
                                                    inputFilePath, phaseIndex, firstScanName));
        }

        const auto& referencePhase = *referencePhaseIter;
        if(phase->getPhaseName() != referencePhase->getPhaseName())
        {
          return MakeErrorResult(-9590,
                                 fmt::format("Phase group {} of scan '{}' in '{}' has material name '{}', but the same group of scan '{}' has material name '{}'. The selected scans form one 3D "
                                             "microstructure and must use identical phase definitions.",
                                             phaseIndex, scanName, inputFilePath, phase->getPhaseName(), firstScanName, referencePhase->getPhaseName()));
        }
        if(phase->getLaueGroup() != referencePhase->getLaueGroup())
        {
          return MakeErrorResult(-9590, fmt::format("Phase group {} of scan '{}' in '{}' has Laue group {}, but the same group of scan '{}' has Laue group {}. The selected scans form one 3D "
                                                    "microstructure and must use identical phase definitions.",
                                                    phaseIndex, scanName, inputFilePath, static_cast<int32>(phase->getLaueGroup()), firstScanName, static_cast<int32>(referencePhase->getLaueGroup())));
        }
        if(phase->getSpaceGroup() != referencePhase->getSpaceGroup())
        {
          return MakeErrorResult(-9590, fmt::format("Phase group {} of scan '{}' in '{}' has space group {}, but the same group of scan '{}' has space group {}. The selected scans form one 3D "
                                                    "microstructure and must use identical phase definitions.",
                                                    phaseIndex, scanName, inputFilePath, phase->getSpaceGroup(), firstScanName, referencePhase->getSpaceGroup()));
        }

        const std::vector<float32> scanLatticeConstants = phase->getLatticeConstants();
        const std::vector<float32> referenceLatticeConstants = referencePhase->getLatticeConstants();
        if(scanLatticeConstants != referenceLatticeConstants)
        {
          return MakeErrorResult(-9590,
                                 fmt::format("Phase group {} of scan '{}' in '{}' has lattice constants [{}], but the same group of scan '{}' has lattice constants [{}]. The selected scans form "
                                             "one 3D microstructure and must use identical phase definitions.",
                                             phaseIndex, scanName, inputFilePath, fmt::join(scanLatticeConstants, ", "), firstScanName, fmt::join(referenceLatticeConstants, ", ")));
        }
      }
    }
    return {};
  };

  if(Result<> phaseCheck = validatePhaseGroups(firstScanName, phases, false); phaseCheck.invalid())
  {
    return MakePreflightErrorResult(phaseCheck.errors().front().code, phaseCheck.errors().front().message);
  }

  // Every other selected scan has to describe the same grid, because the geometry
  // and every cell array are sized from the first scan's header alone. A second
  // reader is used so the checks below do not disturb the header state that the
  // preflight-updated values and the output actions are built from.
  {
    ebsdlib::H5OINAReader scanCheckReader;
    scanCheckReader.setFileName(inputFilePath);
    scanCheckReader.setReadPatternData(false);
    for(const std::string& scanName : pSelectedScanNamesValue.scanNames)
    {
      if(scanName == firstScanName)
      {
        continue;
      }
      scanCheckReader.setHDF5Path(scanName);
      if(const int err = scanCheckReader.readHeaderOnly(); err < 0)
      {
        return MakePreflightErrorResult(
            -9582, fmt::format("An error occurred while reading the header of scan '{}' in '{}'.\n  Error Code: {}\n  Message: {}", scanName, inputFilePath, err, scanCheckReader.getErrorMessage()));
      }
      if(Result<> spacingCheck = validateSpacing(scanName, scanCheckReader); spacingCheck.invalid())
      {
        return MakePreflightErrorResult(spacingCheck.errors().front().code, spacingCheck.errors().front().message);
      }
      if(scanCheckReader.getXDimension() != reader.getXDimension() || scanCheckReader.getYDimension() != reader.getYDimension() || scanCheckReader.getXStep() != reader.getXStep() ||
         scanCheckReader.getYStep() != reader.getYStep())
      {
        return MakePreflightErrorResult(
            -9585, fmt::format("Scan '{}' in '{}' describes a {} x {} grid with steps ({}, {}), but scan '{}' describes a {} x {} grid with steps ({}, {}). Every selected scan must describe the same "
                               "grid, because they are stacked into a single Image Geometry.",
                               scanName, inputFilePath, scanCheckReader.getXDimension(), scanCheckReader.getYDimension(), scanCheckReader.getXStep(), scanCheckReader.getYStep(), firstScanName,
                               reader.getXDimension(), reader.getYDimension(), reader.getXStep(), reader.getYStep()));
      }
      if(Result<> phaseCheck = validatePhaseGroups(scanName, scanCheckReader.getPhaseVector(), true); phaseCheck.invalid())
      {
        return MakePreflightErrorResult(phaseCheck.errors().front().code, phaseCheck.errors().front().message);
      }
    }
  }

  // Create the Image Geometry and its attribute matrices.
  const CreateImageGeometryAction::DimensionType dims = {static_cast<usize>(reader.getXDimension()), static_cast<usize>(reader.getYDimension()), pSelectedScanNamesValue.scanNames.size()};
  const ShapeType tupleDims = {dims[2], dims[1], dims[0]};
  {
    CreateImageGeometryAction::SpacingType spacing = {reader.getXStep(), reader.getYStep(), pZSpacingValue};

    auto createDataGroupAction = std::make_unique<CreateImageGeometryAction>(pImageGeometryNameValue, dims, pOriginValue, spacing, pCellAttributeMatrixNameValue);
    resultOutputActions.value().appendAction(std::move(createDataGroupAction));
  }

  EbsdReaderUtilities::GeneratePreflightScanInformation<ebsdlib::H5OINAReader>(reader, preflightUpdatedValues);
  EbsdReaderUtilities::GeneratePreflightPhaseInformation<ebsdlib::H5OINAReader>(reader, preflightUpdatedValues);

  std::vector<usize> ensembleTupleDims{phases.size() + 1};
  {
    auto createAttributeMatrixAction = std::make_unique<CreateAttributeMatrixAction>(cellEnsembleAMPath, ensembleTupleDims);
    resultOutputActions.value().appendAction(std::move(createAttributeMatrixAction));
  }

  // create the cell ensemble arrays
  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::uint32, ensembleTupleDims, std::vector<usize>{1}, cellEnsembleAMPath.createChildPath(ebsdlib::CtfFile::CrystalStructures));
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }
  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::float32, ensembleTupleDims, std::vector<usize>{6}, cellEnsembleAMPath.createChildPath(ebsdlib::CtfFile::LatticeConstants));
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }
  {
    auto createArrayAction = std::make_unique<CreateStringArrayAction>(ensembleTupleDims, cellEnsembleAMPath.createChildPath(ebsdlib::CtfFile::MaterialName));
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  // create the cell data arrays
  resultOutputActions.value().appendAction(std::make_unique<CreateArrayAction>(DataType::uint8, tupleDims, std::vector<usize>{1}, cellAMPath.createChildPath(ebsdlib::H5OINA::BandContrast)));
  resultOutputActions.value().appendAction(std::make_unique<CreateArrayAction>(DataType::uint8, tupleDims, std::vector<usize>{1}, cellAMPath.createChildPath(ebsdlib::H5OINA::BandSlope)));
  resultOutputActions.value().appendAction(std::make_unique<CreateArrayAction>(DataType::uint8, tupleDims, std::vector<usize>{1}, cellAMPath.createChildPath(ebsdlib::H5OINA::Bands)));
  resultOutputActions.value().appendAction(std::make_unique<CreateArrayAction>(DataType::uint8, tupleDims, std::vector<usize>{1}, cellAMPath.createChildPath(ebsdlib::H5OINA::Error)));
  resultOutputActions.value().appendAction(std::make_unique<CreateArrayAction>(DataType::float32, tupleDims, std::vector<usize>{3}, cellAMPath.createChildPath(ebsdlib::H5OINA::Euler)));
  resultOutputActions.value().appendAction(std::make_unique<CreateArrayAction>(DataType::float32, tupleDims, std::vector<usize>{1}, cellAMPath.createChildPath(ebsdlib::H5OINA::MeanAngularDeviation)));
  if(pConvertPhaseData)
  {
    resultOutputActions.value().appendAction(std::make_unique<CreateArrayAction>(DataType::int32, tupleDims, std::vector<usize>{1}, cellAMPath.createChildPath(ebsdlib::H5OINA::Phase)));
  }
  else
  {
    resultOutputActions.value().appendAction(std::make_unique<CreateArrayAction>(DataType::uint8, tupleDims, std::vector<usize>{1}, cellAMPath.createChildPath(ebsdlib::H5OINA::Phase)));
  }
  resultOutputActions.value().appendAction(std::make_unique<CreateArrayAction>(DataType::float32, tupleDims, std::vector<usize>{1}, cellAMPath.createChildPath(ebsdlib::H5OINA::X)));
  resultOutputActions.value().appendAction(std::make_unique<CreateArrayAction>(DataType::float32, tupleDims, std::vector<usize>{1}, cellAMPath.createChildPath(ebsdlib::H5OINA::Y)));

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ReadH5OinaDataFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                           const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  ReadH5DataInputValues inputValues;

  inputValues.SelectedScanNames = filterArgs.value<OEMEbsdScanSelectionParameter::ValueType>(k_SelectedScanNames_Key);
  inputValues.ReadPatternData = filterArgs.value<bool>(k_ReadPatternData_Key);
  inputValues.ImageGeometryPath = filterArgs.value<DataPath>(k_CreatedImageGeometryPath_Key);
  inputValues.CellEnsembleAttributeMatrixPath = inputValues.ImageGeometryPath.createChildPath(filterArgs.value<std::string>(k_CellEnsembleAttributeMatrixName_Key));
  inputValues.CellAttributeMatrixPath = inputValues.ImageGeometryPath.createChildPath(filterArgs.value<std::string>(k_CellAttributeMatrixName_Key));
  inputValues.ConvertPhaseToInt32 = filterArgs.value<bool>(k_ConvertPhaseToInt32_Key);
  inputValues.EdaxHexagonalAlignment = filterArgs.value<bool>(k_EdaxHexagonalAlignment_Key);

  return ReadH5OinaData(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace nx::core
