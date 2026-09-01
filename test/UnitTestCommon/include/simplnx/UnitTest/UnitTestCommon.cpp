#include "UnitTestCommon.hpp"

#include <zlib.h>

#include <array>
#include <cstring>
#include <fstream>

namespace nx::core::UnitTest
{
DataStructure LoadDataStructure(const fs::path& filepath)
{
  LoadPlugins();
  REQUIRE(fs::exists(filepath));

  auto result = DREAM3D::LoadDataStructure(filepath);
  if(result.invalid())
  {
    for(const auto& error : result.errors())
    {
      UNSCOPED_INFO(fmt::format("[{}] {}", error.code, error.message));
    }
    FAIL(fmt::format("Failed to load DataStructure from '{}'", filepath.string()));
  }
  return std::move(result.value());
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
    fs::remove_all(fmt::format("{}/{}", m_TestFilesDir, m_ExpectedTopLevelOutput), errorCode);
    if(errorCode)
    {
      std::cout << "Removing decompressed data failed: " << errorCode.message() << std::endl;
    }
  }
}

namespace
{
/**
 * @brief Parses an unsigned octal field from a tar header.
 * @param data First field byte.
 * @param length Maximum field length in bytes.
 * @return The parsed value, or 0 for an empty field.
 */
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

/**
 * @brief Tests whether a tar header block contains only zero bytes.
 * @param block Header block to test.
 * @return True if all 512 bytes are zero.
 */
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

/**
 * @brief Validates the checksum in a 512-byte tar header.
 * @param header Header block to validate.
 * @return True if the stored and computed checksums are equal.
 *
 * The checksum uses the unsigned sum of all header bytes. The calculation
 * treats the eight-byte checksum field at offset 148 as spaces.
 */
bool validateChecksum(const std::array<char, 512>& header)
{
  uint64 stored = parseOctal(header.data() + 148, 8);
  uint64 computed = 0;
  for(size_t i = 0; i < 512; i++)
  {
    if(i >= 148 && i < 156)
    {
      computed += ' ';
    }
    else
    {
      computed += static_cast<unsigned char>(header[i]);
    }
  }
  return computed == stored;
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
  std::string gnuLongName;

  while(true)
  {
    {
      int bytesRead = gzread(gz, header.data(), k_BlockSize);
      if(bytesRead == 0)
      {
        break; // The gzip stream reached its end.
      }
      if(bytesRead < 0 || bytesRead != k_BlockSize)
      {
        std::cout << "Failed to read tar header from: " << archivePath << std::endl;
        gzclose(gz);
        return std::make_error_code(std::errc::io_error);
      }
    }

    // This extractor treats the first zero header block as the archive end.
    if(isZeroBlock(header))
    {
      break;
    }

    // Validate each header before its fields control path or size calculations.
    if(!validateChecksum(header))
    {
      std::cout << "Invalid tar header checksum in: " << archivePath << std::endl;
      gzclose(gz);
      return std::make_error_code(std::errc::io_error);
    }

    // The tar prefix starts at byte 345. The entry name starts at byte 0.
    std::string prefix(header.data() + 345, strnlen(header.data() + 345, 155));
    std::string name(header.data(), strnlen(header.data(), 100));
    std::string entryPath = prefix.empty() ? name : (prefix + "/" + name);

    // A preceding GNU long-name entry replaces the fixed-width header name.
    if(!gnuLongName.empty())
    {
      entryPath = std::move(gnuLongName);
      gnuLongName.clear();
    }

    uint64 fileSize = parseOctal(header.data() + 124, 12);
    char typeFlag = header[156];

    // A GNU `L` entry stores the name for the next archive entry.
    if(typeFlag == 'L')
    {
      uint64 blocks = (fileSize + k_BlockSize - 1) / k_BlockSize;
      std::string longName;
      longName.reserve(fileSize);
      std::array<char, k_BlockSize> dataBuf{};
      for(uint64 i = 0; i < blocks; i++)
      {
        int bytesRead = gzread(gz, dataBuf.data(), k_BlockSize);
        if(bytesRead <= 0 || static_cast<size_t>(bytesRead) != k_BlockSize)
        {
          std::cout << "Unexpected end of archive reading GNU long name in: " << archivePath << std::endl;
          gzclose(gz);
          return std::make_error_code(std::errc::io_error);
        }
        uint64 useful = std::min(fileSize - longName.size(), static_cast<uint64>(k_BlockSize));
        longName.append(dataBuf.data(), useful);
      }
      // The GNU name payload can include one trailing null byte.
      if(!longName.empty() && longName.back() == '\0')
      {
        longName.pop_back();
      }
      gnuLongName = std::move(longName);
      continue;
    }

    // A GNU `K` entry contains a long link name. This extractor skips its payload.
    if(typeFlag == 'K')
    {
      uint64 blocks = (fileSize + k_BlockSize - 1) / k_BlockSize;
      std::array<char, k_BlockSize> dataBuf{};
      for(uint64 i = 0; i < blocks; i++)
      {
        int bytesRead = gzread(gz, dataBuf.data(), k_BlockSize);
        if(bytesRead <= 0 || static_cast<size_t>(bytesRead) != k_BlockSize)
        {
          std::cout << "Unexpected end of archive reading GNU long link in: " << archivePath << std::endl;
          gzclose(gz);
          return std::make_error_code(std::errc::io_error);
        }
      }
      continue;
    }

    std::string fullPath = fmt::format("{}/{}", m_TestFilesDir, entryPath);

    // Tar type 5 is a directory. Type 0 or null is a regular file.
    if(typeFlag == '5')
    {
      fs::create_directories(fullPath);
    }
    else if(typeFlag == '0' || typeFlag == '\0')
    {
      // Create the parent before this entry writes its regular-file payload.
      fs::path filePath(fullPath);
      fs::create_directories(filePath.parent_path());

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
        int bytesRead = gzread(gz, dataBuf.data(), k_BlockSize);
        if(bytesRead <= 0 || static_cast<size_t>(bytesRead) != k_BlockSize)
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
      // Skip payload blocks for links and other unsupported entry types.
      uint64 blocks = (fileSize + k_BlockSize - 1) / k_BlockSize;
      for(uint64 i = 0; i < blocks; i++)
      {
        int bytesRead = gzread(gz, header.data(), k_BlockSize);
        if(bytesRead <= 0 || static_cast<size_t>(bytesRead) != k_BlockSize)
        {
          std::cout << "Unexpected end of archive skipping data for: " << entryPath << std::endl;
          gzclose(gz);
          return std::make_error_code(std::errc::io_error);
        }
      }
    }
  }

  gzclose(gz);
  return {};
}

PreferencesSentinel::PreferencesSentinel(nx::core::DataStorageMode mode, int64 largeDataSize)
{
  auto* prefs = nx::core::Application::Instance()->getPreferences();

  // Save both values before this sentinel changes the process-wide preferences.
  m_OriginalMode = prefs->dataStorageMode();
  m_OriginalSize = prefs->valueAs<int64>(nx::core::Preferences::k_LargeDataSize_Key);

  // Apply the test values only after the complete prior state is available.
  prefs->setDataStorageMode(mode);
  prefs->setValue(nx::core::Preferences::k_LargeDataSize_Key, largeDataSize);
}

PreferencesSentinel::~PreferencesSentinel()
{
  auto* prefs = nx::core::Application::Instance()->getPreferences();

  // Restore only the in-memory values. Unit tests must not write the developer's preferences file.
  // A concurrent save or a terminated test could persist a temporary storage mode.
  // Later test processes would then inherit that incorrect mode.
  prefs->setDataStorageMode(m_OriginalMode);
  prefs->setValue(nx::core::Preferences::k_LargeDataSize_Key, m_OriginalSize);
}

} // namespace nx::core::UnitTest
