#include "ReadGrainMapper3DFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/ReadGrainMapper3D.hpp"
#include "OrientationAnalysis/utilities/GrainMapper3DUtilities.hpp"

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

#include <filesystem>

namespace fs = std::filesystem;

using namespace nx::core;
using namespace GrainMapper3DUtilities;

namespace
{

} // namespace

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ReadGrainMapper3DFilter::name() const
{
  return FilterTraits<ReadGrainMapper3DFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ReadGrainMapper3DFilter::className() const
{
  return FilterTraits<ReadGrainMapper3DFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ReadGrainMapper3DFilter::uuid() const
{
  return FilterTraits<ReadGrainMapper3DFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ReadGrainMapper3DFilter::humanName() const
{
  return "Read GrainMapper3D File";
}

//------------------------------------------------------------------------------
std::vector<std::string> ReadGrainMapper3DFilter::defaultTags() const
{
  return {className(), "Reader", "XNovo", "GrainMapper", "HDF5"};
}

//------------------------------------------------------------------------------
Parameters ReadGrainMapper3DFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});

  params.insert(std::make_unique<FileSystemPathParameter>(k_InputFile_Key, "Input File", "The input .hdf5 file path", fs::path("input.h5"), FileSystemPathParameter::ExtensionsType{".h5"},
                                                          FileSystemPathParameter::PathType::InputFile));

  params.insert(
      std::make_unique<BoolParameter>(k_ConvertPhaseToInt32_Key, "Create Compatible Phase Data", "Native Phases data value is uint8. Convert to Int32 for better filter compatibility", true));
  params.insert(std::make_unique<BoolParameter>(k_ConvertOrientationData_Key, "Create Compatible Orientation Data",
                                                "Orientation data such as Quaternions and Rodrigues vectors will be converted to be DREAM3D-NX compatible", true));

  params.insertSeparator(Parameters::Separator{"LabDCT Data"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ReadLabDCT_Key, "Read LabDCT Data", "Read the LabDCT Data", true));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_CreatedDCTImageGeometryPath_Key, "Image Geometry", "The path to the created Image Geometry", DataPath({"LabDCT"})));
  params.insert(std::make_unique<DataObjectNameParameter>(k_CellAttributeMatrixName_Key, "Cell Attribute Matrix", "The name of the cell data attribute matrix for the created Image Geometry",
                                                          ImageGeom::k_CellDataName));
  params.insert(std::make_unique<DataObjectNameParameter>(k_CellEnsembleAttributeMatrixName_Key, "Ensemble Attribute Matrix", "The Attribute Matrix where the phase information is stored.",
                                                          "Cell Ensemble Data"));

  params.insertSeparator(Parameters::Separator{"AbsorptionCT Data"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ReadAbsorptionCT_Key, "Read AbsorptionCT Data", "Read the AbsorptionCT data", true));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_CreatedAbsorptionGeometryPath_Key, "Image Geometry", "The path to the created Image Geometry", DataPath({"AbsorptionCT"})));
  params.insert(std::make_unique<DataObjectNameParameter>(k_CellAbsorptionAttributeMatrixName_Key, "Cell Attribute Matrix", "The name of the cell data attribute matrix for the created Image Geometry",
                                                          ImageGeom::k_CellDataName));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_ReadLabDCT_Key, k_CreatedDCTImageGeometryPath_Key, true);
  params.linkParameters(k_ReadLabDCT_Key, k_CellAttributeMatrixName_Key, true);
  params.linkParameters(k_ReadLabDCT_Key, k_CellEnsembleAttributeMatrixName_Key, true);

  params.linkParameters(k_ReadAbsorptionCT_Key, k_CreatedAbsorptionGeometryPath_Key, true);
  params.linkParameters(k_ReadAbsorptionCT_Key, k_CellAbsorptionAttributeMatrixName_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ReadGrainMapper3DFilter::clone() const
{
  return std::make_unique<ReadGrainMapper3DFilter>();
}

//------------------------------------------------------------------------------
IFilter::VersionType ReadGrainMapper3DFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ReadGrainMapper3DFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                const std::atomic_bool& shouldCancel) const
{
  auto pInputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  auto pConvertPhaseData = filterArgs.value<bool>(k_ConvertPhaseToInt32_Key);
  auto pConvertOrientationData = filterArgs.value<bool>(k_ConvertOrientationData_Key);

  auto pReadLabDCT = filterArgs.value<bool>(k_ReadLabDCT_Key);
  auto pLabDCTImageGeometryPath = filterArgs.value<DataPath>(k_CreatedDCTImageGeometryPath_Key);
  auto pLabDCTCellAttributeMatrixNameValue = filterArgs.value<std::string>(k_CellAttributeMatrixName_Key);
  auto pCellEnsembleAttributeMatrixNameValue = filterArgs.value<std::string>(k_CellEnsembleAttributeMatrixName_Key);

  auto pReadAbsorptionCT = filterArgs.value<bool>(k_ReadAbsorptionCT_Key);
  auto pAbsorptionCTImageGeometryPath = filterArgs.value<DataPath>(k_CreatedAbsorptionGeometryPath_Key);
  auto pAbsorptionCTCellAttributeMatrixNameValue = filterArgs.value<std::string>(k_CellAbsorptionAttributeMatrixName_Key);

  PreflightResult preflightResult;
  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  GrainMapperReader reader(pInputFileValue.string(), pReadLabDCT, pReadAbsorptionCT);
  Result<> result = reader.readHeaderOnly();
  if(result.invalid())
  {
    auto badResult = MakePreflightErrorResult(result.errors().front().code, result.errors().front().message);
    return badResult;
  }

  if(!pReadLabDCT && !pReadAbsorptionCT)
  {
    resultOutputActions.warnings().push_back({-65432, "WARNING: No data is being read by this filter because both Read DCT and Read Absorption are both FALSE."});
  }

  // **************************************************************************
  // LAB DCT DATA SECTION
  if(pReadLabDCT)
  {
    // create the DCT Image Geometry and it's attribute matrices
    const std::vector<usize> dims = reader.getLabDCTDimensions();
    {
      CreateImageGeometryAction::SpacingType spacing = reader.getLabDCTSpacing();
      std::vector<float> origin = reader.getLabDCTOrigin();

      auto createDataGroupAction = std::make_unique<CreateImageGeometryAction>(pLabDCTImageGeometryPath, dims, origin, spacing, pLabDCTCellAttributeMatrixNameValue, IGeometry::LengthUnit::Millimeter);
      resultOutputActions.value().appendAction(std::move(createDataGroupAction));
    }

    // Reverse the DCT Image Dimensions
    const std::vector<usize> tupleDims = {dims[2], dims[1], dims[0]};

    // Get the available Data sets
    DataPath cellAMPath = pLabDCTImageGeometryPath.createChildPath(pLabDCTCellAttributeMatrixNameValue);

    auto nameToDataTypeMap = reader.getNameToDataTypeMap();
    auto nameToCompDimMap = reader.getNameToCompDimMap();
    auto availableDataSets = reader.getDctDatasetNames();
    for(const auto& dataSetName : availableDataSets)
    {
      if(pConvertPhaseData && dataSetName == GrainMapper3DUtilities::Constants::k_PhaseIdName)
      {
        resultOutputActions.value().appendAction(
            std::make_unique<CreateArrayAction>(DataType::int32, tupleDims, std::vector<usize>{nameToCompDimMap[dataSetName]}, cellAMPath.createChildPath(dataSetName)));
      }
      else if(pConvertOrientationData && dataSetName == GrainMapper3DUtilities::Constants::k_RodriguesName)
      {
        resultOutputActions.value().appendAction(std::make_unique<CreateArrayAction>(nameToDataTypeMap[dataSetName], tupleDims, std::vector<usize>{4}, cellAMPath.createChildPath(dataSetName)));
      }
      else
      {
        resultOutputActions.value().appendAction(
            std::make_unique<CreateArrayAction>(nameToDataTypeMap[dataSetName], tupleDims, std::vector<usize>{nameToCompDimMap[dataSetName]}, cellAMPath.createChildPath(dataSetName)));
      }
    }

    // read the DCT phase information
    DataPath cellEnsembleAMPath = pLabDCTImageGeometryPath.createChildPath(pCellEnsembleAttributeMatrixNameValue);

    auto phases = reader.getPhaseInformation();
    std::vector<usize> ensembleTupleDims{phases.size() + 1};
    {
      auto createAttributeMatrixAction = std::make_unique<CreateAttributeMatrixAction>(cellEnsembleAMPath, ensembleTupleDims);
      resultOutputActions.value().appendAction(std::move(createAttributeMatrixAction));
    }

    // create the cell ensemble arrays
    {
      auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::uint32, ensembleTupleDims, std::vector<usize>{1}, cellEnsembleAMPath.createChildPath(GM3DConstants::k_CrystalStructures));
      resultOutputActions.value().appendAction(std::move(createArrayAction));
    }
    {
      auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::float32, ensembleTupleDims, std::vector<usize>{6}, cellEnsembleAMPath.createChildPath(GM3DConstants::k_LatticeConstants));
      resultOutputActions.value().appendAction(std::move(createArrayAction));
    }
    {
      auto createArrayAction = std::make_unique<CreateStringArrayAction>(ensembleTupleDims, cellEnsembleAMPath.createChildPath(GM3DConstants::k_MaterialName));
      resultOutputActions.value().appendAction(std::move(createArrayAction));
    }
  }

  // **************************************************************************
  // ABSORPTION DCT DATA SECTION
  if(pReadAbsorptionCT)
  {
    // create the ABSORPTION Image Geometry and it's attribute matrices
    const std::vector<usize> dims = reader.getAbsorptionCTDimensions();
    {
      CreateImageGeometryAction::SpacingType spacing = reader.getAbsorptionCTSpacing();
      std::vector<float> origin = reader.getAbsorptionCTOrigin();

      auto createDataGroupAction =
          std::make_unique<CreateImageGeometryAction>(pAbsorptionCTImageGeometryPath, dims, origin, spacing, pAbsorptionCTCellAttributeMatrixNameValue, IGeometry::LengthUnit::Millimeter);
      resultOutputActions.value().appendAction(std::move(createDataGroupAction));
    }

    // Reverse the ABSORPTION Image Dimensions
    const std::vector<usize> tupleDims = {dims[2], dims[1], dims[0]};
    // Create the 'Data' data array
    resultOutputActions.value().appendAction(std::make_unique<CreateArrayAction>(
        DataType::uint16, tupleDims, std::vector<usize>{1ULL},
        pAbsorptionCTImageGeometryPath.createChildPath(pAbsorptionCTCellAttributeMatrixNameValue).createChildPath(GrainMapper3DUtilities::Constants::k_DataGroupName)));
  }

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ReadGrainMapper3DFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                              const std::atomic_bool& shouldCancel) const
{
  ReadGrainMapper3DInputValues inputValues;

  inputValues.InputFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);

  inputValues.ReadDctData = filterArgs.value<bool>(k_ReadLabDCT_Key);
  inputValues.DctImageGeometryPath = filterArgs.value<DataPath>(k_CreatedDCTImageGeometryPath_Key);
  inputValues.DctCellAttributeMatrixName = filterArgs.value<std::string>(k_CellAttributeMatrixName_Key);
  inputValues.DctCellEnsembleAttributeMatrixName = filterArgs.value<std::string>(k_CellEnsembleAttributeMatrixName_Key);
  inputValues.ConvertPhaseData = filterArgs.value<bool>(k_ConvertPhaseToInt32_Key);
  inputValues.ConvertOrientationData = filterArgs.value<bool>(k_ConvertOrientationData_Key);

  inputValues.ReadAbsorptionData = filterArgs.value<bool>(k_ReadAbsorptionCT_Key);
  inputValues.AbsorptionImageGeometryPath = filterArgs.value<DataPath>(k_CreatedAbsorptionGeometryPath_Key);
  inputValues.AbsorptionCellAttributeMatrixName = filterArgs.value<std::string>(k_CellAbsorptionAttributeMatrixName_Key);

  return ReadGrainMapper3D(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace nx::core
