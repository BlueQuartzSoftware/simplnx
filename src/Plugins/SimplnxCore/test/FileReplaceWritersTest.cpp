#include <catch2/catch.hpp>

#include "SimplnxCore/Filters/Algorithms/ExecuteProcess.hpp"
#include "SimplnxCore/Filters/Algorithms/WriteAbaqusHexahedron.hpp"
#include "SimplnxCore/Filters/Algorithms/WriteDREAM3D.hpp"
#include "SimplnxCore/Filters/Algorithms/WriteNodesAndElementsFiles.hpp"
#include "SimplnxCore/Filters/WriteVtkStructuredPointsFilter.hpp"
#include "SimplnxCore/utils/PythonPluginTemplateFile.hpp"

#include "simplnx/Common/FileReplaceGuard.hpp"
#include "simplnx/Common/PathText.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/UnitTest/RecordingFileReplaceHandler.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <fmt/format.h>

#include <algorithm>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <set>
#include <string>
#include <vector>

using namespace nx::core;
using namespace nx::core::UnitTest;

namespace fs = std::filesystem;

namespace
{
inline constexpr int32 k_RecorderWarningCode = 99101;
inline constexpr int32 k_RecorderErrorCode = -99102;
inline constexpr int32 k_StagedXdmfMoveErrorCode = -6065;
inline constexpr int32 k_SameNodeAndElementFileCode = -139;

const DataPath k_ImageGeometryPath({"ImageGeometry"});
const DataPath k_CellDataPath = k_ImageGeometryPath.createChildPath("CellData");
const DataPath k_FeatureIdsPath = k_CellDataPath.createChildPath("FeatureIds");
const DataPath k_EdgeGeometryPath({"EdgeGeometry"});

/**
 * @class ScopedWriterTestDirectory
 * @brief Creates an empty directory for one test and removes it with everything in it at scope exit.
 */
class ScopedWriterTestDirectory
{
public:
  explicit ScopedWriterTestDirectory(const std::string& testName)
  : m_Path(fs::temp_directory_path() / "simplnx_file_replace_writers" / testName)
  {
    fs::remove_all(m_Path);
    fs::create_directories(m_Path);
  }

  ~ScopedWriterTestDirectory() noexcept
  {
    std::error_code ignored;
    fs::remove_all(m_Path, ignored);
  }

  ScopedWriterTestDirectory(const ScopedWriterTestDirectory&) = delete;
  ScopedWriterTestDirectory(ScopedWriterTestDirectory&&) = delete;
  ScopedWriterTestDirectory& operator=(const ScopedWriterTestDirectory&) = delete;
  ScopedWriterTestDirectory& operator=(ScopedWriterTestDirectory&&) = delete;

  const fs::path& path() const
  {
    return m_Path;
  }

private:
  fs::path m_Path;
};

/**
 * @brief Returns a second recorder that is registered for the whole test executable.
 * @return The one second recorder of this executable.
 */
RecordingFileReplaceHandler& SecondRecorder()
{
  static const std::shared_ptr<RecordingFileReplaceHandler> s_Recorder = std::make_shared<RecordingFileReplaceHandler>();
  static const std::shared_ptr<void> s_Registration = RegisterRecorder(s_Recorder);
  return *s_Recorder;
}

/**
 * @brief Returns an invalid result with an empty error collection.
 * @return The result.
 */
Result<> InvalidWithoutErrors()
{
  return Result<>{{nonstd::make_unexpected(ErrorCollection{})}};
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
 * @brief Lists the names of the entries of a directory.
 * @param directory Directory to list.
 * @return Sorted entry names.
 */
std::set<std::string> ListEntries(const fs::path& directory)
{
  std::set<std::string> entries;
  for(const fs::directory_entry& entry : fs::directory_iterator(directory))
  {
    entries.insert(PathToUtf8Text(entry.path().filename()));
  }
  return entries;
}

/**
 * @brief Requires a result to be invalid with an error of the given code.
 * @param result Writer result.
 * @param code Expected error code.
 */
void RequireErrorCode(const Result<>& result, int32 code)
{
  SIMPLNX_RESULT_REQUIRE_INVALID(result);
  const bool hasCode = std::any_of(result.errors().begin(), result.errors().end(), [code](const Error& error) { return error.code == code; });
  CHECK(hasCode);
}

/**
 * @brief Requires a result to carry the recorder's warning.
 * @param result Writer result.
 */
void RequireRecorderWarning(const Result<>& result)
{
  const bool hasRecorderWarning = std::any_of(result.warnings().begin(), result.warnings().end(), [](const Warning& warning) { return warning.code == k_RecorderWarningCode; });
  CHECK(hasRecorderWarning);
}

/**
 * @brief Builds a 2 x 2 x 1 image geometry with an int32 feature-ID cell array.
 * @return The structure.
 */
DataStructure MakeImageStructure()
{
  DataStructure dataStructure;
  auto* imageGeom = ImageGeom::Create(dataStructure, k_ImageGeometryPath.getTargetName());
  REQUIRE(imageGeom != nullptr);
  imageGeom->setDimensions({2, 2, 1});
  imageGeom->setOrigin({0.0F, 0.0F, 0.0F});
  imageGeom->setSpacing({1.0F, 1.0F, 1.0F});
  const ShapeType cellShape = {1, 2, 2};
  auto* cellData = AttributeMatrix::Create(dataStructure, k_CellDataPath.getTargetName(), cellShape, imageGeom->getId());
  REQUIRE(cellData != nullptr);
  imageGeom->setCellData(*cellData);
  auto featureIdsStore = DataStoreUtilities::CreateDataStore<int32>(dataStructure, k_FeatureIdsPath, cellShape, {1});
  auto* featureIds = DataArray<int32>::Create(dataStructure, k_FeatureIdsPath.getTargetName(), featureIdsStore, cellData->getId());
  REQUIRE(featureIds != nullptr);
  for(usize index = 0; index < 4; index++)
  {
    featureIds->getDataStoreRef()[index] = static_cast<int32>(index % 2) + 1;
  }
  return dataStructure;
}

/**
 * @brief Builds an edge geometry with two vertices and one edge.
 * @return The structure.
 */
DataStructure MakeEdgeStructure()
{
  DataStructure dataStructure;
  EdgeGeom* geom = EdgeGeom::Create(dataStructure, k_EdgeGeometryPath.getTargetName());
  REQUIRE(geom != nullptr);
  auto* edgeAttrMatrix = AttributeMatrix::Create(dataStructure, "Edge Data", {1}, geom->getId());
  geom->setEdgeAttributeMatrix(*edgeAttrMatrix);
  auto* vertexAttrMatrix = AttributeMatrix::Create(dataStructure, "Vertex Data", {2}, geom->getId());
  geom->setVertexAttributeMatrix(*vertexAttrMatrix);
  Float32Array* vertices = UnitTest::CreateTestDataArray<float32>(dataStructure, "Vertices Store", {2}, {3}, geom->getId());
  const std::vector<float32> vertexValues = {1.0F, 1.5F, 1.75F, 2.0F, 3.0F, 4.0F};
  std::copy(vertexValues.begin(), vertexValues.end(), vertices->begin());
  geom->setVertices(*vertices);
  auto* cells = UnitTest::CreateTestDataArray<IGeometry::MeshIndexType>(dataStructure, "Cells Store", {1}, {2}, geom->getId());
  cells->setValue(0, 0ULL);
  cells->setValue(1, 1ULL);
  geom->setEdgeList(*cells);
  return dataStructure;
}

/**
 * @brief Runs the WriteDREAM3D algorithm with an .xdmf file and no pipeline.
 * @param dataStructure Structure to write.
 * @param exportFilePath Final DREAM3D path.
 * @return The algorithm result.
 */
Result<> RunWriteDream3d(DataStructure& dataStructure, const fs::path& exportFilePath)
{
  WriteDREAM3DInputValues inputValues;
  inputValues.ExportFilePath = exportFilePath;
  inputValues.WriteXdmfFile = true;
  inputValues.UseCompression = false;
  const std::atomic_bool shouldCancel = false;
  const IFilter::MessageHandler messageHandler{[](const IFilter::Message&) {}};
  return WriteDREAM3D(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace

TEST_CASE("SimplnxCore::FileReplaceWriters: WriteDREAM3D runs the hook once on each final file before either changes", "[SimplnxCore][WriteDREAM3DFilter][FileWriteHook]")
{
  const ScopedWriterTestDirectory directory("write_dream3d");
  const fs::path primary = directory.path() / "output.dream3d";
  const fs::path sidecar = directory.path() / "output.xdmf";
  WriteText(primary, "previous dream3d");
  WriteText(sidecar, "previous xdmf");
  DataStructure dataStructure = MakeImageStructure();

  RecordingFileReplaceHandler& primaryRecorder = ProcessRecorder();
  RecordingFileReplaceHandler& sidecarRecorder = SecondRecorder();
  const RecorderDisarmGuard disarmGuard({&primaryRecorder, &sidecarRecorder});
  std::vector<std::string> calls;
  std::vector<bool> areFilesUnchangedAtCall;
  const auto observe = [&](const std::string& name) {
    return [&, name](const fs::path&, FileWriteHookMode) {
      calls.push_back(name);
      areFilesUnchangedAtCall.push_back(ReadText(primary) == "previous dream3d" && ReadText(sidecar) == "previous xdmf");
    };
  };

  SECTION("success: one call per final file, before the files change, with the hook warning kept")
  {
    primaryRecorder.arm(primary, observe("primary"), MakeWarningVoidResult(k_RecorderWarningCode, "recorder warning"), false);
    sidecarRecorder.arm(sidecar, observe("sidecar"), {}, false);
    const Result<> writeResult = RunWriteDream3d(dataStructure, primary);
    SIMPLNX_RESULT_REQUIRE_VALID(writeResult);
    RequireRecorderWarning(writeResult);
    REQUIRE(calls == std::vector<std::string>{"primary", "sidecar"});
    CHECK(areFilesUnchangedAtCall == std::vector<bool>{true, true});
    CHECK(ReadText(primary) != "previous dream3d");
    CHECK(ReadText(sidecar).find("output.dream3d") != std::string::npos);
    // Both temporary directories are gone.
    CHECK(ListEntries(directory.path()) == std::set<std::string>{"output.dream3d", "output.xdmf"});
  }
  SECTION("a refusal of the .xdmf file leaves both final files unchanged, with any code or with no errors")
  {
    for(const Result<>& refusal : {MakeErrorResult(k_RecorderErrorCode, "recorder error"), InvalidWithoutErrors()})
    {
      calls.clear();
      primaryRecorder.arm(primary, observe("primary"), MakeWarningVoidResult(k_RecorderWarningCode, "recorder warning"), false);
      sidecarRecorder.arm(sidecar, observe("sidecar"), refusal, false);
      const Result<> writeResult = RunWriteDream3d(dataStructure, primary);
      SIMPLNX_RESULT_REQUIRE_INVALID(writeResult);
      CHECK(writeResult.errors().size() == refusal.errors().size());
      RequireRecorderWarning(writeResult);
      CHECK(calls == std::vector<std::string>{"primary", "sidecar"});
      CHECK(ReadText(primary) == "previous dream3d");
      CHECK(ReadText(sidecar) == "previous xdmf");
      CHECK(ListEntries(directory.path()) == std::set<std::string>{"output.dream3d", "output.xdmf"});
    }
  }
}

TEST_CASE("SimplnxCore::FileReplaceWriters: WriteDREAM3D returns -6065 when the staged .xdmf file cannot move", "[SimplnxCore][WriteDREAM3DFilter][FileWriteHook]")
{
  const ScopedWriterTestDirectory directory("write_dream3d_staged_move");
  const fs::path primary = directory.path() / "output.dream3d";
  const fs::path sidecar = directory.path() / "output.xdmf";
  WriteText(primary, "previous dream3d");
  WriteText(sidecar, "previous xdmf");
  DataStructure dataStructure = MakeImageStructure();

  // The DREAM3D writer runs the hook on the staged .xdmf path inside the temporary directory of the .dream3d file. At that point
  // the temporary directory of the final .xdmf file exists. A non-empty directory at its temporary file path makes the move fail.
  bool isBlocked = false;
  const std::shared_ptr<void> registration = RegisterFileReplaceHandler([&](const fs::path& target, FileWriteHookMode) -> Result<> {
    const fs::path stagingDirectory = target.parent_path();
    if(isBlocked || target.filename() != sidecar.filename() || stagingDirectory.parent_path() != directory.path())
    {
      return {};
    }
    for(const fs::directory_entry& entry : fs::directory_iterator(directory.path()))
    {
      if(entry.is_directory() && entry.path() != stagingDirectory)
      {
        fs::create_directories(entry.path() / sidecar.filename() / "blocker");
        isBlocked = true;
      }
    }
    return {};
  });

  const Result<> writeResult = RunWriteDream3d(dataStructure, primary);
  REQUIRE(isBlocked);
  RequireErrorCode(writeResult, k_StagedXdmfMoveErrorCode);
  CHECK(writeResult.errors().front().message.find(PathToUtf8Text(sidecar)) != std::string::npos);
  CHECK(ReadText(primary) == "previous dream3d");
  CHECK(ReadText(sidecar) == "previous xdmf");
  CHECK(ListEntries(directory.path()) == std::set<std::string>{"output.dream3d", "output.xdmf"});
}

TEST_CASE("SimplnxCore::FileReplaceWriters: WriteAbaqusHexahedron stops at a refused file before any file changes", "[SimplnxCore][WriteAbaqusHexahedronFilter][FileWriteHook]")
{
  const ScopedWriterTestDirectory directory("write_abaqus_hexahedron");
  DataStructure dataStructure = MakeImageStructure();
  WriteAbaqusHexahedronInputValues inputValues;
  inputValues.UseReducedIntegration = false;
  inputValues.HourglassStiffness = 250;
  inputValues.JobName = "Job";
  inputValues.OutputPath = directory.path();
  inputValues.FilePrefix = "abaqus";
  inputValues.FeatureIdsArrayPath = k_FeatureIdsPath;
  inputValues.ImageGeometryPath = k_ImageGeometryPath;
  inputValues.WriteDummyNode = false;

  RecordingFileReplaceHandler& recorder = ProcessRecorder();
  RecordingFileReplaceHandler& secondRecorder = SecondRecorder();
  const RecorderDisarmGuard disarmGuard({&recorder, &secondRecorder});
  std::vector<std::string> calls;
  recorder.arm(directory.path() / "abaqus_nodes.inp", [&calls](const fs::path&, FileWriteHookMode) { calls.push_back("nodes"); }, {}, false);
  // The second file is refused, so the third to fifth files must not reach the hook.
  secondRecorder.arm(directory.path() / "abaqus_elems.inp", [&calls](const fs::path&, FileWriteHookMode) { calls.push_back("elems"); }, MakeErrorResult(k_RecorderErrorCode, "recorder error"), false);
  int32 laterCallCount = 0;
  const std::shared_ptr<void> registration = RegisterFileReplaceHandler([&](const fs::path& target, FileWriteHookMode) -> Result<> {
    if(target.filename() == "abaqus_sects.inp" || target.filename() == "abaqus_elset.inp" || target.filename() == "abaqus.inp")
    {
      laterCallCount++;
    }
    return {};
  });

  const std::atomic_bool shouldCancel = false;
  const IFilter::MessageHandler messageHandler{[](const IFilter::Message&) {}};
  const Result<> writeResult = WriteAbaqusHexahedron(dataStructure, messageHandler, shouldCancel, &inputValues)();
  RequireErrorCode(writeResult, k_RecorderErrorCode);
  CHECK(calls == std::vector<std::string>{"nodes", "elems"});
  CHECK(laterCallCount == 0);
  // No output file and no temporary directory remain.
  CHECK(ListEntries(directory.path()).empty());
}

TEST_CASE("SimplnxCore::FileReplaceWriters: PythonPluginTemplateFile stops at a refused file", "[SimplnxCore][CreatePythonSkeletonFilter][FileWriteHook]")
{
  const ScopedWriterTestDirectory directory("python_plugin_template");
  RecordingFileReplaceHandler& recorder = ProcessRecorder();
  const RecorderDisarmGuard disarmGuard({&recorder});

  SECTION("WritePythonFilterToPlugin: a refused filter file prevents the documentation file")
  {
    const fs::path pluginSourcePath = directory.path() / "Plugin" / "src" / "Plugin";
    fs::create_directories(pluginSourcePath);
    for(const Result<>& refusal : {MakeErrorResult(k_RecorderErrorCode, "recorder error"), InvalidWithoutErrors()})
    {
      int32 callCount = 0;
      recorder.arm(pluginSourcePath / "MyFilter.py", [&callCount](const fs::path&, FileWriteHookMode) { callCount++; }, refusal, false);
      const Result<> writeResult = WritePythonFilterToPlugin(pluginSourcePath, "MyFilter", "MyFilter", false);
      SIMPLNX_RESULT_REQUIRE_INVALID(writeResult);
      CHECK(writeResult.errors().size() == refusal.errors().size());
      CHECK(callCount == 1);
      CHECK_FALSE(fs::exists(pluginSourcePath / "MyFilter.py"));
      CHECK_FALSE(fs::exists(directory.path() / "Plugin" / "docs" / "MyFilter.md"));
      CHECK(ListEntries(pluginSourcePath).empty());
    }
  }
  SECTION("WritePythonPluginFiles: a refused __init__.py stops before the later files")
  {
    const fs::path pluginRootPath = directory.path() / "MyPlugin";
    recorder.arm(pluginRootPath / "src" / "MyPlugin" / "__init__.py", {}, MakeErrorResult(k_RecorderErrorCode, "recorder error"), false);
    const Result<> writeResult = WritePythonPluginFiles(directory.path(), "MyPlugin", "MyPlugin", "Description", "FirstFilter");
    RequireErrorCode(writeResult, k_RecorderErrorCode);
    CHECK(fs::exists(pluginRootPath / "src" / "MyPlugin" / "Plugin.py"));
    CHECK_FALSE(fs::exists(pluginRootPath / "src" / "MyPlugin" / "__init__.py"));
    CHECK_FALSE(fs::exists(pluginRootPath / "environment.yml"));
    CHECK_FALSE(fs::exists(pluginRootPath / "src" / "MyPlugin" / "FirstFilter.py"));
  }
  SECTION("InsertFilterNameInPluginFiles: a refused Plugin.py leaves __init__.py unchanged")
  {
    const fs::path pluginSourcePath = directory.path() / "Existing";
    fs::create_directories(pluginSourcePath);
    WriteText(pluginSourcePath / "Plugin.py", "import simplnx as nx\n");
    WriteText(pluginSourcePath / "__init__.py", "def get_plugin():\n");
    std::vector<fs::path> calledTargets;
    recorder.arm(
        pluginSourcePath / "Plugin.py", [&calledTargets](const fs::path& target, FileWriteHookMode) { calledTargets.push_back(target); }, MakeErrorResult(k_RecorderErrorCode, "recorder error"),
        false);
    const Result<> insertResult = InsertFilterNameInPluginFiles(pluginSourcePath, "NewFilter");
    RequireErrorCode(insertResult, k_RecorderErrorCode);
    CHECK(calledTargets.size() == 1);
    CHECK(ReadText(pluginSourcePath / "Plugin.py") == "import simplnx as nx\n");
    CHECK(ReadText(pluginSourcePath / "__init__.py") == "def get_plugin():\n");
  }
}

TEST_CASE("SimplnxCore::FileReplaceWriters: WriteNodesAndElementsFiles runs the hook on both files before opening either", "[SimplnxCore][WriteNodesAndElementsFilesFilter][FileWriteHook]")
{
  const ScopedWriterTestDirectory directory("write_nodes_and_elements");
  const fs::path nodeFile = directory.path() / "nodes.txt";
  const fs::path elementFile = directory.path() / "elements.txt";
  DataStructure dataStructure = MakeEdgeStructure();
  WriteNodesAndElementsFilesInputValues inputValues;
  inputValues.SelectedGeometryPath = k_EdgeGeometryPath;
  inputValues.WriteNodeFile = true;
  inputValues.NumberNodes = false;
  inputValues.IncludeNodeFileHeader = false;
  inputValues.NodeFilePath = nodeFile;
  inputValues.WriteElementFile = true;
  inputValues.NumberElements = false;
  inputValues.IncludeElementFileHeader = false;
  inputValues.ElementFilePath = elementFile;
  const std::atomic_bool shouldCancel = false;
  const IFilter::MessageHandler messageHandler{[](const IFilter::Message&) {}};

  RecordingFileReplaceHandler& recorder = ProcessRecorder();
  RecordingFileReplaceHandler& secondRecorder = SecondRecorder();
  const RecorderDisarmGuard disarmGuard({&recorder, &secondRecorder});
  std::vector<std::string> calls;
  std::vector<bool> areFilesAbsentAtCall;
  const auto observe = [&](const std::string& name) {
    return [&, name](const fs::path&, FileWriteHookMode) {
      calls.push_back(name);
      areFilesAbsentAtCall.push_back(!fs::exists(nodeFile) && !fs::exists(elementFile));
    };
  };

  SECTION("success: one call per file before either file exists")
  {
    recorder.arm(nodeFile, observe("nodes"), MakeWarningVoidResult(k_RecorderWarningCode, "recorder warning"), false);
    secondRecorder.arm(elementFile, observe("elements"), {}, false);
    const Result<> writeResult = WriteNodesAndElementsFiles(dataStructure, messageHandler, shouldCancel, &inputValues)();
    SIMPLNX_RESULT_REQUIRE_VALID(writeResult);
    RequireRecorderWarning(writeResult);
    CHECK(calls == std::vector<std::string>{"nodes", "elements"});
    CHECK(areFilesAbsentAtCall == std::vector<bool>{true, true});
    CHECK(fs::exists(nodeFile));
    CHECK(fs::exists(elementFile));
  }
  SECTION("equal node and element paths are rejected with -139 before any hook")
  {
    inputValues.ElementFilePath = directory.path() / "." / "nodes.txt";
    recorder.arm(nodeFile, observe("nodes"), {}, false);
    const Result<> writeResult = WriteNodesAndElementsFiles(dataStructure, messageHandler, shouldCancel, &inputValues)();
    RequireErrorCode(writeResult, k_SameNodeAndElementFileCode);
    CHECK(calls.empty());
    CHECK(ListEntries(directory.path()).empty());
  }
  SECTION("paths through symbolic links are compared by the files they name")
  {
    // Creating a symbolic link can need a privilege on Windows; the section is skipped when the link cannot be created.
    fs::create_directories(directory.path() / "real" / "sub");
    std::error_code linkError;
    fs::create_directory_symlink(directory.path() / "real" / "sub", directory.path() / "link", linkError);
    if(!linkError)
    {
      fs::create_directory_symlink(directory.path() / "real", directory.path() / "alias", linkError);
    }
    if(linkError)
    {
      WARN("symbolic-link capability unavailable: " << linkError.message());
      return;
    }
    inputValues.NodeFilePath = directory.path() / "link" / ".." / "nodes.txt";
    inputValues.ElementFilePath = directory.path() / "nodes.txt";
    const Result<> linkParentResult = WriteNodesAndElementsFiles(dataStructure, messageHandler, shouldCancel, &inputValues)();
#ifdef _WIN32
    // Windows path handling removes ".." before it resolves "link", so "link/../nodes.txt" names the same file.
    RequireErrorCode(linkParentResult, k_SameNodeAndElementFileCode);
#else
    // POSIX resolves "link" first, so "link/../nodes.txt" names real/nodes.txt, a different file.
    SIMPLNX_RESULT_REQUIRE_VALID(linkParentResult);
    CHECK(fs::is_regular_file(directory.path() / "real" / "nodes.txt"));
    CHECK(fs::is_regular_file(directory.path() / "nodes.txt"));
#endif

    // Two absent files reached through a linked folder and its real folder are one file.
    inputValues.NodeFilePath = directory.path() / "real" / "new.txt";
    inputValues.ElementFilePath = directory.path() / "alias" / "new.txt";
    const Result<> sameResult = WriteNodesAndElementsFiles(dataStructure, messageHandler, shouldCancel, &inputValues)();
    RequireErrorCode(sameResult, k_SameNodeAndElementFileCode);
    CHECK_FALSE(fs::exists(directory.path() / "real" / "new.txt"));
  }
  SECTION("a refused element file prevents the node file")
  {
    recorder.arm(nodeFile, observe("nodes"), {}, false);
    secondRecorder.arm(elementFile, observe("elements"), MakeErrorResult(k_RecorderErrorCode, "recorder error"), false);
    const Result<> writeResult = WriteNodesAndElementsFiles(dataStructure, messageHandler, shouldCancel, &inputValues)();
    RequireErrorCode(writeResult, k_RecorderErrorCode);
    CHECK(calls == std::vector<std::string>{"nodes", "elements"});
    CHECK(ListEntries(directory.path()).empty());
  }
}

TEST_CASE("SimplnxCore::FileReplaceWriters: ExecuteProcess runs the hook on the log file before it opens it", "[SimplnxCore][ExecuteProcessFilter][FileWriteHook]")
{
  const ScopedWriterTestDirectory directory("execute_process_log");
  const fs::path logFile = directory.path() / "process.log";
  WriteText(logFile, "previous log");
  DataStructure dataStructure;
  ExecuteProcessInputValues inputValues;
  inputValues.Arguments = "a-program-that-must-not-start";
  inputValues.Blocking = true;
  inputValues.Timeout = 1000;
  inputValues.LogFile = logFile;
  const std::atomic_bool shouldCancel = false;
  const IFilter::MessageHandler messageHandler{[](const IFilter::Message&) {}};

  RecordingFileReplaceHandler& recorder = ProcessRecorder();
  const RecorderDisarmGuard disarmGuard({&recorder});
  int32 callCount = 0;
  bool isLogUnchangedAtCall = false;
  recorder.arm(
      logFile,
      [&](const fs::path&, FileWriteHookMode) {
        callCount++;
        isLogUnchangedAtCall = ReadText(logFile) == "previous log";
      },
      MakeErrorResult(k_RecorderErrorCode, "recorder error"), false);
  const Result<> executeResult = ExecuteProcess(dataStructure, messageHandler, shouldCancel, &inputValues)();
  RequireErrorCode(executeResult, k_RecorderErrorCode);
  CHECK(callCount == 1);
  CHECK(isLogUnchangedAtCall);
  CHECK(ReadText(logFile) == "previous log");
}

TEST_CASE("SimplnxCore::FileReplaceWriters: the hook warning reaches the result of a writer filter", "[SimplnxCore][WriteVtkStructuredPointsFilter][FileWriteHook]")
{
  UnitTest::LoadPlugins();
  const ScopedWriterTestDirectory directory("writer_filter_warning");
  const fs::path outputFile = directory.path() / "output.vtk";
  DataStructure dataStructure = MakeImageStructure();
  WriteVtkStructuredPointsFilter filter;
  Arguments args;
  args.insertOrAssign(WriteVtkStructuredPointsFilter::k_OutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(outputFile));
  args.insertOrAssign(WriteVtkStructuredPointsFilter::k_WriteBinaryFile_Key, std::make_any<bool>(false));
  args.insertOrAssign(WriteVtkStructuredPointsFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeometryPath));
  args.insertOrAssign(WriteVtkStructuredPointsFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{k_FeatureIdsPath}));

  RecordingFileReplaceHandler& recorder = ProcessRecorder();
  const RecorderDisarmGuard disarmGuard({&recorder});
  int32 callCount = 0;
  recorder.arm(outputFile, [&callCount](const fs::path&, FileWriteHookMode) { callCount++; }, MakeWarningVoidResult(k_RecorderWarningCode, "recorder warning"), false);

  const auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  // Preflight does not run the hook.
  CHECK(callCount == 0);
  const auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  CHECK(callCount == 1);
  RequireRecorderWarning(executeResult.result);
  CHECK(fs::exists(outputFile));
}
