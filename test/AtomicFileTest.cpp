#include <catch2/catch.hpp>

#include "simplnx/Common/AtomicFile.hpp"
#include "simplnx/Common/PathText.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/ScopeGuard.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <system_error>

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
