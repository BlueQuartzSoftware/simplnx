#include "SimplnxCore/utils/AvizoWriter.hpp"
#include "SimplnxCore/Filters/Algorithms/WriteAvizoRectilinearCoordinate.hpp"
#include "SimplnxCore/Filters/Algorithms/WriteAvizoUniformCoordinate.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <filesystem>
#include <limits>
#include <memory>
#include <new>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>

using namespace nx::core;

namespace
{
/**
 * @class ObservedAvizoStore
 * @brief Counts source windows and supplies named read diagnostics.
 */
class ObservedAvizoStore : public DataStore<int32>
{
public:
  /**
   * @brief Initializes one-component feature IDs to seven.
   * @param tupleCount Specifies the geometry's X dimension.
   */
  explicit ObservedAvizoStore(usize tupleCount)
  : DataStore<int32>(ShapeType{1, 1, tupleCount}, ShapeType{1}, std::optional<int32>{7})
  {
  }

  /**
   * @brief Copies a source window or reports the selected read failure.
   * @param startIndex Specifies the first source element.
   * @param buffer Receives the feature IDs.
   * @return Source data status with the configured warnings or error.
   */
  Result<> copyIntoBuffer(usize startIndex, nonstd::span<int32> buffer) const override
  {
    ++readCalls;
    if(failReadAt == readCalls)
    {
      auto result = MakeErrorResult(-92561, "Injected Avizo second-block read failure");
      result.warnings().push_back(Warning{-92574, "Source failure warning"});
      return result;
    }
    auto result = DataStore<int32>::copyIntoBuffer(startIndex, buffer);
    if(result.valid() && warnOnRead)
    {
      result.warnings().push_back(Warning{-92573, "Successful source read warning"});
    }
    return result;
  }

  usize failReadAt = 0;
  bool warnOnRead = false;
  mutable usize readCalls = 0;
};

/**
 * @struct AvizoFixture
 * @brief Owns the source data and constructor references for one writer.
 */
struct AvizoFixture
{
  /**
   * @brief Creates the geometry, source store, and private output directory.
   * @param tupleCount Specifies the geometry's X dimension.
   * @param binary Selects the output encoding.
   */
  explicit AvizoFixture(usize tupleCount, bool binary)
  {
    const auto outputBase = std::filesystem::path(unit_test::k_BinaryTestOutputDir.view());
    std::filesystem::create_directories(outputBase);
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    outputRoot = outputBase / ("AvizoStreamFailure-" + std::to_string(stamp));
    REQUIRE(std::filesystem::create_directory(outputRoot));
    values.OutputFile = outputRoot / "output.am";
    values.WriteBinaryFile = binary;
    values.GeometryPath = DataPath({"ImageGeom"});
    values.FeatureIdsArrayPath = DataPath({"ImageGeom", "CellData", "FeatureIds"});
    values.Units = "microns";
    auto* geom = ImageGeom::Create(dataStructure, "ImageGeom");
    REQUIRE(geom != nullptr);
    geom->setDimensions({tupleCount, 1, 1});
    geom->setOrigin({0.0F, 0.0F, 0.0F});
    geom->setSpacing({1.0F, 1.0F, 1.0F});
    auto* cells = AttributeMatrix::Create(dataStructure, "CellData", {1, 1, tupleCount}, geom->getId());
    REQUIRE(cells != nullptr);
    geom->setCellData(*cells);
    source = std::make_shared<ObservedAvizoStore>(tupleCount);
    REQUIRE(Int32Array::Create(dataStructure, "FeatureIds", source, cells->getId()) != nullptr);
  }

  /**
   * @brief Removes this fixture's output files after each writer is destroyed.
   */
  ~AvizoFixture() noexcept
  {
    std::error_code ignored;
    std::filesystem::remove_all(outputRoot, ignored);
  }

  std::filesystem::path outputRoot;
  DataStructure dataStructure;
  std::shared_ptr<ObservedAvizoStore> source;
  IFilter::MessageHandler handler{[](const IFilter::Message&) {}};
  std::atomic_bool shouldCancel = false;
  AvizoWriterInputValues values;
};

/**
 * @class FaultAvizoWriter
 * @brief Supplies per-instance test operations without changing normal stdio dispatch.
 * @tparam WriterType Supplies the real algorithm or the stage test writer.
 *
 * The bundle and its context outlive synchronous execute(). The fixture cannot move.
 * Each close callback consumes its real stream before it reports an injected error.
 */
template <class WriterType>
class FaultAvizoWriter : public WriterType
{
public:
  /**
   * @brief Installs the test-owned operations for this writer only.
   * @param dataStructure Owns the image geometry and feature IDs.
   * @param handler Receives the writer's messages.
   * @param shouldCancel Signals cancellation between windows.
   * @param values Specifies output encoding, paths, geometry, and units.
   */
  FaultAvizoWriter(DataStructure& dataStructure, const IFilter::MessageHandler& handler, const std::atomic_bool& shouldCancel, AvizoWriterInputValues* values)
  : WriterType(dataStructure, handler, shouldCancel, values)
  , m_Operations{this, &Write, &Print, &Flush, &Close}
  {
    this->setOutputOperations(&m_Operations);
  }

  /**
   * @brief Removes the bundle pointer before the bundle's lifetime ends.
   */
  ~FaultAvizoWriter() noexcept override
  {
    this->setOutputOperations(nullptr);
  }

  FaultAvizoWriter(const FaultAvizoWriter&) = delete;
  FaultAvizoWriter(FaultAvizoWriter&&) = delete;
  FaultAvizoWriter& operator=(const FaultAvizoWriter&) = delete;
  FaultAvizoWriter& operator=(FaultAvizoWriter&&) = delete;

  usize failWriteAt = 0;
  usize failPrintAt = 0;
  std::string_view printFormat;
  bool throwFromPrint = false;
  bool failFlush = false;
  bool failClose = false;
  bool throwFromClose = false;
  bool rejectEveryWrite = false;
  usize writeCalls = 0;
  usize printCalls = 0;
  usize matchingPrintCalls = 0;
  usize flushCalls = 0;
  usize closeCalls = 0;
  usize observedElementSize = 0;
  usize observedRequestedElements = 0;
  usize observedWrittenElements = 0;
  int nativeCloseStatus = EOF;

private:
  static usize Write(void* context, FILE* stream, const void* data, usize elementSize, usize count)
  {
    auto& self = *static_cast<FaultAvizoWriter*>(context);
    ++self.writeCalls;
    if(self.rejectEveryWrite)
    {
      // The overflow fixture never transfers its deliberately rejected byte count.
      return 0;
    }
    if(self.writeCalls == self.failWriteAt)
    {
      self.observedElementSize = elementSize;
      self.observedRequestedElements = count;
      self.observedWrittenElements = std::fwrite(data, elementSize, count == 0 ? 0 : count - 1, stream);
      errno = ENOSPC;
      return self.observedWrittenElements;
    }
    return std::fwrite(data, elementSize, count, stream);
  }

  static int Print(void* context, FILE* stream, const char* format, std::va_list args)
  {
    auto& self = *static_cast<FaultAvizoWriter*>(context);
    ++self.printCalls;
    if(self.printFormat.empty() || self.printFormat == format)
    {
      ++self.matchingPrintCalls;
      if(self.matchingPrintCalls == self.failPrintAt)
      {
        if(self.throwFromPrint)
        {
          throw std::runtime_error("Injected variadic print failure");
        }
        errno = ENOSPC;
        return -1;
      }
    }
    // The callback consumes the caller's va_list once and does not retain it.
    return std::vfprintf(stream, format, args);
  }

  static int Flush(void* context, FILE* stream)
  {
    auto& self = *static_cast<FaultAvizoWriter*>(context);
    ++self.flushCalls;
    if(self.failFlush)
    {
      errno = EIO;
      return EOF;
    }
    return std::fflush(stream);
  }

  static int Close(void* context, FILE* stream)
  {
    auto& self = *static_cast<FaultAvizoWriter*>(context);
    ++self.closeCalls;
    if(self.closeCalls != 1)
    {
      // A broken owner must produce a count failure without a second access to a closed stream.
      errno = EIO;
      return EOF;
    }
    self.nativeCloseStatus = std::fclose(stream);
    if(self.throwFromClose)
    {
      throw std::runtime_error("Injected consumed-stream close failure");
    }
    if(self.failClose)
    {
      errno = EIO;
      return EOF;
    }
    return self.nativeCloseStatus;
  }

  typename WriterType::OutputOperations m_Operations;
};
/**
 * @enum StageFault
 * @brief Selects a Result or exception at an execution stage.
 */
enum class StageFault
{
  None,             ///< Both stages return warnings and success.
  HeaderResult,     ///< Header returns the named error.
  DataResult,       ///< Data returns the named error.
  HeaderStandard,   ///< Header throws a standard exception.
  DataStandard,     ///< Data throws a standard exception.
  HeaderUnknown,    ///< Header throws a nonstandard exception.
  DataUnknown,      ///< Data throws a nonstandard exception.
  HeaderAllocation, ///< Header throws an allocation exception.
  DataAllocation    ///< Data throws an allocation exception.
};

/**
 * @class StageAvizoWriter
 * @brief Supplies stage outcomes without adding production fault controls.
 */
class StageAvizoWriter : public AvizoWriter
{
public:
  using AvizoWriter::AvizoWriter;
  StageFault fault = StageFault::None;
  mutable usize headerCalls = 0;
  mutable usize dataCalls = 0;

protected:
  Result<> generateHeader(FILE*) const override
  {
    ++headerCalls;
    if(fault == StageFault::HeaderStandard)
    {
      throw std::runtime_error("Injected header exception");
    }
    if(fault == StageFault::HeaderUnknown)
    {
      throw 17;
    }
    if(fault == StageFault::HeaderAllocation)
    {
      throw std::bad_alloc{};
    }
    auto result = fault == StageFault::HeaderResult ? MakeErrorResult(-92562, "Injected header Result failure") : Result<>{};
    result.warnings().push_back(Warning{-92571, "Header stage warning"});
    return result;
  }

  Result<> writeData(FILE*) const override
  {
    ++dataCalls;
    if(fault == StageFault::DataStandard)
    {
      throw std::runtime_error("Injected data exception");
    }
    if(fault == StageFault::DataUnknown)
    {
      throw 18;
    }
    if(fault == StageFault::DataAllocation)
    {
      throw std::bad_alloc{};
    }
    auto result = fault == StageFault::DataResult ? MakeErrorResult(-92563, "Injected data Result failure") : Result<>{};
    result.warnings().push_back(Warning{-92572, "Data stage warning"});
    return result;
  }
};

class OverflowAvizoWriter : public AvizoWriter
{
public:
  using AvizoWriter::AvizoWriter;
  usize elementSize = 2;
  usize elementCount = std::numeric_limits<usize>::max();

protected:
  Result<> generateHeader(FILE*) const override
  {
    return {};
  }

  Result<> writeData(FILE* outputFile) const override
  {
    const char payload = 0;
    return writeOutputChecked(outputFile, &payload, elementSize, elementCount);
  }
};

template <class WriterType>
void CheckClosedOnce(const WriterType& writer, const AvizoFixture& fixture)
{
  CHECK(writer.closeCalls == 1);
  CHECK(writer.nativeCloseStatus == 0);
  std::error_code error;
  CHECK(std::filesystem::remove(fixture.values.OutputFile, error));
  CHECK_FALSE(error);
  UnitTest::CheckArraysInheritTupleDims(fixture.dataStructure);
}

void CheckOutputFailure(const Result<>& result, const AvizoFixture& fixture)
{
  REQUIRE(result.invalid());
  REQUIRE_FALSE(result.errors().empty());
  CHECK(result.errors().front().message.find(fixture.values.OutputFile.string()) != std::string::npos);
}

template <class AlgorithmType>
void CheckBinaryWindowFailure(usize failedWindow, bool closeAlsoFails)
{
  AvizoFixture fixture(131073, true);
  FaultAvizoWriter<AlgorithmType> writer(fixture.dataStructure, fixture.handler, fixture.shouldCancel, &fixture.values);
  writer.failWriteAt = failedWindow;
  writer.failClose = closeAlsoFails;
  const auto result = writer.execute();
  CheckOutputFailure(result, fixture);
  REQUIRE(result.invalid());
  REQUIRE(result.errors().size() == (closeAlsoFails ? usize{2} : usize{1}));
  const auto& message = result.errors().front().message;
  CHECK(message.find(std::to_string(writer.observedRequestedElements)) != std::string::npos);
  CHECK(message.find(std::to_string(writer.observedWrittenElements)) != std::string::npos);
  CHECK(message.find(std::to_string(writer.observedElementSize)) != std::string::npos);
  CHECK(message.find(std::to_string(writer.observedRequestedElements * writer.observedElementSize)) != std::string::npos);
  CHECK(message.find(std::to_string(writer.observedWrittenElements * writer.observedElementSize)) != std::string::npos);
  CHECK(fixture.source->readCalls == failedWindow);
  CHECK(writer.writeCalls == failedWindow);
  CHECK(writer.flushCalls == 0);
  if(closeAlsoFails)
  {
    CHECK(result.errors().back().message.find(fixture.values.OutputFile.string()) != std::string::npos);
    CHECK(result.errors().back().message.find("close") != std::string::npos);
  }
  CheckClosedOnce(writer, fixture);
}

template <class AlgorithmType>
void CheckTextFailure(std::string_view format, bool binary, usize expectedReads)
{
  AvizoFixture fixture(65537, binary);
  FaultAvizoWriter<AlgorithmType> writer(fixture.dataStructure, fixture.handler, fixture.shouldCancel, &fixture.values);
  writer.printFormat = format;
  writer.failPrintAt = 1;
  const auto result = writer.execute();
  CheckOutputFailure(result, fixture);
  CHECK(writer.matchingPrintCalls == 1);
  CHECK(fixture.source->readCalls == expectedReads);
  CHECK(writer.flushCalls == 0);
  CheckClosedOnce(writer, fixture);
}

template <class AlgorithmType>
void CheckSourceWarnings(bool readFails, bool closeFails)
{
  AvizoFixture fixture(65537, true);
  fixture.source->failReadAt = readFails ? 2 : 0;
  fixture.source->warnOnRead = true;
  FaultAvizoWriter<AlgorithmType> writer(fixture.dataStructure, fixture.handler, fixture.shouldCancel, &fixture.values);
  writer.failClose = closeFails;
  const auto result = writer.execute();
  CHECK(result.valid() == (!readFails && !closeFails));
  if(readFails || closeFails)
  {
    REQUIRE(result.invalid());
    REQUIRE(result.errors().size() == static_cast<usize>(readFails) + static_cast<usize>(closeFails));
    if(readFails)
    {
      CHECK(result.errors().front().code == -92561);
    }
    if(closeFails)
    {
      CHECK(result.errors().back().message.find("close") != std::string::npos);
    }
  }
  REQUIRE(result.warnings().size() == 2);
  CHECK(result.warnings()[0].code == -92573);
  CHECK(result.warnings()[1].code == (readFails ? -92574 : -92573));
  CHECK(fixture.source->readCalls == 2);
  CHECK(writer.writeCalls == (readFails ? usize{1} : (std::is_same_v<AlgorithmType, WriteAvizoUniformCoordinate> ? usize{2} : usize{5})));
  CHECK(writer.flushCalls == (readFails ? usize{0} : usize{1}));
  CheckClosedOnce(writer, fixture);
}
} // namespace

TEST_CASE("SimplnxCore::AvizoWriter: Short binary feature writes stop later reads", "[SimplnxCore][AvizoWriter]")
{
  UnitTest::LoadPlugins();
  const usize failedWindow = GENERATE(usize{1}, usize{2}, usize{3});
  const bool closeAlsoFails = GENERATE(false, true);
  SECTION("Uniform")
  {
    CheckBinaryWindowFailure<WriteAvizoUniformCoordinate>(failedWindow, closeAlsoFails);
  }
  SECTION("Rectilinear")
  {
    CheckBinaryWindowFailure<WriteAvizoRectilinearCoordinate>(failedWindow, closeAlsoFails);
  }
}

TEST_CASE("SimplnxCore::AvizoWriter: Text failures stop production output", "[SimplnxCore][AvizoWriter]")
{
  UnitTest::LoadPlugins();
  const auto item = GENERATE(std::pair<std::string_view, usize>{"# AmiraMesh 3D ASCII 2.0\n", 0}, std::pair<std::string_view, usize>{"%d", 1}, std::pair<std::string_view, usize>{" ", 1});
  SECTION("Uniform")
  {
    CheckTextFailure<WriteAvizoUniformCoordinate>(item.first, false, item.second);
  }
  SECTION("Rectilinear")
  {
    CheckTextFailure<WriteAvizoRectilinearCoordinate>(item.first, false, item.second);
  }
}

TEST_CASE("SimplnxCore::AvizoWriter: Final feature newline is checked", "[SimplnxCore][AvizoWriter]")
{
  UnitTest::LoadPlugins();
  const bool rectilinear = GENERATE(false, true);
  const bool writeBinary = GENERATE(false, true);
  AvizoFixture fixture(1, writeBinary);
  // Both production headers contain one standalone newline before the data newline.
  if(rectilinear)
  {
    FaultAvizoWriter<WriteAvizoRectilinearCoordinate> writer(fixture.dataStructure, fixture.handler, fixture.shouldCancel, &fixture.values);
    writer.printFormat = "\n";
    writer.failPrintAt = 2;
    const auto result = writer.execute();
    CheckOutputFailure(result, fixture);
    CHECK(writer.writeCalls == (writeBinary ? usize{1} : usize{0}));
    CHECK(writer.matchingPrintCalls == 2);
    CheckClosedOnce(writer, fixture);
  }
  else
  {
    FaultAvizoWriter<WriteAvizoUniformCoordinate> writer(fixture.dataStructure, fixture.handler, fixture.shouldCancel, &fixture.values);
    writer.printFormat = "\n";
    writer.failPrintAt = 2;
    const auto result = writer.execute();
    CheckOutputFailure(result, fixture);
    CHECK(writer.writeCalls == (writeBinary ? usize{1} : usize{0}));
    CHECK(writer.matchingPrintCalls == 2);
    CheckClosedOnce(writer, fixture);
  }
  CHECK(fixture.source->readCalls == 1);
}

TEST_CASE("SimplnxCore::AvizoWriter: Rectilinear coordinate failures stop later axes", "[SimplnxCore][AvizoWriter]")
{
  UnitTest::LoadPlugins();
  SECTION("Coordinate delimiter")
  {
    CheckTextFailure<WriteAvizoRectilinearCoordinate>("@2 # x coordinates, then y, then z\n", true, 2);
  }
  SECTION("Binary coordinate")
  {
    const usize failedWrite = GENERATE(usize{2}, usize{3}, usize{4});
    AvizoFixture fixture(8, true);
    FaultAvizoWriter<WriteAvizoRectilinearCoordinate> writer(fixture.dataStructure, fixture.handler, fixture.shouldCancel, &fixture.values);
    writer.failWriteAt = failedWrite;
    const auto result = writer.execute();
    CheckOutputFailure(result, fixture);
    CHECK(writer.writeCalls == failedWrite);
    CHECK(fixture.source->readCalls == 1);
    CHECK(writer.flushCalls == 0);
    CheckClosedOnce(writer, fixture);
  }
  SECTION("Axis newline")
  {
    const usize failedAxis = GENERATE(usize{1}, usize{2}, usize{3});
    const bool writeBinary = GENERATE(false, true);
    AvizoFixture fixture(8, writeBinary);
    FaultAvizoWriter<WriteAvizoRectilinearCoordinate> writer(fixture.dataStructure, fixture.handler, fixture.shouldCancel, &fixture.values);
    writer.printFormat = "\n";
    writer.failPrintAt = 2 + failedAxis;
    const auto result = writer.execute();
    CheckOutputFailure(result, fixture);
    CHECK(writer.writeCalls == (writeBinary ? 1 + failedAxis : usize{0}));
    CHECK(writer.matchingPrintCalls == 2 + failedAxis);
    CheckClosedOnce(writer, fixture);
  }
  SECTION("ASCII coordinate")
  {
    CheckTextFailure<WriteAvizoRectilinearCoordinate>("%f ", false, 2);
  }
}

TEST_CASE("SimplnxCore::AvizoWriter: Flush and consuming close failures are checked", "[SimplnxCore][AvizoWriter]")
{
  UnitTest::LoadPlugins();
  const bool flushFails = GENERATE(false, true);
  const bool closeFails = GENERATE(false, true);
  AvizoFixture fixture(1, true);
  FaultAvizoWriter<StageAvizoWriter> writer(fixture.dataStructure, fixture.handler, fixture.shouldCancel, &fixture.values);
  writer.failFlush = flushFails;
  writer.failClose = closeFails;
  const auto result = writer.execute();
  CHECK(writer.headerCalls == 1);
  CHECK(writer.dataCalls == 1);
  CHECK(writer.flushCalls == 1);
  CHECK(result.valid() == (!flushFails && !closeFails));
  if(flushFails || closeFails)
  {
    CheckOutputFailure(result, fixture);
    REQUIRE(result.invalid());
    REQUIRE(result.errors().size() == static_cast<usize>(flushFails) + static_cast<usize>(closeFails));
    CHECK(result.errors().front().message.find(flushFails ? "flush" : "close") != std::string::npos);
  }
  REQUIRE(result.warnings().size() == 2);
  CHECK(result.warnings()[0].code == -92571);
  CHECK(result.warnings()[1].code == -92572);
  CheckClosedOnce(writer, fixture);
}

TEST_CASE("SimplnxCore::AvizoWriter: Stage Result errors precede consuming close errors", "[SimplnxCore][AvizoWriter]")
{
  UnitTest::LoadPlugins();
  const bool headerFails = GENERATE(false, true);
  AvizoFixture fixture(1, true);
  FaultAvizoWriter<StageAvizoWriter> writer(fixture.dataStructure, fixture.handler, fixture.shouldCancel, &fixture.values);
  writer.fault = headerFails ? StageFault::HeaderResult : StageFault::DataResult;
  writer.failClose = true;
  const auto result = writer.execute();
  REQUIRE(result.invalid());
  REQUIRE(result.errors().size() == 2);
  CHECK(result.errors()[0].code == (headerFails ? -92562 : -92563));
  CHECK(result.errors()[1].message.find(fixture.values.OutputFile.string()) != std::string::npos);
  CHECK(result.errors()[1].message.find("close") != std::string::npos);
  CHECK(writer.dataCalls == (headerFails ? usize{0} : usize{1}));
  CHECK(writer.flushCalls == 0);
  REQUIRE(result.warnings().size() == (headerFails ? usize{1} : usize{2}));
  CHECK(result.warnings().front().code == -92571);
  CheckClosedOnce(writer, fixture);
}

TEST_CASE("SimplnxCore::AvizoWriter: Stage exceptions close once", "[SimplnxCore][AvizoWriter]")
{
  UnitTest::LoadPlugins();
  const auto fault = GENERATE(StageFault::HeaderStandard, StageFault::DataStandard, StageFault::HeaderUnknown, StageFault::DataUnknown);
  const bool closeFails = GENERATE(false, true);
  AvizoFixture fixture(1, true);
  FaultAvizoWriter<StageAvizoWriter> writer(fixture.dataStructure, fixture.handler, fixture.shouldCancel, &fixture.values);
  writer.fault = fault;
  writer.failClose = closeFails;
  const auto result = writer.execute();
  CheckOutputFailure(result, fixture);
  REQUIRE(result.invalid());
  CHECK(result.errors().size() == (closeFails ? usize{2} : usize{1}));
  const bool dataStage = fault == StageFault::DataStandard || fault == StageFault::DataUnknown;
  CHECK(result.errors().front().message.find(dataStage ? "data" : "header") != std::string::npos);
  CHECK(writer.dataCalls == (dataStage ? usize{1} : usize{0}));
  CHECK(writer.flushCalls == 0);
  CHECK(result.warnings().size() == (dataStage ? usize{1} : usize{0}));
  CheckClosedOnce(writer, fixture);
}

TEST_CASE("SimplnxCore::AvizoWriter: Allocation exceptions preserve their type after cleanup", "[SimplnxCore][AvizoWriter]")
{
  UnitTest::LoadPlugins();
  const auto fault = GENERATE(StageFault::HeaderAllocation, StageFault::DataAllocation);
  const bool closeFails = GENERATE(false, true);
  AvizoFixture fixture(1, true);
  FaultAvizoWriter<StageAvizoWriter> writer(fixture.dataStructure, fixture.handler, fixture.shouldCancel, &fixture.values);
  writer.fault = fault;
  writer.failClose = closeFails;
  CHECK_THROWS_AS(writer.execute(), std::bad_alloc);
  CHECK(writer.flushCalls == 0);
  CheckClosedOnce(writer, fixture);
}

TEST_CASE("SimplnxCore::AvizoWriter: Callback print exceptions and close exceptions retain ownership", "[SimplnxCore][AvizoWriter]")
{
  UnitTest::LoadPlugins();
  const bool printThrows = GENERATE(false, true);
  AvizoFixture fixture(1, true);
  FaultAvizoWriter<WriteAvizoUniformCoordinate> writer(fixture.dataStructure, fixture.handler, fixture.shouldCancel, &fixture.values);
  writer.failPrintAt = printThrows ? 1 : 0;
  writer.throwFromPrint = printThrows;
  writer.throwFromClose = !printThrows;
  const auto result = writer.execute();
  CheckOutputFailure(result, fixture);
  CHECK(fixture.source->readCalls == (printThrows ? usize{0} : usize{1}));
  CheckClosedOnce(writer, fixture);
}

TEST_CASE("SimplnxCore::AvizoWriter: Source warnings survive a later read error and close error", "[SimplnxCore][AvizoWriter]")
{
  UnitTest::LoadPlugins();
  const bool readFails = GENERATE(false, true);
  const bool closeFails = GENERATE(false, true);
  SECTION("Uniform")
  {
    CheckSourceWarnings<WriteAvizoUniformCoordinate>(readFails, closeFails);
  }
  SECTION("Rectilinear")
  {
    CheckSourceWarnings<WriteAvizoRectilinearCoordinate>(readFails, closeFails);
  }
}

TEST_CASE("SimplnxCore::AvizoWriter: Faults remain local to one writer", "[SimplnxCore][AvizoWriter]")
{
  UnitTest::LoadPlugins();
  AvizoFixture failed(1, true);
  AvizoFixture healthy(1, true);
  FaultAvizoWriter<WriteAvizoUniformCoordinate> first(failed.dataStructure, failed.handler, failed.shouldCancel, &failed.values);
  FaultAvizoWriter<WriteAvizoUniformCoordinate> second(healthy.dataStructure, healthy.handler, healthy.shouldCancel, &healthy.values);
  first.failWriteAt = 1;
  const auto firstResult = first.execute();
  const auto secondResult = second.execute();
  REQUIRE(firstResult.invalid());
  REQUIRE(secondResult.valid());
  CHECK(second.writeCalls == 1);
  CHECK(second.flushCalls == 1);
  CheckClosedOnce(first, failed);
  CheckClosedOnce(second, healthy);
}

TEST_CASE("SimplnxCore::AvizoWriter: Byte-count overflow stops before the transfer operation", "[SimplnxCore][AvizoWriter]")
{
  UnitTest::LoadPlugins();
  const bool reverseOperands = GENERATE(false, true);
  AvizoFixture fixture(1, true);
  FaultAvizoWriter<OverflowAvizoWriter> writer(fixture.dataStructure, fixture.handler, fixture.shouldCancel, &fixture.values);
  if(reverseOperands)
  {
    std::swap(writer.elementSize, writer.elementCount);
  }
  // The test operation reports any call without accessing the one-byte source.
  writer.rejectEveryWrite = true;
  const auto result = writer.execute();
  CheckOutputFailure(result, fixture);
  REQUIRE(result.invalid());
  CHECK(result.errors().front().message.find(std::to_string(std::numeric_limits<usize>::max())) != std::string::npos);
  CHECK(writer.writeCalls == 0);
  CHECK(writer.flushCalls == 0);
  CheckClosedOnce(writer, fixture);
}
