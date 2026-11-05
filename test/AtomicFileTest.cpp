#include <catch2/catch.hpp>

#include "simplnx/Common/AtomicFile.hpp"
#include "simplnx/Common/FileReplaceGuard.hpp"
#include "simplnx/Common/PathText.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/ScopeGuard.hpp"
#include "simplnx/UnitTest/RecordingFileReplaceHandler.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <system_error>
#include <vector>

using namespace nx::core;

namespace fs = std::filesystem;

namespace
{
/**
 * @class ScopedAtomicFileTestDirectory
 * @brief Creates an empty directory for one test and removes it with everything in it at scope exit.
 */
class ScopedAtomicFileTestDirectory
{
public:
  explicit ScopedAtomicFileTestDirectory(const std::string& testName)
  : m_Path(fs::temp_directory_path() / "simplnx_atomic_file_tests" / testName)
  {
    fs::remove_all(m_Path);
    fs::create_directories(m_Path);
  }

  ~ScopedAtomicFileTestDirectory() noexcept
  {
    std::error_code ignored;
    fs::remove_all(m_Path, ignored);
  }

  ScopedAtomicFileTestDirectory(const ScopedAtomicFileTestDirectory&) = delete;
  ScopedAtomicFileTestDirectory(ScopedAtomicFileTestDirectory&&) = delete;
  ScopedAtomicFileTestDirectory& operator=(const ScopedAtomicFileTestDirectory&) = delete;
  ScopedAtomicFileTestDirectory& operator=(ScopedAtomicFileTestDirectory&&) = delete;

  const fs::path& path() const
  {
    return m_Path;
  }

private:
  fs::path m_Path;
};

/**
 * @brief Writes text to a file, replacing it.
 * @param filePath The file.
 * @param text The content.
 */
void WriteText(const fs::path& filePath, const std::string& text)
{
  std::ofstream stream(filePath, std::ios::binary | std::ios::trunc);
  REQUIRE(stream.is_open());
  stream << text;
  REQUIRE(stream.good());
}

/**
 * @brief Reads the whole content of a file.
 * @param filePath The file.
 * @return The content.
 */
std::string ReadText(const fs::path& filePath)
{
  std::ifstream stream(filePath, std::ios::binary);
  REQUIRE(stream.is_open());
  return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}

/**
 * @brief Creates an AtomicFile for a target, writes its temporary file, commits it and checks every step.
 * @param target The final file. Its directory may not exist yet.
 *
 * The temporary file must have the target's file name inside a new directory beside the target, so the commit is a rename inside one
 * directory tree. After the commit the target holds the content and the temporary directory is removed when the object is destroyed.
 */
void RequireCreateWriteCommit(const fs::path& target)
{
  const std::string content = "atomic file content";
  fs::path stagingDirectory;
  {
    Result<AtomicFile> created = AtomicFile::Create(target);
    SIMPLNX_RESULT_REQUIRE_VALID(created);
    const fs::path tempFilePath = created.value().tempFilePath();
    stagingDirectory = tempFilePath.parent_path();
    CHECK(tempFilePath.filename() == target.filename());
    CHECK(stagingDirectory.parent_path() == target.parent_path());
    CHECK(stagingDirectory != target.parent_path());
    REQUIRE(fs::is_directory(stagingDirectory));

    WriteText(tempFilePath, content);
    const Result<> committed = created.value().commit();
    SIMPLNX_RESULT_REQUIRE_VALID(committed);
    CHECK_FALSE(fs::exists(tempFilePath));
  }
  REQUIRE(fs::exists(target));
  CHECK(ReadText(target) == content);
  CHECK_FALSE(fs::exists(stagingDirectory));
}
} // namespace

// Windows converts a narrow path through the active code page, which cannot represent every character. The staging path must therefore
// be built from path objects, never from narrow strings; these names exercise that on every platform.
TEST_CASE("AtomicFile: a non-ASCII file name stages beside its target and commits", "[simplnx][AtomicFile]")
{
  const ScopedAtomicFileTestDirectory directory("non_ascii_file_name");
  RequireCreateWriteCommit(directory.path() / fs::path(u8"r\u00e9sultats \U0001F600 \u4e2d\u6587.dream3d"));
}

TEST_CASE("AtomicFile: a non-ASCII directory that does not exist yet is created and receives the file", "[simplnx][AtomicFile]")
{
  const ScopedAtomicFileTestDirectory directory("non_ascii_directory");
  RequireCreateWriteCommit(directory.path() / fs::path(u8"donn\u00e9es \U0001F600") / fs::path(u8"\u00e9chantillon.dream3d"));
}

TEST_CASE("AtomicFile: a relative non-ASCII path is made absolute and commits", "[simplnx][AtomicFile]")
{
  const ScopedAtomicFileTestDirectory directory("non_ascii_relative");
  const fs::path previousWorkingDirectory = fs::current_path();
  fs::current_path(directory.path());
  const auto restoreWorkingDirectory = MakeScopeGuard([&previousWorkingDirectory]() noexcept {
    std::error_code ignored;
    fs::current_path(previousWorkingDirectory, ignored);
  });

  const fs::path relativeTarget = fs::path(u8"\u00e9chantillon \U0001F600.dream3d");
  Result<AtomicFile> created = AtomicFile::Create(relativeTarget);
  SIMPLNX_RESULT_REQUIRE_VALID(created);
  const fs::path tempFilePath = created.value().tempFilePath();
  CHECK(tempFilePath.is_absolute());
  CHECK(tempFilePath.filename() == relativeTarget.filename());
  WriteText(tempFilePath, "relative");
  const Result<> committed = created.value().commit();
  SIMPLNX_RESULT_REQUIRE_VALID(committed);
  CHECK(ReadText(directory.path() / relativeTarget) == "relative");
}

TEST_CASE("AtomicFile: a commit error names a non-ASCII temporary file", "[simplnx][AtomicFile]")
{
  const ScopedAtomicFileTestDirectory directory("non_ascii_commit_error");
  Result<AtomicFile> created = AtomicFile::Create(directory.path() / fs::path(u8"\u00e9chec \U0001F600.dream3d"));
  SIMPLNX_RESULT_REQUIRE_VALID(created);
  // No temporary file was written, so the commit fails and must report it without throwing.
  const Result<> committed = created.value().commit();
  REQUIRE(committed.invalid());
  REQUIRE(committed.errors().size() == 1);
  CHECK(committed.errors().front().code == -15781);
  CHECK(committed.errors().front().message.find(PathToUtf8Text(created.value().tempFilePath())) != std::string::npos);
}

namespace
{
/**
 * @brief Lists the names of the entries of a directory.
 * @param directory Directory to list.
 * @return Entry names in directory order.
 */
std::vector<fs::path> ListEntries(const fs::path& directory)
{
  std::vector<fs::path> entries;
  for(const fs::directory_entry& entry : fs::directory_iterator(directory))
  {
    entries.push_back(entry.path().filename());
  }
  return entries;
}
} // namespace

TEST_CASE("AtomicFile: Create runs the file-replace hook once on the absolute final path before the temporary directory exists", "[simplnx][AtomicFile][FileWriteHook]")
{
  const ScopedAtomicFileTestDirectory directory("hook_before_staging");
  const fs::path target = directory.path() / "hooked.dream3d";
  WriteText(target, "previous");
  UnitTest::RecordingFileReplaceHandler& recorder = UnitTest::ProcessRecorder();
  const UnitTest::RecorderDisarmGuard disarmGuard({&recorder});
  constexpr int32 k_RecorderWarningCode = 99011;

  // A relative final path reaches the hook in its absolute form.
  const fs::path previousWorkingDirectory = fs::current_path();
  fs::current_path(directory.path());
  const auto restoreWorkingDirectory = MakeScopeGuard([&previousWorkingDirectory]() noexcept {
    std::error_code ignored;
    fs::current_path(previousWorkingDirectory, ignored);
  });

  // The working directory can be a resolved form of the test directory (for example /private/var for /var on macOS).
  const fs::path absoluteTarget = fs::current_path() / target.filename();
  std::vector<fs::path> calledTargets;
  std::vector<fs::path> entriesAtCall;
  recorder.arm(
      absoluteTarget,
      [&](const fs::path& callTarget, FileWriteHookMode mode) {
        calledTargets.push_back(callTarget);
        entriesAtCall = ListEntries(directory.path());
        CHECK(mode == FileWriteHookMode::ReleaseReaders);
      },
      MakeWarningVoidResult(k_RecorderWarningCode, "recorder warning"), false);

  Result<AtomicFile> created = AtomicFile::Create(target.filename());
  SIMPLNX_RESULT_REQUIRE_VALID(created);
  REQUIRE(calledTargets.size() == 1);
  CHECK(calledTargets.front() == absoluteTarget);
  CHECK(calledTargets.front().is_absolute());
  // The hook ran before the temporary directory existed: only the final file was in the directory.
  CHECK(entriesAtCall == std::vector<fs::path>{target.filename()});
  REQUIRE(created.warnings().size() == 1);
  CHECK(created.warnings().front().code == k_RecorderWarningCode);
  CHECK(ReadText(target) == "previous");
}

TEST_CASE("AtomicFile: a refused Create returns the hook result and creates nothing", "[simplnx][AtomicFile][FileWriteHook]")
{
  const ScopedAtomicFileTestDirectory directory("hook_refusal");
  const fs::path target = directory.path() / "refused.dream3d";
  WriteText(target, "previous");
  UnitTest::RecordingFileReplaceHandler& recorder = UnitTest::ProcessRecorder();
  const UnitTest::RecorderDisarmGuard disarmGuard({&recorder});
  constexpr int32 k_RecorderWarningCode = 99012;
  constexpr int32 k_ArbitraryErrorCode = -99013;

  Result<> errorWithWarning = MakeErrorResult(k_ArbitraryErrorCode, "recorder error");
  errorWithWarning.warnings().push_back(Warning{k_RecorderWarningCode, "recorder warning"});
  const Result<> invalidWithoutErrors = Result<>{{nonstd::make_unexpected(ErrorCollection{})}};
  for(const Result<>& refusal : {errorWithWarning, invalidWithoutErrors})
  {
    int32 callCount = 0;
    recorder.arm(target, [&callCount](const fs::path&, FileWriteHookMode) { callCount++; }, refusal, false);
    Result<AtomicFile> created = AtomicFile::Create(target);
    SIMPLNX_RESULT_REQUIRE_INVALID(created);
    CHECK(callCount == 1);
    REQUIRE(created.errors().size() == refusal.errors().size());
    if(!refusal.errors().empty())
    {
      CHECK(created.errors().front().code == k_ArbitraryErrorCode);
    }
    REQUIRE(created.warnings().size() == refusal.warnings().size());
    // No temporary directory exists and the final file is unchanged.
    CHECK(ListEntries(directory.path()) == std::vector<fs::path>{target.filename()});
    CHECK(ReadText(target) == "previous");
  }
}

TEST_CASE("AtomicFile: CreateAfterHook does not run the file-replace hook", "[simplnx][AtomicFile][FileWriteHook]")
{
  const ScopedAtomicFileTestDirectory directory("create_after_hook");
  const fs::path target = directory.path() / "after-hook.dream3d";
  UnitTest::RecordingFileReplaceHandler& recorder = UnitTest::ProcessRecorder();
  const UnitTest::RecorderDisarmGuard disarmGuard({&recorder});
  // An armed refusal would make Create() fail; CreateAfterHook() must neither call it nor fail.
  int32 callCount = 0;
  recorder.arm(target, [&callCount](const fs::path&, FileWriteHookMode) { callCount++; }, MakeErrorResult(-99014, "recorder error"), false);
  Result<AtomicFile> created = AtomicFile::CreateAfterHook(target);
  SIMPLNX_RESULT_REQUIRE_VALID(created);
  CHECK(callCount == 0);
  WriteText(created.value().tempFilePath(), "after hook");
  const Result<> committed = created.value().commit();
  SIMPLNX_RESULT_REQUIRE_VALID(committed);
  CHECK(ReadText(target) == "after hook");
}
