#include "ITKImportFijiMontageFilter.hpp"

#include "Algorithms/ITKImportFijiMontage.hpp"

#include "ITKImageProcessing/Filters/ITKImageReader.hpp"

#include "complex/Core/Application.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateDataGroupAction.hpp"
#include "complex/Filter/Actions/DeleteDataAction.hpp"
#include "complex/Filter/Actions/RenameDataAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
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
  params.insert(std::make_unique<StringParameter>(k_MontageName_Key, "Name of Created Montage", "", "SomeString"));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_DataContainerPath_Key, "DataContainer Prefix", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_CellAttributeMatrixName_Key, "Cell Attribute Matrix Name", "", DataPath{}));
  params.insert(std::make_unique<StringParameter>(k_ImageDataArrayName_Key, "Image DataArray Name", "", "SomeString"));

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

  auto montage = GridMontage::Create(dataStructure, "Grid");
  if(montage == nullptr)
  {
    return MakePreflightErrorResult(-1111, "Unable to create montage");
  }

  GridMontage gridMontage = GridMontage::New(m_MontageName, m_RowCount, m_ColumnCount);

  int32 rowCountPadding = MontageUtilities::CalculatePaddingDigits(s_HeaderCache[m_InstanceId].maxRow);
  int32 colCountPadding = MontageUtilities::CalculatePaddingDigits(s_HeaderCache[m_InstanceId].maxCol);
  int32 charPaddingCount = std::max(rowCountPadding, colCountPadding);

  for(const auto& bound : s_HeaderCache[m_InstanceId].bounds)
  {
    if(bound.Row < pRowMontageLimitsValue[0] || bound.Row > pRowMontageLimitsValue[1] || bound.Col < pColumnMontageLimitsValue[0] || bound.Col > pColumnMontageLimitsValue[1])
    {
      continue;
    }

    auto* filterListPtr = Application::Instance()->getFilterList();

    // Create our DataContainer Name using a Prefix and a rXXcYY format.
    std::stringstream dcNameStream;
    dcNameStream << pDataContainerPathValue << bound.Filepath.filename().string();
    dcNameStream << "r" << std::setw(charPaddingCount) << std::right << std::setfill('0') << bound.Row;
    dcNameStream << std::setw(1) << "c" << std::setw(charPaddingCount) << bound.Col;

    GridTileIndex gridIndex = gridMontage.getTileIndex(bound.Row, bound.Col);
    // Set the montage's DataContainer for the current index
    gridMontage.setGeometry(gridIndex, dc);

    // Create the Cell Attribute Matrix into which the image data would be read

    auto imageImportFilter = filterListPtr->createFilter(FilterTraits<ITKImageReader>::uuid);
    if(nullptr == imageImportFilter.get())
    {
      continue;
    }

    Arguments imageImportArgs;
    imageImportArgs.insertOrAssign(ITKImageReader::k_FileName_Key, std::make_any<fs::path>(bound.Filepath));
    imageImportArgs.insertOrAssign(ITKImageReader::k_ImageGeometryPath_Key, std::make_any<DataPath>(bound.ImageDataProxy.getParent().getParent()));
    imageImportArgs.insertOrAssign(ITKImageReader::k_CellDataName_Key, std::make_any<DataPath>(bound.ImageDataProxy.getParent()));
    imageImportArgs.insertOrAssign(ITKImageReader::k_ImageDataArrayPath_Key, std::make_any<DataPath>(bound.ImageDataProxy));

    auto result = imageImportFilter->preflight(dataStructure, imageImportArgs, messageHandler, shouldCancel);
    resultOutputActions = MergeResults<OutputActions>(resultOutputActions, result.outputActions);
    if(result.outputActions.invalid())
    {
      continue;
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
        continue;
      }

      // This same filter was used to preflight so as long as nothing changes on disk this really should work....
      Arguments colorToGrayscaleArgs;
      colorToGrayscaleArgs.insertOrAssign("conversion_algorithm", std::make_any<ChoicesParameter::ValueType>(0));
      colorToGrayscaleArgs.insertOrAssign("color_weights", std::make_any<VectorFloat32Parameter::ValueType>(pColorWeightsValue));
      colorToGrayscaleArgs.insertOrAssign("input_data_array_vector", std::make_any<DataPath>(bound.ImageDataProxy));
      colorToGrayscaleArgs.insertOrAssign("output_array_prefix", std::make_any<std::string>("gray"));

      // Run grayscale filter and process results and messages
      auto resultGray = grayScaleFilter->preflight(dataStructure, colorToGrayscaleArgs, messageHandler, shouldCancel);
      resultOutputActions = MergeResults<OutputActions>(resultOutputActions, resultGray.outputActions);
      if(resultGray.outputActions.invalid())
      {
        continue;
      }

      // push back delayed deletion of non-grayscale array
      {
        auto deleteAction = std::make_unique<DeleteDataAction>(bound.ImageDataProxy);
        resultOutputActions.value().appendDeferredAction(std::move(deleteAction));
      }

      // deferred array rename action
      {
        auto renameAction = std::make_unique<RenameDataAction>(bound.ImageDataProxy.getParent().createChildPath("gray" + bound.ImageDataProxy.getTargetName()), bound.ImageDataProxy.getTargetName());
        resultOutputActions.value().appendDeferredAction(std::move(renameAction));
      }
    }
  }

  // If the filter needs to pass back some updated values via a key:value string:string set of values
  // you can declare and update that string here.
  // These variables should be updated with the latest data generated for each variable during preflight.
  // These will be returned through the preflightResult variable to the
  // user interface. You could make these member variables instead if needed.
  std::string montageInformation;

  // If this filter makes changes to the DataStructure in the form of
  // creating/deleting/moving/renaming DataGroups, Geometries, DataArrays then you
  // will need to use one of the `*Actions` classes located in complex/Filter/Actions
  // to relay that information to the preflight and execute methods. This is done by
  // creating an instance of the Action class and then storing it in the resultOutputActions variable.
  // This is done through a `push_back()` method combined with a `std::move()`. For the
  // newly initiated to `std::move` once that code is executed what was once inside the Action class
  // instance variable is *no longer there*. The memory has been moved. If you try to access that
  // variable after this line you will probably get a crash or have subtle bugs. To ensure that this
  // does not happen we suggest using braces `{}` to scope each of the action's declaration and store
  // so that the programmer is not tempted to use the action instance past where it should be used.
  // You have to create your own Actions class if there isn't something specific for your filter's needs
  // These are some proposed Actions based on the FilterParameters used. Please check them for correctness.
  {
    auto createDataGroupAction = std::make_unique<CreateDataGroupAction>(pDataContainerPathValue);
    resultOutputActions.value().appendAction(std::move(createDataGroupAction));
  }

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods.
  // These values should have been updated during the preflightImpl(...) method
  preflightUpdatedValues.push_back({"MontageInformation", montageInformation});

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ITKImportFijiMontageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
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
  auto pDataContainerPathValue = filterArgs.value<DataPath>(k_DataContainerPath_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<DataPath>(k_CellAttributeMatrixName_Key);
  auto pImageDataArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_ImageDataArrayName_Key);

  return {};
}
} // namespace complex
