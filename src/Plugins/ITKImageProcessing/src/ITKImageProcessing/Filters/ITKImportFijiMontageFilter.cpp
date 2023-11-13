#include "ITKImportFijiMontageFilter.hpp"

#include "Algorithms/ITKImportFijiMontage.hpp"

#include "ITKImageProcessing/Filters/ITKImageReader.hpp"

#include "complex/Core/Application.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateDataGroupAction.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"
#include "complex/Utilities/MontageUtilities.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace
{
const Uuid k_ComplexCorePluginId = *Uuid::FromString("05cc618b-781f-4ac0-b9ac-43f26ce1854f");
const Uuid k_ColorToGrayScaleFilterId = *Uuid::FromString("d938a2aa-fee2-4db9-aa2f-2c34a9736580");
const FilterHandle k_ColorToGrayScaleFilterHandle(k_ColorToGrayScaleFilterId, k_ComplexCorePluginId);
std::atomic_int32_t s_InstanceId = 0;
std::map<int32, FijiCache> s_HeaderCache;
} // namespace

namespace complex
{
//------------------------------------------------------------------------------
ITKImportFijiMontageFilter::ITKImportFijiMontageFilter()
: m_InstanceId(s_InstanceId.fetch_add(1))
{
  s_HeaderCache[m_InstanceId] = {};
}

//------------------------------------------------------------------------------
std::string ITKImportFijiMontageFilter::name() const
{
  return FilterTraits<ITKImportFijiMontageFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ITKImportFijiMontageFilter::className() const
{
  return FilterTraits<ITKImportFijiMontageFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ITKImportFijiMontageFilter::uuid() const
{
  return FilterTraits<ITKImportFijiMontageFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKImportFijiMontageFilter::humanName() const
{
  return "Read Fiji Montage (ITK)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKImportFijiMontageFilter::defaultTags() const
{
  return {className(), "IO", "Input", "Read", "Import"};
}

//------------------------------------------------------------------------------
Parameters ITKImportFijiMontageFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<FileSystemPathParameter>(k_InputFile_Key, "Fiji Configuration File", "This is the configuration file in the same directory as all of the identified geometries",
                                                          fs::path(""), FileSystemPathParameter::ExtensionsType{}, FileSystemPathParameter::PathType::InputFile));
  params.insert(std::make_unique<ChoicesParameter>(k_LengthUnit_Key, "Length Unit", "The length unit that will be set into the created image geometry",
                                                   to_underlying(IGeometry::LengthUnit::Micrometer), IGeometry::GetAllLengthUnitStrings()));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ChangeOrigin_Key, "Change Origin", "Set the origin of the mosaic to a user defined value", false));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Origin_Key, "Origin", "The new origin of the mosaic", std::vector<float32>(3), std::vector<std::string>(3)));
  params.insertLinkableParameter(
      std::make_unique<BoolParameter>(k_ConvertToGrayScale_Key, "Convert To GrayScale", "The filter will show an error if the images are already in grayscale format", false));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_ColorWeights_Key, "Color Weighting", "The luminosity values for the conversion", std::vector<float32>{0.2125f, 0.7154f, 0.0721f},
                                                         std::vector<std::string>(3)));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ParentDataGroup_Key, "Parent Imported Images Under a DataGroup", "Create a new DataGroup to hold the  imported images", true));

  params.insertSeparator(Parameters::Separator{"Created Data Objects"});
  params.insert(std::make_unique<StringParameter>(k_DataGroupName_Key, "Name of Created DataGroup", "Name of the overarching parent DataGroup", "Zen DataGroup"));
  params.insert(std::make_unique<StringParameter>(k_DataContainerPath_Key, "Image Geometry Prefix", "A prefix that can be used for each Image Geometry", "Mosaic-"));
  params.insert(std::make_unique<StringParameter>(k_CellAttributeMatrixName_Key, "Cell Attribute Matrix Name", "The name of the Cell Attribute Matrix", "Tile Data"));
  params.insert(std::make_unique<StringParameter>(k_ImageDataArrayName_Key, "Image DataArray Name", "The name of the import image data", "Image"));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_ChangeOrigin_Key, k_Origin_Key, true);
  params.linkParameters(k_ConvertToGrayScale_Key, k_ColorWeights_Key, true);
  params.linkParameters(k_ParentDataGroup_Key, k_DataGroupName_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKImportFijiMontageFilter::clone() const
{
  return std::make_unique<ITKImportFijiMontageFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKImportFijiMontageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                   const std::atomic_bool& shouldCancel) const
{
  auto pInputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  auto pDataGroupNameValue = filterArgs.value<StringParameter::ValueType>(k_DataGroupName_Key);
  auto pParentDataGroupValue = filterArgs.value<bool>(k_ParentDataGroup_Key);
  auto pChangeOriginValue = filterArgs.value<bool>(k_ChangeOrigin_Key);
  auto pOriginValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Origin_Key);
  auto pConvertToGrayScaleValue = filterArgs.value<bool>(k_ConvertToGrayScale_Key);
  auto pColorWeightsValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_ColorWeights_Key);
  auto pDataContainerPathValue = filterArgs.value<StringParameter::ValueType>(k_DataContainerPath_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_CellAttributeMatrixName_Key);
  auto pImageDataArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_ImageDataArrayName_Key);

  PreflightResult preflightResult;
  complex::Result<OutputActions> resultOutputActions = {};
  std::vector<PreflightValue> preflightUpdatedValues;

  ITKImportFijiMontageInputValues inputValues;

  inputValues.allocate = false;
  inputValues.changeOrigin = pChangeOriginValue;
  inputValues.inputFilePath = pInputFileValue;
  inputValues.origin = pOriginValue;
  inputValues.imagePrefix = pDataContainerPathValue;

  // Read from the file if the input file has changed or the input file's time stamp is out of date.
  if(pInputFileValue != s_HeaderCache[m_InstanceId].inputFile || s_HeaderCache[m_InstanceId].timeStamp < fs::last_write_time(pInputFileValue) || s_HeaderCache[m_InstanceId].valuesChanged(inputValues))
  {
    s_HeaderCache[m_InstanceId].flush();

    // Read from the file
    DataStructure throwaway = DataStructure();
    ITKImportFijiMontage algorithm(throwaway, messageHandler, shouldCancel, &inputValues, s_HeaderCache.find(m_InstanceId)->second);
    algorithm.operator()();

    // Update the cached variables
    s_HeaderCache[m_InstanceId].inputFile = pInputFileValue;
    s_HeaderCache[m_InstanceId].timeStamp = fs::last_write_time(pInputFileValue);
    s_HeaderCache[m_InstanceId].inputValues = inputValues;
  }

  if(pParentDataGroupValue)
  {
    auto createAction = std::make_unique<CreateDataGroupAction>(DataPath({pDataGroupNameValue}));
    resultOutputActions.value().appendAction(std::move(createAction));
  }

  auto* filterListPtr = Application::Instance()->getFilterList();
  for(const auto& bound : s_HeaderCache[m_InstanceId].bounds)
  {
    auto imageImportFilter = ITKImageReader();

    DataPath imageDataProxy = {};
    if(pParentDataGroupValue)
    {
      imageDataProxy = DataPath({pDataGroupNameValue, bound.ImageName, pCellAttributeMatrixNameValue, pImageDataArrayNameValue});
    }
    else
    {
      imageDataProxy = DataPath({bound.ImageName, pCellAttributeMatrixNameValue, pImageDataArrayNameValue});
    }

    Arguments imageImportArgs;
    imageImportArgs.insertOrAssign(ITKImageReader::k_FileName_Key, std::make_any<fs::path>(bound.Filepath));
    imageImportArgs.insertOrAssign(ITKImageReader::k_ImageGeometryPath_Key, std::make_any<DataPath>(imageDataProxy.getParent().getParent()));
    imageImportArgs.insertOrAssign(ITKImageReader::k_CellDataName_Key, std::make_any<std::string>(imageDataProxy.getParent().getTargetName()));
    imageImportArgs.insertOrAssign(ITKImageReader::k_ImageDataArrayPath_Key, std::make_any<DataPath>(imageDataProxy));

    auto result = imageImportFilter.preflight(dataStructure, imageImportArgs, messageHandler, shouldCancel);
    if(result.outputActions.invalid())
    {
      return result;
    }
    resultOutputActions = MergeOutputActionResults(resultOutputActions, result.outputActions);
  }

  if(pConvertToGrayScaleValue)
  {
    if(!filterListPtr->containsPlugin(k_ComplexCorePluginId))
    {
      return MakePreflightErrorResult(-18540, "ComplexCore was not instantiated in this instance, so color to grayscale is not a valid option.");
    }
    auto grayScaleFilter = filterListPtr->createFilter(k_ColorToGrayScaleFilterHandle);
    if(nullptr == grayScaleFilter.get())
    {
      return MakePreflightErrorResult(-18541, "Unable to create grayscale filter");
    }
    preflightUpdatedValues.push_back(
        {"GrayScale", fmt::format("Due to execution order grayscale conversion will occur during execution and all the ({}) arrays will be converted to grayscale", pImageDataArrayNameValue)});
  }

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods.
  // These values should have been updated during the preflightImpl(...) method
  preflightUpdatedValues.push_back({"Import Information", s_HeaderCache[m_InstanceId].montageInformation});

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ITKImportFijiMontageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                 const std::atomic_bool& shouldCancel) const
{
  ITKImportFijiMontageInputValues inputValues;

  inputValues.allocate = true;
  inputValues.changeOrigin = filterArgs.value<bool>(k_ChangeOrigin_Key);
  inputValues.convertToGrayScale = filterArgs.value<bool>(k_ConvertToGrayScale_Key);
  inputValues.parentDataGroup = filterArgs.value<bool>(k_ParentDataGroup_Key);
  inputValues.inputFilePath = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  inputValues.lengthUnit = static_cast<IGeometry::LengthUnit>(filterArgs.value<ChoicesParameter::ValueType>(k_LengthUnit_Key));
  inputValues.origin = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Origin_Key);
  inputValues.colorWeights = filterArgs.value<VectorFloat32Parameter::ValueType>(k_ColorWeights_Key);
  inputValues.DataGroupName = filterArgs.value<StringParameter::ValueType>(k_DataGroupName_Key);
  inputValues.imagePrefix = filterArgs.value<StringParameter::ValueType>(k_DataContainerPath_Key);
  inputValues.cellAMName = filterArgs.value<StringParameter::ValueType>(k_CellAttributeMatrixName_Key);
  inputValues.imageDataArrayName = filterArgs.value<StringParameter::ValueType>(k_ImageDataArrayName_Key);

  ITKImportFijiMontage(dataStructure, messageHandler, shouldCancel, &inputValues, s_HeaderCache.find(m_InstanceId)->second)();

  return {};
}
} // namespace complex
