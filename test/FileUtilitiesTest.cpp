#include "simplnx/Utilities/FileUtilities.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/ScopeGuard.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Common/Uuid.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/unit_test/simplnx_test_dirs.hpp"

#include <catch2/catch.hpp>

#include <array>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <utility>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#else
#include <unistd.h>
#endif

using namespace nx::core;
namespace fs = std::filesystem;

namespace
{
class PathFixture
{
public:
  PathFixture()
  : m_OutputRoot(fs::canonical(fs::path(unit_test::k_BinaryTestOutputDir.view())))
  , m_Path(m_OutputRoot / ("fu1856-" + Uuid::GenerateV4().str()))
  {
    REQUIRE(m_Path.parent_path() == m_OutputRoot);
    REQUIRE(fs::create_directory(m_Path));
    REQUIRE(fs::canonical(m_Path) == m_Path);
  }

  ~PathFixture() noexcept
  {
    std::error_code errorCode;
    if(!m_LockedPath.empty())
    {
      fs::permissions(m_LockedPath, m_OriginalPermissions, fs::perm_options::replace, errorCode);
      if(errorCode)
      {
        return;
      }
    }
    const auto resolvedPath = fs::canonical(m_Path, errorCode);
    if(!errorCode && resolvedPath == m_Path)
    {
      fs::remove_all(resolvedPath, errorCode);
    }
  }

  PathFixture(const PathFixture&) = delete;
  PathFixture(PathFixture&&) = delete;
  PathFixture& operator=(const PathFixture&) = delete;
  PathFixture& operator=(PathFixture&&) = delete;

  const fs::path& path() const
  {
    return m_Path;
  }

  void denySearch()
  {
    m_LockedPath = m_Path / "locked";
    REQUIRE(fs::create_directory(m_LockedPath));
    m_OriginalPermissions = fs::status(m_LockedPath).permissions();
    // Keep read and write permission so only the missing search permission blocks descendants.
    fs::permissions(m_LockedPath, fs::perms::owner_read | fs::perms::owner_write, fs::perm_options::replace);
  }

private:
  fs::path m_OutputRoot;
  fs::path m_Path;
  fs::path m_LockedPath;
  fs::perms m_OriginalPermissions = fs::perms::unknown;
};

void RequireFile(const fs::path& path)
{
  std::ofstream stream(path);
  REQUIRE(stream.is_open());
  stream << "fixture";
  stream.close();
  REQUIRE_FALSE(stream.fail());
}
} // namespace

TEST_CASE("FileUtilities::CreateOutputDirectories accepts existing and new directories", "[simplnx][FileUtilities]")
{
  const PathFixture fixture;
  const auto existingResult = CreateOutputDirectories(fixture.path());
  SIMPLNX_RESULT_REQUIRE_VALID(existingResult);
  const auto outputPath = fixture.path() / "new" / "deep";
  const auto newResult = CreateOutputDirectories(outputPath);
  SIMPLNX_RESULT_REQUIRE_VALID(newResult);
  REQUIRE(fs::is_directory(outputPath));
}

TEST_CASE("FileUtilities::CreateOutputDirectories reports the full creation error", "[simplnx][FileUtilities]")
{
  const PathFixture fixture;
  const auto filePath = fixture.path() / "file";
  RequireFile(filePath);
  const auto outputPath = filePath / "out";
  std::error_code expectedError;
  REQUIRE_FALSE(fs::create_directories(outputPath, expectedError));
  REQUIRE(expectedError);

  Result<> result;
  REQUIRE_NOTHROW(result = CreateOutputDirectories(outputPath));
  SIMPLNX_RESULT_REQUIRE_INVALID(result);
  REQUIRE(result.errors().size() == 1);
  CHECK(result.errors()[0].code == -4010);
  const auto utf8Path = outputPath.u8string();
  CHECK(result.errors()[0].message.find(std::string(utf8Path.begin(), utf8Path.end())) != std::string::npos);
  CHECK(result.errors()[0].message.find(std::to_string(expectedError.value())) != std::string::npos);
  CHECK(result.errors()[0].message.find(expectedError.message()) != std::string::npos);
  REQUIRE(fs::is_regular_file(filePath));
}

TEST_CASE("FileUtilities::ValidateDirectoryWritePermission checks writable ancestors without creating directories", "[simplnx][FileUtilities]")
{
  const PathFixture fixture;
  const auto existingDirectory = fixture.path() / "existing";
  REQUIRE(fs::create_directory(existingDirectory));
  const auto existingFile = existingDirectory / "file";
  RequireFile(existingFile);
  const auto missingDirectory = existingDirectory / "new" / "deep";
  const auto missingFile = missingDirectory / "out.txt";

  const std::array<std::pair<fs::path, bool>, 6> cases = {
      {{existingDirectory, false}, {existingFile, true}, {missingDirectory, false}, {missingFile, true}, {fs::relative(existingDirectory), false}, {fs::relative(missingFile), true}}};
  for(const auto& [inputPath, isFile] : cases)
  {
    CAPTURE(inputPath, isFile);
    const auto result = file_utilities::ValidateDirectoryWritePermission(inputPath, isFile);
    SIMPLNX_RESULT_REQUIRE_VALID(result);
  }
  REQUIRE_FALSE(fs::exists(existingDirectory / "new"));
}

TEST_CASE("FileUtilities::CreateOutputDirectories returns a status error below a POSIX directory without search permission", "[simplnx][FileUtilities][PosixPathStatus]")
{
#ifdef _WIN32
  WARN("SIMPLNX_POSIX_PATH_STATUS_SKIPPED: Windows does not implement POSIX directory search permission.");
#else
  if(geteuid() == 0)
  {
    WARN("SIMPLNX_POSIX_PATH_STATUS_SKIPPED: root bypasses POSIX directory search permission.");
    return;
  }
  PathFixture fixture;
  fixture.denySearch();
  const auto outputPath = fixture.path() / "locked" / "new" / "out";
  std::error_code statusError;
  static_cast<void>(fs::exists(outputPath, statusError));
  REQUIRE(statusError == std::errc::permission_denied);

  Result<> result;
  REQUIRE_NOTHROW(result = CreateOutputDirectories(outputPath));
  SIMPLNX_RESULT_REQUIRE_INVALID(result);
  REQUIRE(result.errors().size() == 1);
  CHECK(result.errors()[0].code == -4090);
  CHECK(result.errors()[0].message.find(outputPath.string()) != std::string::npos);
  CHECK(result.errors()[0].message.find(statusError.message()) != std::string::npos);
#endif
}

TEST_CASE("FileUtilities::ValidateDirectoryWritePermission returns a status error below a POSIX directory without search permission", "[simplnx][FileUtilities][PosixPathStatus]")
{
#ifdef _WIN32
  WARN("SIMPLNX_POSIX_PATH_STATUS_SKIPPED: Windows does not implement POSIX directory search permission.");
#else
  if(geteuid() == 0)
  {
    WARN("SIMPLNX_POSIX_PATH_STATUS_SKIPPED: root bypasses POSIX directory search permission.");
    return;
  }
  PathFixture fixture;
  fixture.denySearch();
  const auto directoryPath = fixture.path() / "locked" / "new" / "deep";
  std::error_code statusError;
  static_cast<void>(fs::exists(directoryPath, statusError));
  REQUIRE(statusError == std::errc::permission_denied);

  SECTION("Directory input")
  {
    Result<> result;
    REQUIRE_NOTHROW(result = file_utilities::ValidateDirectoryWritePermission(directoryPath, false));
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().size() == 1);
    CHECK(result.errors()[0].code == -4092);
    CHECK(result.errors()[0].message.find(directoryPath.string()) != std::string::npos);
    CHECK(result.errors()[0].message.find(statusError.message()) != std::string::npos);
  }
  SECTION("File input")
  {
    const auto filePath = directoryPath / "out.txt";
    Result<> result;
    REQUIRE_NOTHROW(result = file_utilities::ValidateDirectoryWritePermission(filePath, true));
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().size() == 1);
    CHECK(result.errors()[0].code == -4092);
    CHECK(result.errors()[0].message.find(filePath.string()) != std::string::npos);
    CHECK(result.errors()[0].message.find(directoryPath.string()) != std::string::npos);
    CHECK(result.errors()[0].message.find(statusError.message()) != std::string::npos);
  }
#endif
}

#ifdef _WIN32
TEST_CASE("FileUtilities returns a Windows status error for a Unicode path", "[simplnx][FileUtilities][WindowsPathStatus]")
{
  const PathFixture fixture;
  const auto directoryPath = fixture.path() / u8"pending-\U0001F600";
  REQUIRE(fs::create_directory(directoryPath));
  const HANDLE directoryHandle = CreateFileW(directoryPath.c_str(), DELETE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
  REQUIRE(directoryHandle != INVALID_HANDLE_VALUE);
  const auto closeDirectory = MakeScopeGuard([directoryHandle]() noexcept { CloseHandle(directoryHandle); });

  // Keep the handle open so the directory remains in the delete-pending state during inspection.
  FILE_DISPOSITION_INFO disposition{TRUE};
  REQUIRE(SetFileInformationByHandle(directoryHandle, FileDispositionInfo, &disposition, sizeof(disposition)) != 0);
  const auto outputPath = directoryPath / "out";
  std::error_code statusError;
  REQUIRE_FALSE(fs::exists(outputPath, statusError));
  REQUIRE(statusError == std::errc::permission_denied);

  const auto checkStatusError = [&statusError](const Result<>& result, int32 resultCode) {
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().size() == 1);
    const auto& error = result.errors()[0];
    CHECK(error.code == resultCode);
    CHECK(error.message.find("pending-\xF0\x9F\x98\x80") != std::string::npos);
    CHECK(error.message.find("Operating system error " + std::to_string(statusError.value())) != std::string::npos);
    CHECK(error.message.find(statusError.message()) != std::string::npos);
  };

  SECTION("Output directory creation")
  {
    Result<> result;
    REQUIRE_NOTHROW(result = CreateOutputDirectories(outputPath));
    checkStatusError(result, -4090);
  }
  SECTION("Output file permission")
  {
    const auto filePath = outputPath / "out.txt";
    Result<> result;
    REQUIRE_NOTHROW(result = FileUtilities::ValidateDirectoryWritePermission(filePath, true));
    checkStatusError(result, -4092);
    CHECK(result.errors()[0].message.find("out.txt") != std::string::npos);
  }
}
#endif
