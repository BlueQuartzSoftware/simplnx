#include "OrientationAnalysis/utilities/EdaxUpPatternFileReader.hpp"

#include "OrientationAnalysis/utilities/EbsdPatternFileUtilities.hpp"

#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"

#include <fmt/format.h>
#include <fmt/ranges.h>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <type_traits>
#include <vector>

namespace nx::core
{
namespace
{
constexpr uint64 k_Version1HeaderSize = 16;
constexpr uint64 k_Version3HeaderSize = 42;
constexpr uint64 k_MinimumPatternDimension = 12;
constexpr uint64 k_MaximumPatternDimension = 4096;
constexpr uint64 k_TargetChunkBytes = 4ULL * 1024ULL * 1024ULL;

template <typename T>
Result<> ReadPayload(const std::filesystem::path& filePath, const EbsdPatternFileInfo& fileInfo, DataArray<T>& outputArray, const std::atomic_bool& shouldCancel,
                     const IFilter::MessageHandler& messageHandler)
{
  std::ifstream stream(filePath, std::ios::binary);
  if(!stream.is_open())
  {
    return MakeErrorResult(k_UpPayloadReadError, fmt::format("Unable to open EBSD pattern file '{}' for pattern import. Check that the file exists and is readable.", filePath.string()));
  }

  stream.seekg(static_cast<std::streamoff>(fileInfo.dataOffset), std::ios::beg);
  if(!stream.good())
  {
    return MakeErrorResult(k_UpPayloadReadError, fmt::format("Unable to seek to pattern payload byte offset {} in EBSD pattern file '{}'.", fileInfo.dataOffset, filePath.string()));
  }

  const uint64 patternsPerChunk = std::max<uint64>(1, k_TargetChunkBytes / fileInfo.patternStride);
  MessageHelper messageHelper(messageHandler);
  auto progressHelper = messageHelper.createProgressMessageHelper();
  progressHelper.setMaxProgresss(static_cast<usize>(fileInfo.numberOfPatterns));
  progressHelper.setProgressMessageTemplate("Reading EBSD patterns: {:.1f}%");
  auto progressMessenger = progressHelper.createProgressMessenger();

  auto& dataStoreRef = outputArray.getDataStoreRef();
  uint64 completedPatterns = 0;
  while(completedPatterns < fileInfo.numberOfPatterns)
  {
    if(shouldCancel)
    {
      return {};
    }

    const uint64 chunkPatternCount = std::min(patternsPerChunk, fileInfo.numberOfPatterns - completedPatterns);
    const uint64 requestedBytes = chunkPatternCount * fileInfo.patternStride;
    std::vector<uint8> buffer(static_cast<usize>(requestedBytes));
    stream.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(requestedBytes));
    const uint64 actualBytes = static_cast<uint64>(stream.gcount());
    if(actualBytes != requestedBytes)
    {
      return MakeErrorResult(k_UpPayloadReadError, fmt::format("Could not read a complete pattern chunk from EBSD pattern file '{}'. Byte offset: {}. Requested bytes: {}. Actual bytes: {}. "
                                                               "Complete patterns imported before the failure: {}.",
                                                               filePath.string(), fileInfo.dataOffset + completedPatterns * fileInfo.patternStride, requestedBytes, actualBytes, completedPatterns));
    }

    const uint64 firstElement = completedPatterns * fileInfo.patternWidth * fileInfo.patternHeight;
    const uint64 elementCount = chunkPatternCount * fileInfo.patternWidth * fileInfo.patternHeight;
    std::vector<T> decodedValues(static_cast<usize>(elementCount));
    if constexpr(std::is_same_v<T, uint8>)
    {
      for(uint64 elementIndex = 0; elementIndex < elementCount; elementIndex++)
      {
        decodedValues[static_cast<usize>(elementIndex)] = buffer[static_cast<usize>(elementIndex)];
      }
    }
    else
    {
      for(uint64 elementIndex = 0; elementIndex < elementCount; elementIndex++)
      {
        const usize byteIndex = static_cast<usize>(elementIndex * 2);
        const uint16 value = static_cast<uint16>(buffer[byteIndex]) | static_cast<uint16>(static_cast<uint16>(buffer[byteIndex + 1]) << 8U);
        decodedValues[static_cast<usize>(elementIndex)] = value;
      }
    }

    if(auto* inMemoryStorePtr = dynamic_cast<DataStore<T>*>(&dataStoreRef); inMemoryStorePtr != nullptr)
    {
      std::copy(decodedValues.begin(), decodedValues.end(), inMemoryStorePtr->data() + static_cast<usize>(firstElement));
    }
    else
    {
      for(uint64 elementIndex = 0; elementIndex < elementCount; elementIndex++)
      {
        dataStoreRef.setValue(static_cast<usize>(firstElement + elementIndex), decodedValues[static_cast<usize>(elementIndex)]);
      }
    }

    completedPatterns += chunkPatternCount;
    progressMessenger.sendProgressMessage(static_cast<usize>(chunkPatternCount));
  }

  return {};
}
} // namespace

EdaxUpPatternFileReader::EdaxUpPatternFileReader(std::filesystem::path filePath)
: m_FilePath(std::move(filePath))
{
}

Result<EbsdPatternFileInfo> EdaxUpPatternFileReader::readFileInfo() const
{
  std::error_code errorCode;
  const uint64 fileSize = std::filesystem::file_size(m_FilePath, errorCode);
  if(errorCode)
  {
    return MakeErrorResult<EbsdPatternFileInfo>(EbsdPatternFileUtilities::k_FileAccessError,
                                                fmt::format("Unable to determine the size of EBSD pattern file '{}': {}", m_FilePath.string(), errorCode.message()));
  }
  if(fileSize < k_Version1HeaderSize)
  {
    return MakeErrorResult<EbsdPatternFileInfo>(EbsdPatternFileUtilities::k_ExactReadError,
                                                fmt::format("EBSD pattern file '{}' is {} bytes, but the smallest UP header is {} bytes.", m_FilePath.string(), fileSize, k_Version1HeaderSize));
  }

  auto minimumHeaderResult = EbsdPatternFileUtilities::ReadExactBytes(m_FilePath, 0, k_Version1HeaderSize);
  if(minimumHeaderResult.invalid())
  {
    return ConvertInvalidResult<EbsdPatternFileInfo>(std::move(minimumHeaderResult));
  }

  const uint32 headerVersion = EbsdPatternFileUtilities::DecodeUInt32LittleEndian(minimumHeaderResult.value().data());
  if(headerVersion == 2 || headerVersion == 0)
  {
    return MakeErrorResult<EbsdPatternFileInfo>(
        k_UnsupportedUpVersionError,
        fmt::format("EDAX UP file '{}' has unsupported header version {}. Released UP header versions are 1 and 3; version 2 has no defined layout.", m_FilePath.string(), headerVersion));
  }

  const bool isVersion1 = headerVersion == 1;
  const bool usesVersion3Layout = headerVersion >= 3;
  if(!isVersion1 && !usesVersion3Layout)
  {
    return MakeErrorResult<EbsdPatternFileInfo>(k_UnsupportedUpVersionError,
                                                fmt::format("EDAX UP file '{}' has unsupported header version {}. Supported versions are 1 and 3.", m_FilePath.string(), headerVersion));
  }

  const uint64 headerSize = isVersion1 ? k_Version1HeaderSize : k_Version3HeaderSize;
  auto headerResult = EbsdPatternFileUtilities::ReadExactBytes(m_FilePath, 0, headerSize);
  if(headerResult.invalid())
  {
    return ConvertInvalidResult<EbsdPatternFileInfo>(std::move(headerResult));
  }
  const auto& bytes = headerResult.value();

  EbsdPatternFileInfo fileInfo;
  fileInfo.formatName = "EDAX UP";
  fileInfo.headerVersion = headerVersion;
  fileInfo.patternWidth = EbsdPatternFileUtilities::DecodeUInt32LittleEndian(bytes.data() + 4);
  fileInfo.patternHeight = EbsdPatternFileUtilities::DecodeUInt32LittleEndian(bytes.data() + 8);
  fileInfo.dataOffset = EbsdPatternFileUtilities::DecodeUInt32LittleEndian(bytes.data() + 12);
  fileInfo.fileSize = fileSize;

  const std::string extension = EbsdPatternFileUtilities::NormalizeExtension(m_FilePath);
  fileInfo.pixelDataType = extension == ".up1" ? DataType::uint8 : DataType::uint16;
  fileInfo.bytesPerPixel = extension == ".up1" ? 1 : 2;

  if(fileInfo.patternWidth < k_MinimumPatternDimension || fileInfo.patternWidth > k_MaximumPatternDimension || fileInfo.patternHeight < k_MinimumPatternDimension ||
     fileInfo.patternHeight > k_MaximumPatternDimension)
  {
    return MakeErrorResult<EbsdPatternFileInfo>(k_InvalidUpDimensionsError,
                                                fmt::format("EDAX UP file '{}' has pattern dimensions {} x {} pixels. Each dimension must be in the inclusive range {} to {} pixels.",
                                                            m_FilePath.string(), fileInfo.patternWidth, fileInfo.patternHeight, k_MinimumPatternDimension, k_MaximumPatternDimension));
  }
  if(fileInfo.dataOffset < headerSize || fileInfo.dataOffset >= fileSize)
  {
    return MakeErrorResult<EbsdPatternFileInfo>(k_InvalidUpDataOffsetError,
                                                fmt::format("EDAX UP version {} file '{}' has data offset {}, but the offset must be at least {} and less than the file size {}.",
                                                            fileInfo.headerVersion, m_FilePath.string(), fileInfo.dataOffset, headerSize, fileSize));
  }

  auto pixelCountResult = EbsdPatternFileUtilities::CheckedMultiply(fileInfo.patternWidth, fileInfo.patternHeight, "the number of pixels per pattern", m_FilePath);
  if(pixelCountResult.invalid())
  {
    return ConvertInvalidResult<EbsdPatternFileInfo>(std::move(pixelCountResult));
  }
  auto strideResult = EbsdPatternFileUtilities::CheckedMultiply(pixelCountResult.value(), fileInfo.bytesPerPixel, "the pattern byte stride", m_FilePath);
  if(strideResult.invalid())
  {
    return ConvertInvalidResult<EbsdPatternFileInfo>(std::move(strideResult));
  }
  fileInfo.patternStride = strideResult.value();

  std::vector<Warning> warnings;
  if(isVersion1)
  {
    const uint64 payloadBytes = fileSize - fileInfo.dataOffset;
    const uint64 residualBytes = payloadBytes % fileInfo.patternStride;
    if(residualBytes != 0)
    {
      return MakeErrorResult<EbsdPatternFileInfo>(
          k_InvalidUpPayloadError, fmt::format("EDAX UP version 1 file '{}' has {} payload bytes after offset {}, which is not an integral number of {}-byte patterns. Residual bytes: {}.",
                                               m_FilePath.string(), payloadBytes, fileInfo.dataOffset, fileInfo.patternStride, residualBytes));
    }
    fileInfo.numberOfPatterns = payloadBytes / fileInfo.patternStride;
    if(fileInfo.numberOfPatterns == 0)
    {
      return MakeErrorResult<EbsdPatternFileInfo>(k_InvalidUpPayloadError,
                                                  fmt::format("EDAX UP version 1 file '{}' contains zero complete patterns after data offset {}.", m_FilePath.string(), fileInfo.dataOffset));
    }
  }
  else
  {
    fileInfo.extraPatterns = bytes[16];
    fileInfo.numberOfColumns = EbsdPatternFileUtilities::DecodeUInt32LittleEndian(bytes.data() + 17);
    fileInfo.numberOfRows = EbsdPatternFileUtilities::DecodeUInt32LittleEndian(bytes.data() + 21);
    fileInfo.isHexagonal = bytes[25] != 0;
    const float64 xStep = EbsdPatternFileUtilities::DecodeFloat64LittleEndian(bytes.data() + 26);
    const float64 yStep = EbsdPatternFileUtilities::DecodeFloat64LittleEndian(bytes.data() + 34);
    if(!std::isfinite(xStep) || !std::isfinite(yStep) || xStep < 0.0 || yStep < 0.0)
    {
      return MakeErrorResult<EbsdPatternFileInfo>(
          k_InvalidUpDimensionsError, fmt::format("EDAX UP version {} file '{}' has invalid scan steps. X step: {} micrometers. Y step: {} micrometers. Values must be finite and nonnegative.",
                                                  fileInfo.headerVersion, m_FilePath.string(), xStep, yStep));
    }
    if(xStep > 0.0)
    {
      fileInfo.xStep = xStep;
    }
    if(yStep > 0.0)
    {
      fileInfo.yStep = yStep;
    }
    if(xStep == 0.0 || yStep == 0.0)
    {
      warnings.push_back({k_UnknownUpStepWarning, fmt::format("EDAX UP version {} file '{}' has an unknown scan step represented by zero. X step: {} micrometers. Y step: {} micrometers.",
                                                              fileInfo.headerVersion, m_FilePath.string(), xStep, yStep)});
    }

    auto gridPatternCountResult = EbsdPatternFileUtilities::CheckedMultiply(fileInfo.numberOfRows.value(), fileInfo.numberOfColumns.value(), "the version 3 grid pattern count", m_FilePath);
    if(gridPatternCountResult.invalid())
    {
      return ConvertInvalidResult<EbsdPatternFileInfo>(std::move(gridPatternCountResult));
    }
    fileInfo.numberOfPatterns = gridPatternCountResult.value();
    if(fileInfo.numberOfPatterns == 0)
    {
      return MakeErrorResult<EbsdPatternFileInfo>(k_InvalidUpPayloadError, fmt::format("EDAX UP version {} file '{}' describes an empty grid with {} rows and {} columns.", fileInfo.headerVersion,
                                                                                       m_FilePath.string(), fileInfo.numberOfRows.value(), fileInfo.numberOfColumns.value()));
    }

    auto gridPayloadSizeResult = EbsdPatternFileUtilities::CheckedMultiply(fileInfo.numberOfPatterns, fileInfo.patternStride, "the version 3 grid payload size", m_FilePath);
    if(gridPayloadSizeResult.invalid())
    {
      return ConvertInvalidResult<EbsdPatternFileInfo>(std::move(gridPayloadSizeResult));
    }
    auto gridFileSizeResult = EbsdPatternFileUtilities::CheckedAdd(fileInfo.dataOffset, gridPayloadSizeResult.value(), "the version 3 grid file size", m_FilePath);
    if(gridFileSizeResult.invalid())
    {
      return ConvertInvalidResult<EbsdPatternFileInfo>(std::move(gridFileSizeResult));
    }
    const uint64 oppositeBytesPerPixel = fileInfo.bytesPerPixel == 1 ? 2 : 1;
    bool matchesOppositePixelWidth = false;
    auto oppositeStrideResult = EbsdPatternFileUtilities::CheckedMultiply(fileInfo.patternWidth * fileInfo.patternHeight, oppositeBytesPerPixel, "the opposite pixel-width pattern stride", m_FilePath);
    if(oppositeStrideResult.valid())
    {
      auto oppositePayloadResult = EbsdPatternFileUtilities::CheckedMultiply(fileInfo.numberOfPatterns, oppositeStrideResult.value(), "the opposite pixel-width grid payload", m_FilePath);
      if(oppositePayloadResult.valid())
      {
        auto oppositeFileSizeResult = EbsdPatternFileUtilities::CheckedAdd(fileInfo.dataOffset, oppositePayloadResult.value(), "the opposite pixel-width file size", m_FilePath);
        matchesOppositePixelWidth = oppositeFileSizeResult.valid() && fileSize == oppositeFileSizeResult.value();
      }
    }

    if(fileSize < gridFileSizeResult.value())
    {
      const std::string extensionHint = matchesOppositePixelWidth ? " The payload size exactly matches the opposite UP pixel width; verify the .up1/.up2 extension." : "";
      return MakeErrorResult<EbsdPatternFileInfo>(matchesOppositePixelWidth ? k_UpExtensionMismatchError : k_InvalidUpPayloadError,
                                                  fmt::format("EDAX UP version {} file '{}' is too small for its grid. Expected at least {} bytes for {} patterns; actual file size: {}.{}",
                                                              fileInfo.headerVersion, m_FilePath.string(), gridFileSizeResult.value(), fileInfo.numberOfPatterns, fileSize, extensionHint));
    }

    const uint64 trailingBytes = fileSize - gridFileSizeResult.value();
    const uint64 declaredExtraBytes = static_cast<uint64>(fileInfo.extraPatterns) * fileInfo.patternStride;
    if(fileInfo.extraPatterns > 0)
    {
      warnings.push_back({k_ExtraUpPatternsWarning, fmt::format("EDAX UP version {} file '{}' declares {} extra patterns. They will be skipped; {} grid patterns will be imported.",
                                                                fileInfo.headerVersion, m_FilePath.string(), fileInfo.extraPatterns, fileInfo.numberOfPatterns)});
      if(trailingBytes != declaredExtraBytes)
      {
        warnings.push_back({k_ExtraUpPatternsSizeWarning,
                            fmt::format("EDAX UP version {} file '{}' declares {} extra patterns ({} bytes), but {} trailing bytes follow the grid payload. The complete grid will still be imported.",
                                        fileInfo.headerVersion, m_FilePath.string(), fileInfo.extraPatterns, declaredExtraBytes, trailingBytes)});
      }
    }
    else if(trailingBytes != 0)
    {
      if(matchesOppositePixelWidth)
      {
        return MakeErrorResult<EbsdPatternFileInfo>(k_UpExtensionMismatchError,
                                                    fmt::format("EDAX UP version {} file '{}' has a payload size that exactly matches the opposite UP pixel width. Verify the .up1/.up2 extension.",
                                                                fileInfo.headerVersion, m_FilePath.string()));
      }
      return MakeErrorResult<EbsdPatternFileInfo>(
          k_InvalidUpPayloadError, fmt::format("EDAX UP version {} file '{}' has {} undeclared trailing bytes after the complete grid payload. Expected file size: {}. Actual file size: {}.",
                                               fileInfo.headerVersion, m_FilePath.string(), trailingBytes, gridFileSizeResult.value(), fileSize));
    }
  }

  if(fileInfo.headerVersion > 3)
  {
    warnings.push_back(
        {k_FutureUpVersionWarning, fmt::format("EDAX UP file '{}' reports header version {}. It will be read using the version 3 layout.", m_FilePath.string(), fileInfo.headerVersion)});
  }
  if(fileInfo.dataOffset != headerSize)
  {
    warnings.push_back({k_UnusualUpDataOffsetWarning, fmt::format("EDAX UP version {} file '{}' uses data offset {}; the usual offset is {}. The file-supplied offset will be used.",
                                                                  fileInfo.headerVersion, m_FilePath.string(), fileInfo.dataOffset, headerSize)});
  }

  Result<EbsdPatternFileInfo> result{fileInfo};
  result.warnings() = std::move(warnings);
  return result;
}

Result<> EdaxUpPatternFileReader::readPatternData(const EbsdPatternFileInfo& fileInfo, IDataArray& outputArray, const std::atomic_bool& shouldCancel,
                                                  const IFilter::MessageHandler& messageHandler) const
{
  const ShapeType& componentShape = outputArray.getComponentShape();
  if(outputArray.getDataType() != fileInfo.pixelDataType || outputArray.getNumberOfTuples() != fileInfo.numberOfPatterns ||
     componentShape != ShapeType{static_cast<usize>(fileInfo.patternHeight), static_cast<usize>(fileInfo.patternWidth)})
  {
    const std::string componentDescription = componentShape.size() == 2 ? fmt::format("{} x {}", componentShape[0], componentShape[1]) : fmt::format("[{}]", fmt::join(componentShape, ", "));
    return MakeErrorResult(k_UpDestinationMismatchError,
                           fmt::format("Output DataArray '{}' does not match EBSD pattern file '{}'. Array type: '{}', tuples: {}, components: {}. File type: '{}', patterns: {}, "
                                       "pattern size: {} x {}.",
                                       outputArray.getName(), m_FilePath.string(), DataTypeToString(outputArray.getDataType()), outputArray.getNumberOfTuples(), componentDescription,
                                       DataTypeToString(fileInfo.pixelDataType), fileInfo.numberOfPatterns, fileInfo.patternHeight, fileInfo.patternWidth));
  }

  if(fileInfo.pixelDataType == DataType::uint8)
  {
    return ReadPayload(m_FilePath, fileInfo, dynamic_cast<UInt8Array&>(outputArray), shouldCancel, messageHandler);
  }
  return ReadPayload(m_FilePath, fileInfo, dynamic_cast<UInt16Array&>(outputArray), shouldCancel, messageHandler);
}
} // namespace nx::core
