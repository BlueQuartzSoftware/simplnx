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

  ~ObservedAvizoStore() override = default;
  ObservedAvizoStore(const ObservedAvizoStore&) = default;
  ObservedAvizoStore(ObservedAvizoStore&&) = default;
  ObservedAvizoStore& operator=(const ObservedAvizoStore&) = delete;
  ObservedAvizoStore& operator=(ObservedAvizoStore&&) noexcept = default;

  /**
   * @brief Copies a source window or reports the selected read failure.
   * @param startIndex Specifies the first source element.
   * @param buffer Receives the feature IDs.
   * @return Source data status with the configured warnings or error.
   */
  Result<> copyIntoBuffer(usize startIndex, nonstd::span<int32> buffer) const override
  {
    ++read_calls;
    if(fail_read_at == read_calls)
    {
      auto result = MakeErrorResult(-92561, "Injected Avizo second-block read failure");
      result.warnings().push_back(Warning{.code = -92574, .message = "Source failure warning"});
      return result;
    }
    auto result = DataStore<int32>::copyIntoBuffer(startIndex, buffer);
    if(result.valid() && warn_on_read)
    {
      result.warnings().push_back(Warning{.code = -92573, .message = "Successful source read warning"});
    }
    return result;
  }

  usize fail_read_at = 0;
  bool warn_on_read = false;
  mutable usize read_calls = 0;
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
    output_root = outputBase / ("AvizoStreamFailure-" + std::to_string(stamp));
    REQUIRE(std::filesystem::create_directory(output_root));
    values.output_file = output_root / "output.am";
    values.write_binary_file = binary;
    values.geometry_path = DataPath({"ImageGeom"});
    values.feature_ids_array_path = DataPath({"ImageGeom", "CellData", "FeatureIds"});
    values.units = "microns";
    auto* geomPtr = ImageGeom::Create(data_structure, "ImageGeom");
    REQUIRE(geomPtr != nullptr);
    geomPtr->setDimensions({tupleCount, 1, 1});
    REQUIRE(geomPtr->setOrigin({0.0F, 0.0F, 0.0F}).valid());
    REQUIRE(geomPtr->setSpacing({1.0F, 1.0F, 1.0F}).valid());
    auto* cellsPtr = AttributeMatrix::Create(data_structure, "CellData", {1, 1, tupleCount}, geomPtr->getId());
    REQUIRE(cellsPtr != nullptr);
    geomPtr->setCellData(*cellsPtr);
    source = std::make_shared<ObservedAvizoStore>(tupleCount);
    REQUIRE(Int32ArrayType::Create(data_structure, "FeatureIds", source, cellsPtr->getId()) != nullptr);
  }

  /**
   * @brief Removes this fixture's output files after each writer is destroyed.
   */
  ~AvizoFixture() noexcept
  {
    std::error_code ignored;
    std::filesystem::remove_all(output_root, ignored);
  }

  AvizoFixture(const AvizoFixture&) = delete;
  AvizoFixture(AvizoFixture&&) = delete;
  AvizoFixture& operator=(const AvizoFixture&) = delete;
  AvizoFixture& operator=(AvizoFixture&&) = delete;

  std::filesystem::path output_root;
  DataStructure data_structure;
  std::shared_ptr<ObservedAvizoStore> source;
  IFilter::MessageHandler handler{[](const IFilter::Message&) {}};
  std::atomic_bool should_cancel = false;
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

  usize fail_write_at = 0;
  usize fail_print_at = 0;
  std::string_view print_format;
  bool throw_from_print = false;
  bool fail_flush = false;
  bool fail_close = false;
  bool throw_from_close = false;
  bool reject_every_write = false;
  usize write_calls = 0;
  usize print_calls = 0;
  usize matching_print_calls = 0;
  usize flush_calls = 0;
  usize close_calls = 0;
  usize observed_element_size = 0;
  usize observed_requested_elements = 0;
  usize observed_written_elements = 0;
  int native_close_status = EOF;

private:
  static usize Write(void* context, FILE* stream, const void* data, usize writeElementSize, usize count)
  {
    auto& self = *static_cast<FaultAvizoWriter*>(context);
    ++self.write_calls;
    if(self.reject_every_write)
    {
      // The overflow fixture never transfers its deliberately rejected byte count.
      return 0;
    }
    if(self.write_calls == self.fail_write_at)
    {
      self.observed_element_size = writeElementSize;
      self.observed_requested_elements = count;
      self.observed_written_elements = std::fwrite(data, writeElementSize, count == 0 ? 0 : count - 1, stream);
      errno = ENOSPC;
      return self.observed_written_elements;
    }
    return std::fwrite(data, writeElementSize, count, stream);
  }

  static int Print(void* context, FILE* stream, const char* format, std::va_list args)
  {
    auto& self = *static_cast<FaultAvizoWriter*>(context);
    ++self.print_calls;
    if(self.print_format.empty() || self.print_format == format)
    {
      ++self.matching_print_calls;
      if(self.matching_print_calls == self.fail_print_at)
      {
        if(self.throw_from_print)
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
    ++self.flush_calls;
    if(self.fail_flush)
    {
      errno = EIO;
      return EOF;
    }
    return std::fflush(stream);
  }

  static int Close(void* context, FILE* stream)
  {
    auto& self = *static_cast<FaultAvizoWriter*>(context);
    ++self.close_calls;
    if(self.close_calls != 1)
    {
      // A broken owner must produce a count failure without a second access to a closed stream.
      errno = EIO;
      return EOF;
    }
    self.native_close_status = std::fclose(stream);
    if(self.throw_from_close)
    {
      throw std::runtime_error("Injected consumed-stream close failure");
    }
    if(self.fail_close)
    {
      errno = EIO;
      return EOF;
    }
    return self.native_close_status;
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

  ~StageAvizoWriter() override = default;
  StageAvizoWriter(const StageAvizoWriter&) = delete;
  StageAvizoWriter(StageAvizoWriter&&) = delete;
  StageAvizoWriter& operator=(const StageAvizoWriter&) = delete;
  StageAvizoWriter& operator=(StageAvizoWriter&&) = delete;

  StageFault fault = StageFault::None;
  mutable usize header_calls = 0;
  mutable usize data_calls = 0;

protected:
  Result<> generateHeader(FILE* /*outputFile*/) const override
  {
    ++header_calls;
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
    result.warnings().push_back(Warning{.code = -92571, .message = "Header stage warning"});
    return result;
  }

  Result<> writeData(FILE* /*outputFile*/) const override
  {
    ++data_calls;
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
    result.warnings().push_back(Warning{.code = -92572, .message = "Data stage warning"});
    return result;
  }
};

class OverflowAvizoWriter : public AvizoWriter
{
public:
  using AvizoWriter::AvizoWriter;

  ~OverflowAvizoWriter() override = default;
  OverflowAvizoWriter(const OverflowAvizoWriter&) = delete;
  OverflowAvizoWriter(OverflowAvizoWriter&&) = delete;
  OverflowAvizoWriter& operator=(const OverflowAvizoWriter&) = delete;
  OverflowAvizoWriter& operator=(OverflowAvizoWriter&&) = delete;

  usize element_size = 2;
  usize element_count = std::numeric_limits<usize>::max();

protected:
  Result<> generateHeader(FILE* /*outputFile*/) const override
  {
    return {};
  }

  Result<> writeData(FILE* outputFile) const override
  {
    const char payload = 0;
    return writeOutputChecked(outputFile, &payload, element_size, element_count);
  }
};

template <class WriterType>
void CheckClosedOnce(const WriterType& writer, const AvizoFixture& fixture)
{
  CHECK(writer.close_calls == 1);
  CHECK(writer.native_close_status == 0);
  std::error_code error;
  CHECK(std::filesystem::remove(fixture.values.output_file, error));
  CHECK_FALSE(error);
  unit_test::CheckArraysInheritTupleDims(fixture.data_structure);
}

void CheckOutputFailure(const Result<>& result, const AvizoFixture& fixture)
{
  REQUIRE(result.invalid());
  REQUIRE_FALSE(result.errors().empty());
  CHECK(result.errors().front().message.find(fixture.values.output_file.string()) != std::string::npos);
}

template <class AlgorithmType>
void CheckBinaryWindowFailure(usize failedWindow, bool closeAlsoFails)
{
  AvizoFixture fixture(131073, true);
  FaultAvizoWriter<AlgorithmType> writer(fixture.data_structure, fixture.handler, fixture.should_cancel, &fixture.values);
  writer.fail_write_at = failedWindow;
  writer.fail_close = closeAlsoFails;
  const auto result = writer.execute();
  CheckOutputFailure(result, fixture);
  REQUIRE(result.invalid());
  REQUIRE(result.errors().size() == (closeAlsoFails ? usize{2} : usize{1}));
  const auto& message = result.errors().front().message;
  CHECK(message.find(std::to_string(writer.observed_requested_elements)) != std::string::npos);
  CHECK(message.find(std::to_string(writer.observed_written_elements)) != std::string::npos);
  CHECK(message.find(std::to_string(writer.observed_element_size)) != std::string::npos);
  CHECK(message.find(std::to_string(writer.observed_requested_elements * writer.observed_element_size)) != std::string::npos);
  CHECK(message.find(std::to_string(writer.observed_written_elements * writer.observed_element_size)) != std::string::npos);
  CHECK(fixture.source->read_calls == failedWindow);
  CHECK(writer.write_calls == failedWindow);
  CHECK(writer.flush_calls == 0);
  if(closeAlsoFails)
  {
    CHECK(result.errors().back().message.find(fixture.values.output_file.string()) != std::string::npos);
    CHECK(result.errors().back().message.find("close") != std::string::npos);
  }
  CheckClosedOnce(writer, fixture);
}

template <class AlgorithmType>
void CheckTextFailure(std::string_view format, bool binary, usize expectedReads)
{
  AvizoFixture fixture(65537, binary);
  FaultAvizoWriter<AlgorithmType> writer(fixture.data_structure, fixture.handler, fixture.should_cancel, &fixture.values);
  writer.print_format = format;
  writer.fail_print_at = 1;
  const auto result = writer.execute();
  CheckOutputFailure(result, fixture);
  CHECK(writer.matching_print_calls == 1);
  CHECK(fixture.source->read_calls == expectedReads);
  CHECK(writer.flush_calls == 0);
  CheckClosedOnce(writer, fixture);
}

template <class AlgorithmType>
void CheckSourceWarnings(bool readFails, bool closeFails)
{
  AvizoFixture fixture(65537, true);
  fixture.source->fail_read_at = readFails ? 2 : 0;
  fixture.source->warn_on_read = true;
  FaultAvizoWriter<AlgorithmType> writer(fixture.data_structure, fixture.handler, fixture.should_cancel, &fixture.values);
  writer.fail_close = closeFails;
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
  CHECK(fixture.source->read_calls == 2);
  CHECK(writer.write_calls == (readFails ? usize{1} : (std::is_same_v<AlgorithmType, WriteAvizoUniformCoordinate> ? usize{2} : usize{5})));
  CHECK(writer.flush_calls == (readFails ? usize{0} : usize{1}));
  CheckClosedOnce(writer, fixture);
}
} // namespace

TEST_CASE("SimplnxCore::AvizoWriter: Short binary feature writes stop later reads", "[SimplnxCore][AvizoWriter]")
{
  unit_test::LoadPlugins();
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
  unit_test::LoadPlugins();
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
  unit_test::LoadPlugins();
  const bool rectilinear = GENERATE(false, true);
  const bool writeBinary = GENERATE(false, true);
  AvizoFixture fixture(1, writeBinary);
  // Both production headers contain one standalone newline before the data newline.
  if(rectilinear)
  {
    FaultAvizoWriter<WriteAvizoRectilinearCoordinate> writer(fixture.data_structure, fixture.handler, fixture.should_cancel, &fixture.values);
    writer.print_format = "\n";
    writer.fail_print_at = 2;
    const auto result = writer.execute();
    CheckOutputFailure(result, fixture);
    CHECK(writer.write_calls == (writeBinary ? usize{1} : usize{0}));
    CHECK(writer.matching_print_calls == 2);
    CheckClosedOnce(writer, fixture);
  }
  else
  {
    FaultAvizoWriter<WriteAvizoUniformCoordinate> writer(fixture.data_structure, fixture.handler, fixture.should_cancel, &fixture.values);
    writer.print_format = "\n";
    writer.fail_print_at = 2;
    const auto result = writer.execute();
    CheckOutputFailure(result, fixture);
    CHECK(writer.write_calls == (writeBinary ? usize{1} : usize{0}));
    CHECK(writer.matching_print_calls == 2);
    CheckClosedOnce(writer, fixture);
  }
  CHECK(fixture.source->read_calls == 1);
}

TEST_CASE("SimplnxCore::AvizoWriter: Rectilinear coordinate failures stop later axes", "[SimplnxCore][AvizoWriter]")
{
  unit_test::LoadPlugins();
  SECTION("Coordinate delimiter")
  {
    CheckTextFailure<WriteAvizoRectilinearCoordinate>("@2 # x coordinates, then y, then z\n", true, 2);
  }
  SECTION("Binary coordinate")
  {
    const usize failedWrite = GENERATE(usize{2}, usize{3}, usize{4});
    AvizoFixture fixture(8, true);
    FaultAvizoWriter<WriteAvizoRectilinearCoordinate> writer(fixture.data_structure, fixture.handler, fixture.should_cancel, &fixture.values);
    writer.fail_write_at = failedWrite;
    const auto result = writer.execute();
    CheckOutputFailure(result, fixture);
    CHECK(writer.write_calls == failedWrite);
    CHECK(fixture.source->read_calls == 1);
    CHECK(writer.flush_calls == 0);
    CheckClosedOnce(writer, fixture);
  }
  SECTION("Axis newline")
  {
    const usize failedAxis = GENERATE(usize{1}, usize{2}, usize{3});
    const bool writeBinary = GENERATE(false, true);
    AvizoFixture fixture(8, writeBinary);
    FaultAvizoWriter<WriteAvizoRectilinearCoordinate> writer(fixture.data_structure, fixture.handler, fixture.should_cancel, &fixture.values);
    writer.print_format = "\n";
    writer.fail_print_at = 2 + failedAxis;
    const auto result = writer.execute();
    CheckOutputFailure(result, fixture);
    CHECK(writer.write_calls == (writeBinary ? 1 + failedAxis : usize{0}));
    CHECK(writer.matching_print_calls == 2 + failedAxis);
    CheckClosedOnce(writer, fixture);
  }
  SECTION("ASCII coordinate")
  {
    CheckTextFailure<WriteAvizoRectilinearCoordinate>("%f ", false, 2);
  }
}

TEST_CASE("SimplnxCore::AvizoWriter: Flush and consuming close failures are checked", "[SimplnxCore][AvizoWriter]")
{
  unit_test::LoadPlugins();
  const bool flushFails = GENERATE(false, true);
  const bool closeFails = GENERATE(false, true);
  AvizoFixture fixture(1, true);
  FaultAvizoWriter<StageAvizoWriter> writer(fixture.data_structure, fixture.handler, fixture.should_cancel, &fixture.values);
  writer.fail_flush = flushFails;
  writer.fail_close = closeFails;
  const auto result = writer.execute();
  CHECK(writer.header_calls == 1);
  CHECK(writer.data_calls == 1);
  CHECK(writer.flush_calls == 1);
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
  unit_test::LoadPlugins();
  const bool headerFails = GENERATE(false, true);
  AvizoFixture fixture(1, true);
  FaultAvizoWriter<StageAvizoWriter> writer(fixture.data_structure, fixture.handler, fixture.should_cancel, &fixture.values);
  writer.fault = headerFails ? StageFault::HeaderResult : StageFault::DataResult;
  writer.fail_close = true;
  const auto result = writer.execute();
  REQUIRE(result.invalid());
  REQUIRE(result.errors().size() == 2);
  CHECK(result.errors()[0].code == (headerFails ? -92562 : -92563));
  CHECK(result.errors()[1].message.find(fixture.values.output_file.string()) != std::string::npos);
  CHECK(result.errors()[1].message.find("close") != std::string::npos);
  CHECK(writer.data_calls == (headerFails ? usize{0} : usize{1}));
  CHECK(writer.flush_calls == 0);
  REQUIRE(result.warnings().size() == (headerFails ? usize{1} : usize{2}));
  CHECK(result.warnings().front().code == -92571);
  CheckClosedOnce(writer, fixture);
}

TEST_CASE("SimplnxCore::AvizoWriter: Stage exceptions close once", "[SimplnxCore][AvizoWriter]")
{
  unit_test::LoadPlugins();
  const auto fault = GENERATE(StageFault::HeaderStandard, StageFault::DataStandard, StageFault::HeaderUnknown, StageFault::DataUnknown);
  const bool closeFails = GENERATE(false, true);
  AvizoFixture fixture(1, true);
  FaultAvizoWriter<StageAvizoWriter> writer(fixture.data_structure, fixture.handler, fixture.should_cancel, &fixture.values);
  writer.fault = fault;
  writer.fail_close = closeFails;
  const auto result = writer.execute();
  CheckOutputFailure(result, fixture);
  REQUIRE(result.invalid());
  CHECK(result.errors().size() == (closeFails ? usize{2} : usize{1}));
  const bool dataStage = fault == StageFault::DataStandard || fault == StageFault::DataUnknown;
  CHECK(result.errors().front().message.find(dataStage ? "data" : "header") != std::string::npos);
  CHECK(writer.data_calls == (dataStage ? usize{1} : usize{0}));
  CHECK(writer.flush_calls == 0);
  CHECK(result.warnings().size() == (dataStage ? usize{1} : usize{0}));
  CheckClosedOnce(writer, fixture);
}

TEST_CASE("SimplnxCore::AvizoWriter: Allocation exceptions preserve their type after cleanup", "[SimplnxCore][AvizoWriter]")
{
  unit_test::LoadPlugins();
  const auto fault = GENERATE(StageFault::HeaderAllocation, StageFault::DataAllocation);
  const bool closeFails = GENERATE(false, true);
  AvizoFixture fixture(1, true);
  FaultAvizoWriter<StageAvizoWriter> writer(fixture.data_structure, fixture.handler, fixture.should_cancel, &fixture.values);
  writer.fault = fault;
  writer.fail_close = closeFails;
  CHECK_THROWS_AS(writer.execute(), std::bad_alloc);
  CHECK(writer.flush_calls == 0);
  CheckClosedOnce(writer, fixture);
}

TEST_CASE("SimplnxCore::AvizoWriter: Callback print exceptions and close exceptions retain ownership", "[SimplnxCore][AvizoWriter]")
{
  unit_test::LoadPlugins();
  const bool printThrows = GENERATE(false, true);
  AvizoFixture fixture(1, true);
  FaultAvizoWriter<WriteAvizoUniformCoordinate> writer(fixture.data_structure, fixture.handler, fixture.should_cancel, &fixture.values);
  writer.fail_print_at = printThrows ? 1 : 0;
  writer.throw_from_print = printThrows;
  writer.throw_from_close = !printThrows;
  const auto result = writer.execute();
  CheckOutputFailure(result, fixture);
  CHECK(fixture.source->read_calls == (printThrows ? usize{0} : usize{1}));
  CheckClosedOnce(writer, fixture);
}

TEST_CASE("SimplnxCore::AvizoWriter: Source warnings survive a later read error and close error", "[SimplnxCore][AvizoWriter]")
{
  unit_test::LoadPlugins();
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
  unit_test::LoadPlugins();
  AvizoFixture failed(1, true);
  AvizoFixture healthy(1, true);
  FaultAvizoWriter<WriteAvizoUniformCoordinate> first(failed.data_structure, failed.handler, failed.should_cancel, &failed.values);
  FaultAvizoWriter<WriteAvizoUniformCoordinate> second(healthy.data_structure, healthy.handler, healthy.should_cancel, &healthy.values);
  first.fail_write_at = 1;
  const auto firstResult = first.execute();
  const auto secondResult = second.execute();
  REQUIRE(firstResult.invalid());
  REQUIRE(secondResult.valid());
  CHECK(second.write_calls == 1);
  CHECK(second.flush_calls == 1);
  CheckClosedOnce(first, failed);
  CheckClosedOnce(second, healthy);
}

TEST_CASE("SimplnxCore::AvizoWriter: Byte-count overflow stops before the transfer operation", "[SimplnxCore][AvizoWriter]")
{
  unit_test::LoadPlugins();
  const bool reverseOperands = GENERATE(false, true);
  AvizoFixture fixture(1, true);
  FaultAvizoWriter<OverflowAvizoWriter> writer(fixture.data_structure, fixture.handler, fixture.should_cancel, &fixture.values);
  if(reverseOperands)
  {
    std::swap(writer.element_size, writer.element_count);
  }
  // The test operation reports any call without accessing the one-byte source.
  writer.reject_every_write = true;
  const auto result = writer.execute();
  CheckOutputFailure(result, fixture);
  REQUIRE(result.invalid());
  CHECK(result.errors().front().message.find(std::to_string(std::numeric_limits<usize>::max())) != std::string::npos);
  CHECK(writer.write_calls == 0);
  CHECK(writer.flush_calls == 0);
  CheckClosedOnce(writer, fixture);
}
