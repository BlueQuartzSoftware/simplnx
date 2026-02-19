#include "UnitTestCommon.hpp"

#include "simplnx/Parameters/Dream3dImportParameter.hpp"

#include <zlib.h>

#include <array>
#include <cstring>
#include <fstream>

namespace nx::core::UnitTest
{
DataStructure LoadDataStructure(const fs::path& filepath)
{
  // Ensure the plugins a loaded.
  LoadPlugins();

  INFO(fmt::format("Error loading file: '{}'  ", filepath.string()));
  REQUIRE(fs::exists(filepath));

  DataStructure dataStructure;

  // const Uuid k_SimplnxCorePluginId = *Uuid::FromString("05cc618b-781f-4ac0-b9ac-43f26ce1854f");
  auto* filterList = Application::Instance()->getFilterList();
  /*************************************************************************
   * ReadDREAM3DFilter
   ************************************************************************/
  constexpr Uuid k_ReadDREAM3DFilterId = *Uuid::FromString("0dbd31c7-19e0-4077-83ef-f4a6459a0e2d");
  const FilterHandle k_ReadDREAM3DFilterHandle(k_ReadDREAM3DFilterId, k_SimplnxCorePluginId);

  auto filterPtr = filterList->createFilter(k_ReadDREAM3DFilterHandle);
  REQUIRE(nullptr != filterPtr);

  Arguments args;
  args.insertOrAssign("import_data_object", std::make_any<Dream3dImportParameter::ImportData>(Dream3dImportParameter::ImportData{filepath, Dream3dImportParameter::PathImportPolicy::All}));

  // Preflight the filter and check result
  auto preflightResult = filterPtr->preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filterPtr->execute(dataStructure, args); //, nullptr, IFilter::MessageHandler{[](const IFilter::Message& message) { fmt::print("{}\n", message.message); }});
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  return dataStructure;
}

TestFileSentinel::TestFileSentinel(std::string testFilesDir, std::string inputArchiveName, std::string expectedTopLevelOutput, bool decompressFiles, bool removeTemp)
: m_TestFilesDir(std::move(testFilesDir))
, m_InputArchiveName(std::move(inputArchiveName))
, m_ExpectedTopLevelOutput(std::move(expectedTopLevelOutput))
, m_Decompress(decompressFiles)
, m_RemoveTemp(removeTemp)
{
  if(m_Decompress)
  {
    const auto errorCode = decompress();
    if(errorCode)
    {
      std::cout << "std::error_code.value(): " << errorCode.value() << std::endl;
      std::cout << "std::error_code.message(): " << errorCode.message() << std::endl;
      //        REQUIRE(errorCode.value() == 0);
    }
  }
}

TestFileSentinel::~TestFileSentinel()
{
  if(m_RemoveTemp)
  {
    std::error_code errorCode;
    std::filesystem::remove_all(fmt::format("{}/{}", m_TestFilesDir, m_ExpectedTopLevelOutput), errorCode);
    if(errorCode)
    {
      std::cout << "Removing decompressed data failed: " << errorCode.message() << std::endl;
    }
  }
}

namespace
{
// Parse an octal field from a tar header, returning 0 on empty/null fields
uint64 parseOctal(const char* data, size_t length)
{
  uint64 value = 0;
  for(size_t i = 0; i < length; i++)
  {
    if(data[i] == '\0' || data[i] == ' ')
    {
      break;
    }
    value = value * 8 + static_cast<uint64>(data[i] - '0');
  }
  return value;
}

// Check if a 512-byte tar header block is all zeros (end-of-archive marker)
bool isZeroBlock(const std::array<char, 512>& block)
{
  for(char c : block)
  {
    if(c != '\0')
    {
      return false;
    }
  }
  return true;
}
} // namespace

std::error_code TestFileSentinel::decompress()
{
  const std::string archivePath = fmt::format("{}/{}", m_TestFilesDir, m_InputArchiveName);

  gzFile gz = gzopen(archivePath.c_str(), "rb");
  if(gz == nullptr)
  {
    std::cout << "Failed to open archive: " << archivePath << std::endl;
    return std::make_error_code(std::errc::no_such_file_or_directory);
  }

  constexpr size_t k_BlockSize = 512;
  std::array<char, k_BlockSize> header{};

  while(true)
  {
    int bytesRead = gzread(gz, header.data(), k_BlockSize);
    if(bytesRead == 0)
    {
      break; // EOF
    }
    if(bytesRead < 0 || bytesRead != k_BlockSize)
    {
      std::cout << "Failed to read tar header from: " << archivePath << std::endl;
      gzclose(gz);
      return std::make_error_code(std::errc::io_error);
    }

    // Two consecutive zero blocks mark end of archive
    if(isZeroBlock(header))
    {
      break;
    }

    // Parse tar header fields
    // offset 345, 155 bytes: prefix; offset 0, 100 bytes: name
    std::string prefix(header.data() + 345, strnlen(header.data() + 345, 155));
    std::string name(header.data(), strnlen(header.data(), 100));
    std::string entryPath = prefix.empty() ? name : (prefix + "/" + name);

    uint64 fileSize = parseOctal(header.data() + 124, 12);
    char typeFlag = header[156];

    std::string fullPath = fmt::format("{}/{}", m_TestFilesDir, entryPath);

    // typeFlag: '5' = directory, '0' or '\0' = regular file, '2' = symlink
    if(typeFlag == '5')
    {
      std::filesystem::create_directories(fullPath);
    }
    else if(typeFlag == '0' || typeFlag == '\0')
    {
      // Ensure parent directory exists
      std::filesystem::path filePath(fullPath);
      std::filesystem::create_directories(filePath.parent_path());

      std::ofstream outFile(fullPath, std::ios::binary);
      if(!outFile)
      {
        std::cout << "Failed to create file: " << fullPath << std::endl;
        gzclose(gz);
        return std::make_error_code(std::errc::io_error);
      }

      uint64 remaining = fileSize;
      std::array<char, k_BlockSize> dataBuf{};
      while(remaining > 0)
      {
        int toRead = gzread(gz, dataBuf.data(), k_BlockSize);
        if(toRead <= 0)
        {
          std::cout << "Unexpected end of archive reading: " << entryPath << std::endl;
          gzclose(gz);
          return std::make_error_code(std::errc::io_error);
        }
        uint64 writeSize = std::min(remaining, static_cast<uint64>(k_BlockSize));
        outFile.write(dataBuf.data(), static_cast<std::streamsize>(writeSize));
        remaining -= writeSize;
      }
    }
    else
    {
      // For other types (symlinks, etc.), skip the data blocks
      uint64 blocks = (fileSize + k_BlockSize - 1) / k_BlockSize;
      for(uint64 i = 0; i < blocks; i++)
      {
        gzread(gz, header.data(), k_BlockSize);
      }
    }
  }

  gzclose(gz);
  return {};
}

PreferencesSentinel::PreferencesSentinel(std::string largeDataFormat, int64 largeDataSize, bool forceOocData)
{
  auto* prefs = nx::core::Application::Instance()->getPreferences();

  // Save current preference values
  m_OriginalFormat = prefs->largeDataFormat();
  m_OriginalSize = prefs->valueAs<int64>(nx::core::Preferences::k_LargeDataSize_Key);
  m_OriginalForceOoc = prefs->forceOocData();

  // Set new preference values
  prefs->setLargeDataFormat(std::move(largeDataFormat));
  prefs->setValue(nx::core::Preferences::k_LargeDataSize_Key, largeDataSize);
  prefs->setForceOocData(forceOocData);
}

PreferencesSentinel::~PreferencesSentinel()
{
  auto* prefs = nx::core::Application::Instance()->getPreferences();

  // Restore original preference values
  prefs->setLargeDataFormat(m_OriginalFormat);
  prefs->setValue(nx::core::Preferences::k_LargeDataSize_Key, m_OriginalSize);
  prefs->setForceOocData(m_OriginalForceOoc);

  // Save preferences to disk
  nx::core::Application::Instance()->savePreferences();
}

} // namespace nx::core::UnitTest
