#include <catch2/catch.hpp>

#include "simplnx/Common/AtomicFile.hpp"
#include "simplnx/Common/FileReplaceGuard.hpp"
#include "simplnx/Common/PathText.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IO/HDF5/DataStructureWriter.hpp"
#include "simplnx/DataStructure/Montage/GridMontage.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/UnitTest/FailCloseVfd.hpp"
#include "simplnx/UnitTest/RecordingFileReplaceHandler.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"

#include <fmt/core.h>

#include <algorithm>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <map>
#include <memory>
#include <new>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using namespace nx::core;
using namespace nx::core::UnitTest;

namespace fs = std::filesystem;

namespace
{
/**
 * @brief One observed hook call.
 */
struct HookCall
{
  std::string handlerName;
  fs::path target;
  FileWriteHookMode mode = FileWriteHookMode::ReleaseReaders;
};

/**
 * @brief Creates a handler that logs each call and returns a fixed result.
 * @param handlerName Name stored with each logged call, so a test can check the call order.
 * @param calls Log that receives the calls. It must outlive the registration.
 * @param result Result that the handler returns.
 * @return Handler for RegisterFileReplaceHandler().
 */
FileReplaceHandler MakeLoggingHandler(const std::string& handlerName, std::vector<HookCall>& calls, Result<> result = {})
{
  return [handlerName, &calls, result](const fs::path& target, FileWriteHookMode mode) {
    calls.push_back({handlerName, target, mode});
    return result;
  };
}

/**
 * @brief Creates a recorder callback that logs each matching call.
 * @param handlerName Name stored with each logged call.
 * @param calls Log that receives the calls. It must outlive the recorder arming.
 * @return Callback for RecordingFileReplaceHandler::arm().
 */
RecordingFileReplaceHandler::HookCallback MakeRecordingCallback(const std::string& handlerName, std::vector<HookCall>& calls)
{
  return [handlerName, &calls](const fs::path& target, FileWriteHookMode mode) { calls.push_back({handlerName, target, mode}); };
}

/**
 * @brief Returns an invalid result with an empty error collection.
 * @return The result. It is a legal invalid Result that MergeResults() would turn into a success.
 */
Result<> InvalidWithoutErrors()
{
  return Result<>{{nonstd::make_unexpected(ErrorCollection{})}};
}

inline constexpr int32 k_RecorderWarningCode = 99001;
inline constexpr int32 k_RecorderErrorCode = -99002;
inline constexpr int32 k_XdmfDestinationRejectedCode = -6057;
inline constexpr int32 k_FailedCloseCode = -6056;
inline constexpr int32 k_UnwritableTypeCode = -5;
inline constexpr int32 k_XdmfOpenErrorCode = -6062;
inline constexpr int32 k_XdmfWriteErrorCode = -6063;
inline constexpr int32 k_XdmfCloseErrorCode = -6064;

/**
 * @brief Name of the top-level object every writer test writes or appends.
 */
inline constexpr const char* k_WrittenObjectName = "Appended";

/**
 * @brief Name stored in the call log for the process recorder.
 */
inline constexpr const char* k_ProcessRecorderName = "process-recorder";

/**
 * @brief Returns a second recorder that is registered for the whole test executable.
 * @return The one sidecar recorder of this executable.
 *
 * One recorder handles one target. A test that needs a result for the primary file and another one for the sidecar
 * arms the process recorder on the primary file and this recorder on the sidecar.
 */
RecordingFileReplaceHandler& SidecarRecorder()
{
  static const std::shared_ptr<RecordingFileReplaceHandler> s_Recorder = std::make_shared<RecordingFileReplaceHandler>();
  static const std::shared_ptr<void> s_Registration = RegisterRecorder(s_Recorder);
  return *s_Recorder;
}

/**
 * @class ScopedTextStreamFailure
 * @brief Makes the text file writers simulate a stream failure until the end of the scope.
 */
class ScopedTextStreamFailure
{
public:
  explicit ScopedTextStreamFailure(TextStreamFailureForTesting failure)
  {
    SetTextStreamFailureForTesting(failure);
  }

  ~ScopedTextStreamFailure() noexcept
  {
    SetTextStreamFailureForTesting(TextStreamFailureForTesting::None);
  }

  ScopedTextStreamFailure(const ScopedTextStreamFailure&) = delete;
  ScopedTextStreamFailure(ScopedTextStreamFailure&&) = delete;
  ScopedTextStreamFailure& operator=(const ScopedTextStreamFailure&) = delete;
  ScopedTextStreamFailure& operator=(ScopedTextStreamFailure&&) = delete;
};

/**
 * @class ScopedTestDirectory
 * @brief Creates an empty directory for one test and removes it with everything in it at scope exit.
 */
class ScopedTestDirectory
{
public:
  explicit ScopedTestDirectory(const std::string& testName)
  : m_Path(fs::temp_directory_path() / "simplnx_file_write_hook" / testName)
  {
    fs::remove_all(m_Path);
    fs::create_directories(m_Path);
  }

  ~ScopedTestDirectory() noexcept
  {
    std::error_code ignored;
    fs::remove_all(m_Path, ignored);
  }

  ScopedTestDirectory(const ScopedTestDirectory&) = delete;
  ScopedTestDirectory(ScopedTestDirectory&&) = delete;
  ScopedTestDirectory& operator=(const ScopedTestDirectory&) = delete;
  ScopedTestDirectory& operator=(ScopedTestDirectory&&) = delete;

  const fs::path& path() const
  {
    return m_Path;
  }

private:
  fs::path m_Path;
};

/**
 * @brief Builds a structure with one top-level group holding one int32 array.
 * @param topLevelName Name of the top-level group.
 * @param value Value of every element, so two structures write different bytes.
 * @return The structure.
 */
DataStructure MakeWriterTestStructure(const std::string& topLevelName, int32 value)
{
  DataStructure dataStructure;
  auto* group = DataGroup::Create(dataStructure, topLevelName);
  REQUIRE(group != nullptr);
  auto* array = Int32Array::CreateWithStore<Int32DataStore>(dataStructure, "Values", ShapeType{4}, ShapeType{1}, group->getId());
  REQUIRE(array != nullptr);
  array->fill(value);
  return dataStructure;
}

/**
 * @brief Builds a structure whose top-level object is a GridMontage, which no HDF5 IO factory can write.
 * @return The structure; writers fail with -5 after the file is open.
 */
DataStructure MakeUnwritableStructure()
{
  DataStructure dataStructure;
  REQUIRE(GridMontage::Create(dataStructure, k_WrittenObjectName) != nullptr);
  return dataStructure;
}

/**
 * @brief Writes a closed .dream3d file without an armed recorder, so tests have an existing target or append fixture.
 * @param filePath File to create or replace.
 */
void WriteExistingFile(const fs::path& filePath)
{
  const DataStructure fixtureStructure = MakeWriterTestStructure("Existing", 7);
  const Result<> fixtureResult = DREAM3D::WriteFile(filePath, fixtureStructure);
  SIMPLNX_RESULT_REQUIRE_VALID(fixtureResult);
}

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
 * @brief Reads every byte of a file.
 * @param filePath File to read.
 * @return The bytes.
 */
std::vector<char> ReadFileBytes(const fs::path& filePath)
{
  std::ifstream stream(filePath, std::ios::binary);
  REQUIRE(stream.good());
  return std::vector<char>(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
}

/**
 * @brief Returns the UTF-8 form of a path through the standard library, as the expected text of a diagnostic.
 * @param path A path whose name has a UTF-8 form (no unpaired UTF-16 surrogate).
 * @return The path as UTF-8 text.
 *
 * `u8string()` is independent of PathToUtf8Text(), and unlike `string()` it does not depend on the active code page.
 */
std::string Utf8Text(const fs::path& path)
{
  const std::u8string text = path.u8string();
  return std::string(text.begin(), text.end());
}

/**
 * @brief Describes every entry of a directory, so a test can prove that a rejected write changed nothing.
 * @param directory Directory to describe; entries of subdirectories are not listed.
 * @return Entry name mapped to "link:<target>", "directory" or "file:<bytes>".
 */
std::map<std::string, std::string> SnapshotDirectory(const fs::path& directory)
{
  std::map<std::string, std::string> snapshot;
  for(const fs::directory_entry& entry : fs::directory_iterator(directory))
  {
    const fs::file_status status = fs::symlink_status(entry.path());
    std::string description;
    if(fs::is_symlink(status))
    {
      description = "link:" + PathToUtf8Text(fs::read_symlink(entry.path()));
    }
    else if(fs::is_directory(status))
    {
      description = "directory";
    }
    else
    {
      const std::vector<char> bytes = ReadFileBytes(entry.path());
      description = "file:" + std::string(bytes.begin(), bytes.end());
    }
    snapshot.emplace(PathToUtf8Text(entry.path().filename()), std::move(description));
  }
  return snapshot;
}

/**
 * @brief Creates a symbolic link, or reports why the filesystem refused.
 * @param target Path the link points to; it does not need to exist.
 * @param link Link to create.
 * @return True when the link exists; false after a WARN when the filesystem or the account refused (Windows without the
 * symbolic-link privilege).
 */
bool CreateSymlinkOrWarn(const fs::path& target, const fs::path& link)
{
  std::error_code linkError;
  fs::create_symlink(target, link, linkError);
  if(linkError)
  {
    WARN("symbolic-link capability unavailable: " << linkError.message());
    return false;
  }
  return true;
}

/**
 * @brief Creates a hard link, or reports why the filesystem refused.
 * @param filePath Existing file.
 * @param link Link to create.
 * @return True when the link names the same file; false after a WARN when the filesystem refused.
 */
bool CreateHardLinkOrWarn(const fs::path& filePath, const fs::path& link)
{
  std::error_code linkError;
  fs::create_hard_link(filePath, link, linkError);
  if(linkError)
  {
    WARN("hard-link capability unavailable: " << linkError.message());
    return false;
  }
  REQUIRE(fs::equivalent(filePath, link));
  return true;
}

/**
 * @brief Writes a file with its XDMF sidecar and requires the write to be rejected before the hook or any file change.
 * @param primary Primary path as the caller passes it (absolute or relative).
 * @param directory Directory that holds every fixture of the case.
 *
 * The process recorder is armed on the primary path: the destination check runs before the hook, so it must see no call.
 */
void RequireXdmfDestinationRejected(const fs::path& primary, const fs::path& directory)
{
  INFO("primary: " << Utf8Text(primary));
  RecordingFileReplaceHandler& recorder = ProcessRecorder();
  std::vector<HookCall> calls;
  recorder.arm(primary, MakeRecordingCallback(k_ProcessRecorderName, calls), {}, false);
  const RecorderDisarmGuard disarmGuard({&recorder});

  const std::map<std::string, std::string> before = SnapshotDirectory(directory);
  const DataStructure dataStructure = MakeWriterTestStructure(k_WrittenObjectName, 3);
  const Result<> writeResult = DREAM3D::WriteFile(primary, dataStructure, Pipeline{}, true);
  REQUIRE(writeResult.invalid());
  REQUIRE(writeResult.errors().size() == 1);
  CHECK(writeResult.errors().front().code == k_XdmfDestinationRejectedCode);
  INFO("error: " << writeResult.errors().front().message);
  CHECK(writeResult.errors().front().message.find(Utf8Text(primary.filename())) != std::string::npos);
  CHECK(calls.empty());
  CHECK(SnapshotDirectory(directory) == before);
}

/**
 * @brief Runs RequireXdmfDestinationRejected() with the absolute primary and with the same path relative to the working directory.
 * @param absolutePrimary Absolute primary path.
 * @param directory Directory that holds every fixture of the case.
 */
void RequireXdmfDestinationRejectedBothForms(const fs::path& absolutePrimary, const fs::path& directory)
{
  RequireXdmfDestinationRejected(absolutePrimary, directory);
  // Lexical: a relative path must name the same directory entry, not the file a link resolves to.
  const fs::path relativeDirectory = directory.lexically_relative(fs::current_path());
  if(relativeDirectory.empty())
  {
    WARN("no relative form: the test directory and the working directory have different roots");
    return;
  }
  RequireXdmfDestinationRejected(relativeDirectory / absolutePrimary.filename(), directory);
}

/**
 * @brief Writes a file with its sidecar and requires both to exist afterwards.
 * @param primary Primary path as the caller passes it.
 * @param primaryOnDisk Where the primary file must appear.
 */
void RequireBothWritten(const fs::path& primary, const fs::path& primaryOnDisk)
{
  INFO("primary: " << Utf8Text(primary));
  const DataStructure dataStructure = MakeWriterTestStructure(k_WrittenObjectName, 3);
  const Result<> writeResult = DREAM3D::WriteFile(primary, dataStructure, Pipeline{}, true);
  SIMPLNX_RESULT_REQUIRE_VALID(writeResult);
  CHECK(fs::is_regular_file(primaryOnDisk));
  CHECK(fs::is_regular_file(fs::path(primaryOnDisk).replace_extension(".xdmf")));
}

/**
 * @struct PathWriterCase
 * @brief One path-based writer under test.
 */
struct PathWriterCase
{
  std::string name;
  bool isAppend = false;
  std::function<Result<>(const fs::path& filePath, const DataStructure& dataStructure)> write;
};

/**
 * @brief Returns every path-based writer that must run the hook once on its path.
 * @return DREAM3D::WriteFile, DataStructureWriter::WriteFile, DataStructureWriter::AppendFile, DREAM3D::AppendFile and
 * DREAM3D::WriteRecoveryFile in its full mode, in that order.
 *
 * Append writers append k_WrittenObjectName to an existing file; the other writers replace the file.
 */
std::vector<PathWriterCase> PathWriterCases()
{
  const DataPath appendPath({k_WrittenObjectName});
  return {
      {"DREAM3D::WriteFile", false, [](const fs::path& filePath, const DataStructure& dataStructure) { return DREAM3D::WriteFile(filePath, dataStructure); }},
      {"DataStructureWriter::WriteFile", false, [](const fs::path& filePath, const DataStructure& dataStructure) { return HDF5::DataStructureWriter::WriteFile(dataStructure, filePath); }},
      {"DataStructureWriter::AppendFile", true,
       [appendPath](const fs::path& filePath, const DataStructure& dataStructure) { return HDF5::DataStructureWriter::AppendFile(filePath, dataStructure, appendPath); }},
      {"DREAM3D::AppendFile", true, [appendPath](const fs::path& filePath, const DataStructure& dataStructure) { return DREAM3D::AppendFile(filePath, dataStructure, appendPath); }},
      {"DREAM3D::WriteRecoveryFile full", false, [](const fs::path& filePath, const DataStructure& dataStructure) { return DREAM3D::WriteRecoveryFile(filePath, dataStructure); }},
  };
}

/**
 * @brief Requires a result to carry the recorder's warning.
 * @param result Writer result.
 */
void RequireRecorderWarning(const Result<>& result)
{
  const bool hasRecorderWarning =
      std::any_of(result.warnings().begin(), result.warnings().end(), [](const Warning& warning) { return warning.code == k_RecorderWarningCode && warning.message == "recorder warning"; });
  CHECK(hasRecorderWarning);
}

/**
 * @brief Requires a result to be invalid with an error of the given code.
 * @param result Writer result.
 * @param code Expected error code.
 */
void RequireErrorCode(const Result<>& result, int32 code)
{
  SIMPLNX_RESULT_REQUIRE_INVALID(result);
  std::string messages;
  for(const Error& error : result.errors())
  {
    messages += fmt::format("[{}] {}\n", error.code, error.message);
  }
  INFO(messages);
  const bool hasCode = std::any_of(result.errors().begin(), result.errors().end(), [code](const Error& error) { return error.code == code; });
  CHECK(hasCode);
}
/**
 * @brief Calls BeforeFileReplace() and requires a valid result.
 * @param target Target path of the call.
 */
void RequireHookValid(const fs::path& target)
{
  const Result<> hookResult = BeforeFileReplace(target);
  SIMPLNX_RESULT_REQUIRE_VALID(hookResult);
}
} // namespace

TEST_CASE("FileReplaceGuard: with no handler the call returns a valid result without warnings", "[FileWriteHook]")
{
  // Each test case runs in its own process, and this one registers no handler.
  const Result<> hookResult = BeforeFileReplace(fs::path("no_handler_dir") / "target.dream3d");
  SIMPLNX_RESULT_REQUIRE_VALID(hookResult);
  CHECK(hookResult.warnings().empty());
}

TEST_CASE("FileReplaceGuard: handlers run in registration order with the target and the mode", "[FileWriteHook]")
{
  const fs::path target = fs::path("hook_order_dir") / "hook_target.dream3d";
  std::vector<HookCall> calls;
  const std::shared_ptr<void> registrationA = RegisterFileReplaceHandler(MakeLoggingHandler("handler-a", calls));
  const std::shared_ptr<void> registrationB = RegisterFileReplaceHandler(MakeLoggingHandler("handler-b", calls));
  const std::shared_ptr<void> registrationC = RegisterFileReplaceHandler(MakeLoggingHandler("handler-c", calls));
  REQUIRE(registrationA != nullptr);

  const Result<> hookResult = BeforeFileReplace(target);
  SIMPLNX_RESULT_REQUIRE_VALID(hookResult);
  CHECK(hookResult.warnings().empty());
  REQUIRE(calls.size() == 3);
  CHECK(calls[0].handlerName == "handler-a");
  CHECK(calls[1].handlerName == "handler-b");
  CHECK(calls[2].handlerName == "handler-c");
  for(const HookCall& call : calls)
  {
    CHECK(call.target == target);
    CHECK(call.mode == FileWriteHookMode::ReleaseReaders);
  }
}

TEST_CASE("FileReplaceGuard: the first error stops the fan-out and keeps the earlier warnings in front", "[FileWriteHook]")
{
  const fs::path target = fs::path("hook_error_dir") / "hook_target.dream3d";
  constexpr int32 k_WarningCodeA = -9101;
  constexpr int32 k_WarningCodeB = -9102;
  constexpr int32 k_ArbitraryErrorCode = -9103;

  Result<> errorWithWarning = MakeErrorResult(k_ArbitraryErrorCode, "handler-b cannot make the replacement safe");
  errorWithWarning.warnings().push_back(Warning{k_WarningCodeB, "handler-b warning"});

  std::vector<HookCall> calls;
  const std::shared_ptr<void> registrationA = RegisterFileReplaceHandler(MakeLoggingHandler("handler-a", calls, MakeWarningVoidResult(k_WarningCodeA, "handler-a warning")));
  const std::shared_ptr<void> registrationB = RegisterFileReplaceHandler(MakeLoggingHandler("handler-b", calls, errorWithWarning));
  const std::shared_ptr<void> registrationC = RegisterFileReplaceHandler(MakeLoggingHandler("handler-c", calls));

  const Result<> hookResult = BeforeFileReplace(target);
  SIMPLNX_RESULT_REQUIRE_INVALID(hookResult);
  REQUIRE(hookResult.errors().size() == 1);
  CHECK(hookResult.errors().front().code == k_ArbitraryErrorCode);
  REQUIRE(hookResult.warnings().size() == 2);
  CHECK(hookResult.warnings()[0].code == k_WarningCodeA);
  CHECK(hookResult.warnings()[1].code == k_WarningCodeB);
  REQUIRE(calls.size() == 2);
  CHECK(calls[0].handlerName == "handler-a");
  CHECK(calls[1].handlerName == "handler-b");
}

TEST_CASE("FileReplaceGuard: an invalid result with no errors stays invalid", "[FileWriteHook]")
{
  const fs::path target = fs::path("hook_empty_error_dir") / "hook_target.dream3d";
  constexpr int32 k_WarningCodeA = -9111;
  REQUIRE(InvalidWithoutErrors().invalid());

  std::vector<HookCall> calls;
  const std::shared_ptr<void> registrationA = RegisterFileReplaceHandler(MakeLoggingHandler("handler-a", calls, MakeWarningVoidResult(k_WarningCodeA, "handler-a warning")));
  const std::shared_ptr<void> registrationB = RegisterFileReplaceHandler(MakeLoggingHandler("handler-b", calls, InvalidWithoutErrors()));
  const std::shared_ptr<void> registrationC = RegisterFileReplaceHandler(MakeLoggingHandler("handler-c", calls));

  const Result<> hookResult = BeforeFileReplace(target);
  SIMPLNX_RESULT_REQUIRE_INVALID(hookResult);
  CHECK(hookResult.errors().empty());
  REQUIRE(hookResult.warnings().size() == 1);
  CHECK(hookResult.warnings()[0].code == k_WarningCodeA);
  REQUIRE(calls.size() == 2);
}

TEST_CASE("FileReplaceGuard: the warnings of every handler are kept in order on success", "[FileWriteHook]")
{
  constexpr int32 k_WarningCodeA = -9121;
  constexpr int32 k_WarningCodeB = -9122;
  std::vector<HookCall> calls;
  const std::shared_ptr<void> registrationA = RegisterFileReplaceHandler(MakeLoggingHandler("handler-a", calls, MakeWarningVoidResult(k_WarningCodeA, "handler-a warning")));
  const std::shared_ptr<void> registrationB = RegisterFileReplaceHandler(MakeLoggingHandler("handler-b", calls, MakeWarningVoidResult(k_WarningCodeB, "handler-b warning")));

  const Result<> hookResult = BeforeFileReplace(fs::path("hook_warning_dir") / "hook_target.dream3d");
  SIMPLNX_RESULT_REQUIRE_VALID(hookResult);
  REQUIRE(hookResult.warnings().size() == 2);
  CHECK(hookResult.warnings()[0].code == k_WarningCodeA);
  CHECK(hookResult.warnings()[1].code == k_WarningCodeB);
}

TEST_CASE("FileReplaceGuard: releasing the token unregisters the handler", "[FileWriteHook]")
{
  const fs::path target = fs::path("hook_token_dir") / "hook_target.dream3d";
  std::vector<HookCall> calls;
  std::shared_ptr<void> registration = RegisterFileReplaceHandler(MakeLoggingHandler("handler-a", calls));
  std::shared_ptr<void> registrationCopy = registration;

  RequireHookValid(target);
  REQUIRE(calls.size() == 1);

  // A copy of the token keeps the handler registered.
  registration.reset();
  RequireHookValid(target);
  REQUIRE(calls.size() == 2);

  registrationCopy.reset();
  RequireHookValid(target);
  CHECK(calls.size() == 2);

  // An empty handler registers nothing.
  CHECK(RegisterFileReplaceHandler(FileReplaceHandler{}) == nullptr);
}

TEST_CASE("FileReplaceGuard: a call from inside a handler returns -6061 and calls no handler", "[FileWriteHook]")
{
  const ScopedTestDirectory directory("nested_call");
  const fs::path outerTarget = directory.path() / "outer.dream3d";
  const fs::path innerTarget = directory.path() / "inner.dream3d";
  int32 handlerCallCount = 0;
  Result<> nestedResult;
  Result<AtomicFile> nestedAtomicFileResult = MakeErrorResult<AtomicFile>(0, "not called");
  const std::shared_ptr<void> registration = RegisterFileReplaceHandler([&](const fs::path& target, FileWriteHookMode) -> Result<> {
    handlerCallCount++;
    if(target == outerTarget)
    {
      nestedResult = BeforeFileReplace(innerTarget);
      // A writer called from a handler fails the same way and changes nothing.
      nestedAtomicFileResult = AtomicFile::Create(innerTarget);
    }
    return {};
  });

  const std::map<std::string, std::string> before = SnapshotDirectory(directory.path());
  const Result<> outerResult = BeforeFileReplace(outerTarget);
  SIMPLNX_RESULT_REQUIRE_VALID(outerResult);
  CHECK(handlerCallCount == 1);

  SIMPLNX_RESULT_REQUIRE_INVALID(nestedResult);
  REQUIRE(nestedResult.errors().size() == 1);
  CHECK(nestedResult.errors().front().code == k_FileReplaceNestedError);
  CHECK(nestedResult.errors().front().message.find(Utf8Text(innerTarget)) != std::string::npos);

  SIMPLNX_RESULT_REQUIRE_INVALID(nestedAtomicFileResult);
  REQUIRE(nestedAtomicFileResult.errors().size() == 1);
  CHECK(nestedAtomicFileResult.errors().front().code == k_FileReplaceNestedError);
  CHECK(SnapshotDirectory(directory.path()) == before);

  // Outside a handler the same call works again.
  RequireHookValid(innerTarget);
  CHECK(handlerCallCount == 2);
}

TEST_CASE("FileReplaceGuard: a handler exception propagates and clears the nested-call mark", "[FileWriteHook]")
{
  const fs::path target = fs::path("hook_exception_dir") / "hook_target.dream3d";
  bool throwsException = true;
  int32 laterHandlerCallCount = 0;
  const std::shared_ptr<void> throwingRegistration = RegisterFileReplaceHandler([&throwsException](const fs::path&, FileWriteHookMode) -> Result<> {
    if(throwsException)
    {
      throw std::runtime_error("handler failure");
    }
    return {};
  });
  const std::shared_ptr<void> laterRegistration = RegisterFileReplaceHandler([&laterHandlerCallCount](const fs::path&, FileWriteHookMode) -> Result<> {
    laterHandlerCallCount++;
    return {};
  });

  CHECK_THROWS_WITH(BeforeFileReplace(target), "handler failure");
  CHECK(laterHandlerCallCount == 0);

  // A mark left set would make this call fail with -6061.
  throwsException = false;
  const Result<> hookResult = BeforeFileReplace(target);
  SIMPLNX_RESULT_REQUIRE_VALID(hookResult);
  CHECK(laterHandlerCallCount == 1);
}

TEST_CASE("FileReplaceGuard: the policy applies per thread, the innermost wins, and destruction restores it", "[FileWriteHook]")
{
  const fs::path target = fs::path("hook_policy_dir") / "hook_target.dream3d";
  std::vector<HookCall> calls;
  const std::shared_ptr<void> registration = RegisterFileReplaceHandler(MakeLoggingHandler("handler-a", calls));

  RequireHookValid(target);
  {
    const ScopedFileReplacePolicy outerPolicy(FileWriteHookMode::RefuseIfRead);
    RequireHookValid(target);
    {
      const ScopedFileReplacePolicy innerPolicy(FileWriteHookMode::ReleaseReaders);
      RequireHookValid(target);
    }
    RequireHookValid(target);

    // Another thread does not see the policy of this thread. The main thread joins it before it reads the log.
    Result<> otherThreadResult;
    std::thread otherThread([&otherThreadResult, &target]() { otherThreadResult = BeforeFileReplace(target); });
    otherThread.join();
    SIMPLNX_RESULT_REQUIRE_VALID(otherThreadResult);
  }
  RequireHookValid(target);

  REQUIRE(calls.size() == 6);
  CHECK(calls[0].mode == FileWriteHookMode::ReleaseReaders);
  CHECK(calls[1].mode == FileWriteHookMode::RefuseIfRead);
  CHECK(calls[2].mode == FileWriteHookMode::ReleaseReaders);
  CHECK(calls[3].mode == FileWriteHookMode::RefuseIfRead);
  CHECK(calls[4].mode == FileWriteHookMode::ReleaseReaders);
  CHECK(calls[5].mode == FileWriteHookMode::ReleaseReaders);
}

TEST_CASE("FileReplaceGuard: a handler can register and unregister handlers during a call", "[FileWriteHook]")
{
  const fs::path target = fs::path("hook_registration_dir") / "hook_target.dream3d";
  std::vector<HookCall> calls;
  std::shared_ptr<void> registrationB = RegisterFileReplaceHandler(MakeLoggingHandler("handler-b", calls));
  std::shared_ptr<void> registrationC;
  int32 callNumber = 0;
  // The registry mutex is not held while a handler runs, so these registry changes cannot deadlock.
  const std::shared_ptr<void> registrationA = RegisterFileReplaceHandler([&](const fs::path& callTarget, FileWriteHookMode mode) -> Result<> {
    calls.push_back({"handler-a", callTarget, mode});
    callNumber++;
    if(callNumber == 1)
    {
      registrationC = RegisterFileReplaceHandler(MakeLoggingHandler("handler-c", calls));
    }
    if(callNumber == 2)
    {
      registrationB.reset();
    }
    return {};
  });

  // Call 1: handler-c registers during the call, so it runs only from the next call on.
  RequireHookValid(target);
  // Call 2: handler-b is unregistered during the call, after the call copied the list, so it still runs once.
  RequireHookValid(target);
  // Call 3: handler-b no longer runs.
  RequireHookValid(target);

  std::vector<std::string> names;
  std::transform(calls.begin(), calls.end(), std::back_inserter(names), [](const HookCall& call) { return call.handlerName; });
  const std::vector<std::string> expectedNames = {"handler-b", "handler-a", "handler-b", "handler-a", "handler-c", "handler-a", "handler-c"};
  CHECK(names == expectedNames);
}

TEST_CASE("FileReplaceGuard: releasing a handler that owns another registration does not deadlock", "[FileWriteHook]")
{
  const fs::path target = fs::path("hook_owning_dir") / "hook_target.dream3d";
  std::vector<HookCall> calls;
  std::shared_ptr<void> registrationA = RegisterFileReplaceHandler(MakeLoggingHandler("handler-a", calls));
  // Handler B owns a copy of the token of handler A, so A stays registered until B is destroyed.
  std::shared_ptr<void> registrationB = RegisterFileReplaceHandler([registrationOfA = registrationA, &calls](const fs::path& callTarget, FileWriteHookMode mode) -> Result<> {
    calls.push_back({"handler-b", callTarget, mode});
    return {};
  });
  registrationA.reset();
  RequireHookValid(target);
  REQUIRE(calls.size() == 2);

  // Destroying B destroys the last token of A inside the release of B. A deadlock here ends at the ctest timeout.
  registrationB.reset();
  calls.clear();
  RequireHookValid(target);
  CHECK(calls.empty());
}

TEST_CASE("FileReplaceGuard: a failed token allocation leaves no handler registered", "[FileWriteHook]")
{
  const fs::path target = fs::path("hook_allocation_dir") / "hook_target.dream3d";
  std::vector<HookCall> calls;
  FailNextFileReplaceRegistrationForTesting();
  CHECK_THROWS_AS(RegisterFileReplaceHandler(MakeLoggingHandler("handler-a", calls)), std::bad_alloc);
  RequireHookValid(target);
  CHECK(calls.empty());

  // The seam clears itself, so the next registration works.
  const std::shared_ptr<void> registration = RegisterFileReplaceHandler(MakeLoggingHandler("handler-b", calls));
  RequireHookValid(target);
  REQUIRE(calls.size() == 1);
  CHECK(calls[0].handlerName == "handler-b");
}

TEST_CASE("FileWriteHook: ReplaceFileWithText and OpenFileForReplace run the hook once before the file changes", "[FileWriteHook]")
{
  const ScopedTestDirectory directory("replace_file_with_text");
  const fs::path target = directory.path() / "converted.d3dpipeline";
  RecordingFileReplaceHandler& recorder = ProcessRecorder();
  const RecorderDisarmGuard disarmGuard({&recorder});
  const Result<> recorderWarning = MakeWarningVoidResult(k_RecorderWarningCode, "recorder warning");
  const std::vector<char> previousBytes = {'p', 'r', 'e', 'v', 'i', 'o', 'u', 's'};
  WriteText(target, "previous");
  int32 callCount = 0;
  bool isTargetUnchangedAtCall = false;
  const auto observe = [&](const fs::path&, FileWriteHookMode) {
    callCount++;
    isTargetUnchangedAtCall = ReadFileBytes(target) == previousBytes;
  };

  SECTION("ReplaceFileWithText success keeps the hook warning")
  {
    recorder.arm(target, observe, recorderWarning, false);
    const Result<> writeResult = ReplaceFileWithText(target, "new text");
    SIMPLNX_RESULT_REQUIRE_VALID(writeResult);
    RequireRecorderWarning(writeResult);
    CHECK(callCount == 1);
    CHECK(isTargetUnchangedAtCall);
    CHECK(ReadFileBytes(target) == std::vector<char>{'n', 'e', 'w', ' ', 't', 'e', 'x', 't'});
  }
  SECTION("OpenFileForReplace success keeps the hook warning and truncates")
  {
    recorder.arm(target, observe, recorderWarning, false);
    std::fstream stream;
    const Result<> openResult = OpenFileForReplace(target, stream);
    SIMPLNX_RESULT_REQUIRE_VALID(openResult);
    RequireRecorderWarning(openResult);
    CHECK(callCount == 1);
    CHECK(isTargetUnchangedAtCall);
    stream << "log";
    stream.close();
    CHECK(ReadFileBytes(target) == std::vector<char>{'l', 'o', 'g'});
  }
  SECTION("a refusal with any code or with no errors returns invalid and changes nothing")
  {
    for(const Result<>& refusal : {MakeErrorResult(k_RecorderErrorCode, "recorder error"), InvalidWithoutErrors()})
    {
      recorder.arm(target, {}, refusal, false);
      const Result<> writeResult = ReplaceFileWithText(target, "new text");
      SIMPLNX_RESULT_REQUIRE_INVALID(writeResult);
      CHECK(writeResult.errors().size() == refusal.errors().size());
      std::fstream stream;
      const Result<> openResult = OpenFileForReplace(target, stream);
      SIMPLNX_RESULT_REQUIRE_INVALID(openResult);
      CHECK(openResult.errors().size() == refusal.errors().size());
      CHECK_FALSE(stream.is_open());
      CHECK(ReadFileBytes(target) == previousBytes);
    }
  }
  SECTION("an open failure returns -6067 with the hook warnings")
  {
    const fs::path directoryTarget = directory.path() / "is-a-directory";
    fs::create_directories(directoryTarget);
    recorder.arm(directoryTarget, {}, recorderWarning, false);
    const Result<> writeResult = ReplaceFileWithText(directoryTarget, "new text");
    RequireErrorCode(writeResult, k_ReplaceFileOpenError);
    CHECK(writeResult.errors().front().message.find(Utf8Text(directoryTarget)) != std::string::npos);
    RequireRecorderWarning(writeResult);
    std::fstream stream;
    const Result<> openResult = OpenFileForReplace(directoryTarget, stream);
    RequireErrorCode(openResult, k_ReplaceFileOpenError);
    RequireRecorderWarning(openResult);

    // A stream whose exception mask makes the open throw gets the same result instead of an exception.
    std::fstream throwingStream;
    throwingStream.exceptions(std::ios_base::failbit | std::ios_base::badbit);
    Result<> throwingOpenResult;
    REQUIRE_NOTHROW(throwingOpenResult = OpenFileForReplace(directoryTarget, throwingStream));
    RequireErrorCode(throwingOpenResult, k_ReplaceFileOpenError);
    RequireRecorderWarning(throwingOpenResult);
    CHECK_FALSE(throwingStream.is_open());
  }
  SECTION("a write failure returns -6068 with the hook warnings")
  {
    const ScopedTextStreamFailure failure(TextStreamFailureForTesting::Write);
    recorder.arm(target, {}, recorderWarning, false);
    const Result<> writeResult = ReplaceFileWithText(target, "new text");
    RequireErrorCode(writeResult, k_ReplaceFileWriteError);
    CHECK(writeResult.errors().front().message.find(Utf8Text(target)) != std::string::npos);
    RequireRecorderWarning(writeResult);
  }
  SECTION("a close failure returns -6069 with the hook warnings")
  {
    const ScopedTextStreamFailure failure(TextStreamFailureForTesting::Close);
    recorder.arm(target, {}, recorderWarning, false);
    const Result<> writeResult = ReplaceFileWithText(target, "new text");
    RequireErrorCode(writeResult, k_ReplaceFileCloseError);
    CHECK(writeResult.errors().front().message.find(Utf8Text(target)) != std::string::npos);
    RequireRecorderWarning(writeResult);
  }
}

TEST_CASE("FileWriteHook: DREAM3D::WriteFile runs the hook once on the file and once on its sidecar before writing", "[FileWriteHook]")
{
  const ScopedTestDirectory directory("write_file_hook_order");
  const fs::path primary = directory.path() / "hooked.dream3d";
  const fs::path sidecar = directory.path() / "hooked.xdmf";
  WriteExistingFile(primary);
  const std::vector<char> primaryBytes = ReadFileBytes(primary);
  const DataStructure dataStructure = MakeWriterTestStructure(k_WrittenObjectName, 3);

  RecordingFileReplaceHandler& recorder = ProcessRecorder();
  RecordingFileReplaceHandler& sidecarRecorder = SidecarRecorder();
  const RecorderDisarmGuard disarmGuard({&recorder, &sidecarRecorder});
  std::vector<HookCall> calls;
  std::vector<bool> isPrimaryUnchangedAtCall;
  std::vector<bool> isSidecarAbsentAtCall;
  const auto observeFiles = [&](const std::string& handlerName) {
    return [&, handlerName](const fs::path& target, FileWriteHookMode mode) {
      calls.push_back({handlerName, target, mode});
      isPrimaryUnchangedAtCall.push_back(ReadFileBytes(primary) == primaryBytes);
      isSidecarAbsentAtCall.push_back(!fs::exists(sidecar));
    };
  };

  SECTION("both hooks run once before either file changes")
  {
    recorder.arm(primary, observeFiles("primary"), {}, false);
    sidecarRecorder.arm(sidecar, observeFiles("sidecar"), {}, false);
    const Result<> writeResult = DREAM3D::WriteFile(primary, dataStructure, Pipeline{}, true);
    SIMPLNX_RESULT_REQUIRE_VALID(writeResult);
    REQUIRE(calls.size() == 2);
    CHECK(calls[0].handlerName == "primary");
    CHECK(calls[0].target == primary);
    CHECK(calls[1].handlerName == "sidecar");
    CHECK(calls[1].target == sidecar);
    for(usize index = 0; index < calls.size(); index++)
    {
      CHECK(calls[index].mode == FileWriteHookMode::ReleaseReaders);
      CHECK(isPrimaryUnchangedAtCall[index]);
      CHECK(isSidecarAbsentAtCall[index]);
    }
    CHECK(ReadFileBytes(primary) != primaryBytes);
    CHECK(fs::exists(sidecar));
  }
  SECTION("a sidecar hook error leaves both files unchanged")
  {
    sidecarRecorder.arm(sidecar, observeFiles("sidecar"), MakeErrorResult(k_RecorderErrorCode, "recorder error"), false);
    const Result<> writeResult = DREAM3D::WriteFile(primary, dataStructure, Pipeline{}, true);
    RequireErrorCode(writeResult, k_RecorderErrorCode);
    REQUIRE(calls.size() == 1);
    CHECK(ReadFileBytes(primary) == primaryBytes);
    CHECK_FALSE(fs::exists(sidecar));
  }
  SECTION("a primary hook error leaves both files unchanged and skips the sidecar hook")
  {
    recorder.arm(primary, observeFiles("primary"), MakeErrorResult(k_RecorderErrorCode, "recorder error"), false);
    sidecarRecorder.arm(sidecar, observeFiles("sidecar"), {}, false);
    const Result<> writeResult = DREAM3D::WriteFile(primary, dataStructure, Pipeline{}, true);
    RequireErrorCode(writeResult, k_RecorderErrorCode);
    REQUIRE(calls.size() == 1);
    CHECK(calls[0].handlerName == "primary");
    CHECK(ReadFileBytes(primary) == primaryBytes);
    CHECK_FALSE(fs::exists(sidecar));
  }
  SECTION("without a sidecar the hook runs only on the primary file")
  {
    recorder.arm(primary, observeFiles("primary"), {}, false);
    sidecarRecorder.arm(sidecar, observeFiles("sidecar"), {}, false);
    const Result<> writeResult = DREAM3D::WriteFile(primary, dataStructure, Pipeline{}, false);
    SIMPLNX_RESULT_REQUIRE_VALID(writeResult);
    REQUIRE(calls.size() == 1);
    CHECK(calls[0].handlerName == "primary");
    CHECK_FALSE(fs::exists(sidecar));
  }
}

TEST_CASE("FileWriteHook: path writers run the hook once before the file changes", "[FileWriteHook]")
{
  const ScopedTestDirectory directory("path_writer_hook");
  const DataStructure dataStructure = MakeWriterTestStructure(k_WrittenObjectName, 3);
  RecordingFileReplaceHandler& recorder = ProcessRecorder();
  const RecorderDisarmGuard disarmGuard({&recorder});

  for(const PathWriterCase& writerCase : PathWriterCases())
  {
    DYNAMIC_SECTION(writerCase.name)
    {
      const fs::path target = directory.path() / "target.dream3d";
      WriteExistingFile(target);
      const std::vector<char> targetBytes = ReadFileBytes(target);

      // Exactly one call per writer call, made before the file changes. Writers that delegate to a FileIO& overload do not run
      // the hook again.
      std::vector<HookCall> calls;
      bool isTargetUnchangedAtCall = false;
      recorder.arm(
          target,
          [&](const fs::path& callTarget, FileWriteHookMode mode) {
            calls.push_back({k_ProcessRecorderName, callTarget, mode});
            isTargetUnchangedAtCall = ReadFileBytes(target) == targetBytes;
          },
          {}, false);
      const Result<> writeResult = writerCase.write(target, dataStructure);
      SIMPLNX_RESULT_REQUIRE_VALID(writeResult);
      REQUIRE(calls.size() == 1);
      CHECK(calls[0].target == target);
      CHECK(calls[0].mode == FileWriteHookMode::ReleaseReaders);
      CHECK(isTargetUnchangedAtCall);
      CHECK(ReadFileBytes(target) != targetBytes);

      // A hook error is returned before the file is opened, with any code and also with no error at all.
      for(const Result<>& refusal : {MakeErrorResult(k_RecorderErrorCode, "recorder error"), InvalidWithoutErrors()})
      {
        recorder.disarm();
        WriteExistingFile(target);
        const std::vector<char> freshBytes = ReadFileBytes(target);
        calls.clear();
        recorder.arm(target, MakeRecordingCallback(k_ProcessRecorderName, calls), refusal, false);
        const Result<> failedResult = writerCase.write(target, dataStructure);
        SIMPLNX_RESULT_REQUIRE_INVALID(failedResult);
        CHECK(failedResult.errors().size() == refusal.errors().size());
        CHECK(calls.size() == 1);
        CHECK(ReadFileBytes(target) == freshBytes);
      }
    }
  }
}

TEST_CASE("FileWriteHook: WriteRecoveryFile runs the hook once in redirect mode", "[FileWriteHook]")
{
  const ScopedTestDirectory directory("recovery_redirect_hook");
  const fs::path target = directory.path() / "recovery.dream3d";
  const fs::path userDataFile = directory.path() / "user.dream3d";
  WriteExistingFile(target);
  const std::vector<char> targetBytes = ReadFileBytes(target);
  const DataStructure dataStructure = MakeWriterTestStructure(k_WrittenObjectName, 3);
  RecordingFileReplaceHandler& recorder = ProcessRecorder();
  const RecorderDisarmGuard disarmGuard({&recorder});

  std::vector<HookCall> calls;
  bool isTargetUnchangedAtCall = false;
  recorder.arm(
      target,
      [&](const fs::path& callTarget, FileWriteHookMode mode) {
        calls.push_back({k_ProcessRecorderName, callTarget, mode});
        isTargetUnchangedAtCall = ReadFileBytes(target) == targetBytes;
      },
      MakeWarningVoidResult(k_RecorderWarningCode, "recorder warning"), false);
  const Result<> writeResult = DREAM3D::WriteRecoveryFile(target, dataStructure, Pipeline{}, userDataFile);
  SIMPLNX_RESULT_REQUIRE_VALID(writeResult);
  RequireRecorderWarning(writeResult);
  REQUIRE(calls.size() == 1);
  CHECK(calls[0].target == target);
  CHECK(isTargetUnchangedAtCall);
  const Result<std::optional<fs::path>> redirectResult = DREAM3D::ReadUserDataFilePathAttribute(target);
  SIMPLNX_RESULT_REQUIRE_VALID(redirectResult);
  REQUIRE(redirectResult.value().has_value());
  CHECK(redirectResult.value().value() == fs::absolute(userDataFile));

  // A refusal leaves the existing file unchanged.
  WriteExistingFile(target);
  const std::vector<char> freshBytes = ReadFileBytes(target);
  calls.clear();
  recorder.arm(target, MakeRecordingCallback(k_ProcessRecorderName, calls), MakeErrorResult(k_RecorderErrorCode, "recorder error"), false);
  const Result<> refusedResult = DREAM3D::WriteRecoveryFile(target, dataStructure, Pipeline{}, userDataFile);
  RequireErrorCode(refusedResult, k_RecorderErrorCode);
  CHECK(calls.size() == 1);
  CHECK(ReadFileBytes(target) == freshBytes);
}

TEST_CASE("FileWriteHook: WriteXdmf runs the hook once and reports its own stream failures", "[FileWriteHook]")
{
  const ScopedTestDirectory directory("write_xdmf");
  const DataStructure dataStructure = MakeWriterTestStructure(k_WrittenObjectName, 3);
  RecordingFileReplaceHandler& recorder = ProcessRecorder();
  const RecorderDisarmGuard disarmGuard({&recorder});
  const Result<> recorderWarning = MakeWarningVoidResult(k_RecorderWarningCode, "recorder warning");
  const fs::path target = directory.path() / "standalone.xdmf";
  std::vector<HookCall> calls;

  SECTION("the hook runs once before the file changes")
  {
    WriteText(target, "previous");
    bool isTargetUnchangedAtCall = false;
    recorder.arm(
        target,
        [&](const fs::path& callTarget, FileWriteHookMode mode) {
          calls.push_back({k_ProcessRecorderName, callTarget, mode});
          isTargetUnchangedAtCall = ReadFileBytes(target) == std::vector<char>{'p', 'r', 'e', 'v', 'i', 'o', 'u', 's'};
        },
        recorderWarning, false);
    const Result<> writeResult = DREAM3D::WriteXdmf(target, dataStructure, "data.dream3d");
    SIMPLNX_RESULT_REQUIRE_VALID(writeResult);
    RequireRecorderWarning(writeResult);
    REQUIRE(calls.size() == 1);
    CHECK(calls[0].target == target);
    CHECK(isTargetUnchangedAtCall);
    CHECK(ReadFileBytes(target) != std::vector<char>{'p', 'r', 'e', 'v', 'i', 'o', 'u', 's'});
  }
  SECTION("a refusal with any code or with no errors returns invalid and changes nothing")
  {
    for(const Result<>& refusal : {MakeErrorResult(k_RecorderErrorCode, "recorder error"), InvalidWithoutErrors()})
    {
      WriteText(target, "previous");
      recorder.arm(target, {}, refusal, false);
      const Result<> writeResult = DREAM3D::WriteXdmf(target, dataStructure, "data.dream3d");
      SIMPLNX_RESULT_REQUIRE_INVALID(writeResult);
      CHECK(writeResult.errors().size() == refusal.errors().size());
      CHECK(ReadFileBytes(target) == std::vector<char>{'p', 'r', 'e', 'v', 'i', 'o', 'u', 's'});
    }
  }
  SECTION("an open failure returns -6062 with the hook warnings")
  {
    // A directory at the path cannot be opened as a file.
    fs::create_directories(target);
    recorder.arm(target, {}, recorderWarning, false);
    const Result<> writeResult = DREAM3D::WriteXdmf(target, dataStructure, "data.dream3d");
    RequireErrorCode(writeResult, k_XdmfOpenErrorCode);
    CHECK(writeResult.errors().front().message.find(Utf8Text(target)) != std::string::npos);
    RequireRecorderWarning(writeResult);
  }
  SECTION("a write failure returns -6063 with the hook warnings")
  {
    const ScopedTextStreamFailure failure(TextStreamFailureForTesting::Write);
    recorder.arm(target, {}, recorderWarning, false);
    const Result<> writeResult = DREAM3D::WriteXdmf(target, dataStructure, "data.dream3d");
    RequireErrorCode(writeResult, k_XdmfWriteErrorCode);
    CHECK(writeResult.errors().front().message.find(Utf8Text(target)) != std::string::npos);
    RequireRecorderWarning(writeResult);
  }
  SECTION("a close failure returns -6064 with the hook warnings")
  {
    const ScopedTextStreamFailure failure(TextStreamFailureForTesting::Close);
    recorder.arm(target, {}, recorderWarning, false);
    const Result<> writeResult = DREAM3D::WriteXdmf(target, dataStructure, "data.dream3d");
    RequireErrorCode(writeResult, k_XdmfCloseErrorCode);
    CHECK(writeResult.errors().front().message.find(Utf8Text(target)) != std::string::npos);
    RequireRecorderWarning(writeResult);
  }
}

TEST_CASE("FileWriteHook: DREAM3D::WriteFile propagates sidecar stream failures after the DREAM3D file is written", "[FileWriteHook]")
{
  const ScopedTestDirectory directory("write_file_sidecar_failures");
  const fs::path primary = directory.path() / "with-sidecar.dream3d";
  const fs::path sidecar = directory.path() / "with-sidecar.xdmf";
  const DataStructure dataStructure = MakeWriterTestStructure(k_WrittenObjectName, 3);
  RecordingFileReplaceHandler& recorder = ProcessRecorder();
  RecordingFileReplaceHandler& sidecarRecorder = SidecarRecorder();
  const RecorderDisarmGuard disarmGuard({&recorder, &sidecarRecorder});
  std::vector<HookCall> calls;
  recorder.arm(primary, MakeRecordingCallback("primary", calls), MakeWarningVoidResult(k_RecorderWarningCode, "recorder warning"), false);
  sidecarRecorder.arm(sidecar, MakeRecordingCallback("sidecar", calls), {}, false);

  SECTION("open failure")
  {
    fs::create_directories(sidecar);
    const Result<> writeResult = DREAM3D::WriteFile(primary, dataStructure, Pipeline{}, true);
    RequireErrorCode(writeResult, k_XdmfOpenErrorCode);
    RequireRecorderWarning(writeResult);
  }
  SECTION("write failure")
  {
    const ScopedTextStreamFailure failure(TextStreamFailureForTesting::Write);
    const Result<> writeResult = DREAM3D::WriteFile(primary, dataStructure, Pipeline{}, true);
    RequireErrorCode(writeResult, k_XdmfWriteErrorCode);
    RequireRecorderWarning(writeResult);
  }
  SECTION("close failure")
  {
    const ScopedTextStreamFailure failure(TextStreamFailureForTesting::Close);
    const Result<> writeResult = DREAM3D::WriteFile(primary, dataStructure, Pipeline{}, true);
    RequireErrorCode(writeResult, k_XdmfCloseErrorCode);
    RequireRecorderWarning(writeResult);
  }
  // The internal sidecar writer does not run the hook a second time, and the DREAM3D file is complete.
  REQUIRE(calls.size() == 2);
  CHECK(calls[0].handlerName == "primary");
  CHECK(calls[1].handlerName == "sidecar");
  CHECK(fs::is_regular_file(primary));
  const DataStructure readBack = UnitTest::LoadDataStructure(primary);
  CHECK(readBack.getData(DataPath({k_WrittenObjectName, "Values"})) != nullptr);
}

TEST_CASE("FileWriteHook: PathToUtf8Text renders a path as UTF-8 without throwing", "[FileWriteHook]")
{
  SECTION("an empty path is empty text")
  {
    CHECK(PathToUtf8Text(fs::path{}).empty());
  }
  SECTION("ASCII and non-ASCII names are their UTF-8 bytes")
  {
    // Universal character names keep the expected bytes independent of the source encoding the compiler assumes.
    const fs::path asciiPath = fs::path("folder") / "out.dream3d";
    CHECK(PathToUtf8Text(asciiPath) == Utf8Text(asciiPath));
    const fs::path nonAsciiPath = fs::path("folder") / fs::path(u8"café-文件-\U0001F600.dream3d");
    const std::string expectedName = "caf\xC3\xA9-\xE6\x96\x87\xE4\xBB\xB6-\xF0\x9F\x98\x80.dream3d";
    REQUIRE_NOTHROW(PathToUtf8Text(nonAsciiPath));
    CHECK(PathToUtf8Text(nonAsciiPath) == Utf8Text(nonAsciiPath));
    CHECK(PathToUtf8Text(nonAsciiPath.filename()) == expectedName);
  }
#ifdef _WIN32
  SECTION("an unpaired UTF-16 surrogate becomes U+FFFD")
  {
    // u8string() throws for this name; the rendering replaces the code unit instead.
    const fs::path surrogatePath(std::wstring{L'a', static_cast<wchar_t>(0xD800), L'b'});
    REQUIRE_NOTHROW(PathToUtf8Text(surrogatePath));
    const std::string replacementCharacter = "\xEF\xBF\xBD";
    CHECK(PathToUtf8Text(surrogatePath) == "a" + replacementCharacter + "b");
  }
#endif
}

TEST_CASE("FileWriteHook: WriteFile rejects colliding XDMF destinations", "[FileWriteHook]")
{
  const ScopedTestDirectory directory("xdmf_collisions");
  const fs::path& dir = directory.path();
  const fs::path unrelated = dir / "unrelated.bin";
  {
    std::ofstream stream(unrelated, std::ios::binary);
    stream << "unrelated file";
  }

  SECTION("a primary with the XDMF extension in any letter case")
  {
    RequireXdmfDestinationRejectedBothForms(dir / "out.xdmf", dir);
    RequireXdmfDestinationRejectedBothForms(dir / "out.XdMf", dir);
  }
  SECTION("a primary whose name ends with a space or a period, initially absent")
  {
    RequireXdmfDestinationRejectedBothForms(dir / "out.xdmf ", dir);
    RequireXdmfDestinationRejectedBothForms(dir / "out.XDMF.", dir);
    RequireXdmfDestinationRejectedBothForms(dir / "out.dream3d ", dir);
  }
  SECTION("a primary that is a hard link of its existing sidecar")
  {
    const fs::path sidecar = dir / "hard.xdmf";
    {
      std::ofstream stream(sidecar, std::ios::binary);
      stream << "existing sidecar";
    }
    if(CreateHardLinkOrWarn(sidecar, dir / "hard.dream3d"))
    {
      RequireXdmfDestinationRejectedBothForms(dir / "hard.dream3d", dir);
    }
  }
  // Universal character names keep the names independent of the source encoding the compiler assumes. The emoji is outside every
  // legacy Windows code page, so std::filesystem::path::string() throws for these names there unless the process code page is UTF-8.
  SECTION("a non-ASCII primary with the XDMF extension")
  {
    RequireXdmfDestinationRejectedBothForms(dir / fs::path(u8"数据-café-\U0001F600.xdmf"), dir);
  }
  SECTION("a non-ASCII primary that is a hard link of its existing sidecar")
  {
    const fs::path sidecar = dir / fs::path(u8"硬-café-\U0001F600.xdmf");
    {
      std::ofstream stream(sidecar, std::ios::binary);
      stream << "existing sidecar";
    }
    const fs::path primary = dir / fs::path(u8"硬-café-\U0001F600.dream3d");
    if(CreateHardLinkOrWarn(sidecar, primary))
    {
      RequireXdmfDestinationRejectedBothForms(primary, dir);
    }
  }
#ifdef _WIN32
  SECTION("a primary name with an unpaired UTF-16 surrogate is rendered with U+FFFD")
  {
    // A lone high surrogate is a valid NTFS name but has no UTF-8 form, so u8string() throws for it as well.
    const fs::path primary = dir / fs::path(std::wstring{L'b', L'a', L'd', static_cast<wchar_t>(0xD800), L'.', L'x', L'd', L'm', L'f'});
    const std::map<std::string, std::string> before = SnapshotDirectory(dir);
    const DataStructure dataStructure = MakeWriterTestStructure(k_WrittenObjectName, 3);
    const Result<> writeResult = DREAM3D::WriteFile(primary, dataStructure, Pipeline{}, true);
    REQUIRE(writeResult.invalid());
    REQUIRE(writeResult.errors().size() == 1);
    CHECK(writeResult.errors().front().code == k_XdmfDestinationRejectedCode);
    INFO("error: " << writeResult.errors().front().message);
    CHECK(writeResult.errors().front().message.find("bad\xEF\xBF\xBD.xdmf") != std::string::npos);
    CHECK(SnapshotDirectory(dir) == before);
  }
#endif
  SECTION("a sidecar that is a dangling symbolic link")
  {
    if(CreateSymlinkOrWarn(dir / "missing-target.bin", dir / "dangling.xdmf"))
    {
      RequireXdmfDestinationRejectedBothForms(dir / "dangling.dream3d", dir);
    }
  }
  SECTION("a sidecar that is a symbolic link to an existing file")
  {
    if(CreateSymlinkOrWarn(unrelated, dir / "linked.xdmf"))
    {
      RequireXdmfDestinationRejectedBothForms(dir / "linked.dream3d", dir);
    }
  }
  SECTION("a primary that is a symbolic link to its sidecar path")
  {
    {
      std::ofstream stream(dir / "self.xdmf", std::ios::binary);
      stream << "existing sidecar";
    }
    if(CreateSymlinkOrWarn(dir / "self.xdmf", dir / "self.dream3d"))
    {
      RequireXdmfDestinationRejectedBothForms(dir / "self.dream3d", dir);
    }
  }
  SECTION("a primary that is a symbolic link to an unrelated file")
  {
    if(CreateSymlinkOrWarn(unrelated, dir / "other.dream3d"))
    {
      RequireXdmfDestinationRejectedBothForms(dir / "other.dream3d", dir);
    }
  }
  SECTION("a primary that is a dangling symbolic link")
  {
    if(CreateSymlinkOrWarn(dir / "nowhere.dream3d", dir / "dangling-primary.dream3d"))
    {
      RequireXdmfDestinationRejectedBothForms(dir / "dangling-primary.dream3d", dir);
    }
  }
  SECTION("a primary reached through a chain of two links")
  {
    if(CreateSymlinkOrWarn(dir / "chain.xdmf", dir / "chain-step.bin") && CreateSymlinkOrWarn(dir / "chain-step.bin", dir / "chain.dream3d"))
    {
      RequireXdmfDestinationRejectedBothForms(dir / "chain.dream3d", dir);
    }
  }
  SECTION("controls: plain paths write both files")
  {
    RequireBothWritten(dir / "plain.dream3d", dir / "plain.dream3d");
    const fs::path relativeDirectory = dir.lexically_relative(fs::current_path());
    if(relativeDirectory.empty())
    {
      WARN("no relative form: the test directory and the working directory have different roots");
    }
    else
    {
      RequireBothWritten(relativeDirectory / "relative.dream3d", dir / "relative.dream3d");
    }
  }
#ifndef _WIN32
  SECTION("control: a primary inside a symbolically linked directory writes both files")
  {
    fs::create_directories(dir / "real");
    std::error_code linkError;
    fs::create_directory_symlink(dir / "real", dir / "linked-dir", linkError);
    REQUIRE_FALSE(linkError);
    RequireBothWritten(dir / "linked-dir" / "inside.dream3d", dir / "real" / "inside.dream3d");
  }
#else
  SECTION("control: a primary inside a junction directory writes both files")
  {
    fs::create_directories(dir / "real");
    const fs::path junction = dir / "junction-dir";
    // Junctions need no privilege; std::filesystem cannot create them, so the test uses the shell built-in.
    const std::wstring command = L"cmd /c mklink /J \"" + junction.wstring() + L"\" \"" + (dir / "real").wstring() + L"\" >NUL";
    if(_wsystem(command.c_str()) != 0 || !fs::exists(junction))
    {
      WARN("junction capability unavailable: mklink /J failed");
    }
    else
    {
      RequireBothWritten(junction / "inside.dream3d", dir / "real" / "inside.dream3d");
    }
  }
#endif
}

TEST_CASE("FileWriteHook: writers keep hook warnings", "[FileWriteHook]")
{
  const ScopedTestDirectory directory("writer_hook_warnings");
  RecordingFileReplaceHandler& recorder = ProcessRecorder();
  RecordingFileReplaceHandler& sidecarRecorder = SidecarRecorder();
  const RecorderDisarmGuard disarmGuard({&recorder, &sidecarRecorder});
  const DataStructure dataStructure = MakeWriterTestStructure(k_WrittenObjectName, 3);
  const Result<> recorderWarning = MakeWarningVoidResult(k_RecorderWarningCode, "recorder warning");

  for(const PathWriterCase& writerCase : PathWriterCases())
  {
    DYNAMIC_SECTION(writerCase.name)
    {
      const fs::path target = directory.path() / "target.dream3d";
      std::error_code ignored;
      fs::remove(target, ignored);
      if(writerCase.isAppend)
      {
        WriteExistingFile(target);
      }

      SECTION("after a successful write")
      {
        recorder.arm(target, {}, recorderWarning, false);
        const Result<> writeResult = writerCase.write(target, dataStructure);
        SIMPLNX_RESULT_REQUIRE_VALID(writeResult);
        RequireRecorderWarning(writeResult);
      }
      SECTION("after an open failure")
      {
        const fs::path missingTarget = directory.path() / "missing-directory" / "target.dream3d";
        recorder.arm(missingTarget, {}, recorderWarning, false);
        const Result<> writeResult = writerCase.write(missingTarget, dataStructure);
        REQUIRE(writeResult.invalid());
        RequireRecorderWarning(writeResult);
        CHECK_FALSE(fs::exists(missingTarget));
      }
      SECTION("after a close failure")
      {
        const hid_t driverId = FailCloseVfd::RegisterFailCloseVfd();
        REQUIRE(driverId >= 0);
        const int32 failuresBefore = FailCloseVfd::CloseFailureCount();
        recorder.arm(target, {}, recorderWarning, false);
        Result<> writeResult;
        {
          const FailCloseVfd::FailCloseArmGuard armGuard(driverId);
          writeResult = writerCase.write(target, dataStructure);
        }
        CHECK(FailCloseVfd::CloseFailureCount() > failuresBefore);
        RequireErrorCode(writeResult, k_FailedCloseCode);
        RequireRecorderWarning(writeResult);
      }
      SECTION("after a serialization failure past the open")
      {
        const DataStructure unwritable = MakeUnwritableStructure();
        recorder.arm(target, {}, recorderWarning, false);
        const Result<> writeResult = writerCase.write(target, unwritable);
        RequireErrorCode(writeResult, k_UnwritableTypeCode);
        RequireRecorderWarning(writeResult);
      }
      SECTION("when the recorder's own error carries the warning")
      {
        Result<> errorWithWarning = MakeErrorResult(k_RecorderErrorCode, "recorder error");
        errorWithWarning.warnings().push_back(Warning{k_RecorderWarningCode, "recorder warning"});
        recorder.arm(target, {}, errorWithWarning, false);
        const Result<> writeResult = writerCase.write(target, dataStructure);
        REQUIRE(writeResult.invalid());
        REQUIRE(writeResult.errors().size() == 1);
        CHECK(writeResult.errors().front().code == k_RecorderErrorCode);
        RequireRecorderWarning(writeResult);
      }
    }
  }

  SECTION("DREAM3D::WriteFile after a later sidecar hook error")
  {
    const fs::path primary = directory.path() / "with-sidecar.dream3d";
    const fs::path sidecar = directory.path() / "with-sidecar.xdmf";
    recorder.arm(primary, {}, recorderWarning, false);
    sidecarRecorder.arm(sidecar, {}, MakeErrorResult(k_RecorderErrorCode, "sidecar recorder error"), false);
    const Result<> writeResult = DREAM3D::WriteFile(primary, dataStructure, Pipeline{}, true);
    REQUIRE(writeResult.invalid());
    REQUIRE(writeResult.errors().size() == 1);
    CHECK(writeResult.errors().front().code == k_RecorderErrorCode);
    RequireRecorderWarning(writeResult);
    CHECK_FALSE(fs::exists(primary));
    CHECK_FALSE(fs::exists(sidecar));
  }
}
