#include "ITKImportFijiMontageFilter.hpp"

#include "Algorithms/ITKImportFijiMontage.hpp"

#include "ITKImageProcessing/Filters/ITKImageReader.hpp"

#include "complex/Core/Application.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateDataGroupAction.hpp"
#include "complex/Filter/Actions/CreateGridMontageAction.hpp"
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
  return "ITK Import Fiji Montage";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKImportFijiMontageFilter::defaultTags() const
{
  return {"IO", "Input", "Read", "Import"};
}

//------------------------------------------------------------------------------
Parameters ITKImportFijiMontageFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<FileSystemPathParameter>(k_InputFile_Key, "Fiji Configuration File", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::ExtensionsType{},
                                                          FileSystemPathParameter::PathType::InputFile));
  params.insert(
      std::make_unique<VectorInt32Parameter>(k_ColumnMontageLimits_Key, "Montage Column Start/End [Inclusive, Zero Based]", "", std::vector<int32>(2), std::vector<std::string>{"start", "end"}));
  params.insert(std::make_unique<VectorInt32Parameter>(k_RowMontageLimits_Key, "Montage Row Start/End [Inclusive, Zero Based]", "", std::vector<int32>(2), std::vector<std::string>{"start", "end"}));
  params.insert(std::make_unique<ChoicesParameter>(k_LengthUnit_Key, "Length Unit", "The length unit that will be set into the created image geometry", 0, IGeometry::GetAllLengthUnitStrings()));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ChangeOrigin_Key, "Change Origin", "", false));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Origin_Key, "Origin", "", std::vector<float32>(3), std::vector<std::string>(3)));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ConvertToGrayScale_Key, "Convert To GrayScale", "", false));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_ColorWeights_Key, "Color Weighting", "", std::vector<float32>(3), std::vector<std::string>(3)));

  params.insertSeparator(Parameters::Separator{"Created Data Objects"});
  params.insert(std::make_unique<StringParameter>(k_MontageName_Key, "Name of Created Montage", "", "Montage"));
  params.insert(std::make_unique<StringParameter>(k_DataContainerPath_Key, "ImageGeom Prefix", "", "Image"));
  params.insert(std::make_unique<StringParameter>(k_CellAttributeMatrixName_Key, "Cell Attribute Matrix Name", "", "Cell AM"));
  params.insert(std::make_unique<StringParameter>(k_ImageDataArrayName_Key, "Image DataArray Name", "", "Data Array"));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_ChangeOrigin_Key, k_Origin_Key, true);
  params.linkParameters(k_ConvertToGrayScale_Key, k_ColorWeights_Key, true);

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
  auto pMontageNameValue = filterArgs.value<StringParameter::ValueType>(k_MontageName_Key);
  auto pColumnMontageLimitsValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_ColumnMontageLimits_Key);
  auto pRowMontageLimitsValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_RowMontageLimits_Key);
  auto pLengthUnitValue = filterArgs.value<ChoicesParameter::ValueType>(k_LengthUnit_Key);
  auto pChangeOriginValue = filterArgs.value<bool>(k_ChangeOrigin_Key);
  auto pOriginValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Origin_Key);
  auto pConvertToGrayScaleValue = filterArgs.value<bool>(k_ConvertToGrayScale_Key);
  auto pColorWeightsValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_ColorWeights_Key);
  auto pDataContainerPathValue = filterArgs.value<StringParameter::ValueType>(k_DataContainerPath_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_CellAttributeMatrixName_Key);
  auto pImageDataArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_ImageDataArrayName_Key);

  PreflightResult preflightResult;
  complex::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  // Read from the file if the input file has changed or the input file's time stamp is out of date.
  if(pInputFileValue != s_HeaderCache[m_InstanceId].inputFile || s_HeaderCache[m_InstanceId].timeStamp < fs::last_write_time(pInputFileValue))
  {
    ITKImportFijiMontageInputValues inputValues;

    inputValues.allocate = false;
    inputValues.changeOrigin = pChangeOriginValue;
    inputValues.convertToGrayScale = pConvertToGrayScaleValue;
    inputValues.inputFilePath = pInputFileValue;
    inputValues.lengthUnit = static_cast<IGeometry::LengthUnit>(pLengthUnitValue);
    inputValues.columnMontageLimits = pColumnMontageLimitsValue;
    inputValues.rowMontageLimits = pRowMontageLimitsValue;
    inputValues.origin = pOriginValue;
    inputValues.colorWeights = pColorWeightsValue;
    inputValues.montageName = pMontageNameValue;
    inputValues.imagePrefix = pDataContainerPathValue;
    inputValues.cellAMName = pCellAttributeMatrixNameValue;
    inputValues.imageDataArrayName = pDataContainerPathValue;

    // Read from the file
    DataStructure throwaway = DataStructure();
    ITKImportFijiMontage algorithm(throwaway, messageHandler, shouldCancel, &inputValues, s_HeaderCache[m_InstanceId]);
    algorithm.operator()();

    // Cache the results from algorithm run
    s_HeaderCache[m_InstanceId] = algorithm.getCache();

    // Update the cached variables
    s_HeaderCache[m_InstanceId].inputFile = pInputFileValue;
    s_HeaderCache[m_InstanceId].timeStamp = fs::last_write_time(pInputFileValue);
  }

  // create montage
  {
    auto createAction = std::make_unique<CreateGridMontageAction>(DataPath({pMontageNameValue}), CreateGridMontageAction::DimensionType{}, CreateGridMontageAction::OriginType{},
                                                                  CreateGridMontageAction::SpacingType{});
    resultOutputActions.value().appendAction(std::move(createAction));
  }

  auto* filterListPtr = Application::Instance()->getFilterList();
  for(const auto& bound : s_HeaderCache[m_InstanceId].bounds)
  {
    if(bound.Row < pRowMontageLimitsValue[0] || bound.Row > pRowMontageLimitsValue[1] || bound.Col < pColumnMontageLimitsValue[0] || bound.Col > pColumnMontageLimitsValue[1])
    {
      continue;
    }

    auto imageImportFilter = filterListPtr->createFilter(FilterTraits<ITKImageReader>::uuid);
    if(nullptr == imageImportFilter.get())
    {
      continue;
    }

    Arguments imageImportArgs;
    imageImportArgs.insertOrAssign(ITKImageReader::k_FileName_Key, std::make_any<fs::path>(bound.Filepath));
    imageImportArgs.insertOrAssign(ITKImageReader::k_ImageGeometryPath_Key, std::make_any<DataPath>(bound.ImageDataProxy.getParent().getParent()));
    imageImportArgs.insertOrAssign(ITKImageReader::k_CellDataName_Key, std::make_any<std::string>(bound.ImageDataProxy.getParent().getTargetName()));
    imageImportArgs.insertOrAssign(ITKImageReader::k_ImageDataArrayPath_Key, std::make_any<DataPath>(bound.ImageDataProxy));

    auto result = imageImportFilter->preflight(dataStructure, imageImportArgs, messageHandler, shouldCancel);
    resultOutputActions = MergeResults<OutputActions>(resultOutputActions, result.outputActions);
    if(result.outputActions.invalid())
    {
      continue;
    }
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
  preflightUpdatedValues.push_back({"MontageInformation", s_HeaderCache[m_InstanceId].montageInformation});

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
  inputValues.inputFilePath = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  inputValues.lengthUnit = static_cast<IGeometry::LengthUnit>(filterArgs.value<ChoicesParameter::ValueType>(k_LengthUnit_Key));
  inputValues.columnMontageLimits = filterArgs.value<VectorInt32Parameter::ValueType>(k_ColumnMontageLimits_Key);
  inputValues.rowMontageLimits = filterArgs.value<VectorInt32Parameter::ValueType>(k_RowMontageLimits_Key);
  inputValues.origin = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Origin_Key);
  inputValues.colorWeights = filterArgs.value<VectorFloat32Parameter::ValueType>(k_ColorWeights_Key);
  inputValues.montageName = filterArgs.value<StringParameter::ValueType>(k_MontageName_Key);
  inputValues.imagePrefix = filterArgs.value<StringParameter::ValueType>(k_DataContainerPath_Key);
  inputValues.cellAMName = filterArgs.value<StringParameter::ValueType>(k_CellAttributeMatrixName_Key);
  inputValues.imageDataArrayName = filterArgs.value<StringParameter::ValueType>(k_ImageDataArrayName_Key);

  ITKImportFijiMontage(dataStructure, messageHandler, shouldCancel, &inputValues, s_HeaderCache[m_InstanceId])();

  return {};
}
} // namespace complex
