#include "simplnx/Common/FileReplacement.hpp"
#include "simplnx/Common/FileReplacementDetails.hpp"
#include "simplnx/Common/ScopeGuard.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Common/Uuid.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/unit_test/simplnx_test_dirs.hpp"

#include <catch2/catch.hpp>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iterator>
#include <string>
#include <system_error>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#endif

using namespace nx::core;

namespace fs = std::filesystem;

namespace
{
struct ScriptedReplacement
{
  std::vector<std::error_code> Results;
  std::vector<std::chrono::milliseconds::rep> WaitsMs;
  usize Calls = 0;
};

void writeFile(const fs::path& path, const std::string& contents)
{
  std::ofstream output(path, std::ios::binary);
  REQUIRE(output.is_open());
  output << contents;
  output.close();
  REQUIRE_FALSE(output.fail());
}

std::string readFile(const fs::path& path)
{
  std::ifstream input(path, std::ios::binary);
  REQUIRE(input.is_open());
  return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}
} // namespace

TEST_CASE("FileReplacement falls back once and prefers a retryable primary error", "[simplnx]")
{
  const auto retryablePrimaryError = std::make_error_code(std::errc::device_or_resource_busy);
  const auto permanentPrimaryError = std::make_error_code(std::errc::invalid_argument);
  const auto retryableFallbackError = std::make_error_code(std::errc::permission_denied);
  std::error_code primaryResult;
  std::error_code fallbackResult;
  usize expectedFallbackCalls = 1;
  usize expectedPredicateCalls = 0;
  std::error_code expectedResult;

  SECTION("The primary rename succeeds")
  {
    expectedFallbackCalls = 0;
  }
  SECTION("The fallback succeeds after the primary rename fails")
  {
    primaryResult = retryablePrimaryError;
  }
  SECTION("Both renames fail and the primary error permits a retry")
  {
    primaryResult = retryablePrimaryError;
    fallbackResult = retryableFallbackError;
    expectedPredicateCalls = 1;
    expectedResult = retryablePrimaryError;
  }
  SECTION("Both renames fail and the primary error is permanent")
  {
    primaryResult = permanentPrimaryError;
    fallbackResult = retryableFallbackError;
    expectedPredicateCalls = 1;
    expectedResult = retryableFallbackError;
  }

  usize primaryCalls = 0;
  usize fallbackCalls = 0;
  usize predicateCalls = 0;
  const auto result = detail::RenameWithFallback(
      [&] {
        CHECK((fallbackCalls == 0));
        ++primaryCalls;
        return primaryResult;
      },
      [&] {
        CHECK((primaryCalls == 1));
        ++fallbackCalls;
        return fallbackResult;
      },
      [&](const std::error_code& error) {
        CHECK((fallbackCalls == 1));
        CHECK((error == primaryResult));
        ++predicateCalls;
        return error == retryablePrimaryError || error == retryableFallbackError;
      });
  CHECK((primaryCalls == 1));
  CHECK((fallbackCalls == expectedFallbackCalls));
  CHECK((predicateCalls == expectedPredicateCalls));
  CHECK((result == expectedResult));
}

TEST_CASE("InstallReplacementFile bounds the retries", "[simplnx]")
{
  ScriptedReplacement scripted;
  const detail::FileReplacementSteps steps{.Rename =
                                               [&scripted](const fs::path& replacement, const fs::path& destination) {
                                                 REQUIRE((replacement == "replacement"));
                                                 REQUIRE((destination == "destination"));
                                                 return scripted.Results.at(scripted.Calls++);
                                               },
                                           .IsTransient = [](const std::error_code& error, const fs::path&, const fs::path&) { return error == std::errc::permission_denied; },
                                           .Wait = [&scripted](std::chrono::milliseconds wait) { scripted.WaitsMs.push_back(wait.count()); }};

  const std::error_code transient = std::make_error_code(std::errc::permission_denied);
  const std::error_code permanent = std::make_error_code(std::errc::no_such_file_or_directory);

  SECTION("Success needs no wait")
  {
    scripted.Results = {std::error_code{}};
    const auto outcome = detail::InstallReplacementFile("replacement", "destination", steps);
    REQUIRE_FALSE(outcome.Error);
    CHECK((outcome.Attempts == 1));
    CHECK((outcome.Waited.count() == 0));
    CHECK((scripted.Calls == 1));
    CHECK(scripted.WaitsMs.empty());
  }

  SECTION("Transient errors clear after three waits")
  {
    scripted.Results = {transient, transient, transient, std::error_code{}};
    const auto outcome = detail::InstallReplacementFile("replacement", "destination", steps);
    REQUIRE_FALSE(outcome.Error);
    CHECK((outcome.Attempts == 4));
    CHECK((outcome.Waited.count() == 7));
    CHECK((scripted.Calls == 4));
    const std::vector<std::chrono::milliseconds::rep> expectedWaitsMs{1, 2, 4};
    CHECK((scripted.WaitsMs == expectedWaitsMs));
  }

  SECTION("Permanent error ends the operation")
  {
    scripted.Results = {permanent};
    const auto outcome = detail::InstallReplacementFile("replacement", "destination", steps);
    REQUIRE((outcome.Error == permanent));
    CHECK((outcome.Attempts == 1));
    CHECK((outcome.Waited.count() == 0));
    CHECK((scripted.Calls == 1));
    CHECK(scripted.WaitsMs.empty());
  }

  SECTION("A transient error persists after the last wait")
  {
    scripted.Results.assign(38, transient);
    const auto outcome = detail::InstallReplacementFile("replacement", "destination", steps);
    REQUIRE((outcome.Error == transient));
    CHECK(outcome.RetriesExhausted);
    CHECK((outcome.Attempts == 38));
    CHECK((outcome.Waited.count() == 2000));
    CHECK((scripted.Calls == 38));
    // The first six waits use 63 ms. Thirty 64 ms waits leave 17 ms.
    std::vector<std::chrono::milliseconds::rep> expectedWaitsMs{1, 2, 4, 8, 16, 32};
    expectedWaitsMs.insert(expectedWaitsMs.end(), 30, 64);
    expectedWaitsMs.push_back(17);
    CHECK((scripted.WaitsMs == expectedWaitsMs));
  }
}

TEST_CASE("FileReplacement identifies known preservation and uncertain rename errors", "[simplnx]")
{
  FileReplacementOutcome outcome{.Error = std::make_error_code(std::errc::io_error), .Attempts = 1};
  bool unchanged = true;
  SECTION("No rename attempted")
  {
    CHECK_FALSE(outcome.RenameAttempted);
  }
  SECTION("Retries exhausted with refusals")
  {
    outcome.Error = std::make_error_code(std::errc::permission_denied);
    outcome.RenameAttempted = true;
    outcome.RetriesExhausted = true;
  }
  SECTION("Definite refusal")
  {
    const auto refusal = GENERATE(std::errc::permission_denied, std::errc::operation_not_permitted, std::errc::read_only_file_system, std::errc::is_a_directory, std::errc::not_a_directory,
                                  std::errc::directory_not_empty, std::errc::file_exists, std::errc::no_such_file_or_directory, std::errc::cross_device_link, std::errc::filename_too_long,
                                  std::errc::device_or_resource_busy, std::errc::text_file_busy);
    outcome.Error = std::make_error_code(refusal);
    outcome.RenameAttempted = true;
  }
#ifdef _WIN32
  SECTION("Native Windows refusal")
  {
    const auto refusal = GENERATE(ERROR_ACCESS_DENIED, ERROR_SHARING_VIOLATION, ERROR_LOCK_VIOLATION);
    outcome.Error = {static_cast<int>(refusal), std::system_category()};
    outcome.RenameAttempted = true;
  }
#endif
  SECTION("Uncertain rename error")
  {
    const auto uncertain = GENERATE(std::errc::io_error, std::errc::timed_out, std::errc::no_lock_available);
    outcome.Error = std::make_error_code(uncertain);
    outcome.RenameAttempted = true;
    unchanged = false;
  }
  CHECK((FileReplacementLeftDestinationUnchanged(outcome) == unchanged));
  const auto message = DescribeFileReplacementFailure(outcome, "example.txt");
  if(unchanged)
  {
    CHECK_THAT(message, Catch::Matchers::StartsWith("Could not replace output file 'example.txt'. The destination is unchanged."));
  }
  else
  {
    CHECK((message == "Could not confirm that output file 'example.txt' was replaced. Check the file before you run again. " + DescribeFileReplacementSystemError(outcome)));
  }
}

TEST_CASE("FileReplacement describes what failed and how to recover", "[simplnx]")
{
  const auto error = std::make_error_code(std::errc::permission_denied);
  const fs::path destination = "example.txt";
  std::string description;
  SECTION("One attempt")
  {
    description = DescribeFileReplacementFailure({.Error = error, .Attempts = 1}, destination);
    CHECK_THAT(description, Catch::Matchers::Contains("The file or the output folder may be read-only, or you may not have permission to replace the file."));
    CHECK_THAT(description, !Catch::Matchers::Contains("open in another program"));
    CHECK_THAT(description, Catch::Matchers::EndsWith(error.message()));
    CHECK_THAT(description, !Catch::Matchers::Contains("(tried "));
  }
  SECTION("Exhausted attempts")
  {
    description = DescribeFileReplacementFailure({.Error = error, .Attempts = 38, .Waited = std::chrono::milliseconds{2000}, .RetriesExhausted = true}, destination);
    CHECK_THAT(description, !Catch::Matchers::Contains("read-only"));
    CHECK_THAT(description,
               Catch::Matchers::Contains("It may be open in another program, or you may not have permission to replace it. Close it and run again. Operating system error: " + error.message()));
    CHECK_THAT(description, Catch::Matchers::EndsWith("(tried 38 times over 2000 ms)."));
  }
  CHECK_THAT(description, Catch::Matchers::StartsWith("Could not replace output file 'example.txt'. The destination is unchanged."));
  CHECK((description.find("example.txt") == description.rfind("example.txt")));
}

TEST_CASE("InstallReplacementFile replaces real files", "[simplnx]")
{
  const fs::path directory = fs::path(unit_test::k_BinaryTestOutputDir.view()) / ("file_replacement_" + Uuid::GenerateV4().str());
  REQUIRE(fs::create_directory(directory));
  const auto cleanup = MakeScopeGuard([&directory]() noexcept {
    std::error_code error;
    fs::remove_all(directory, error);
  });
  const fs::path destination = directory / u8"destination-\u03a9.txt";
  const fs::path replacement = directory / "replacement.txt";
  writeFile(destination, "old");

  SECTION("A complete replacement becomes the destination")
  {
    writeFile(replacement, "new");
    const auto outcome = InstallReplacementFile(replacement, destination);
    REQUIRE_FALSE(outcome.Error);
    CHECK((readFile(destination) == "new"));
    CHECK_FALSE(fs::exists(replacement));
  }

  SECTION("A missing replacement fails immediately")
  {
    const auto outcome = InstallReplacementFile(replacement, destination);
    REQUIRE(outcome.Error);
    CHECK((outcome.Attempts == 1));
    CHECK((outcome.Waited.count() == 0));
    CHECK((readFile(destination) == "old"));
  }

  SECTION("A flush failure prevents both renames and preserves both files")
  {
    writeFile(replacement, "new");
    auto steps = detail::PlatformFileReplacementSteps(FileReplacementDurability::WriteThrough);
    const auto flushError = std::make_error_code(std::errc::io_error);
    steps.Flush = [&](const fs::path& path) {
      CHECK((path == replacement));
      return flushError;
    };
    steps.Rename = [](const fs::path&, const fs::path&) {
      FAIL("No rename may run after a failed flush");
      return std::error_code{};
    };
    steps.IsTransient = [](const std::error_code&, const fs::path&, const fs::path&) { return false; };
    steps.Wait = [](std::chrono::milliseconds) { FAIL("A flush error must not be retried"); };
    const auto outcome = detail::InstallReplacementFile(replacement, destination, steps);
    CHECK((outcome.Error == flushError));
    CHECK_FALSE(outcome.RenameAttempted);
    CHECK((outcome.Attempts == 1));
    CHECK_FALSE(outcome.RetriesExhausted);
    CHECK((readFile(destination) == "old"));
    CHECK((readFile(replacement) == "new"));
  }

  SECTION("Transient flush failures share the rename wait budget")
  {
    writeFile(replacement, "new");
    const bool exhausted = GENERATE(false, true);
    auto steps = detail::PlatformFileReplacementSteps();
    const auto rename = steps.Rename;
    const auto transient = std::make_error_code(std::errc::device_or_resource_busy);
    usize flushCalls = 0;
    usize successfulFlushes = 0;
    usize renameCalls = 0;
    std::chrono::milliseconds waited{0};
    steps.Flush = [&](const fs::path& path) {
      CHECK((path == replacement));
      CHECK((successfulFlushes == 0));
      ++flushCalls;
      if(exhausted || flushCalls == 1)
      {
        return transient;
      }
      ++successfulFlushes;
      return std::error_code{};
    };
    steps.Rename = [&](const fs::path& from, const fs::path& to) {
      REQUIRE((successfulFlushes == 1));
      ++renameCalls;
      // Also exercise a rename refusal after the flush used part of the budget.
      return renameCalls == 1 ? transient : rename(from, to);
    };
    steps.IsTransient = [transient](const std::error_code& error, const fs::path&, const fs::path&) { return error == transient; };
    steps.Wait = [&](std::chrono::milliseconds wait) { waited += wait; };
    const auto outcome = detail::InstallReplacementFile(replacement, destination, steps);
    CHECK((outcome.Waited == waited));
    CHECK((outcome.RetriesExhausted == exhausted));
    CHECK((outcome.RenameAttempted == !exhausted));
    if(exhausted)
    {
      CHECK((outcome.Error == transient));
      CHECK((outcome.Waited == detail::k_FileReplacementTotalWaitLimitMs));
      CHECK((outcome.Attempts == flushCalls));
      CHECK((renameCalls == 0));
      CHECK((successfulFlushes == 0));
      CHECK((readFile(destination) == "old"));
      CHECK((readFile(replacement) == "new"));
      CHECK_THAT(DescribeFileReplacementSystemError(outcome), Catch::Matchers::Contains("tried " + std::to_string(flushCalls) + " times over " + std::to_string(waited.count()) + " ms"));
    }
    else
    {
      REQUIRE_FALSE(outcome.Error);
      CHECK((flushCalls == 2));
      CHECK((successfulFlushes == 1));
      CHECK((renameCalls == 2));
      CHECK((outcome.Attempts == 3));
      CHECK((outcome.Waited == std::chrono::milliseconds{3}));
      CHECK((readFile(destination) == "new"));
      CHECK_FALSE(fs::exists(replacement));
    }
  }

  SECTION("A transient flush failure clears before the first rename")
  {
    writeFile(replacement, "new");
    auto steps = detail::PlatformFileReplacementSteps();
    const auto rename = steps.Rename;
    usize flushCalls = 0;
    usize renameCalls = 0;
    steps.Flush = [&](const fs::path&) { return ++flushCalls == 1 ? std::make_error_code(std::errc::device_or_resource_busy) : std::error_code{}; };
    steps.Rename = [&](const fs::path& from, const fs::path& to) {
      REQUIRE((flushCalls == 2));
      ++renameCalls;
      return rename(from, to);
    };
    steps.IsTransient = [](const std::error_code& error, const fs::path&, const fs::path&) { return error == std::errc::device_or_resource_busy; };
    steps.Wait = [](std::chrono::milliseconds) {};
    const auto outcome = detail::InstallReplacementFile(replacement, destination, steps);
    REQUIRE_FALSE(outcome.Error);
    CHECK(outcome.RenameAttempted);
    CHECK((flushCalls == 2));
    CHECK((renameCalls == 1));
    CHECK((readFile(destination) == "new"));
    CHECK_FALSE(fs::exists(replacement));
  }

  SECTION("A successful flush precedes every rename attempt")
  {
    writeFile(replacement, "new");
    auto steps = detail::PlatformFileReplacementSteps();
    const auto rename = steps.Rename;
    bool flushed = false;
    bool refused = false;
    steps.Flush = [&](const fs::path&) {
      CHECK_FALSE(flushed);
      flushed = true;
      return std::error_code{};
    };
    steps.Rename = [&](const fs::path& from, const fs::path& to) {
      REQUIRE(flushed);
      if(!refused)
      {
        refused = true;
        return std::make_error_code(std::errc::device_or_resource_busy);
      }
      return rename(from, to);
    };
    steps.IsTransient = [](const std::error_code& error, const fs::path&, const fs::path&) { return error == std::errc::device_or_resource_busy; };
    steps.Wait = [](std::chrono::milliseconds) {};
    const auto outcome = detail::InstallReplacementFile(replacement, destination, steps);
    REQUIRE_FALSE(outcome.Error);
    CHECK((readFile(destination) == "new"));
    CHECK_FALSE(fs::exists(replacement));
  }

  SECTION("An absent destination receives the new file")
  {
    REQUIRE(fs::remove(destination));
    writeFile(replacement, "new");
    const auto outcome = InstallReplacementFile(replacement, destination);
    REQUIRE_FALSE(outcome.Error);
    CHECK((readFile(destination) == "new"));
    CHECK_FALSE(fs::exists(replacement));
  }

  SECTION("A relative destination resolves from the working directory")
  {
    writeFile(replacement, "new");
    const auto relativeDestination = fs::relative(destination);
    REQUIRE(relativeDestination.is_relative());
    const auto outcome = InstallReplacementFile(replacement, relativeDestination);
    REQUIRE_FALSE(outcome.Error);
    CHECK((readFile(destination) == "new"));
    CHECK_FALSE(fs::exists(replacement));
  }

#ifdef _WIN32
  SECTION("A full-sharing indexer handle follows the volume rename capability")
  {
    writeFile(replacement, "new");
    const bool supportsPosixRename = UnitTest::VolumeSupportsPosixRename(directory);
    {
      const HANDLE held = CreateFileW(destination.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
      REQUIRE((held != INVALID_HANDLE_VALUE));
      const auto closeHandle = MakeScopeGuard([held]() noexcept { CloseHandle(held); });
      const auto outcome = InstallReplacementFile(replacement, destination);
      if(supportsPosixRename)
      {
        REQUIRE_FALSE(outcome.Error);
        CHECK((readFile(destination) == "new"));
        CHECK_FALSE(fs::exists(replacement));
      }
      else
      {
        REQUIRE(outcome.Error);
        CHECK((readFile(destination) == "old"));
        CHECK((readFile(replacement) == "new"));
      }
    }
    if(!supportsPosixRename)
    {
      REQUIRE_FALSE(InstallReplacementFile(replacement, destination).Error);
      CHECK((readFile(destination) == "new"));
      CHECK_FALSE(fs::exists(replacement));
    }
  }

  SECTION("A forced conventional fallback preserves held files and succeeds after release")
  {
    writeFile(replacement, "new");
    auto steps = detail::PlatformFileReplacementSteps();
    steps.Rename = [](const fs::path& from, const fs::path& to) {
      return detail::RenameWithFallback(
          [] { return std::error_code{ERROR_NOT_SUPPORTED, std::system_category()}; },
          [&] { return MoveFileExW(from.c_str(), to.c_str(), MOVEFILE_REPLACE_EXISTING) == 0 ? std::error_code{static_cast<int>(GetLastError()), std::system_category()} : std::error_code{}; },
          [](const std::error_code&) { return false; });
    };
    steps.Wait = [](std::chrono::milliseconds) {};
    {
      const HANDLE held = CreateFileW(destination.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
      REQUIRE((held != INVALID_HANDLE_VALUE));
      const auto closeHandle = MakeScopeGuard([held]() noexcept { CloseHandle(held); });
      const auto outcome = detail::InstallReplacementFile(replacement, destination, steps);
      REQUIRE(outcome.Error);
      CHECK(outcome.RetriesExhausted);
      CHECK((readFile(destination) == "old"));
      CHECK((readFile(replacement) == "new"));
    }
    REQUIRE_FALSE(detail::InstallReplacementFile(replacement, destination, steps).Error);
    CHECK((readFile(destination) == "new"));
    CHECK_FALSE(fs::exists(replacement));
  }

  SECTION("A handle without delete sharing is released during a wait")
  {
    writeFile(replacement, "new");
    HANDLE held = CreateFileW(destination.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    REQUIRE((held != INVALID_HANDLE_VALUE));
    const auto closeHandle = MakeScopeGuard([&held]() noexcept {
      if(held != INVALID_HANDLE_VALUE)
      {
        CloseHandle(held);
      }
    });
    auto steps = detail::PlatformFileReplacementSteps();
    steps.Wait = [&held](std::chrono::milliseconds) {
      if(held != INVALID_HANDLE_VALUE)
      {
        REQUIRE((CloseHandle(held) != 0));
        held = INVALID_HANDLE_VALUE;
      }
    };

    const auto outcome = detail::InstallReplacementFile(replacement, destination, steps);
    REQUIRE_FALSE(outcome.Error);
    CHECK((outcome.Attempts >= 2));
    CHECK((readFile(destination) == "new"));
    CHECK_FALSE(fs::exists(replacement));
  }

  SECTION("A persistent handle without delete sharing leaves both files intact")
  {
    writeFile(replacement, "new");
    const HANDLE held = CreateFileW(destination.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    REQUIRE((held != INVALID_HANDLE_VALUE));
    const auto closeHandle = MakeScopeGuard([held]() noexcept { CloseHandle(held); });
    auto steps = detail::PlatformFileReplacementSteps();
    steps.Wait = [](std::chrono::milliseconds) {};

    const auto outcome = detail::InstallReplacementFile(replacement, destination, steps);
    CHECK((outcome.Error.category() == std::system_category()));
    CHECK((outcome.Error.value() == ERROR_SHARING_VIOLATION || outcome.Error.value() == ERROR_ACCESS_DENIED));
    CHECK((outcome.Waited == detail::k_FileReplacementTotalWaitLimitMs));
    CHECK(outcome.RetriesExhausted);
    CHECK((readFile(destination) == "old"));
    CHECK((readFile(replacement) == "new"));
  }

  SECTION("A read-only destination fails after one attempt")
  {
    writeFile(replacement, "new");
    const DWORD originalAttributes = GetFileAttributesW(destination.c_str());
    REQUIRE((originalAttributes != INVALID_FILE_ATTRIBUTES));
    const auto restoreAttributes = MakeScopeGuard([&destination, originalAttributes]() noexcept { SetFileAttributesW(destination.c_str(), originalAttributes); });
    REQUIRE((SetFileAttributesW(destination.c_str(), (originalAttributes & ~static_cast<DWORD>(FILE_ATTRIBUTE_NORMAL)) | FILE_ATTRIBUTE_READONLY) != 0));
    auto steps = detail::PlatformFileReplacementSteps();
    steps.Wait = [](std::chrono::milliseconds) {};

    const auto outcome = detail::InstallReplacementFile(replacement, destination, steps);
    CHECK((outcome.Error.category() == std::system_category()));
    CHECK((outcome.Error.value() == ERROR_ACCESS_DENIED));
    CHECK((outcome.Attempts == 1));
    CHECK_FALSE(outcome.RetriesExhausted);
    CHECK((readFile(destination) == "old"));
    CHECK((readFile(replacement) == "new"));
  }

  SECTION("A replacement handle without delete sharing leaves both files intact")
  {
    writeFile(replacement, "new");
    const HANDLE held = CreateFileW(replacement.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    REQUIRE((held != INVALID_HANDLE_VALUE));
    const auto closeHandle = MakeScopeGuard([held]() noexcept { CloseHandle(held); });
    auto steps = detail::PlatformFileReplacementSteps();
    steps.Wait = [](std::chrono::milliseconds) {};

    const auto outcome = detail::InstallReplacementFile(replacement, destination, steps);
    CHECK((outcome.Error.category() == std::system_category()));
    CHECK((outcome.Error.value() == ERROR_SHARING_VIOLATION));
    CHECK(outcome.RetriesExhausted);
    CHECK((readFile(destination) == "old"));
    CHECK((readFile(replacement) == "new"));
  }

  SECTION("Write-through replacement installs the new file")
  {
    writeFile(replacement, "new");
    const auto outcome = InstallReplacementFile(replacement, destination, FileReplacementDurability::WriteThrough);
    REQUIRE_FALSE(outcome.Error);
    CHECK((readFile(destination) == "new"));
    CHECK_FALSE(fs::exists(replacement));
  }
#endif
}
