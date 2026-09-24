#include "OrientationAnalysis/Filters/ReadEbsdPatternFileFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/ReadEbsdPatternFile.hpp"
#include "OrientationAnalysis/utilities/EbsdPatternFileReaderFactory.hpp"
#include "OrientationAnalysis/utilities/EbsdPatternFileUtilities.hpp"

#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"

#include <fmt/format.h>
#include <fmt/ranges.h>

#include <filesystem>

namespace fs = std::filesystem;

namespace nx::core
{
std::string ReadEbsdPatternFileFilter::name() const
{
  return FilterTraits<ReadEbsdPatternFileFilter>::name.str();
}

std::string ReadEbsdPatternFileFilter::className() const
{
  return FilterTraits<ReadEbsdPatternFileFilter>::className;
}

Uuid ReadEbsdPatternFileFilter::uuid() const
{
  return FilterTraits<ReadEbsdPatternFileFilter>::uuid;
}

std::string ReadEbsdPatternFileFilter::humanName() const
{
  return "Read EBSD Pattern File";
}

std::vector<std::string> ReadEbsdPatternFileFilter::defaultTags() const
{
  return {className(), "IO", "Input", "Read", "Import", "EBSD", "Pattern"};
}

Parameters ReadEbsdPatternFileFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input File"});
  params.insert(std::make_unique<FileSystemPathParameter>(k_InputFile_Key, "Input File", "EDAX EBSD pattern file to read", fs::path(), FileSystemPathParameter::ExtensionsType{".up1", ".up2"},
                                                          FileSystemPathParameter::PathType::InputFile));

  params.insertSeparator(Parameters::Separator{"Version 1 Scan Dimensions"});
  params.insertLinkableParameter(
      std::make_unique<BoolParameter>(k_SetScanDimensions_Key, "Set Version 1 Scan Dimensions",
                                      "Use explicit scan rows and columns for a version 1 file created outside an Attribute Matrix. Version 1 files do not store scan geometry.", false));
  params.insert(std::make_unique<UInt64Parameter>(k_NumberOfRows_Key, "Number of Rows", "Number of pattern rows for a version 1 file", 1));
  params.insert(std::make_unique<UInt64Parameter>(k_NumberOfColumns_Key, "Number of Columns", "Number of pattern columns for a version 1 file", 1));

  params.insertSeparator(Parameters::Separator{"Output Data Array"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputArrayPath_Key, "Output Pattern Data Array", "Path to the created EBSD pattern DataArray", DataPath({"Patterns"})));

  params.linkParameters(k_SetScanDimensions_Key, k_NumberOfRows_Key, true);
  params.linkParameters(k_SetScanDimensions_Key, k_NumberOfColumns_Key, true);
  return params;
}

IFilter::VersionType ReadEbsdPatternFileFilter::parametersVersion() const
{
  return 1;
}

IFilter::UniquePointer ReadEbsdPatternFileFilter::clone() const
{
  return std::make_unique<ReadEbsdPatternFileFilter>();
}

IFilter::PreflightResult ReadEbsdPatternFileFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                  const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  const fs::path inputFile = executionContext.getAbsolutePath(filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key));
  const bool setScanDimensions = filterArgs.value<bool>(k_SetScanDimensions_Key);
  const uint64 numberOfRows = filterArgs.value<uint64>(k_NumberOfRows_Key);
  const uint64 numberOfColumns = filterArgs.value<uint64>(k_NumberOfColumns_Key);
  const DataPath outputArrayPath = filterArgs.value<DataPath>(k_OutputArrayPath_Key);

  auto readerResult = CreateEbsdPatternFileReader(inputFile);
  if(readerResult.invalid())
  {
    return {ConvertInvalidResult<OutputActions>(std::move(readerResult))};
  }
  auto fileInfoResult = readerResult.value()->readFileInfo();
  if(fileInfoResult.invalid())
  {
    return {ConvertInvalidResult<OutputActions>(std::move(fileInfoResult))};
  }
  const EbsdPatternFileInfo& fileInfo = fileInfoResult.value();

  auto patternCountSizeResult = EbsdPatternFileUtilities::CheckedToSize(fileInfo.numberOfPatterns, "the imported pattern count", inputFile);
  if(patternCountSizeResult.invalid())
  {
    return {ConvertInvalidResult<OutputActions>(std::move(patternCountSizeResult))};
  }
  auto patternHeightSizeResult = EbsdPatternFileUtilities::CheckedToSize(fileInfo.patternHeight, "the pattern height", inputFile);
  if(patternHeightSizeResult.invalid())
  {
    return {ConvertInvalidResult<OutputActions>(std::move(patternHeightSizeResult))};
  }
  auto patternWidthSizeResult = EbsdPatternFileUtilities::CheckedToSize(fileInfo.patternWidth, "the pattern width", inputFile);
  if(patternWidthSizeResult.invalid())
  {
    return {ConvertInvalidResult<OutputActions>(std::move(patternWidthSizeResult))};
  }

  Result<OutputActions> outputActions;
  outputActions.warnings() = std::move(fileInfoResult.warnings());
  ShapeType tupleShape;
  const auto* parentAttributeMatrixPtr = dataStructure.getDataAs<AttributeMatrix>(outputArrayPath.getParent());
  if(parentAttributeMatrixPtr != nullptr)
  {
    tupleShape = parentAttributeMatrixPtr->getShape();
    if(parentAttributeMatrixPtr->getNumberOfTuples() != fileInfo.numberOfPatterns)
    {
      return {MakeErrorResult<OutputActions>(
          -78040, fmt::format("Output Attribute Matrix '{}' has {} tuples, but EBSD pattern file '{}' contains {} importable patterns. Select an Attribute Matrix with exactly {} tuples or create "
                              "the output array outside an Attribute Matrix.",
                              outputArrayPath.getParent().toString(), parentAttributeMatrixPtr->getNumberOfTuples(), inputFile.string(), fileInfo.numberOfPatterns, fileInfo.numberOfPatterns))};
    }
    if(setScanDimensions)
    {
      outputActions.warnings().push_back({-78041, fmt::format("Version 1 scan dimensions were supplied, but output array '{}' is inside Attribute Matrix '{}'. The Attribute Matrix tuple shape "
                                                              "will be used.",
                                                              outputArrayPath.toString(), outputArrayPath.getParent().toString())});
    }
  }
  else if(fileInfo.headerVersion == 1 && setScanDimensions)
  {
    if(numberOfRows == 0 || numberOfColumns == 0)
    {
      return {MakeErrorResult<OutputActions>(
          -78042, fmt::format("Version 1 scan dimensions for EBSD pattern file '{}' must be positive. Rows: {}. Columns: {}.", inputFile.string(), numberOfRows, numberOfColumns))};
    }
    auto patternCountResult = EbsdPatternFileUtilities::CheckedMultiply(numberOfRows, numberOfColumns, "the user-supplied version 1 scan pattern count", inputFile);
    if(patternCountResult.invalid())
    {
      return {ConvertInvalidResult<OutputActions>(std::move(patternCountResult))};
    }
    if(patternCountResult.value() != fileInfo.numberOfPatterns)
    {
      return {MakeErrorResult<OutputActions>(-78043, fmt::format("Version 1 scan dimensions for EBSD pattern file '{}' describe {} patterns ({} rows x {} columns), but the file contains {} complete "
                                                                 "patterns.",
                                                                 inputFile.string(), patternCountResult.value(), numberOfRows, numberOfColumns, fileInfo.numberOfPatterns))};
    }
    tupleShape = {static_cast<usize>(numberOfRows), static_cast<usize>(numberOfColumns)};
  }
  else if(fileInfo.headerVersion == 1)
  {
    tupleShape = {patternCountSizeResult.value()};
  }
  else
  {
    auto numberOfRowsSizeResult = EbsdPatternFileUtilities::CheckedToSize(fileInfo.numberOfRows.value(), "the stored scan row count", inputFile);
    if(numberOfRowsSizeResult.invalid())
    {
      return {ConvertInvalidResult<OutputActions>(std::move(numberOfRowsSizeResult))};
    }
    auto numberOfColumnsSizeResult = EbsdPatternFileUtilities::CheckedToSize(fileInfo.numberOfColumns.value(), "the stored scan column count", inputFile);
    if(numberOfColumnsSizeResult.invalid())
    {
      return {ConvertInvalidResult<OutputActions>(std::move(numberOfColumnsSizeResult))};
    }
    tupleShape = {numberOfRowsSizeResult.value(), numberOfColumnsSizeResult.value()};
    if(setScanDimensions)
    {
      outputActions.warnings().push_back({-78044, fmt::format("Version 1 scan dimensions were supplied for EBSD pattern file '{}', but header version {} stores its own scan dimensions. The file "
                                                              "dimensions will be used.",
                                                              inputFile.string(), fileInfo.headerVersion)});
    }
  }

  const ShapeType componentShape = {patternHeightSizeResult.value(), patternWidthSizeResult.value()};
  outputActions.value().appendAction(std::make_unique<CreateArrayAction>(fileInfo.pixelDataType, tupleShape, componentShape, outputArrayPath));

  std::vector<PreflightValue> updatedValues;
  std::string fileSummary = fmt::format("Format: {}\nHeader Version: {}\nPixel Type: {}\nPattern Size: {} x {} pixels\nImported Patterns: {}", fileInfo.formatName, fileInfo.headerVersion,
                                        DataTypeToString(fileInfo.pixelDataType), fileInfo.patternWidth, fileInfo.patternHeight, fileInfo.numberOfPatterns);
  if(fileInfo.numberOfRows.has_value() && fileInfo.numberOfColumns.has_value())
  {
    fileSummary +=
        fmt::format("\nStored Scan Grid: {} rows x {} columns\nGrid Type: {}", fileInfo.numberOfRows.value(), fileInfo.numberOfColumns.value(), fileInfo.isHexagonal ? "Hexagonal" : "Square");
    fileSummary += fmt::format("\nX Step: {}\nY Step: {}", fileInfo.xStep.has_value() ? fmt::format("{} micrometers", fileInfo.xStep.value()) : std::string("unknown"),
                               fileInfo.yStep.has_value() ? fmt::format("{} micrometers", fileInfo.yStep.value()) : std::string("unknown"));
  }
  if(fileInfo.extraPatterns > 0)
  {
    fileSummary += fmt::format("\nSkipped Extra Patterns: {}", fileInfo.extraPatterns);
  }
  fileSummary += fmt::format("\nOutput Tuple Shape: [{}]\nOutput Component Shape: [{}]", fmt::join(tupleShape, ", "), fmt::join(componentShape, ", "));
  updatedValues.push_back({"EBSD Pattern File Information", std::move(fileSummary)});
  return {std::move(outputActions), std::move(updatedValues)};
}

Result<> ReadEbsdPatternFileFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  ReadEbsdPatternFileInputValues inputValues;
  inputValues.inputFile = executionContext.getAbsolutePath(filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key));
  inputValues.outputArrayPath = filterArgs.value<DataPath>(k_OutputArrayPath_Key);
  inputValues.setScanDimensions = filterArgs.value<bool>(k_SetScanDimensions_Key);
  inputValues.numberOfRows = filterArgs.value<uint64>(k_NumberOfRows_Key);
  inputValues.numberOfColumns = filterArgs.value<uint64>(k_NumberOfColumns_Key);
  return ReadEbsdPatternFile(dataStructure, inputValues, shouldCancel, messageHandler)();
}

Result<Arguments> ReadEbsdPatternFileFilter::FromSIMPLJson(const nlohmann::json& json)
{
  return {ReadEbsdPatternFileFilter().getDefaultArguments()};
}
} // namespace nx::core
