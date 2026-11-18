#include "simplnx/Common/FileReplacement.hpp"
#include "simplnx/Common/ScopeGuard.hpp"
#include "simplnx/Common/Uuid.hpp"
#include "simplnx/unit_test/simplnx_test_dirs.hpp"

#include <catch2/catch.hpp>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#endif

using namespace nx::core;
using namespace std::chrono_literals;

namespace fs = std::filesystem;

namespace
{
struct ScriptedReplacement
{
  std::vector<FileReplacementAttempt> results;
  std::vector<std::chrono::milliseconds> waits;
  usize calls = 0;

  FileReplacementSteps steps()
  {
    return {.rename =
                [this](const fs::path& replacement, const fs::path& destination) {
                  REQUIRE(replacement == "replacement");
                  REQUIRE(destination == "destination");
                  return results.at(calls++);
                },
            .isTransient = [](const std::error_code& error) { return error == std::errc::permission_denied; },
            .wait = [this](std::chrono::milliseconds wait) { waits.push_back(wait); }};
  }
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

TEST_CASE("InstallReplacementFile bounds the retries", "[simplnx]")
{
  ScriptedReplacement scripted;
  const std::error_code transient = std::make_error_code(std::errc::permission_denied);
  const std::error_code permanent = std::make_error_code(std::errc::no_such_file_or_directory);

  SECTION("Success needs no wait")
  {
    scripted.results = {{.error = {}, .renamed = true}};
    const auto outcome = InstallReplacementFile("replacement", "destination", scripted.steps());
    REQUIRE_FALSE(outcome.error);
    CHECK(outcome.renamed);
    CHECK(outcome.attempts == 1);
    CHECK(outcome.waited.count() == 0);
    CHECK(scripted.calls == 1);
    CHECK(scripted.waits.empty());
  }

  SECTION("Transient errors clear after three waits")
  {
    scripted.results = {{.error = transient}, {.error = transient}, {.error = transient}, {.error = {}, .renamed = true}};
    const auto outcome = InstallReplacementFile("replacement", "destination", scripted.steps());
    REQUIRE_FALSE(outcome.error);
    CHECK(outcome.renamed);
    CHECK(outcome.attempts == 4);
    CHECK(outcome.waited.count() == 7);
    CHECK(scripted.calls == 4);
    const std::vector expectedWaits{1ms, 2ms, 4ms};
    CHECK(scripted.waits == expectedWaits);
  }

  SECTION("Permanent error ends the operation")
  {
    scripted.results = {{.error = permanent}};
    const auto outcome = InstallReplacementFile("replacement", "destination", scripted.steps());
    REQUIRE(outcome.error == permanent);
    CHECK_FALSE(outcome.renamed);
    CHECK(outcome.attempts == 1);
    CHECK(outcome.waited.count() == 0);
    CHECK(scripted.calls == 1);
    CHECK(scripted.waits.empty());
  }

  SECTION("A transient error persists after the last wait")
  {
    scripted.results.assign(38, {.error = transient});
    const auto outcome = InstallReplacementFile("replacement", "destination", scripted.steps());
    REQUIRE(outcome.error == transient);
    CHECK_FALSE(outcome.renamed);
    CHECK(outcome.retriesExhausted);
    CHECK(outcome.attempts == 38);
    CHECK(outcome.waited.count() == 2000);
    CHECK(scripted.calls == 38);
    // The first six waits use 63 ms. Thirty 64 ms waits leave 17 ms.
    std::vector expectedWaits{1ms, 2ms, 4ms, 8ms, 16ms, 32ms};
    expectedWaits.insert(expectedWaits.end(), 30, 64ms);
    expectedWaits.push_back(17ms);
    CHECK(scripted.waits == expectedWaits);
  }

  SECTION("A failed flush after rename does not retry the rename")
  {
    scripted.results = {{.error = transient, .renamed = true}};
    const auto outcome = InstallReplacementFile("replacement", "destination", scripted.steps());
    REQUIRE(outcome.error == transient);
    CHECK(outcome.renamed);
    CHECK_FALSE(outcome.retriesExhausted);
    CHECK(outcome.attempts == 1);
    CHECK(scripted.calls == 1);
    CHECK(scripted.waits.empty());
    CHECK_THAT(DescribeFileReplacementFailure(outcome, "destination"), Catch::Matchers::Contains("Could not flush the replacement"));
  }
}

TEST_CASE("FileReplacement describes what failed and how to recover", "[simplnx]")
{
  const auto error = std::make_error_code(std::errc::permission_denied);
  const fs::path destination = "example.txt";
  std::string description;
  SECTION("One attempt")
  {
    description = DescribeFileReplacementFailure({.error = error, .attempts = 1}, destination);
    CHECK_THAT(description, Catch::Matchers::Contains("Check that the files and the output folder are accessible"));
    CHECK_THAT(description, Catch::Matchers::EndsWith(error.message()));
    CHECK_THAT(description, !Catch::Matchers::Contains("attempts"));
  }
  SECTION("Exhausted attempts")
  {
    description = DescribeFileReplacementFailure({.error = error, .attempts = 38, .waited = 2000ms, .retriesExhausted = true}, destination);
    CHECK_THAT(description, Catch::Matchers::Contains("probably open in another program or in this program, or is read-only"));
    CHECK_THAT(description, Catch::Matchers::Contains("Windows Search indexer, a virus scanner, and a backup tool"));
    CHECK_THAT(description, Catch::Matchers::Contains("Make sure the file is closed and writable"));
    CHECK_THAT(description, Catch::Matchers::EndsWith("38 attempts, 2000 ms total requested wait)."));
    const auto osError = description.find(error.message());
    REQUIRE(osError != std::string::npos);
    CHECK(description.find("Make sure") < osError);
  }
  CHECK_THAT(description, Catch::Matchers::StartsWith("Could not replace output file 'example.txt'"));
  CHECK(description.find("example.txt") == description.rfind("example.txt"));
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
    REQUIRE_FALSE(outcome.error);
    CHECK(readFile(destination) == "new");
    CHECK_FALSE(fs::exists(replacement));
  }

  SECTION("A missing replacement fails immediately")
  {
    const auto outcome = InstallReplacementFile(replacement, destination, PlatformFileReplacementSteps());
    REQUIRE(outcome.error);
    CHECK(outcome.attempts == 1);
    CHECK(outcome.waited.count() == 0);
    CHECK(readFile(destination) == "old");
  }

#ifdef _WIN32
  SECTION("A full-sharing indexer handle permits the rename without a retry")
  {
    writeFile(replacement, "new");
    const HANDLE held = CreateFileW(destination.c_str(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    REQUIRE(held != INVALID_HANDLE_VALUE);
    const auto closeHandle = MakeScopeGuard([held]() noexcept { CloseHandle(held); });
    auto steps = PlatformFileReplacementSteps();
    steps.isTransient = [](const std::error_code&) { return false; };
    steps.wait = [](std::chrono::milliseconds) { FAIL("A full-sharing handle must permit replacement without waiting."); };

    const auto outcome = InstallReplacementFile(replacement, destination, steps);
    REQUIRE_FALSE(outcome.error);
    CHECK(readFile(destination) == "new");
    CHECK_FALSE(fs::exists(replacement));
  }

  SECTION("A handle without delete sharing is released during a wait")
  {
    writeFile(replacement, "new");
    HANDLE held = CreateFileW(destination.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    REQUIRE(held != INVALID_HANDLE_VALUE);
    const auto closeHandle = MakeScopeGuard([&held]() noexcept {
      if(held != INVALID_HANDLE_VALUE)
      {
        CloseHandle(held);
      }
    });
    auto steps = PlatformFileReplacementSteps();
    steps.wait = [&held](std::chrono::milliseconds) {
      if(held != INVALID_HANDLE_VALUE)
      {
        REQUIRE(CloseHandle(held) != 0);
        held = INVALID_HANDLE_VALUE;
      }
    };

    const auto outcome = InstallReplacementFile(replacement, destination, steps);
    REQUIRE_FALSE(outcome.error);
    CHECK(outcome.attempts >= 2);
    CHECK(readFile(destination) == "new");
    CHECK_FALSE(fs::exists(replacement));
  }

  SECTION("A persistent handle without delete sharing leaves both files intact")
  {
    writeFile(replacement, "new");
    const HANDLE held = CreateFileW(destination.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    REQUIRE(held != INVALID_HANDLE_VALUE);
    const auto closeHandle = MakeScopeGuard([held]() noexcept { CloseHandle(held); });
    auto steps = PlatformFileReplacementSteps();
    steps.wait = [](std::chrono::milliseconds) {};

    const auto outcome = InstallReplacementFile(replacement, destination, steps);
    CHECK(outcome.error.category() == std::system_category());
    CHECK((outcome.error.value() == ERROR_SHARING_VIOLATION || outcome.error.value() == ERROR_ACCESS_DENIED));
    CHECK((outcome.waited == k_FileReplacementTotalWaitLimitMs));
    CHECK(readFile(destination) == "old");
    CHECK(readFile(replacement) == "new");
  }

  SECTION("Write-through replacement installs the new file")
  {
    writeFile(replacement, "new");
    const auto outcome = InstallReplacementFile(replacement, destination, FileReplacementDurability::WriteThrough);
    REQUIRE_FALSE(outcome.error);
    CHECK(readFile(destination) == "new");
    CHECK_FALSE(fs::exists(replacement));
  }
#endif
}
