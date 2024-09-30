#include "ConvertHexGridToSquareGridFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/ConvertHexGridToSquareGrid.hpp"

#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/GeneratedFileListParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

#include "EbsdLib/IO/HKL/CtfConstants.h"
#include "EbsdLib/IO/TSL/AngConstants.h"
#include "EbsdLib/IO/TSL/AngReader.h"

#include <filesystem>
#include <sstream>

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
struct ConvertHexGridToSquareGridFilterCache
{
  fs::path workingPath;
  std::string preflightReturnString;
  fs::file_time_type lastWrite;
};

std::atomic_int32_t s_InstanceId = 0;
std::map<int32, ConvertHexGridToSquareGridFilterCache> s_HeaderCache;
} // namespace

namespace nx::core
{
//------------------------------------------------------------------------------
ConvertHexGridToSquareGridFilter::ConvertHexGridToSquareGridFilter()
: m_InstanceId(s_InstanceId.fetch_add(1))
{
  s_HeaderCache[m_InstanceId] = {};
}

//------------------------------------------------------------------------------
ConvertHexGridToSquareGridFilter::~ConvertHexGridToSquareGridFilter() noexcept
{
  s_HeaderCache.erase(m_InstanceId);
}

//------------------------------------------------------------------------------
std::string ConvertHexGridToSquareGridFilter::name() const
{
  return FilterTraits<ConvertHexGridToSquareGridFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ConvertHexGridToSquareGridFilter::className() const
{
  return FilterTraits<ConvertHexGridToSquareGridFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ConvertHexGridToSquareGridFilter::uuid() const
{
  return FilterTraits<ConvertHexGridToSquareGridFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ConvertHexGridToSquareGridFilter::humanName() const
{
  return "Convert EDAX Hex Grid to Square Grid (.ang)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ConvertHexGridToSquareGridFilter::defaultTags() const
{
  return {className(), "Processing", "Conversion"};
}

//------------------------------------------------------------------------------
Parameters ConvertHexGridToSquareGridFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_MultipleFiles_Key, "Convert Multiple Files", "True=Convert a list of files, False, just convert a single file", false));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Spacing_Key, "Spacing", "Specifies the new spacing values", std::vector<float32>{1.0f, 1.0f}, std::vector<std::string>{"X", "Y"}));
  params.insert(std::make_unique<FileSystemPathParameter>(k_InputPath_Key, "Input File", "Path to the single file to convert", fs::path("input.ang"), FileSystemPathParameter::ExtensionsType{".ang"},
                                                          FileSystemPathParameter::PathType::InputFile, false));
  params.insert(std::make_unique<GeneratedFileListParameter>(k_GeneratedFileList_Key, "Generated File List",
                                                             "The values that are used to generate the input file list. See GeneratedFileListParameter for more information.",
                                                             GeneratedFileListParameter::ValueType{}));

  params.insertSeparator(Parameters::Separator{"Output File"});
  params.insert(std::make_unique<FileSystemPathParameter>(k_OutputPath_Key, "Output Directory", "The path to store the converted file. Should NOT be in the same directory.", fs::path(""),
                                                          FileSystemPathParameter::ExtensionsType{}, FileSystemPathParameter::PathType::OutputDir, true));
  params.insert(std::make_unique<StringParameter>(k_OutputPrefix_Key, "Output Prefix", "A string to prepend the name of each converted file", "Sqr_"));

  // Link Parameters
  params.linkParameters(k_MultipleFiles_Key, k_GeneratedFileList_Key, true);
  params.linkParameters(k_MultipleFiles_Key, k_InputPath_Key, false);

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ConvertHexGridToSquareGridFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ConvertHexGridToSquareGridFilter::clone() const
{
  return std::make_unique<ConvertHexGridToSquareGridFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ConvertHexGridToSquareGridFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                         const std::atomic_bool& shouldCancel) const
{
  auto pUseMultipleFilesValue = filterArgs.value<bool>(k_MultipleFiles_Key);
  auto pInputFileListInfoValue = filterArgs.value<GeneratedFileListParameter::ValueType>(k_GeneratedFileList_Key);
  auto pInputSingleFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputPath_Key);

  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  FileSystemPathParameter::ValueType inputPath = pInputSingleFile;

  if(pUseMultipleFilesValue)
  {
    std::vector<std::string> files = pInputFileListInfoValue.generate();

    if(files.empty())
    {
      return MakePreflightErrorResult(-44600, "GeneratedFileList must not be empty");
    }

    inputPath = fs::path(files[0]);
  }

  if(inputPath.extension() == ("." + EbsdLib::Ctf::FileExt))
  {
    return MakePreflightErrorResult(-44601, "Ctf files are not on a hexagonal grid and do not need to be converted");
  }
  else if(inputPath.extension() != ("." + EbsdLib::Ang::FileExt))
  {
    return MakePreflightErrorResult(-44602, "The file extension was not detected correctly");
  }

  if(!fs::exists(inputPath))
  {
    return MakePreflightErrorResult(-446003, "Please select a valid file path to run");
  }

  if(!fs::exists(s_HeaderCache[m_InstanceId].workingPath) || s_HeaderCache[m_InstanceId].workingPath != inputPath ||
     fs::last_write_time(s_HeaderCache[m_InstanceId].workingPath) > s_HeaderCache[m_InstanceId].lastWrite || s_HeaderCache[m_InstanceId].preflightReturnString.empty())
  {
    s_HeaderCache[m_InstanceId].workingPath = inputPath;
    s_HeaderCache[m_InstanceId].lastWrite = fs::last_write_time(inputPath);
    AngReader reader;
    reader.setFileName(inputPath.string());
    reader.setReadHexGrid(true);
    int32 err = reader.readFile();
    if(err < 0 && err != -600)
    {
      return MakePreflightErrorResult(reader.getErrorCode(), reader.getErrorMessage());
    }
    if(err == -600)
    {
      nx::core::Warning warning;
      warning.code = reader.getErrorCode();
      warning.message = reader.getErrorMessage();
      resultOutputActions.m_Warnings.emplace_back(std::move(warning));
    }

    std::stringstream preflightString;
    preflightString << "Hex Grid X Spacing: " << reader.getXStep() << "\n"
                    << "Hex Grid Y Spacing: " << reader.getYStep();
    s_HeaderCache[m_InstanceId].preflightReturnString = preflightString.str();
  }

  preflightUpdatedValues.emplace_back(PreflightValue{"Suggested Spacing", s_HeaderCache[m_InstanceId].preflightReturnString});

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ConvertHexGridToSquareGridFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                       const std::atomic_bool& shouldCancel) const
{
  ConvertHexGridToSquareGridInputValues inputValues;

  inputValues.MultiFile = filterArgs.value<bool>(k_MultipleFiles_Key);
  inputValues.XYSpacing = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Spacing_Key);
  inputValues.OutputPath = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputPath_Key);
  inputValues.OutputFilePrefix = filterArgs.value<std::string>(k_OutputPrefix_Key);
  inputValues.InputFileListInfo = filterArgs.value<GeneratedFileListParameter::ValueType>(k_GeneratedFileList_Key);
  if(inputValues.MultiFile)
  {
    inputValues.InputPath = fs::path(inputValues.InputFileListInfo.inputPath);
  }
  else
  {
    inputValues.InputPath = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputPath_Key);
  }

  return ConvertHexGridToSquareGrid(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace nx::core
