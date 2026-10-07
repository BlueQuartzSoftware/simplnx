#include "AtomicFile.hpp"

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Utilities/FileUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

#include <fmt/format.h>

#include <array>
#include <cstdint>
#include <filesystem>
#include <random>
#include <string>
#include <system_error>
#include <utility>

#if defined(_WIN32)
#include <fstream>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#if defined(__linux__) || defined(__APPLE__)
#include <sys/xattr.h>
#endif

using namespace nx::core;

namespace fs = std::filesystem;

namespace
{
constexpr std::array<char, 62> k_Chars = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U',
                                          'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'};

/**
 * @brief Gets the persistent random generator for the current thread.
 * @return Per-thread random generator.
 */
std::mt19937_64& getRandomGenerator()
{
  thread_local std::mt19937_64 generator(std::random_device{}());
  return generator;
}

/**
 * @brief Creates a random name for one temporary output directory.
 * @return A 24-character alphanumeric directory name.
 */
std::string createRandomDirectoryName()
{
  std::uniform_int_distribution<std::uint32_t> distribution(0, k_Chars.size() - 1);
  std::mt19937_64& generator = getRandomGenerator();

  std::string randomDirectoryName;
  randomDirectoryName.reserve(24);
  for(std::uint32_t index = 0; index < 24; ++index)
  {
    randomDirectoryName += k_Chars.at(distribution(generator));
  }
  return randomDirectoryName;
}

/**
 * @brief Sets one platform-specific extended file attribute.
 * @param path File or directory that receives the attribute.
 * @param key Attribute key.
 * @param value Attribute value.
 * @return True when the platform accepted the attribute.
 */
bool setExtendedAttribute(const fs::path& path, const std::string& key, const std::string& value)
{
#ifdef _WIN32
  // Set-Content -Path <path> -Stream <name> -Value <value>
  // PowerShell Set-Content appends a carriage return and line feed to the stream.
  // Retain the line ending to match the Dropbox command example.
  // An alternate data stream uses the path form "<path>:<stream>".
  fs::path attributePath = path;
  attributePath += ":" + key;
  std::ofstream outputStream(attributePath);
  outputStream << value << "\n";
  outputStream.close();
  return !outputStream.fail();
#elif defined(__linux__)
  // attr -s <name> -V <value> <path>.
  // The `attr` command automatically prepends "user." to the key.
  const std::string attributeKey = "user." + key;
  return setxattr(path.c_str(), attributeKey.c_str(), value.data(), value.size(), 0) == 0;
#elif defined(__APPLE__)
  // xattr -w <name> <value> <path>.
  return setxattr(path.c_str(), key.c_str(), value.data(), value.size(), 0, 0) == 0;
#else
  static_cast<void>(path);
  static_cast<void>(key);
  static_cast<void>(value);
  return false;
#endif
}

/**
 * @brief Marks a temporary directory to be ignored by supported file-sync services.
 * @param path Temporary directory path.
 * @return True when all applicable attributes were set.
 */
bool setIgnoreFileSyncAttributes(const fs::path& path)
{
  // https://help.dropbox.com/sync/ignored-files
  const bool dropboxResult = setExtendedAttribute(path, "com.dropbox.ignored", "1");

  // Dropbox for macOS on File Provider uses a second attribute.
  // Set both attributes because this code cannot detect the active integration.
#ifdef __APPLE__
  const bool fileProviderResult = setExtendedAttribute(path, "com.apple.fileprovider.ignore#P", "1");
  return dropboxResult && fileProviderResult;
#else
  return dropboxResult;
#endif
}
} // namespace

Result<AtomicFile> AtomicFile::Create(fs::path filename)
{
  AtomicFile atomicFile(std::move(filename));

  // If the path is relative, make it absolute.
  if(!atomicFile.m_FilePath.is_absolute())
  {
    try
    {
      atomicFile.m_FilePath = fs::absolute(atomicFile.m_FilePath);
    } catch(const std::filesystem::filesystem_error& error)
    {
      return MakeErrorResult<AtomicFile>(-15780, fmt::format("Could not resolve output path '{}' as an absolute path: {}.", atomicFile.m_FilePath.string(), error.what()));
    }
  }

  // Validate write permissions.
  {
    auto result = FileUtilities::ValidateDirectoryWritePermission(atomicFile.m_FilePath, true);
    if(result.invalid())
    {
      return ConvertInvalidResult<AtomicFile>(std::move(result));
    }
  }

  {
    const fs::path outputDirectory = atomicFile.m_FilePath.parent_path();

    // Make the requested output directory before creating an exclusive temporary sibling.
    auto result = CreateOutputDirectories(outputDirectory);
    if(result.invalid())
    {
      return ConvertInvalidResult<AtomicFile>(std::move(result));
    }

    constexpr usize kMaxCreateAttempts = 32;
    for(usize attempt = 0; attempt < kMaxCreateAttempts; ++attempt)
    {
      const fs::path temporaryDirectory = outputDirectory / createRandomDirectoryName();
      std::error_code createError;
      if(fs::create_directory(temporaryDirectory, createError))
      {
        atomicFile.m_TempFilePath = temporaryDirectory / atomicFile.m_FilePath.filename();
        break;
      }
      if(createError)
      {
        return MakeErrorResult<AtomicFile>(-15783, fmt::format("Could not create temporary output directory '{}': {}.", temporaryDirectory.string(), createError.message()));
      }
    }
    if(atomicFile.m_TempFilePath.empty())
    {
      return MakeErrorResult<AtomicFile>(-15784,
                                         fmt::format("Could not create a unique temporary output directory beside '{}' after {} attempts.", atomicFile.m_FilePath.string(), kMaxCreateAttempts));
    }

    // A cloud-sync hint is optional and must not add diagnostics to an embedding application.
    static_cast<void>(setIgnoreFileSyncAttributes(atomicFile.m_TempFilePath.parent_path()));
  }

  return {std::move(atomicFile)}; // NOLINT(modernize-use-designated-initializers) Result stores inherited aggregate state.
}

AtomicFile::AtomicFile(fs::path filename)
: m_FilePath(std::move(filename))
{
}

AtomicFile::~AtomicFile() noexcept
{
  removeTempFile();
}

AtomicFile::AtomicFile(AtomicFile&& other) noexcept
: m_FilePath(std::move(other.m_FilePath))
, m_TempFilePath(std::move(other.m_TempFilePath))
{
  other.m_FilePath.clear();
  other.m_TempFilePath.clear();
}

AtomicFile& AtomicFile::operator=(AtomicFile&& other) noexcept
{
  if(this != &other)
  {
    removeTempFile();
    m_FilePath = std::move(other.m_FilePath);
    m_TempFilePath = std::move(other.m_TempFilePath);
    other.m_FilePath.clear();
    other.m_TempFilePath.clear();
  }
  return *this;
}

fs::path AtomicFile::tempFilePath() const
{
  return m_TempFilePath;
}

Result<> AtomicFile::commit()
{
  std::error_code existsError;
  const bool tempFileExists = fs::exists(m_TempFilePath, existsError);
  if(existsError)
  {
    return MakeErrorResult(-15781, fmt::format("Could not inspect temporary output file '{}': {}.", m_TempFilePath.string(), existsError.message()));
  }
  if(!tempFileExists)
  {
    return MakeErrorResult(-15781, fmt::format("Temporary output file '{}' does not exist. Write the output before calling commit().", m_TempFilePath.string()));
  }

#if defined(_WIN32)
  if(MoveFileExW(m_TempFilePath.c_str(), m_FilePath.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) == 0)
  {
    const std::error_code renameError(static_cast<int>(GetLastError()), std::system_category());
    return MakeErrorResult(-15782, fmt::format("Could not replace output file '{}' with temporary file '{}': {}.", m_FilePath.string(), m_TempFilePath.string(), renameError.message()));
  }
#else
  std::error_code renameError;
  fs::rename(m_TempFilePath, m_FilePath, renameError);
  if(renameError)
  {
    return MakeErrorResult(-15782, fmt::format("Could not replace output file '{}' with temporary file '{}': {}.", m_FilePath.string(), m_TempFilePath.string(), renameError.message()));
  }
#endif

  return {};
}

void AtomicFile::removeTempFile() const noexcept
{
  if(m_TempFilePath.empty())
  {
    return;
  }

  std::error_code removeError;
  fs::remove_all(m_TempFilePath.parent_path(), removeError);
  static_cast<void>(removeError);
}
