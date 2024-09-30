#include "ReadH5EbsdFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/ReadH5Ebsd.hpp"
#include "OrientationAnalysis/Parameters/ReadH5EbsdFileParameter.h"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "simplnx/Filter/Actions/CreateImageGeometryAction.hpp"
#include "simplnx/Filter/Actions/CreateStringArrayAction.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"

#include "EbsdLib/IO/HKL/CtfFields.h"
#include "EbsdLib/IO/HKL/H5CtfVolumeReader.h"
#include "EbsdLib/IO/TSL/AngFields.h"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include "EbsdLib/IO/TSL/H5AngVolumeReader.h"

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ReadH5EbsdFilter::name() const
{
  return FilterTraits<ReadH5EbsdFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ReadH5EbsdFilter::className() const
{
  return FilterTraits<ReadH5EbsdFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ReadH5EbsdFilter::uuid() const
{
  return FilterTraits<ReadH5EbsdFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ReadH5EbsdFilter::humanName() const
{
  return "Read H5EBSD File";
}

//------------------------------------------------------------------------------
std::vector<std::string> ReadH5EbsdFilter::defaultTags() const
{
  return {className(), "IO", "Input", "Read", "Import", "EDAX", "ANG", "EBSD", "CTF", "Oxford"};
}

//------------------------------------------------------------------------------
Parameters ReadH5EbsdFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<ReadH5EbsdFileParameter>(k_ReadH5EbsdParameter_Key, "Import H5Ebsd File", "Object that holds all relevant information to import data from the file.",
                                                          ReadH5EbsdFileParameter::ValueType{}));

  params.insertSeparator(Parameters::Separator{"Output Image Geometry"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_CreatedImageGeometryPath_Key, "Image Geometry", "The path to the created Image Geometry", DataPath({ImageGeom::k_TypeName})));
  params.insertSeparator(Parameters::Separator{"Output Cell Attribute Matrix"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_CellAttributeMatrixName_Key, "Cell Attribute Matrix", "The name of the cell data attribute matrix for the created Image Geometry",
                                                          ImageGeom::k_CellDataName));
  params.insertSeparator(Parameters::Separator{"Output Ensemble Attribute Matrix"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_CellEnsembleAttributeMatrixName_Key, "Ensemble Attribute Matrix", "The Attribute Matrix where the phase information is stored.",
                                                          "Cell Ensemble Data"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ReadH5EbsdFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ReadH5EbsdFilter::clone() const
{
  return std::make_unique<ReadH5EbsdFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ReadH5EbsdFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                         const std::atomic_bool& shouldCancel) const
{

  auto pReadH5EbsdFilterValue = filterArgs.value<ReadH5EbsdFileParameter::ValueType>(k_ReadH5EbsdParameter_Key);
  auto imageGeomPath = filterArgs.value<DataPath>(k_CreatedImageGeometryPath_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<std::string>(k_CellAttributeMatrixName_Key);
  DataPath cellAttributeMatrixPath = imageGeomPath.createChildPath(pCellAttributeMatrixNameValue);
  auto pCellEnsembleAttributeMatrixNameValue = imageGeomPath.createChildPath(filterArgs.value<std::string>(k_CellEnsembleAttributeMatrixName_Key));

  PreflightResult preflightResult;

  nx::core::Result<OutputActions> resultOutputActions;

  std::vector<PreflightValue> preflightUpdatedValues;

  H5EbsdVolumeReader::Pointer reader = H5EbsdVolumeReader::New();
  reader->setFileName(pReadH5EbsdFilterValue.inputFilePath);
  int32_t err = reader->readVolumeInfo();
  if(err < 0)
  {
    // This really should have been hit during the parameter validation... but just in case.
    preflightUpdatedValues.push_back({"ERROR:", fmt::format("Could not read input file {}", pReadH5EbsdFilterValue.inputFilePath)});
    return {MakeErrorResult<OutputActions>(-67500, fmt::format("Could not read input file '{}'", pReadH5EbsdFilterValue.inputFilePath)), std::move(preflightUpdatedValues)};
  }

  std::vector<int64_t> dims = {0, 0, 0};
  reader->getDims(dims.at(0), dims.at(1), dims.at(2));

  CreateImageGeometryAction::DimensionType imageGeomDims = {static_cast<size_t>(dims[0]), static_cast<size_t>(dims[1]),
                                                            static_cast<size_t>(pReadH5EbsdFilterValue.endSlice - pReadH5EbsdFilterValue.startSlice + 1)};
  std::vector<size_t> tupleDims = {imageGeomDims[2], imageGeomDims[1], imageGeomDims[0]};

  CreateImageGeometryAction::SpacingType spacing = {1.0F, 1.0F, 1.0F};
  reader->getSpacing(spacing.at(0), spacing.at(1), spacing.at(2));

  CreateImageGeometryAction::OriginType origin = {0.0F, 0.0F, 0.0F};

  resultOutputActions.value().appendAction(
      std::make_unique<CreateImageGeometryAction>(std::move(imageGeomPath), std::move(imageGeomDims), std::move(origin), std::move(spacing), pCellAttributeMatrixNameValue));

  EbsdLib::OEM m_Manufacturer = {EbsdLib::OEM::Unknown};
  std::string manufacturer = reader->getManufacturer();
  if(manufacturer == EbsdLib::Ang::Manufacturer)
  {
    m_Manufacturer = EbsdLib::OEM::EDAX;
  }
  else if(manufacturer == EbsdLib::Ctf::Manufacturer)
  {
    m_Manufacturer = EbsdLib::OEM::Oxford;
  }

  std::vector<std::string> names;
  if(m_Manufacturer == EbsdLib::OEM::EDAX)
  {
    AngFields angFeatures;
    reader = H5AngVolumeReader::New();
    reader->setFileName(pReadH5EbsdFilterValue.inputFilePath);
    names = angFeatures.getFilterFeatures<std::vector<std::string>>();
  }
  else if(m_Manufacturer == EbsdLib::OEM::Oxford)
  {
    CtfFields cfeatures;
    reader = H5CtfVolumeReader::New();
    reader->setFileName(pReadH5EbsdFilterValue.inputFilePath);
    names = cfeatures.getFilterFeatures<std::vector<std::string>>();
  }
  else
  {
    return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
  }

  std::set<std::string> m_SelectedArrayNames;
  for(const auto& selectedArrayName : pReadH5EbsdFilterValue.selectedArrayNames)
  {
    m_SelectedArrayNames.insert(selectedArrayName);
  }

  std::vector<size_t> cDims = {1ULL};
  for(int32_t i = 0; i < names.size(); ++i)
  {
    // First check to see if the name is in our list of names to read.
    if(m_SelectedArrayNames.find(names[i]) == m_SelectedArrayNames.end())
    {
      continue;
    }
    if(reader->getPointerType(names[i]) == EbsdLib::NumericTypes::Type::Int32)
    {
      const DataPath dataArrayPath = cellAttributeMatrixPath.createChildPath(names[i]);
      auto action = std::make_unique<CreateArrayAction>(nx::core::DataType::int32, tupleDims, cDims, dataArrayPath);
      resultOutputActions.value().appendAction(std::move(action));
    }
    else if(reader->getPointerType(names[i]) == EbsdLib::NumericTypes::Type::Float)
    {
      const DataPath dataArrayPath = cellAttributeMatrixPath.createChildPath(names[i]);
      auto action = std::make_unique<CreateArrayAction>(nx::core::DataType::float32, tupleDims, cDims, dataArrayPath);
      resultOutputActions.value().appendAction(std::move(action));
    }
  }

  // Only read these arrays if the user wants them
  if(m_SelectedArrayNames.find(EbsdLib::CellData::EulerAngles) != m_SelectedArrayNames.end())
  {
    cDims[0] = 3;
    const DataPath dataArrayPath = cellAttributeMatrixPath.createChildPath(EbsdLib::CellData::EulerAngles);
    auto action = std::make_unique<CreateArrayAction>(nx::core::DataType::float32, tupleDims, cDims, dataArrayPath);
    resultOutputActions.value().appendAction(std::move(action));
  }

  // Only read the phases if the user wants it.
  if(m_SelectedArrayNames.find(EbsdLib::H5Ebsd::Phases) != m_SelectedArrayNames.end())
  {
    cDims[0] = 1;
    const DataPath dataArrayPath = cellAttributeMatrixPath.createChildPath(EbsdLib::H5Ebsd::Phases);
    auto action = std::make_unique<CreateArrayAction>(nx::core::DataType::int32, tupleDims, cDims, dataArrayPath);
    resultOutputActions.value().appendAction(std::move(action));
  }

  // Now create the Ensemble arrays for the XTal Structures, Material Names and LatticeConstants
  cDims[0] = 1;
  tupleDims = {static_cast<size_t>(reader->getNumPhases() + 1)};
  if(tupleDims[0] == 0)
  {
    return {MakeErrorResult<OutputActions>(-67501,
                                           fmt::format("H5EBSD File: Number of phases is ZERO. This should not have happened. Please check the source data.", pReadH5EbsdFilterValue.inputFilePath)),
            std::move(preflightUpdatedValues)};
  }

  // Create the Ensemble AttributeMatrix
  {
    auto createDataGroupAction = std::make_unique<CreateAttributeMatrixAction>(pCellEnsembleAttributeMatrixNameValue, tupleDims);
    resultOutputActions.value().appendAction(std::move(createDataGroupAction));
  }

  {
    const DataPath dataArrayPath = pCellEnsembleAttributeMatrixNameValue.createChildPath(EbsdLib::EnsembleData::CrystalStructures);
    auto action = std::make_unique<CreateArrayAction>(nx::core::DataType::uint32, tupleDims, cDims, dataArrayPath);
    resultOutputActions.value().appendAction(std::move(action));
  }

  {
    const DataPath dataArrayPath = pCellEnsembleAttributeMatrixNameValue.createChildPath(EbsdLib::EnsembleData::MaterialName);
    auto action = std::make_unique<CreateStringArrayAction>(tupleDims, dataArrayPath);
    resultOutputActions.value().appendAction(std::move(action));
  }

  cDims[0] = 6;
  {
    const DataPath dataArrayPath = pCellEnsembleAttributeMatrixNameValue.createChildPath(EbsdLib::EnsembleData::LatticeConstants);
    auto action = std::make_unique<CreateArrayAction>(nx::core::DataType::float32, tupleDims, cDims, dataArrayPath);
    resultOutputActions.value().appendAction(std::move(action));
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ReadH5EbsdFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                       const std::atomic_bool& shouldCancel) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pReadH5EbsdFilterValue = filterArgs.value<ReadH5EbsdFileParameter::ValueType>(k_ReadH5EbsdParameter_Key);
  auto pDataContainerNameValue = filterArgs.value<DataPath>(k_CreatedImageGeometryPath_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<std::string>(k_CellAttributeMatrixName_Key);
  auto pCellEnsembleAttributeMatrixNameValue = pDataContainerNameValue.createChildPath(filterArgs.value<std::string>(k_CellEnsembleAttributeMatrixName_Key));

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  ReadH5EbsdInputValues inputValues;
  inputValues.inputFilePath = pReadH5EbsdFilterValue.inputFilePath;
  inputValues.startSlice = pReadH5EbsdFilterValue.startSlice;
  inputValues.endSlice = pReadH5EbsdFilterValue.endSlice;
  inputValues.eulerRepresentation = pReadH5EbsdFilterValue.eulerRepresentation;
  inputValues.hdf5DataPaths = pReadH5EbsdFilterValue.selectedArrayNames;
  inputValues.useRecommendedTransform = pReadH5EbsdFilterValue.useRecommendedTransform;
  inputValues.dataContainerPath = pDataContainerNameValue;
  inputValues.cellAttributeMatrixPath = pDataContainerNameValue.createChildPath(pCellAttributeMatrixNameValue);
  inputValues.cellEnsembleMatrixPath = pCellEnsembleAttributeMatrixNameValue;

  ReadH5Ebsd readH5Ebsd(dataStructure, messageHandler, shouldCancel, &inputValues);
  return readH5Ebsd();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_ReadH5EbsdKey = "ReadH5Ebsd";
constexpr StringLiteral k_DataContainerNameKey = "DataContainerName";
constexpr StringLiteral k_CellAttributeMatrixNameKey = "CellAttributeMatrixName";
constexpr StringLiteral k_CellEnsembleAttributeMatrixNameKey = "CellEnsembleAttributeMatrixName";
} // namespace SIMPL

namespace SIMPLConversionCustom
{
struct ReadH5EbsdFilterParameterConverter
{
  using ParameterType = ReadH5EbsdFileParameter;
  using ValueType = ParameterType::ValueType;

  static constexpr StringLiteral k_InputFileKey = "InputFile";
  static constexpr StringLiteral k_SelectedArrayNamesKey = "SelectedArrayNames";
  static constexpr StringLiteral k_ZStartIndexKey = "ZStartIndex";
  static constexpr StringLiteral k_ZEndIndexKey = "ZEndIndex";
  static constexpr StringLiteral k_RefFrameZDirKey = "RefFrameZDir";
  static constexpr StringLiteral k_UseTransformationsKey = "UseTransformations";

  static Result<ValueType> convert(const nlohmann::json& json)
  {
    if(!json[k_InputFileKey].is_string())
    {
      return MakeErrorResult<ValueType>(-2, fmt::format("H5EbsdReaderParameterConverter json '{}' is not a string", json[k_InputFileKey].dump()));
    }
    if(!json[k_SelectedArrayNamesKey].is_array())
    {
      return MakeErrorResult<ValueType>(-1, fmt::format("H5EbsdReaderParameterConverter json '{}' is not an array", json[k_SelectedArrayNamesKey].dump()));
    }
    if(!json[k_ZStartIndexKey].is_number_integer())
    {
      return MakeErrorResult<ValueType>(-1, fmt::format("H5EbsdReaderParameterConverter json '{}' is not an integer", json[k_ZStartIndexKey].dump()));
    }
    if(!json[k_ZEndIndexKey].is_number_integer())
    {
      return MakeErrorResult<ValueType>(-1, fmt::format("H5EbsdReaderParameterConverter json '{}' is not an integer", json[k_ZEndIndexKey].dump()));
    }
    if(!json[k_RefFrameZDirKey].is_number_integer())
    {
      return MakeErrorResult<ValueType>(-1, fmt::format("H5EbsdReaderParameterConverter json '{}' is not an integer", json[k_RefFrameZDirKey].dump()));
    }
    if(!json[k_UseTransformationsKey].is_number_integer())
    {
      return MakeErrorResult<ValueType>(-1, fmt::format("H5EbsdReaderParameterConverter json '{}' is not an integer", json[k_UseTransformationsKey].dump()));
    }

    for(const auto& iter : json[k_SelectedArrayNamesKey])
    {
      if(!iter.is_string())
      {
        return MakeErrorResult<ValueType>(-2, fmt::format("H5EbsdReaderParameterConverter array name json '{}' is not a string", iter.dump()));
      }
    }

    ParameterType::ValueType value;
    value.inputFilePath = json[k_InputFileKey].get<std::string>();
    value.startSlice = json[k_ZStartIndexKey].get<int32>();
    value.endSlice = json[k_ZEndIndexKey].get<int32>();
    value.eulerRepresentation = json[k_RefFrameZDirKey].get<int32>() - 1;
    value.useRecommendedTransform = static_cast<bool>(json[k_UseTransformationsKey].get<int32>());
    value.selectedArrayNames = json[k_SelectedArrayNamesKey].get<std::vector<std::string>>();

    return {std::move(value)};
  }
};
} // namespace SIMPLConversionCustom
} // namespace

Result<Arguments> ReadH5EbsdFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ReadH5EbsdFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertTopParameters<SIMPLConversionCustom::ReadH5EbsdFilterParameterConverter>(args, json, k_ReadH5EbsdParameter_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerCreationFilterParameterConverter>(args, json, SIMPL::k_DataContainerNameKey, k_CreatedImageGeometryPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_CellAttributeMatrixNameKey, k_CellAttributeMatrixName_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_CellEnsembleAttributeMatrixNameKey, k_CellEnsembleAttributeMatrixName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
