/**
 * @file AtomicFileTest.cpp
 * @brief Tests transactional file replacement and temporary-path ownership.
 */

#include "simplnx/Common/AtomicFile.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/ScopeGuard.hpp"
#include "simplnx/Common/Uuid.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/unit_test/simplnx_test_dirs.hpp"

#include <catch2/catch.hpp>

#include <filesystem>
#include <fstream>
#include <ios>
#include <iterator>
#include <string>
#include <system_error>
#include <utility>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#endif

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
/**
 * @brief Reads an entire text file.
 * @param filePath File to read.
 * @return Complete file contents.
 */
std::string readTextFile(const fs::path& filePath)
{
  std::ifstream inputStream(filePath);
  return {std::istreambuf_iterator<char>(inputStream), std::istreambuf_iterator<char>()};
}

/**
 * @brief Writes text to an AtomicFile temporary path.
 * @param atomicFile Transaction whose temporary file receives the text.
 * @param text Text to write.
 */
void writeTemporaryFile(const AtomicFile& atomicFile, const std::string& text)
{
  std::ofstream outputStream(atomicFile.tempFilePath(), std::ios::trunc);
  REQUIRE(outputStream.is_open());
  outputStream << text;
  outputStream.close();
  REQUIRE_FALSE(outputStream.fail());
}
} // namespace

TEST_CASE("AtomicFile::CommitReplacesExistingFile")
{
  const fs::path outputPath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "atomic_file_replace.txt";
  {
    std::ofstream existingOutput(outputPath, std::ios::trunc);
    existingOutput << "old contents";
  }

  auto atomicFileResult = AtomicFile::Create(outputPath);
  REQUIRE(atomicFileResult.valid());
  AtomicFile atomicFile = std::move(atomicFileResult.value());
  const fs::path temporaryDirectory = atomicFile.tempFilePath().parent_path();
  writeTemporaryFile(atomicFile, "new contents");

  const Result<> commitResult = atomicFile.commit();
  REQUIRE(commitResult.valid());
  REQUIRE(readTextFile(outputPath) == "new contents");

  atomicFile.removeTempFile();
  REQUIRE_FALSE(fs::exists(temporaryDirectory));
  fs::remove(outputPath);
}

TEST_CASE("AtomicFile preserves the destination and temporary output after a failed replacement", "[simplnx]")
{
  const fs::path directory = fs::path(unit_test::k_BinaryTestOutputDir.view()) / ("atomic_file_" + Uuid::GenerateV4().str());
  REQUIRE(fs::create_directory(directory));
  const auto cleanup = MakeScopeGuard([&directory]() noexcept {
    std::error_code error;
    fs::remove_all(directory, error);
  });
  const fs::path destination = directory / "output.txt";
  const fs::path existingChild = destination / "existing.txt";
  fs::path temporaryDirectory;
  {
    auto atomicFileResult = AtomicFile::Create(destination);
    SIMPLNX_RESULT_REQUIRE_VALID(atomicFileResult); // NOLINT(cppcoreguidelines-avoid-do-while) Project Result assertion macro.
    AtomicFile atomicFile = std::move(atomicFileResult.value());
    const fs::path temporaryPath = atomicFile.tempFilePath();
    temporaryDirectory = temporaryPath.parent_path();
    writeTemporaryFile(atomicFile, "new contents");
    REQUIRE(fs::create_directory(destination));
    {
      std::ofstream output(existingChild);
      output << "old contents";
      output.close();
      REQUIRE_FALSE(output.fail());
    }

    const auto result = atomicFile.commit();
    SIMPLNX_RESULT_REQUIRE_INVALID(result); // NOLINT(cppcoreguidelines-avoid-do-while) Project Result assertion macro.
    REQUIRE(result.errors().size() == 1);
    CHECK(result.errors().front().code == -15782);
    CHECK_THAT(result.errors().front().message, Catch::Matchers::Contains(destination.string()));
    CHECK(readTextFile(existingChild) == "old contents");
    CHECK(readTextFile(temporaryPath) == "new contents");
  }

  CHECK_FALSE(fs::exists(temporaryDirectory));
  CHECK(readTextFile(existingChild) == "old contents");
}

#ifdef _WIN32
TEST_CASE("AtomicFile::CommitReplacesFileWithFullSharingHandle", "[simplnx]")
{
  const fs::path directory = fs::path(unit_test::k_BinaryTestOutputDir.view()) / ("atomic_file_" + Uuid::GenerateV4().str());
  REQUIRE(fs::create_directory(directory));
  const auto cleanup = MakeScopeGuard([&directory]() noexcept {
    std::error_code error;
    fs::remove_all(directory, error);
  });
  const fs::path outputPath = directory / "output.txt";
  {
    std::ofstream existingOutput(outputPath);
    REQUIRE(existingOutput.is_open());
    existingOutput << "old contents";
    existingOutput.close();
    REQUIRE_FALSE(existingOutput.fail());
  }

  auto atomicFileResult = AtomicFile::Create(outputPath);
  SIMPLNX_RESULT_REQUIRE_VALID(atomicFileResult); // NOLINT(cppcoreguidelines-avoid-do-while) Project Result assertion macro.
  AtomicFile atomicFile = std::move(atomicFileResult.value());
  writeTemporaryFile(atomicFile, "new contents");

  const bool supportsPosixRename = UnitTest::VolumeSupportsPosixRename(directory);
  {
    const HANDLE held = CreateFileW(outputPath.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    REQUIRE(held != INVALID_HANDLE_VALUE);
    const auto closeHandle = MakeScopeGuard([held]() noexcept { CloseHandle(held); });
    const auto commitResult = atomicFile.commit();
    if(supportsPosixRename)
    {
      SIMPLNX_RESULT_REQUIRE_VALID(commitResult);
      CHECK((readTextFile(outputPath) == "new contents"));
    }
    else
    {
      SIMPLNX_RESULT_REQUIRE_INVALID(commitResult);
      REQUIRE((commitResult.errors().size() == 1));
      CHECK((commitResult.errors().front().code == -15782));
      CHECK((readTextFile(outputPath) == "old contents"));
      CHECK((readTextFile(atomicFile.tempFilePath()) == "new contents"));
    }
  }
  if(!supportsPosixRename)
  {
    const auto retry = atomicFile.commit();
    SIMPLNX_RESULT_REQUIRE_VALID(retry);
    CHECK((readTextFile(outputPath) == "new contents"));
    CHECK_FALSE(fs::exists(atomicFile.tempFilePath()));
  }
}
#endif

TEST_CASE("AtomicFile::AbandonedTransactionRemovesTemporaryOutput")
{
  const fs::path outputPath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "atomic_file_abandoned.txt";
  fs::remove(outputPath);
  fs::path temporaryDirectory;
  {
    auto atomicFileResult = AtomicFile::Create(outputPath);
    REQUIRE(atomicFileResult.valid());
    const AtomicFile atomicFile = std::move(atomicFileResult.value());
    temporaryDirectory = atomicFile.tempFilePath().parent_path();
    writeTemporaryFile(atomicFile, "uncommitted contents");
    REQUIRE(fs::is_regular_file(atomicFile.tempFilePath()));
  }

  REQUIRE_FALSE(fs::exists(temporaryDirectory));
  REQUIRE_FALSE(fs::exists(outputPath));
}

TEST_CASE("AtomicFile::MoveAssignmentTransfersTemporaryOutput")
{
  const fs::path firstOutputPath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "atomic_file_move_first.txt";
  const fs::path secondOutputPath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "atomic_file_move_second.txt";
  fs::remove(firstOutputPath);
  fs::remove(secondOutputPath);

  auto firstResult = AtomicFile::Create(firstOutputPath);
  auto secondResult = AtomicFile::Create(secondOutputPath);
  REQUIRE(firstResult.valid());
  REQUIRE(secondResult.valid());
  AtomicFile firstAtomicFile = std::move(firstResult.value());
  AtomicFile secondAtomicFile = std::move(secondResult.value());
  const fs::path firstTemporaryDirectory = firstAtomicFile.tempFilePath().parent_path();
  const fs::path secondTemporaryDirectory = secondAtomicFile.tempFilePath().parent_path();
  writeTemporaryFile(firstAtomicFile, "discarded contents");
  writeTemporaryFile(secondAtomicFile, "transferred contents");

  firstAtomicFile = std::move(secondAtomicFile);
  REQUIRE_FALSE(fs::exists(firstTemporaryDirectory));
  const Result<> commitResult = firstAtomicFile.commit();
  REQUIRE(commitResult.valid());
  REQUIRE(readTextFile(secondOutputPath) == "transferred contents");
  REQUIRE_FALSE(fs::exists(firstOutputPath));

  firstAtomicFile.removeTempFile();
  REQUIRE_FALSE(fs::exists(secondTemporaryDirectory));
  fs::remove(secondOutputPath);
}
