#include <catch2/catch.hpp>

#include "simplnx/Common/PathText.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IO/Generic/DataIOCollection.hpp"
#include "simplnx/DataStructure/IO/Generic/IDataIOManager.hpp"
#include "simplnx/DataStructure/IO/HDF5/DataStructureWriter.hpp"
#include "simplnx/DataStructure/Montage/GridMontage.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/UnitTest/FailCloseVfd.hpp"
#include "simplnx/UnitTest/RecordingFileWriteManager.hpp"
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
#include <string>
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
  std::string managerName;
  fs::path target;
  FileWriteHookMode mode = FileWriteHookMode::ReleaseReaders;
};

/**
 * @brief Disarms recorders when a test ends, also after a failed assertion.
 *
 * The guard does not own the recorders. Each recorder must outlive the guard.
 */
class DisarmGuard
{
public:
  explicit DisarmGuard(std::vector<RecordingFileWriteManager*> recorders)
  : m_Recorders(std::move(recorders))
  {
  }
  ~DisarmGuard() noexcept
  {
    for(RecordingFileWriteManager* recorder : m_Recorders)
    {
      recorder->disarm();
    }
  }
  DisarmGuard(const DisarmGuard&) = delete;
  DisarmGuard(DisarmGuard&&) = delete;
  DisarmGuard& operator=(const DisarmGuard&) = delete;
  DisarmGuard& operator=(DisarmGuard&&) = delete;

private:
  std::vector<RecordingFileWriteManager*> m_Recorders;
};

/**
 * @brief Restores the I/O managers and plugins of the application after a test deletes and recreates it.
 *
 * A new Application has a new DataIOCollection with only the built-in managers, and it has no loaded plugins. Other
 * tests in the same process expect the managers that a test bootstrap registered, for example the out-of-core manager.
 * They also expect the plugins that UnitTest::LoadPlugins() loaded once. The restorer keeps the managers of the
 * application that exists at construction. At destruction it registers each kept manager that the current collection
 * does not have. It also loads the plugins again if the first application had plugins.
 */
class ApplicationStateRestorer
{
public:
  ApplicationStateRestorer()
  {
    const std::shared_ptr<Application> application = Application::GetOrCreateInstance();
    for(const auto& [formatName, manager] : application->getIOCollection())
    {
      m_Managers.push_back(manager);
    }
    m_HadPlugins = !application->getPluginList().empty();
  }

  ~ApplicationStateRestorer() noexcept
  {
    try
    {
      const std::shared_ptr<Application> application = Application::GetOrCreateInstance();
      DataIOCollection& ioCollection = application->getIOCollection();
      for(const std::shared_ptr<IDataIOManager>& manager : m_Managers)
      {
        if(ioCollection.getManager(manager->formatName()) == nullptr)
        {
          const Result<> registrationResult = ioCollection.addIOManager(manager);
          CHECK(registrationResult.valid());
        }
      }
      if(m_HadPlugins && application->getPluginList().empty())
      {
        const Result<> loadResult = application->loadPlugins(SIMPLNX_BUILD_DIR, true);
        CHECK(loadResult.valid());
      }
    } catch(const std::exception& exception)
    {
      FAIL_CHECK(fmt::format("Cannot restore the application state: {}", exception.what()));
    }
  }

  ApplicationStateRestorer(const ApplicationStateRestorer&) = delete;
  ApplicationStateRestorer(ApplicationStateRestorer&&) = delete;
  ApplicationStateRestorer& operator=(const ApplicationStateRestorer&) = delete;
  ApplicationStateRestorer& operator=(ApplicationStateRestorer&&) = delete;

private:
  std::vector<std::shared_ptr<IDataIOManager>> m_Managers;
  bool m_HadPlugins = false;
};

/**
 * @brief Creates a recorder callback that logs each matching call.
 * @param managerName Name stored with each logged call, so a test can check the manager order.
 * @param calls Log that receives the calls. It must outlive the recorder arming.
 * @return Callback for RecordingFileWriteManager::arm().
 */
RecordingFileWriteManager::HookCallback MakeRecordingCallback(const std::string& managerName, std::vector<HookCall>& calls)
{
  return [managerName, &calls](const fs::path& target, FileWriteHookMode mode) { calls.push_back({managerName, target, mode}); };
}
} // namespace

TEST_CASE("FileWriteHook: beforeFileWrite forwards target and mode to every manager", "[FileWriteHook]")
{
  const fs::path target = fs::path("hook_forwarding_dir") / "hook_target.dream3d";
  DataStructure dataStructure;

  auto recorderA = std::make_shared<RecordingFileWriteManager>("rec-a");
  auto recorderB = std::make_shared<RecordingFileWriteManager>("rec-b");
  DataIOCollection collection;
  Result<> addResultA = collection.addIOManager(recorderA);
  SIMPLNX_RESULT_REQUIRE_VALID(addResultA);
  Result<> addResultB = collection.addIOManager(recorderB);
  SIMPLNX_RESULT_REQUIRE_VALID(addResultB);
  const DisarmGuard disarmGuard({recorderA.get(), recorderB.get()});

  std::vector<HookCall> calls;
  recorderA->arm(target, MakeRecordingCallback("rec-a", calls), {}, false);
  recorderB->arm(target, MakeRecordingCallback("rec-b", calls), {}, false);

  Result<> refuseResult = collection.beforeFileWrite(dataStructure, target, FileWriteHookMode::RefuseIfRead);
  SIMPLNX_RESULT_REQUIRE_VALID(refuseResult);
  REQUIRE(refuseResult.warnings().empty());
  REQUIRE(calls.size() == 2);
  CHECK(calls[0].managerName == "rec-a");
  CHECK(calls[1].managerName == "rec-b");
  for(const HookCall& call : calls)
  {
    CHECK(call.target == target);
    CHECK(call.mode == FileWriteHookMode::RefuseIfRead);
  }

  calls.clear();
  Result<> defaultResult = collection.beforeFileWrite(dataStructure, target);
  SIMPLNX_RESULT_REQUIRE_VALID(defaultResult);
  REQUIRE(calls.size() == 2);
  CHECK(calls[0].managerName == "rec-a");
  CHECK(calls[1].managerName == "rec-b");
  for(const HookCall& call : calls)
  {
    CHECK(call.target == target);
    CHECK(call.mode == FileWriteHookMode::ReleaseReaders);
  }

  // A recorder handles only its armed target.
  calls.clear();
  Result<> otherTargetResult = collection.beforeFileWrite(dataStructure, target.parent_path() / "other_target.dream3d");
  SIMPLNX_RESULT_REQUIRE_VALID(otherTargetResult);
  CHECK(calls.empty());
}

TEST_CASE("FileWriteHook: beforeFileWrite stops at the first error and keeps warnings", "[FileWriteHook]")
{
  const fs::path target = fs::path("hook_error_dir") / "hook_target.dream3d";
  DataStructure dataStructure;

  auto recorderA = std::make_shared<RecordingFileWriteManager>("rec-a");
  auto recorderB = std::make_shared<RecordingFileWriteManager>("rec-b");
  auto recorderC = std::make_shared<RecordingFileWriteManager>("rec-c");
  DataIOCollection collection;
  Result<> addResultA = collection.addIOManager(recorderA);
  SIMPLNX_RESULT_REQUIRE_VALID(addResultA);
  Result<> addResultB = collection.addIOManager(recorderB);
  SIMPLNX_RESULT_REQUIRE_VALID(addResultB);
  Result<> addResultC = collection.addIOManager(recorderC);
  SIMPLNX_RESULT_REQUIRE_VALID(addResultC);
  const DisarmGuard disarmGuard({recorderA.get(), recorderB.get(), recorderC.get()});

  constexpr int32 k_WarningCodeA = -9101;
  constexpr int32 k_WarningCodeB = -9102;
  constexpr int32 k_ReleaseErrorCode = -6039;

  Result<> errorWithWarning = MakeErrorResult(k_ReleaseErrorCode, "rec-b cannot release a reader");
  errorWithWarning.warnings().push_back(Warning{k_WarningCodeB, "rec-b warning"});

  std::vector<HookCall> calls;
  recorderA->arm(target, MakeRecordingCallback("rec-a", calls), MakeWarningVoidResult(k_WarningCodeA, "rec-a warning"), false);
  recorderB->arm(target, MakeRecordingCallback("rec-b", calls), errorWithWarning, false);
  recorderC->arm(target, MakeRecordingCallback("rec-c", calls), {}, false);

  Result<> hookResult = collection.beforeFileWrite(dataStructure, target);
  REQUIRE(hookResult.invalid());
  REQUIRE(hookResult.errors().size() == 1);
  CHECK(hookResult.errors().front().code == k_ReleaseErrorCode);
  REQUIRE(hookResult.warnings().size() == 2);
  CHECK(hookResult.warnings()[0].code == k_WarningCodeA);
  CHECK(hookResult.warnings()[1].code == k_WarningCodeB);

  REQUIRE(calls.size() == 2);
  CHECK(calls[0].managerName == "rec-a");
  CHECK(calls[1].managerName == "rec-b");
}

TEST_CASE("FileWriteHook: beforeFileWrite keeps an invalid result with no errors invalid", "[FileWriteHook]")
{
  const fs::path target = fs::path("hook_empty_error_dir") / "hook_target.dream3d";
  DataStructure dataStructure;

  auto recorderA = std::make_shared<RecordingFileWriteManager>("rec-a");
  auto recorderB = std::make_shared<RecordingFileWriteManager>("rec-b");
  auto recorderC = std::make_shared<RecordingFileWriteManager>("rec-c");
  DataIOCollection collection;
  Result<> addResultA = collection.addIOManager(recorderA);
  SIMPLNX_RESULT_REQUIRE_VALID(addResultA);
  Result<> addResultB = collection.addIOManager(recorderB);
  SIMPLNX_RESULT_REQUIRE_VALID(addResultB);
  Result<> addResultC = collection.addIOManager(recorderC);
  SIMPLNX_RESULT_REQUIRE_VALID(addResultC);
  const DisarmGuard disarmGuard({recorderA.get(), recorderB.get(), recorderC.get()});

  constexpr int32 k_WarningCodeA = -9111;
  // An unexpected with an empty error collection is a legal invalid Result.
  const Result<> invalidWithoutErrors = Result<>{{nonstd::make_unexpected(ErrorCollection{})}};
  REQUIRE(invalidWithoutErrors.invalid());

  std::vector<HookCall> calls;
  recorderA->arm(target, MakeRecordingCallback("rec-a", calls), MakeWarningVoidResult(k_WarningCodeA, "rec-a warning"), false);
  recorderB->arm(target, MakeRecordingCallback("rec-b", calls), invalidWithoutErrors, false);
  recorderC->arm(target, MakeRecordingCallback("rec-c", calls), {}, false);

  Result<> hookResult = collection.beforeFileWrite(dataStructure, target);
  REQUIRE(hookResult.invalid());
  CHECK(hookResult.errors().empty());
  REQUIRE(hookResult.warnings().size() == 1);
  CHECK(hookResult.warnings()[0].code == k_WarningCodeA);

  REQUIRE(calls.size() == 2);
  CHECK(calls[0].managerName == "rec-a");
  CHECK(calls[1].managerName == "rec-b");
}

TEST_CASE("FileWriteHook: beforeFileWrite keeps the warnings of every manager on success", "[FileWriteHook]")
{
  const fs::path target = fs::path("hook_warning_dir") / "hook_target.dream3d";
  DataStructure dataStructure;

  auto recorderA = std::make_shared<RecordingFileWriteManager>("rec-a");
  auto recorderB = std::make_shared<RecordingFileWriteManager>("rec-b");
  DataIOCollection collection;
  Result<> addResultA = collection.addIOManager(recorderA);
  SIMPLNX_RESULT_REQUIRE_VALID(addResultA);
  Result<> addResultB = collection.addIOManager(recorderB);
  SIMPLNX_RESULT_REQUIRE_VALID(addResultB);
  const DisarmGuard disarmGuard({recorderA.get(), recorderB.get()});

  constexpr int32 k_WarningCodeA = -9121;
  constexpr int32 k_WarningCodeB = -9122;
  recorderA->arm(target, {}, MakeWarningVoidResult(k_WarningCodeA, "rec-a warning"), false);
  recorderB->arm(target, {}, MakeWarningVoidResult(k_WarningCodeB, "rec-b warning"), false);

  Result<> hookResult = collection.beforeFileWrite(dataStructure, target);
  SIMPLNX_RESULT_REQUIRE_VALID(hookResult);
  REQUIRE(hookResult.warnings().size() == 2);
  CHECK(hookResult.warnings()[0].code == k_WarningCodeA);
  CHECK(hookResult.warnings()[1].code == k_WarningCodeB);
}

TEST_CASE("FileWriteHook: the application recorder survives an application reset", "[FileWriteHook]")
{
  RecordingFileWriteManager& recorder = ApplicationRecorder();
  REQUIRE(Application::GetOrCreateInstance()->getIOCollection().getManager(k_ApplicationRecorderName).get() == &recorder);

  const ApplicationStateRestorer applicationRestorer;
  Application::DeleteInstance();
  REQUIRE(Application::Instance() == nullptr);
  REQUIRE(Application::GetOrCreateInstance()->getIOCollection().getManager(k_ApplicationRecorderName) == nullptr);

  RecordingFileWriteManager& recorderAfterReset = ApplicationRecorder();
  REQUIRE(&recorderAfterReset == &recorder);
  DataIOCollection& ioCollection = Application::GetOrCreateInstance()->getIOCollection();
  REQUIRE(ioCollection.getManager(k_ApplicationRecorderName).get() == &recorder);

  const fs::path target = fs::path("hook_reset_dir") / "hook_target.dream3d";
  std::vector<HookCall> calls;
  recorder.arm(target, MakeRecordingCallback(k_ApplicationRecorderName, calls), {}, true);
  const DisarmGuard disarmGuard({&recorder});

  DataStructure dataStructure;
  Result<> hookResult = ioCollection.beforeFileWrite(dataStructure, target, FileWriteHookMode::RefuseIfRead);
  SIMPLNX_RESULT_REQUIRE_VALID(hookResult);
  REQUIRE(calls.size() == 1);
  CHECK(calls[0].target == target);
  CHECK(calls[0].mode == FileWriteHookMode::RefuseIfRead);
  CHECK_FALSE(recorder.isArmed());
}

TEST_CASE("FileWriteHook: DataStructure counts getAllDataPaths calls in test builds", "[FileWriteHook]")
{
  DataStructure dataStructure;
  REQUIRE(DataGroup::Create(dataStructure, "Group") != nullptr);

  const uint64 countBefore = DataStructure::GetAllDataPathsCallCountForTesting();
  const std::vector<DataPath> paths = dataStructure.getAllDataPaths();
  CHECK(paths.size() == 1);
  CHECK(DataStructure::GetAllDataPathsCallCountForTesting() == countBefore + 1);

  (void)dataStructure.getAllDataPaths();
  (void)dataStructure.getAllDataPaths();
  CHECK(DataStructure::GetAllDataPathsCallCountForTesting() == countBefore + 3);

  // Other path queries are not counted.
  (void)dataStructure.getAllDataObjectIds();
  CHECK(DataStructure::GetAllDataPathsCallCountForTesting() == countBefore + 3);
}

namespace
{
inline constexpr int32 k_RecorderWarningCode = 99001;
inline constexpr int32 k_RecorderErrorCode = -99002;
inline constexpr int32 k_XdmfDestinationRejectedCode = -6057;
inline constexpr int32 k_FailedCloseCode = -6056;
inline constexpr int32 k_UnwritableTypeCode = -5;

/**
 * @brief Name of the second recorder that SidecarRecorder() registers in the application's I/O collection.
 *
 * It sorts after k_ApplicationRecorderName, so it runs after the application recorder.
 */
inline constexpr const char* k_SidecarRecorderName = "zz-test-recorder-sidecar";

/**
 * @brief Name of the top-level object every writer test writes or appends.
 */
inline constexpr const char* k_WrittenObjectName = "Appended";

/**
 * @brief Returns a second recorder registered in the I/O collection of the current application.
 * @return The one sidecar recorder of this executable.
 *
 * One recorder handles one target. A test that needs a warning from the primary file's hook call and an error from the
 * sidecar's hook call arms the application recorder on the primary file and this recorder on the sidecar. Like
 * ApplicationRecorder(), it registers again when the current application does not hold it.
 */
RecordingFileWriteManager& SidecarRecorder()
{
  static const std::shared_ptr<RecordingFileWriteManager> recorder = std::make_shared<RecordingFileWriteManager>(k_SidecarRecorderName);
  DataIOCollection& ioCollection = Application::GetOrCreateInstance()->getIOCollection();
  if(ioCollection.getManager(k_SidecarRecorderName) != recorder)
  {
    Result<> registrationResult = ioCollection.addIOManager(recorder);
    SIMPLNX_RESULT_REQUIRE_VALID(registrationResult);
  }
  return *recorder;
}

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
 * @brief Writes a closed .dream3d file without the hook armed, so tests have an existing target or append fixture.
 * @param filePath File to create or replace.
 */
void WriteExistingFile(const fs::path& filePath)
{
  const DataStructure fixtureStructure = MakeWriterTestStructure("Existing", 7);
  const Result<> fixtureResult = DREAM3D::WriteFile(filePath, fixtureStructure);
  SIMPLNX_RESULT_REQUIRE_VALID(fixtureResult);
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
 * The application recorder is armed on the primary path: the destination check runs before the hook, so it must see
 * no call.
 */
void RequireXdmfDestinationRejected(const fs::path& primary, const fs::path& directory)
{
  INFO("primary: " << Utf8Text(primary));
  RecordingFileWriteManager& recorder = ApplicationRecorder();
  std::vector<HookCall> calls;
  recorder.arm(primary, MakeRecordingCallback(k_ApplicationRecorderName, calls), {}, false);
  const DisarmGuard disarmGuard({&recorder});

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
 * @brief Returns every path-based writer that must run the hook.
 * @return DREAM3D::WriteFile, DataStructureWriter::WriteFile, DataStructureWriter::AppendFile and DREAM3D::AppendFile, in that order.
 *
 * Append writers append k_WrittenObjectName to an existing file; write writers replace the file.
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
  REQUIRE(result.invalid());
  std::string messages;
  for(const Error& error : result.errors())
  {
    messages += fmt::format("[{}] {}\n", error.code, error.message);
  }
  INFO(messages);
  const bool hasCode = std::any_of(result.errors().begin(), result.errors().end(), [code](const Error& error) { return error.code == code; });
  CHECK(hasCode);
}
} // namespace

TEST_CASE("FileWriteHook: DREAM3D::WriteFile runs the hook on the file and its sidecar before writing", "[FileWriteHook]")
{
  const ScopedTestDirectory directory("write_file_hook_order");
  const fs::path primary = directory.path() / "hooked.dream3d";
  const fs::path sidecar = directory.path() / "hooked.xdmf";
  WriteExistingFile(primary);
  const std::vector<char> primaryBytes = ReadFileBytes(primary);
  const DataStructure dataStructure = MakeWriterTestStructure(k_WrittenObjectName, 3);

  RecordingFileWriteManager& recorder = ApplicationRecorder();
  const DisarmGuard disarmGuard({&recorder});
  std::vector<HookCall> calls;
  bool isPrimaryUnchangedAtCall = false;
  bool isSidecarAbsentAtCall = false;
  const auto observeFiles = [&](const fs::path& target, FileWriteHookMode mode) {
    calls.push_back({k_ApplicationRecorderName, target, mode});
    isPrimaryUnchangedAtCall = ReadFileBytes(primary) == primaryBytes;
    isSidecarAbsentAtCall = !fs::exists(sidecar);
  };

  SECTION("the hook runs on the primary file before it changes")
  {
    recorder.arm(primary, observeFiles, {}, false);
    const Result<> writeResult = DREAM3D::WriteFile(primary, dataStructure, Pipeline{}, true);
    SIMPLNX_RESULT_REQUIRE_VALID(writeResult);
    REQUIRE(calls.size() == 1);
    CHECK(calls[0].target == primary);
    CHECK(calls[0].mode == FileWriteHookMode::ReleaseReaders);
    CHECK(isPrimaryUnchangedAtCall);
    CHECK(isSidecarAbsentAtCall);
    CHECK(ReadFileBytes(primary) != primaryBytes);
    CHECK(fs::exists(sidecar));
  }
  SECTION("the hook runs on the sidecar before either file changes")
  {
    recorder.arm(sidecar, observeFiles, {}, false);
    const Result<> writeResult = DREAM3D::WriteFile(primary, dataStructure, Pipeline{}, true);
    SIMPLNX_RESULT_REQUIRE_VALID(writeResult);
    REQUIRE(calls.size() == 1);
    CHECK(calls[0].target == sidecar);
    CHECK(calls[0].mode == FileWriteHookMode::ReleaseReaders);
    CHECK(isPrimaryUnchangedAtCall);
    CHECK(isSidecarAbsentAtCall);
    CHECK(fs::exists(sidecar));
  }
  SECTION("a sidecar hook error leaves the primary file unchanged")
  {
    recorder.arm(sidecar, observeFiles, MakeErrorResult(k_RecorderErrorCode, "recorder error"), false);
    const Result<> writeResult = DREAM3D::WriteFile(primary, dataStructure, Pipeline{}, true);
    REQUIRE(writeResult.invalid());
    REQUIRE(writeResult.errors().size() == 1);
    CHECK(writeResult.errors().front().code == k_RecorderErrorCode);
    REQUIRE(calls.size() == 1);
    CHECK(ReadFileBytes(primary) == primaryBytes);
    CHECK_FALSE(fs::exists(sidecar));
  }
  SECTION("a primary hook error leaves the primary file unchanged")
  {
    recorder.arm(primary, observeFiles, MakeErrorResult(k_RecorderErrorCode, "recorder error"), false);
    const Result<> writeResult = DREAM3D::WriteFile(primary, dataStructure, Pipeline{}, true);
    REQUIRE(writeResult.invalid());
    REQUIRE(writeResult.errors().size() == 1);
    CHECK(writeResult.errors().front().code == k_RecorderErrorCode);
    REQUIRE(calls.size() == 1);
    CHECK(ReadFileBytes(primary) == primaryBytes);
    CHECK_FALSE(fs::exists(sidecar));
  }
  SECTION("without a sidecar the hook does not run on the sidecar path")
  {
    recorder.arm(sidecar, observeFiles, {}, false);
    const Result<> writeResult = DREAM3D::WriteFile(primary, dataStructure, Pipeline{}, false);
    SIMPLNX_RESULT_REQUIRE_VALID(writeResult);
    CHECK(calls.empty());
    CHECK_FALSE(fs::exists(sidecar));
  }
}

TEST_CASE("FileWriteHook: DataStructureWriter and AppendFile run the hook", "[FileWriteHook]")
{
  const ScopedTestDirectory directory("writer_and_append_hook");
  const DataStructure dataStructure = MakeWriterTestStructure(k_WrittenObjectName, 3);
  RecordingFileWriteManager& recorder = ApplicationRecorder();
  const DisarmGuard disarmGuard({&recorder});

  for(const PathWriterCase& writerCase : PathWriterCases())
  {
    if(writerCase.name == "DREAM3D::WriteFile")
    {
      continue;
    }
    DYNAMIC_SECTION(writerCase.name)
    {
      const fs::path target = directory.path() / "target.dream3d";
      WriteExistingFile(target);
      const std::vector<char> targetBytes = ReadFileBytes(target);

      // Exactly one call per writer call, made before the file changes. DREAM3D::AppendFile delegates to the writer's
      // FileIO& overload, which does not run the hook again.
      std::vector<HookCall> calls;
      bool isTargetUnchangedAtCall = false;
      recorder.arm(
          target,
          [&](const fs::path& callTarget, FileWriteHookMode mode) {
            calls.push_back({k_ApplicationRecorderName, callTarget, mode});
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

      // A hook error is returned before the file is opened.
      WriteExistingFile(target);
      const std::vector<char> freshBytes = ReadFileBytes(target);
      calls.clear();
      recorder.arm(target, MakeRecordingCallback(k_ApplicationRecorderName, calls), MakeErrorResult(k_RecorderErrorCode, "recorder error"), false);
      const Result<> failedResult = writerCase.write(target, dataStructure);
      REQUIRE(failedResult.invalid());
      REQUIRE(failedResult.errors().size() == 1);
      CHECK(failedResult.errors().front().code == k_RecorderErrorCode);
      CHECK(calls.size() == 1);
      CHECK(ReadFileBytes(target) == freshBytes);
    }
  }
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
    const fs::path nonAsciiPath = fs::path("folder") / fs::path(u8"caf\u00E9-\u6587\u4EF6-\U0001F600.dream3d");
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
    RequireXdmfDestinationRejectedBothForms(dir / fs::path(u8"\u6570\u636E-caf\u00E9-\U0001F600.xdmf"), dir);
  }
  SECTION("a non-ASCII primary that is a hard link of its existing sidecar")
  {
    const fs::path sidecar = dir / fs::path(u8"\u786C-caf\u00E9-\U0001F600.xdmf");
    {
      std::ofstream stream(sidecar, std::ios::binary);
      stream << "existing sidecar";
    }
    const fs::path primary = dir / fs::path(u8"\u786C-caf\u00E9-\U0001F600.dream3d");
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
  RecordingFileWriteManager& recorder = ApplicationRecorder();
  RecordingFileWriteManager& sidecarRecorder = SidecarRecorder();
  const DisarmGuard disarmGuard({&recorder, &sidecarRecorder});
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
