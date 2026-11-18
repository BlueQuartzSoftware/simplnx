/**
 * @file AtomicFileTest.cpp
 * @brief Tests transactional file replacement and temporary-path ownership.
 */

#include "simplnx/Common/AtomicFile.hpp"
#include "simplnx/Common/FileReplacement.hpp"
#include "simplnx/Common/Result.hpp"
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
#include "simplnx/Common/ScopeGuard.hpp"
#include "simplnx/Common/Uuid.hpp"

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

TEST_CASE("AtomicFile reports a flush failure after replacing the output", "[simplnx]")
{
  const auto flushError = std::make_error_code(std::errc::io_error);
  const FileReplacementOutcome outcome{.Error = flushError, .Attempts = 1, .Renamed = true};
  const auto result = AtomicFile::commitResult(outcome, "temporary.txt", "output.txt");
  SIMPLNX_RESULT_REQUIRE_INVALID(result); // NOLINT(cppcoreguidelines-avoid-do-while) Project Result assertion macro.
  REQUIRE((result.errors().size() == 1));
  CHECK((result.errors().front().code == -15785));
  CHECK_THAT(result.errors().front().message, Catch::Matchers::Contains("was replaced, but the system could not confirm that the data reached the disk"));
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

  const HANDLE held = CreateFileW(outputPath.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  REQUIRE(held != INVALID_HANDLE_VALUE);
  const auto closeHandle = MakeScopeGuard([held]() noexcept { CloseHandle(held); });

  const auto commitResult = atomicFile.commit();
  SIMPLNX_RESULT_REQUIRE_VALID(commitResult); // NOLINT(cppcoreguidelines-avoid-do-while) Project Result assertion macro.
  CHECK((readTextFile(outputPath) == "new contents"));
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
