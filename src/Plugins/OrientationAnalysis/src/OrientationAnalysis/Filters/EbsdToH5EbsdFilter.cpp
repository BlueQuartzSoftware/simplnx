#include "EbsdToH5EbsdFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/EbsdToH5Ebsd.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/GeneratedFileListParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include <filesystem>
#include <sstream>

namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string EbsdToH5EbsdFilter::name() const
{
  return FilterTraits<EbsdToH5EbsdFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string EbsdToH5EbsdFilter::className() const
{
  return FilterTraits<EbsdToH5EbsdFilter>::className;
}

//------------------------------------------------------------------------------
Uuid EbsdToH5EbsdFilter::uuid() const
{
  return FilterTraits<EbsdToH5EbsdFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string EbsdToH5EbsdFilter::humanName() const
{
  return "Import Orientation File(s) to H5EBSD";
}

//------------------------------------------------------------------------------
std::vector<std::string> EbsdToH5EbsdFilter::defaultTags() const
{
  return {className(), "IO", "Input", "Read", "Import", "Ebsd", "EDAX", "Oxford", "Convert"};
}

//------------------------------------------------------------------------------
Parameters EbsdToH5EbsdFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Conversion Parameters"});
  params.insert(std::make_unique<Float32Parameter>(k_ZSpacing_Key, "Z Spacing (Microns)", "The spacing between each slice of data", 1.0F));
  params.insert(std::make_unique<ChoicesParameter>(k_StackingOrder_Key, "Stacking Order", "The order the files should be placed into the ", EbsdToH5EbsdInputConstants::k_LowToHigh,
                                                   EbsdToH5EbsdInputConstants::k_StackingChoices));
  params.insert(std::make_unique<ChoicesParameter>(k_ReferenceFrame_Key, "Reference Frame Options", "The reference frame transformation. 0=EDAX(.ang), 1=Oxford(.ctf), 2=No/Unknown Transformation, 3=HEDM-IceNine", EbsdToH5EbsdInputConstants::k_Edax, EbsdToH5EbsdInputConstants::k_TransformChoices));

  params.insertSeparator(Parameters::Separator{"Output Parameters"});
  params.insert(std::make_unique<FileSystemPathParameter>(k_OutputPath_Key, "Output H5Ebsd File", "The path to the generated .h5ebsd file", fs::path(""), FileSystemPathParameter::ExtensionsType{".h5ebsd"},
                                                          FileSystemPathParameter::PathType::OutputFile, true));

  params.insertSeparator(Parameters::Separator{"Orientation Source Data"});
  params.insert(std::make_unique<GeneratedFileListParameter>(k_InputFileListInfo_Key, "Input File List", "The values that are used to generate the input file list. See GeneratedFileListParameter for more information.", GeneratedFileListParameter::ValueType{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer EbsdToH5EbsdFilter::clone() const
{
  return std::make_unique<EbsdToH5EbsdFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult EbsdToH5EbsdFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                           const std::atomic_bool& shouldCancel) const
{
  auto generatedFileListInfo = filterArgs.value<GeneratedFileListParameter::ValueType>(k_InputFileListInfo_Key);
  auto referenceFrame = filterArgs.value<ChoicesParameter::ValueType>(k_ReferenceFrame_Key);

  PreflightResult preflightResult;

  complex::Result<OutputActions> resultOutputActions;

  std::vector<PreflightValue> preflightUpdatedValues;

  if(generatedFileListInfo.fileExtension != ".ang" && generatedFileListInfo.fileExtension != ".ctf")
  {
    return {MakePreflightErrorResult(-60800, "Only .ang and .ctf files are supported")};
  }

  generatedFileListInfo.ordering = complex::FilePathGenerator::Ordering::LowToHigh;
  std::vector<std::string> fileList = generatedFileListInfo.generate();
  if(fileList.empty())
  {
    return {MakePreflightErrorResult(-60801, "Generated file list is empty.")};
  }

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  std::stringstream description;
  switch(referenceFrame)
  {
  case EbsdToH5EbsdInputConstants::k_Edax:
    description << "Manufacturer: "
                << "Edax - TSL"
                << "\n"
                << "Sample Reference Transformation: "
                << "180 @ <010>"
                << "\n"
                << "Euler Transformation: "
                << "90 @ <001>"
                << "\n";
    break;

  case EbsdToH5EbsdInputConstants::k_Oxford:
    description << "Manufacturer: "
                << "Oxford - HKL"
                << "\n"
                << "Sample Reference Transformation: "
                << "180 @ <010>"
                << "\n"
                << "Euler Transformation: "
                << "0 @ <001>"
                << "\n";
    break;

  case EbsdToH5EbsdInputConstants::k_Hedm:
    description << "Manufacturer: "
                << "HEDM - IceNine"
                << "\n"
                << "Sample Reference Transformation: "
                << "0 @ <001>"
                << "\n"
                << "Euler Transformation: "
                << "0 @ <001>"
                << "\n";
    break;

  default:
    description << "Manufacturer: "
                << "Unknown"
                << "\n"
                << "Sample Reference Transformation: "
                << "0 @ <001>"
                << "\n"
                << "Euler Transformation: "
                << "0 @ <001>"
                << "\n";
    break;
  }

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  preflightUpdatedValues.push_back({"Reference Type Notes", description.str()});

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> EbsdToH5EbsdFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                         const std::atomic_bool& shouldCancel) const
{
  EbsdToH5EbsdInputValues inputValues;

  inputValues.ZSpacing = filterArgs.value<Float32Parameter::ValueType>(k_ZSpacing_Key);
  inputValues.StackingOrder = filterArgs.value<ChoicesParameter::ValueType>(k_StackingOrder_Key);
  inputValues.ReferenceFrame = filterArgs.value<ChoicesParameter::ValueType>(k_ReferenceFrame_Key);
  inputValues.OutputPath = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputPath_Key);
  inputValues.InputFileListInfo = filterArgs.value<GeneratedFileListParameter::ValueType>(k_InputFileListInfo_Key);
  // ALWAYS use LowToHigh (no matter what the user happened to click in the UI). We need
  // the list ordered from low to high to be correct. The Stacking Order is a piece of
  // meta-data that is stored in the HDF5 file and used when reading the .h5ebsd file
  inputValues.InputFileListInfo.ordering = complex::FilePathGenerator::Ordering::LowToHigh;

  return EbsdToH5Ebsd(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
