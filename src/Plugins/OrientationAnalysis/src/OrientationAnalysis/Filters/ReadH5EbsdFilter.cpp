#include "ReadH5EbsdFilter.hpp"

#include "simplnx/Utilities/ImageIO/ReaderGeometryOverrides.hpp"

#include "OrientationAnalysis/Filters/Algorithms/ReadH5Ebsd.hpp"
#include "OrientationAnalysis/Parameters/ReadH5EbsdFileParameter.h"
#include "OrientationAnalysis/utilities/EbsdReaderUtilities.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "simplnx/Filter/Actions/CreateImageGeometryAction.hpp"
#include "simplnx/Filter/Actions/CreateStringArrayAction.hpp"
#include "simplnx/Filter/Actions/UpdateImageGeomAction.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"

#include <EbsdLib/IO/HKL/CtfFields.h>
#include <EbsdLib/IO/HKL/H5CtfVolumeReader.h>
#include <EbsdLib/IO/TSL/AngFields.h>

#include "simplnx/Utilities/ImageRotationUtilities.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <EbsdLib/IO/TSL/H5AngVolumeReader.h>

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

  params.insertSeparator(Parameters::Separator{"Geometry Metadata Overrides"});
  image_io::InsertGeometryOverrideParameters(params);

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
IFilter::VersionType ReadH5EbsdFilter::parametersVersion() const
{
  // Version 2: Added Set Origin and Set Spacing.
  return 2;
}

//------------------------------------------------------------------------------
IFilter::UniquePointerType ReadH5EbsdFilter::clone() const
{
  return std::make_unique<ReadH5EbsdFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ReadH5EbsdFilter::preflightImpl(const DataStructure& /*dataStructure*/, const Arguments& filterArgs, const MessageHandler& /*messageHandler*/,
                                                         const std::atomic_bool& /*shouldCancel*/, const ExecutionContext& /*executionContext*/) const
{
  auto pReadH5EbsdFilterValue = filterArgs.value<ReadH5EbsdFileParameter::ValueType>(k_ReadH5EbsdParameter_Key);
  auto imageGeomPath = filterArgs.value<DataPath>(k_CreatedImageGeometryPath_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<std::string>(k_CellAttributeMatrixName_Key);
  DataPath cellAttributeMatrixPath = imageGeomPath.createChildPath(pCellAttributeMatrixNameValue);
  auto pCellEnsembleAttributeMatrixNameValue = imageGeomPath.createChildPath(filterArgs.value<std::string>(k_CellEnsembleAttributeMatrixName_Key));

  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  ebsdlib::H5EbsdVolumeReader::Pointer reader = ebsdlib::H5EbsdVolumeReader::New();
  reader->setFileName(pReadH5EbsdFilterValue.input_file_path);
  int32_t err = reader->readVolumeInfo();
  if(err < 0)
  {
    // This really should have been hit during the parameter validation... but just in case.
    preflightUpdatedValues.push_back({"ERROR:", fmt::format("Could not read input file {}", pReadH5EbsdFilterValue.input_file_path)});
    return {.output_actions = MakeErrorResult<OutputActions>(-67500, fmt::format("Could not read input file '{}'", pReadH5EbsdFilterValue.input_file_path)),
            .output_values = std::move(preflightUpdatedValues)};
  }

  std::vector<int64_t> dims = {0, 0, 0};
  reader->getDims(dims.at(0), dims.at(1), dims.at(2));

  CreateImageGeometryAction::DimensionType imageGeomDims = {static_cast<size_t>(dims[0]), static_cast<size_t>(dims[1]),
                                                            static_cast<size_t>(pReadH5EbsdFilterValue.end_slice - pReadH5EbsdFilterValue.start_slice + 1)};
  std::vector<size_t> tupleDims = {imageGeomDims[2], imageGeomDims[1], imageGeomDims[0]};

  CreateImageGeometryAction::SpacingType spacing = {1.0F, 1.0F, 1.0F};
  reader->getSpacing(spacing.at(0), spacing.at(1), spacing.at(2));

  CreateImageGeometryAction::OriginType origin = {0.0F, 0.0F, 0.0F};
  if(auto geometryResult = image_io::GeometryOverrides::FromArguments(filterArgs).apply(pReadH5EbsdFilterValue.input_file_path, origin, spacing); geometryResult.invalid())
  {
    return {.output_actions = ConvertInvalidResult<OutputActions>(std::move(geometryResult))};
  }

  resultOutputActions.value().appendAction(std::make_unique<CreateImageGeometryAction>(imageGeomPath, imageGeomDims, origin, spacing, pCellAttributeMatrixNameValue));

  ebsdlib::OEM mManufacturer = {ebsdlib::OEM::Unknown};
  std::string manufacturer = reader->getManufacturer();
  if(manufacturer == ebsdlib::Ang::Manufacturer)
  {
    mManufacturer = ebsdlib::OEM::EDAX;
  }
  else if(manufacturer == ebsdlib::Ctf::Manufacturer)
  {
    mManufacturer = ebsdlib::OEM::Oxford;
  }

  std::vector<std::string> names;
  if(mManufacturer == ebsdlib::OEM::EDAX)
  {
    ebsdlib::AngFields angFeatures;
    reader = ebsdlib::H5AngVolumeReader::New();
    reader->setFileName(pReadH5EbsdFilterValue.input_file_path);
    names = angFeatures.getFilterFeatures<std::vector<std::string>>();

    ebsd_reader_utilities::GeneratePreflightScanInformation<ebsdlib::H5AngVolumeReader>(dynamic_cast<ebsdlib::H5AngVolumeReader&>(*reader.get()), preflightUpdatedValues, FloatVec3Type(spacing));
    ebsd_reader_utilities::GeneratePreflightPhaseInformation<ebsdlib::H5AngVolumeReader>(dynamic_cast<ebsdlib::H5AngVolumeReader&>(*reader.get()), preflightUpdatedValues);
  }
  else if(mManufacturer == ebsdlib::OEM::Oxford)
  {
    ebsdlib::CtfFields cfeatures;
    reader = ebsdlib::H5CtfVolumeReader::New();
    reader->setFileName(pReadH5EbsdFilterValue.input_file_path);
    names = cfeatures.getFilterFeatures<std::vector<std::string>>();
    ebsd_reader_utilities::GeneratePreflightScanInformation<ebsdlib::H5CtfVolumeReader>(dynamic_cast<ebsdlib::H5CtfVolumeReader&>(*reader.get()), preflightUpdatedValues, FloatVec3Type(spacing));
    ebsd_reader_utilities::GeneratePreflightPhaseInformation<ebsdlib::H5CtfVolumeReader>(dynamic_cast<ebsdlib::H5CtfVolumeReader&>(*reader.get()), preflightUpdatedValues);
  }
  else
  {
    return {.output_actions = std::move(resultOutputActions), .output_values = std::move(preflightUpdatedValues)};
  }

  std::set<std::string> mSelectedArrayNames;
  for(const auto& selectedArrayName : pReadH5EbsdFilterValue.selected_array_names)
  {
    mSelectedArrayNames.insert(selectedArrayName);
  }

  std::vector<size_t> cDims = {1ULL};
  for(const auto& name : names)
  {
    // First check to see if the name is in our list of names to read.
    if(mSelectedArrayNames.find(name) == mSelectedArrayNames.end())
    {
      continue;
    }
    if(reader->getPointerType(name) == ebsdlib::NumericTypes::Type::Int32)
    {
      const DataPath dataArrayPath = cellAttributeMatrixPath.createChildPath(name);
      auto action = std::make_unique<CreateArrayAction>(nx::core::DataType::int32, tupleDims, cDims, dataArrayPath);
      resultOutputActions.value().appendAction(std::move(action));
    }
    else if(reader->getPointerType(name) == ebsdlib::NumericTypes::Type::Float)
    {
      const DataPath dataArrayPath = cellAttributeMatrixPath.createChildPath(name);
      auto action = std::make_unique<CreateArrayAction>(nx::core::DataType::float32, tupleDims, cDims, dataArrayPath);
      resultOutputActions.value().appendAction(std::move(action));
    }
  }

  // Only read these arrays if the user wants them
  if(mSelectedArrayNames.find(ebsdlib::CellData::EulerAngles) != mSelectedArrayNames.end())
  {
    cDims[0] = 3;
    const DataPath dataArrayPath = cellAttributeMatrixPath.createChildPath(ebsdlib::CellData::EulerAngles);
    auto action = std::make_unique<CreateArrayAction>(nx::core::DataType::float32, tupleDims, cDims, dataArrayPath);
    resultOutputActions.value().appendAction(std::move(action));
  }

  // Only read the phases if the user wants it.
  if(mSelectedArrayNames.find(ebsdlib::H5Ebsd::Phases) != mSelectedArrayNames.end())
  {
    cDims[0] = 1;
    const DataPath dataArrayPath = cellAttributeMatrixPath.createChildPath(ebsdlib::H5Ebsd::Phases);
    auto action = std::make_unique<CreateArrayAction>(nx::core::DataType::int32, tupleDims, cDims, dataArrayPath);
    resultOutputActions.value().appendAction(std::move(action));
  }

  // Now create the Ensemble arrays for the XTal Structures, Material Names and LatticeConstants
  cDims[0] = 1;
  tupleDims = {static_cast<size_t>(reader->getNumPhases() + 1)};
  if(tupleDims[0] == 0)
  {
    return {.output_actions = MakeErrorResult<OutputActions>(
                -67501, fmt::format("H5EBSD File: Number of phases is ZERO. This should not have happened. Please check the source data.", pReadH5EbsdFilterValue.input_file_path)),
            .output_values = std::move(preflightUpdatedValues)};
  }

  // Create the Ensemble AttributeMatrix
  {
    auto createDataGroupAction = std::make_unique<CreateAttributeMatrixAction>(pCellEnsembleAttributeMatrixNameValue, tupleDims);
    resultOutputActions.value().appendAction(std::move(createDataGroupAction));
  }

  {
    const DataPath dataArrayPath = pCellEnsembleAttributeMatrixNameValue.createChildPath(nx::core::h5_ebsd_ensemble_data::k_CrystalStructures);
    auto action = std::make_unique<CreateArrayAction>(nx::core::DataType::uint32, tupleDims, cDims, dataArrayPath);
    resultOutputActions.value().appendAction(std::move(action));
  }

  {
    const DataPath dataArrayPath = pCellEnsembleAttributeMatrixNameValue.createChildPath(nx::core::h5_ebsd_ensemble_data::k_MaterialName);
    auto action = std::make_unique<CreateStringArrayAction>(tupleDims, dataArrayPath);
    resultOutputActions.value().appendAction(std::move(action));
  }

  cDims[0] = 6;
  {
    const DataPath dataArrayPath = pCellEnsembleAttributeMatrixNameValue.createChildPath(nx::core::h5_ebsd_ensemble_data::k_LatticeConstants);
    auto action = std::make_unique<CreateArrayAction>(nx::core::DataType::float32, tupleDims, cDims, dataArrayPath);
    resultOutputActions.value().appendAction(std::move(action));
  }

  if(pReadH5EbsdFilterValue.use_recommended_transform)
  {
    auto sampleTransAngle = reader->getSampleTransformationAngle();
    if(sampleTransAngle > 0)
    {
      auto sampleTransAxis = reader->getSampleTransformationAxis();
      const std::vector<float32> rotAxisAngle = {sampleTransAxis[0], sampleTransAxis[1], sampleTransAxis[2], sampleTransAngle};
      image_rotation_utilities::Matrix4fRType rotationMatrix = image_rotation_utilities::GenerateRotationTransformationMatrix(rotAxisAngle);
      std::array<float32, 6> arr{
          origin[0], origin[1], origin[2], origin[0] + (imageGeomDims[0] * spacing[0]), origin[1] + (imageGeomDims[1] * spacing[1]), origin[2] + (imageGeomDims[2] * spacing[2])};
      BoundingBox3DfType boundingBox = BoundingBox3DfType(arr);
      auto minMaxCoords = image_rotation_utilities::DetermineMinMaxCoords(boundingBox, rotationMatrix);
      FloatVec3Type transformedOrigin = {minMaxCoords[0], minMaxCoords[2], minMaxCoords[4]};
      resultOutputActions.value().appendAction(std::make_unique<UpdateImageGeomAction>(transformedOrigin, spacing, imageGeomPath));
    }
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {.output_actions = std::move(resultOutputActions), .output_values = std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ReadH5EbsdFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* /*pipelineNode*/, const MessageHandler& messageHandler,
                                       const std::atomic_bool& shouldCancel, const ExecutionContext& /*executionContext*/) const
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
  inputValues.input_file_path = pReadH5EbsdFilterValue.input_file_path;
  inputValues.start_slice = pReadH5EbsdFilterValue.start_slice;
  inputValues.end_slice = pReadH5EbsdFilterValue.end_slice;
  inputValues.euler_representation = pReadH5EbsdFilterValue.euler_representation;
  inputValues.hdf5_data_paths = pReadH5EbsdFilterValue.selected_array_names;
  inputValues.use_recommended_transform = pReadH5EbsdFilterValue.use_recommended_transform;
  inputValues.data_container_path = pDataContainerNameValue;
  inputValues.cell_attribute_matrix_path = pDataContainerNameValue.createChildPath(pCellAttributeMatrixNameValue);
  inputValues.cell_ensemble_matrix_path = pCellEnsembleAttributeMatrixNameValue;

  ReadH5Ebsd readH5Ebsd(dataStructure, messageHandler, shouldCancel, &inputValues);
  return readH5Ebsd();
}

namespace
{
namespace simpl
{
constexpr StringLiteralType k_ReadH5EbsdKey = "ReadH5Ebsd";
constexpr StringLiteralType k_DataContainerNameKey = "DataContainerName";
constexpr StringLiteralType k_CellAttributeMatrixNameKey = "CellAttributeMatrixName";
constexpr StringLiteralType k_CellEnsembleAttributeMatrixNameKey = "CellEnsembleAttributeMatrixName";
} // namespace simpl

namespace simpl_conversion_custom
{
struct ReadH5EbsdFilterParameterConverter
{
  using ParameterType = ReadH5EbsdFileParameter;
  using ValueType = ParameterType::ValueType;

  static constexpr StringLiteralType k_InputFileKey = "InputFile";
  static constexpr StringLiteralType k_SelectedArrayNamesKey = "SelectedArrayNames";
  static constexpr StringLiteralType k_ZStartIndexKey = "ZStartIndex";
  static constexpr StringLiteralType k_ZEndIndexKey = "ZEndIndex";
  static constexpr StringLiteralType k_RefFrameZDirKey = "RefFrameZDir";
  static constexpr StringLiteralType k_UseTransformationsKey = "UseTransformations";

  static Result<ValueType> Convert(const nlohmann::json& json)
  {
    static constexpr std::array<StringLiteralType, 6> k_RequiredKeys = {k_InputFileKey, k_SelectedArrayNamesKey, k_ZStartIndexKey, k_ZEndIndexKey, k_RefFrameZDirKey, k_UseTransformationsKey};
    for(const auto& key : k_RequiredKeys)
    {
      if(!json.contains(key.view()))
      {
        return MakeErrorResult<ValueType>(-3, fmt::format("H5EbsdReaderParameterConverter json is missing required key '{}'", key.view()));
      }
    }

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
    if(!json[k_UseTransformationsKey].is_number_integer() && !json[k_UseTransformationsKey].is_boolean())
    {
      return MakeErrorResult<ValueType>(-1, fmt::format("H5EbsdReaderParameterConverter json '{}' is not an integer or boolean", json[k_UseTransformationsKey].dump()));
    }

    for(const auto& iter : json[k_SelectedArrayNamesKey])
    {
      if(!iter.is_string())
      {
        return MakeErrorResult<ValueType>(-2, fmt::format("H5EbsdReaderParameterConverter array name json '{}' is not a string", iter.dump()));
      }
    }

    ParameterType::ValueType value;
    value.input_file_path = json[k_InputFileKey].get<std::string>();
    value.start_slice = json[k_ZStartIndexKey].get<int32>();
    value.end_slice = json[k_ZEndIndexKey].get<int32>();
    value.euler_representation = json[k_RefFrameZDirKey].get<int32>() - 1;
    value.use_recommended_transform = json[k_UseTransformationsKey].is_boolean() ? json[k_UseTransformationsKey].get<bool>() : static_cast<bool>(json[k_UseTransformationsKey].get<int32>());
    value.selected_array_names = json[k_SelectedArrayNamesKey].get<std::vector<std::string>>();

    return {std::move(value)};
  }
};
} // namespace simpl_conversion_custom
} // namespace

Result<Arguments> ReadH5EbsdFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ReadH5EbsdFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion_custom::ReadH5EbsdFilterParameterConverter>(args, json, simpl::k_ReadH5EbsdKey, k_ReadH5EbsdParameter_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DCPathBuilderFilterParameterConverter>(args, json, simpl::k_DataContainerNameKey, k_CreatedImageGeometryPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::LinkedPathCreationFilterParameterConverter>(args, json, simpl::k_CellAttributeMatrixNameKey, k_CellAttributeMatrixName_Key));
  results.push_back(
      simpl_conversion::ConvertParameter<simpl_conversion::LinkedPathCreationFilterParameterConverter>(args, json, simpl::k_CellEnsembleAttributeMatrixNameKey, k_CellEnsembleAttributeMatrixName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
