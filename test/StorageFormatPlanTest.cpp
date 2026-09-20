#include "simplnx/Common/ScopeGuard.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/Core/Preferences.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/EmptyDataStore.hpp"
#include "simplnx/DataStructure/IO/Generic/DataIOCollection.hpp"
#include "simplnx/DataStructure/IO/Generic/IDataIOManager.hpp"
#include "simplnx/DataStructure/IO/Generic/IDataStoreFormatResolver.hpp"
#include "simplnx/DataStructure/IO/Generic/IOConstants.hpp"
#include "simplnx/DataStructure/IO/Generic/InMemoryFormatResolver.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/ImportH5ObjectPathsAction.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dPreflightCache.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/FileIO.hpp"
#include "simplnx/Utilities/StoreCopyUtilities.hpp"

#if defined(SIMPLNX_STORE_COPY_STRICT_FORMAT) && SIMPLNX_STORE_COPY_STRICT_FORMAT
#include "SimplnxOoc/OocDataIOManager.hpp"
#endif

#include "simplnx/unit_test/simplnx_test_dirs.hpp"

#include <catch2/catch.hpp>

#include <H5Dpublic.h>
#include <H5Ppublic.h>
#include <H5Spublic.h>
#include <H5Tpublic.h>

#include <array>
#include <chrono>
#include <filesystem>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

using namespace nx::core;
namespace fs = std::filesystem;

namespace
{
constexpr uint64 k_LogicalBytes = 100'000'000'000ULL;
constexpr StringLiteral k_LargeArrayName = "LargeMetadata";
constexpr StringLiteral k_SelectedFormat = "storage-format-plan-test";
const DataPath k_LargeArrayPath({k_LargeArrayName});

template <class Store>
concept HasLegacyFormatGetter = requires(const Store& store) { store.dataFormat(); };

template <class Store>
concept HasPlannedFormatGetter = requires(const Store& store) { store.getPlannedDataFormat(); };

template <class Store>
concept HasSelectedFactory = requires(const ShapeType& shape) { Store::Create(shape, shape, std::string{}); };

template <class Action>
concept HasActionFormatRequestAccessor = requires(const Action& action) { action.dataFormat(); };

class SelectedFormatResolver : public IDataStoreFormatResolver
{
public:
  explicit SelectedFormatResolver(std::string selectedFormat)
  : m_SelectedFormat(std::move(selectedFormat))
  {
  }

  std::string resolveFormat([[maybe_unused]] const DataStructure& dataStructure, [[maybe_unused]] const DataPath& arrayPath, [[maybe_unused]] DataType numericType,
                            [[maybe_unused]] uint64 dataSizeBytes) const override
  {
    return m_SelectedFormat;
  }

private:
  std::string m_SelectedFormat;
};

class RecordingFormatResolver : public IDataStoreFormatResolver
{
public:
  mutable std::vector<DataPath> paths;
  mutable std::vector<DataType> numericTypes;
  mutable std::vector<uint64> logicalBytes;

  std::string resolveFormat([[maybe_unused]] const DataStructure& dataStructure, const DataPath& arrayPath, DataType numericType, uint64 dataSizeBytes) const override
  {
    paths.push_back(arrayPath);
    numericTypes.push_back(numericType);
    logicalBytes.push_back(dataSizeBytes);
    return {};
  }
};

class ThrowingFormatResolver : public IDataStoreFormatResolver
{
public:
  explicit ThrowingFormatResolver(bool throwBadAllocation)
  : m_ThrowBadAllocation(throwBadAllocation)
  {
  }

  std::string resolveFormat([[maybe_unused]] const DataStructure& dataStructure, [[maybe_unused]] const DataPath& arrayPath, [[maybe_unused]] DataType numericType,
                            [[maybe_unused]] uint64 dataSizeBytes) const override
  {
    if(m_ThrowBadAllocation)
    {
      throw std::bad_alloc();
    }
    throw std::runtime_error("injected resolver failure");
  }

private:
  bool m_ThrowBadAllocation = false;
};

class PlaceholderPlanManager : public IDataIOManager
{
public:
  PlaceholderPlanManager()
  {
    addDataStoreCreationFnc(k_SelectedFormat.str(), [](DataType, const ShapeType&, const ShapeType&, const std::optional<ShapeType>&) -> std::unique_ptr<IDataStore> {
      throw std::runtime_error("The metadata-only import must not invoke the value-store factory");
    });
  }

  std::string formatName() const override
  {
    return k_SelectedFormat.str();
  }
};

struct ScopedTempFile
{
  explicit ScopedTempFile(fs::path filePath)
  : path(std::move(filePath))
  {
  }

  ~ScopedTempFile()
  {
    std::error_code errorCode;
    fs::remove(path, errorCode);
  }

  fs::path path;
};

void CreateLargeMetadataFile(const fs::path& filePath)
{
  HDF5::FileIO fileWriter = HDF5::FileIO::WriteFile(filePath);
  REQUIRE(fileWriter.isValid());
  auto fileWriterFileVersionWriteResult = fileWriter.writeStringAttribute("FileVersion", DREAM3D::k_CurrentFileVersion.str());
  SIMPLNX_RESULT_REQUIRE_VALID(fileWriterFileVersionWriteResult);
  auto dataStructureGroup = fileWriter.createGroup(Constants::k_DataStructureTag);
  REQUIRE(dataStructureGroup.isValid());
  auto dataStructureGroupNextIdWriteResult = dataStructureGroup.writeScalarAttribute(Constants::k_NextIdTag, DataObject::IdType{2});
  SIMPLNX_RESULT_REQUIRE_VALID(dataStructureGroupNextIdWriteResult);

  {
    std::lock_guard<std::mutex> hdf5Lock(HDF5::Support::ApiLock());
    const std::array<hsize_t, 1> dimensions{static_cast<hsize_t>(k_LogicalBytes)};
    const std::array<hsize_t, 1> chunkDimensions{1024 * 1024};
    const hid_t dataspaceId = H5Screate_simple(static_cast<int>(dimensions.size()), dimensions.data(), nullptr);
    REQUIRE(dataspaceId >= 0);
    auto dataspaceGuard = MakeScopeGuard([dataspaceId]() noexcept { H5Sclose(dataspaceId); });
    const hid_t propertyListId = H5Pcreate(H5P_DATASET_CREATE);
    REQUIRE(propertyListId >= 0);
    auto propertyListGuard = MakeScopeGuard([propertyListId]() noexcept { H5Pclose(propertyListId); });
    REQUIRE(H5Pset_chunk(propertyListId, static_cast<int>(chunkDimensions.size()), chunkDimensions.data()) >= 0);
    REQUIRE(H5Pset_alloc_time(propertyListId, H5D_ALLOC_TIME_LATE) >= 0);
    REQUIRE(H5Pset_fill_time(propertyListId, H5D_FILL_TIME_NEVER) >= 0);
    const hid_t datasetId = H5Dcreate2(dataStructureGroup.getId(), k_LargeArrayName.c_str(), H5T_NATIVE_UINT8, dataspaceId, H5P_DEFAULT, propertyListId, H5P_DEFAULT);
    REQUIRE(datasetId >= 0);
    auto datasetGuard = MakeScopeGuard([datasetId]() noexcept { H5Dclose(datasetId); });
    REQUIRE(H5Dget_storage_size(datasetId) == 0);
  }

  auto datasetWriter = dataStructureGroup.openDataset(k_LargeArrayName.str());
  auto datasetWriterObjectTypeWriteResult = datasetWriter.writeStringAttribute(Constants::k_ObjectTypeTag, DataArray<uint8>::GetTypeName());
  SIMPLNX_RESULT_REQUIRE_VALID(datasetWriterObjectTypeWriteResult);
  auto datasetWriterObjectIdWriteResult = datasetWriter.writeScalarAttribute(Constants::k_ObjectIdTag, DataObject::IdType{1});
  SIMPLNX_RESULT_REQUIRE_VALID(datasetWriterObjectIdWriteResult);
  auto datasetWriterImportableWriteResult = datasetWriter.writeScalarAttribute(Constants::k_ImportableTag, int32{1});
  SIMPLNX_RESULT_REQUIRE_VALID(datasetWriterImportableWriteResult);
  auto datasetWriterTupleShapeWriteResult = datasetWriter.writeVectorAttribute(IOConstants::k_TupleShapeTag, ShapeType{static_cast<usize>(k_LogicalBytes)});
  SIMPLNX_RESULT_REQUIRE_VALID(datasetWriterTupleShapeWriteResult);
  auto datasetWriterComponentShapeWriteResult = datasetWriter.writeVectorAttribute(IOConstants::k_ComponentShapeTag, ShapeType{1});
  SIMPLNX_RESULT_REQUIRE_VALID(datasetWriterComponentShapeWriteResult);
}
} // namespace

TEST_CASE("StorageFormatPlan: numeric selection validates capability without allocating", "[StorageFormatPlan][T04][T08][T09]")
{
  auto manager = std::make_shared<PlaceholderPlanManager>();
  auto managerRegistrationResult = Application::GetOrCreateInstance()->getIOCollection().addIOManager(manager);
  SIMPLNX_RESULT_REQUIRE_VALID(managerRegistrationResult);

  auto emptyMemory = ValidateNumericStorageFormat("");
  SIMPLNX_RESULT_REQUIRE_VALID(emptyMemory);
  REQUIRE(emptyMemory.value().empty());

  auto namedMemory = ValidateNumericStorageFormat(Preferences::k_InMemoryFormat.str());
  SIMPLNX_RESULT_REQUIRE_VALID(namedMemory);
  REQUIRE(namedMemory.value().empty());

  DataStructure destination;
  auto resolver = std::make_shared<RecordingFormatResolver>();
  resolver->paths.clear();
  resolver->numericTypes.clear();
  resolver->logicalBytes.clear();
  destination.setFormatResolver(resolver);
  const DataPath path({"Selected"});

  auto explicitMemory = ResolveNumericStorageFormat(destination, path, DataType::float32, 48, Preferences::k_InMemoryFormat.str());
  SIMPLNX_RESULT_REQUIRE_VALID(explicitMemory);
  REQUIRE(explicitMemory.value().empty());
  REQUIRE(resolver->paths.empty());

  auto automaticMemory = ResolveNumericStorageFormat(destination, path, DataType::float32, 48, "");
  SIMPLNX_RESULT_REQUIRE_VALID(automaticMemory);
  REQUIRE(automaticMemory.value().empty());
  REQUIRE(resolver->paths == std::vector<DataPath>{path});
  REQUIRE(resolver->numericTypes == std::vector<DataType>{DataType::float32});
  REQUIRE(resolver->logicalBytes == std::vector<uint64>{48});

  auto repeatedValidation = ValidateNumericStorageFormat(Preferences::k_InMemoryFormat.str());
  SIMPLNX_RESULT_REQUIRE_VALID(repeatedValidation);
  REQUIRE(repeatedValidation.value().empty());
  REQUIRE(resolver->paths == std::vector<DataPath>{path});
  REQUIRE(resolver->numericTypes == std::vector<DataType>{DataType::float32});
  REQUIRE(resolver->logicalBytes == std::vector<uint64>{48});

  destination.setFormatResolver(std::make_shared<SelectedFormatResolver>(k_SelectedFormat.str()));
  auto automatic = ResolveNumericStorageFormat(destination, path, DataType::float32, 48, "");
  SIMPLNX_RESULT_REQUIRE_VALID(automatic);
  REQUIRE(automatic.value() == k_SelectedFormat.str());
}

TEST_CASE("StorageFormatPlan: numeric selection reports unsupported formats and resolver failures", "[StorageFormatPlan][T04][T21]")
{
  const std::string unavailableFormat = "storage-format-plan-unavailable";
  auto unavailable = ValidateNumericStorageFormat(unavailableFormat);
#if defined(SIMPLNX_STORE_COPY_STRICT_FORMAT) && SIMPLNX_STORE_COPY_STRICT_FORMAT
  REQUIRE(unavailable.invalid());
  REQUIRE(unavailable.errors().size() == 1);
  REQUIRE(unavailable.errors()[0].code == -10600);
  REQUIRE(unavailable.errors()[0].message.find(unavailableFormat) != std::string::npos);
  auto unavailableFactory = EmptyDataStore<uint8>::Create(ShapeType{1}, ShapeType{1}, unavailableFormat);
  REQUIRE(unavailableFactory.invalid());
  REQUIRE(unavailableFactory.errors().size() == 1);
  REQUIRE(unavailableFactory.errors()[0].code == -10600);
  REQUIRE(unavailableFactory.errors()[0].message.find(unavailableFormat) != std::string::npos);
#else
  SIMPLNX_RESULT_REQUIRE_VALID(unavailable);
  REQUIRE(unavailable.value().empty());
  auto unavailableFactory = EmptyDataStore<uint8>::Create(ShapeType{1}, ShapeType{1}, unavailableFormat);
  SIMPLNX_RESULT_REQUIRE_VALID(unavailableFactory);
  REQUIRE(unavailableFactory.value()->getPlannedStoreType() == IDataStore::StoreType::InMemory);
  REQUIRE(unavailableFactory.value()->getDataFormat().empty());
#endif

  DataStructure destination;
  const DataPath path({"ResolverFailure"});
  destination.setFormatResolver(std::make_shared<ThrowingFormatResolver>(false));
  auto failure = ResolveNumericStorageFormat(destination, path, DataType::uint8, 17, "");
  REQUIRE(failure.invalid());
  REQUIRE(failure.errors().size() == 1);
  REQUIRE(failure.errors()[0].code == -10601);
  REQUIRE(failure.errors()[0].message.find(path.toString()) != std::string::npos);
  REQUIRE(failure.errors()[0].message.find("injected resolver failure") != std::string::npos);

  destination.setFormatResolver(std::make_shared<ThrowingFormatResolver>(true));
  REQUIRE_THROWS_AS(ResolveNumericStorageFormat(destination, path, DataType::uint8, 17, ""), std::bad_alloc);
}

TEST_CASE("StorageFormatPlan: factory creates validated memory and registered OOC placeholders", "[StorageFormatPlan][T01][T02][T04][T07][T21]")
{
  auto memoryResult = EmptyDataStore<float32>::Create(ShapeType{2, 3}, ShapeType{2}, "");
  SIMPLNX_RESULT_REQUIRE_VALID(memoryResult);
  REQUIRE(memoryResult.value()->getStoreType() == IDataStore::StoreType::Empty);
  REQUIRE(memoryResult.value()->getPlannedStoreType() == IDataStore::StoreType::InMemory);
  REQUIRE(memoryResult.value()->memoryUsage() == 48);
  REQUIRE_THROWS(memoryResult.value()->getValue(0));

  auto namedMemoryResult = EmptyDataStore<float32>::Create(ShapeType{2, 3}, ShapeType{2}, Preferences::k_InMemoryFormat.str());
  SIMPLNX_RESULT_REQUIRE_VALID(namedMemoryResult);
  REQUIRE(namedMemoryResult.value()->getPlannedStoreType() == IDataStore::StoreType::InMemory);
  REQUIRE(namedMemoryResult.value()->getDataFormat().empty());
  REQUIRE(namedMemoryResult.value()->memoryUsage() == 48);

  auto manager = std::make_shared<PlaceholderPlanManager>();
  auto managerRegistrationResult = Application::GetOrCreateInstance()->getIOCollection().addIOManager(manager);
  SIMPLNX_RESULT_REQUIRE_VALID(managerRegistrationResult);
  auto registeredResult = EmptyDataStore<uint8>::Create(ShapeType{6}, ShapeType{1}, k_SelectedFormat.str());
  SIMPLNX_RESULT_REQUIRE_VALID(registeredResult);
  REQUIRE(registeredResult.value()->getStoreType() == IDataStore::StoreType::Empty);
  REQUIRE(registeredResult.value()->getPlannedStoreType() == IDataStore::StoreType::OutOfCore);
  REQUIRE(registeredResult.value()->getDataFormat() == k_SelectedFormat.str());
  REQUIRE(registeredResult.value()->memoryUsage() == 0);

#if defined(SIMPLNX_STORE_COPY_STRICT_FORMAT) && SIMPLNX_STORE_COPY_STRICT_FORMAT
  SimplnxOoc::registerIOManager(Application::GetOrCreateInstance()->getIOCollection());
  auto oocResult = EmptyDataStore<float32>::Create(ShapeType{2, 3}, ShapeType{2}, "HDF5-OOC");
  SIMPLNX_RESULT_REQUIRE_VALID(oocResult);
  REQUIRE(oocResult.value()->getStoreType() == IDataStore::StoreType::Empty);
  REQUIRE(oocResult.value()->getPlannedStoreType() == IDataStore::StoreType::OutOfCore);
  REQUIRE(oocResult.value()->getDataFormat() == "HDF5-OOC");
  REQUIRE(oocResult.value()->memoryUsage() == 0);
#endif
}

TEST_CASE("StorageFormatPlan: factory preserves scalar and zero-dimension metadata", "[StorageFormatPlan][T07][T22]")
{
  auto emptyTuples = EmptyDataStore<uint64>::Create(ShapeType{}, ShapeType{3}, "");
  SIMPLNX_RESULT_REQUIRE_VALID(emptyTuples);
  REQUIRE(emptyTuples.value()->getTupleShape().empty());
  REQUIRE(emptyTuples.value()->getComponentShape() == ShapeType{3});
  REQUIRE(emptyTuples.value()->getNumberOfTuples() == 1);
  REQUIRE(emptyTuples.value()->getNumberOfComponents() == 3);
  REQUIRE(emptyTuples.value()->getSize() == 3);
  REQUIRE(emptyTuples.value()->memoryUsage() == 3 * sizeof(uint64));

  auto emptyComponents = EmptyDataStore<uint64>::Create(ShapeType{3}, ShapeType{}, "");
  SIMPLNX_RESULT_REQUIRE_VALID(emptyComponents);
  REQUIRE(emptyComponents.value()->getTupleShape() == ShapeType{3});
  REQUIRE(emptyComponents.value()->getComponentShape().empty());
  REQUIRE(emptyComponents.value()->getNumberOfTuples() == 3);
  REQUIRE(emptyComponents.value()->getNumberOfComponents() == 1);
  REQUIRE(emptyComponents.value()->getSize() == 3);
  REQUIRE(emptyComponents.value()->memoryUsage() == 3 * sizeof(uint64));

  auto scalar = EmptyDataStore<uint64>::Create(ShapeType{}, ShapeType{}, "");
  SIMPLNX_RESULT_REQUIRE_VALID(scalar);
  REQUIRE(scalar.value()->getTupleShape().empty());
  REQUIRE(scalar.value()->getComponentShape().empty());
  REQUIRE(scalar.value()->getNumberOfTuples() == 1);
  REQUIRE(scalar.value()->getNumberOfComponents() == 1);
  REQUIRE(scalar.value()->getSize() == 1);
  REQUIRE(scalar.value()->memoryUsage() == sizeof(uint64));

  auto zeroDimension = EmptyDataStore<uint64>::Create(ShapeType{4, 0, 8}, ShapeType{2}, "");
  SIMPLNX_RESULT_REQUIRE_VALID(zeroDimension);
  REQUIRE(zeroDimension.value()->getTupleShape() == ShapeType{4, 0, 8});
  REQUIRE(zeroDimension.value()->getNumberOfTuples() == 0);
  REQUIRE(zeroDimension.value()->getNumberOfComponents() == 2);
  REQUIRE(zeroDimension.value()->getSize() == 0);
  REQUIRE(zeroDimension.value()->memoryUsage() == 0);

  auto zeroComponentDimension = EmptyDataStore<uint64>::Create(ShapeType{3}, ShapeType{2, 0}, "");
  SIMPLNX_RESULT_REQUIRE_VALID(zeroComponentDimension);
  REQUIRE(zeroComponentDimension.value()->getComponentShape() == ShapeType{2, 0});
  REQUIRE(zeroComponentDimension.value()->getNumberOfTuples() == 3);
  REQUIRE(zeroComponentDimension.value()->getNumberOfComponents() == 0);
  REQUIRE(zeroComponentDimension.value()->getSize() == 0);
  REQUIRE(zeroComponentDimension.value()->memoryUsage() == 0);

  const auto maximum = std::numeric_limits<usize>::max();
  auto tupleOverflow = EmptyDataStore<uint8>::Create(ShapeType{maximum, 2}, ShapeType{1}, "");
  REQUIRE(tupleOverflow.invalid());
  REQUIRE_FALSE(tupleOverflow.errors().empty());
  REQUIRE(tupleOverflow.errors()[0].code == -10602);
  auto componentOverflow = EmptyDataStore<uint8>::Create(ShapeType{1}, ShapeType{maximum, 2}, "");
  REQUIRE(componentOverflow.invalid());
  REQUIRE_FALSE(componentOverflow.errors().empty());
  REQUIRE(componentOverflow.errors()[0].code == -10602);
  auto elementOverflow = EmptyDataStore<uint8>::Create(ShapeType{maximum}, ShapeType{2}, "");
  REQUIRE(elementOverflow.invalid());
  REQUIRE_FALSE(elementOverflow.errors().empty());
  REQUIRE(elementOverflow.errors()[0].code == -10602);
  auto byteOverflow = EmptyDataStore<uint64>::Create(ShapeType{maximum}, ShapeType{1}, "");
  REQUIRE(byteOverflow.invalid());
  REQUIRE_FALSE(byteOverflow.errors().empty());
  REQUIRE(byteOverflow.errors()[0].code == -10602);
}

TEST_CASE("StorageFormatPlan: placeholder resize validates complete counts before mutation", "[StorageFormatPlan][T22]")
{
  const usize maximum = std::numeric_limits<usize>::max();
  auto valueStore = EmptyDataStore<uint8>::Create({3}, {2}, "");
  SIMPLNX_RESULT_REQUIRE_VALID(valueStore);
  auto valueOverflow = valueStore.value()->resizeTuples({maximum});
  REQUIRE(valueOverflow.invalid());
  CHECK(valueStore.value()->getTupleShape() == ShapeType{3});
  CHECK(valueStore.value()->getNumberOfTuples() == 3);
  CHECK(valueStore.value()->getDataFormat().empty());

  auto byteStore = EmptyDataStore<uint64>::Create({3}, {1}, "");
  SIMPLNX_RESULT_REQUIRE_VALID(byteStore);
  auto byteOverflow = byteStore.value()->resizeTuples({maximum / sizeof(uint64) + 1});
  REQUIRE(byteOverflow.invalid());
  CHECK(byteStore.value()->getTupleShape() == ShapeType{3});
  CHECK(byteStore.value()->getNumberOfTuples() == 3);
  CHECK(byteStore.value()->getDataFormat().empty());

  REQUIRE(byteStore.value()->resizeTuples({}).valid());
  CHECK(byteStore.value()->getTupleShape().empty());
  CHECK(byteStore.value()->getNumberOfTuples() == 1);
  REQUIRE(byteStore.value()->resizeTuples({0}).valid());
  CHECK(byteStore.value()->getTupleShape() == ShapeType{0});
  CHECK(byteStore.value()->getNumberOfTuples() == 0);
}

TEST_CASE("StorageFormatPlan: construction API is mandatory", "[StorageFormatPlan][T03]")
{
  using Store = EmptyDataStore<float32>;
  CHECK_FALSE(std::is_default_constructible_v<Store>);
  CHECK_FALSE((std::is_constructible_v<Store, ShapeType, ShapeType>));
  CHECK_FALSE((std::is_constructible_v<Store, ShapeType, ShapeType, std::string>));
  CHECK(HasSelectedFactory<Store>);
}

TEST_CASE("StorageFormatPlan: obsolete store getters are unavailable", "[StorageFormatPlan][T06]")
{
  using Store = EmptyDataStore<float32>;
  CHECK_FALSE(HasLegacyFormatGetter<Store>);
  CHECK_FALSE(HasPlannedFormatGetter<Store>);
  CHECK(HasActionFormatRequestAccessor<CreateArrayAction>);
}

TEST_CASE("StorageFormatPlan: common getter preserves the recorded source plan", "[StorageFormatPlan]")
{
  auto manager = std::make_shared<PlaceholderPlanManager>();
  REQUIRE(Application::GetOrCreateInstance()->getIOCollection().addIOManager(manager).valid());
  auto sourceResult = EmptyDataStore<float32>::Create(ShapeType{4}, ShapeType{1}, k_SelectedFormat.str());
  SIMPLNX_RESULT_REQUIRE_VALID(sourceResult);
  REQUIRE(sourceResult.value()->getStoreType() == IDataStore::StoreType::Empty);
  REQUIRE(sourceResult.value()->getPlannedStoreType() == IDataStore::StoreType::OutOfCore);
  REQUIRE(sourceResult.value()->getDataFormat() == k_SelectedFormat.str());
}

TEST_CASE("StorageFormatPlan: Empty conversion selects metadata without allocating values", "[StorageFormatPlan][T20]")
{
  auto manager = std::make_shared<PlaceholderPlanManager>();
  REQUIRE(Application::GetOrCreateInstance()->getIOCollection().addIOManager(manager).valid());
  auto sourceResult = EmptyDataStore<float32>::Create(ShapeType{3}, ShapeType{1}, "");
  SIMPLNX_RESULT_REQUIRE_VALID(sourceResult);

  auto unchanged = DataStoreUtilities::ConvertDataStore<float32>(*sourceResult.value(), Preferences::k_InMemoryFormat.str());
  CHECK(unchanged == nullptr);
  CHECK(sourceResult.value()->getStoreType() == IDataStore::StoreType::Empty);

  auto converted = DataStoreUtilities::ConvertDataStore<float32>(*sourceResult.value(), k_SelectedFormat.str());
  REQUIRE(converted != nullptr);
  CHECK(converted->getStoreType() == IDataStore::StoreType::Empty);
  CHECK(converted->getDataFormat() == k_SelectedFormat.str());
  CHECK(converted->getTupleShape() == ShapeType{3});
  CHECK(converted->getComponentShape() == ShapeType{1});
}

TEST_CASE("StorageFormatPlan: import resolver receives 100 GB logical bytes from an OOC placeholder", "[StorageFormatPlan][T15][T22]")
{
  if constexpr(std::numeric_limits<usize>::max() < k_LogicalBytes)
  {
    SUCCEED("A 100 GB logical array exceeds this host's supported usize range");
    return;
  }

  const fs::path filePath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "StorageFormatPlan_100GB_Metadata.dream3d";
  ScopedTempFile fileGuard(filePath);
  CreateLargeMetadataFile(filePath);
  fs::last_write_time(filePath, fs::last_write_time(filePath) - std::chrono::seconds(10));

  auto& cache = DREAM3D::Dream3dPreflightCache::Instance();
  cache.clear();
  cache.resetStats();
  auto inMemoryResolver = std::make_shared<InMemoryFormatResolver>();
  auto cleanup = MakeScopeGuard([&cache, inMemoryResolver]() noexcept {
    cache.clear();
    DataStructure::setDefaultFormatResolver(inMemoryResolver);
  });

  auto manager = std::make_shared<PlaceholderPlanManager>();
  auto managerRegistrationResult = Application::GetOrCreateInstance()->getIOCollection().addIOManager(manager);
  SIMPLNX_RESULT_REQUIRE_VALID(managerRegistrationResult);
  DataStructure::setDefaultFormatResolver(std::make_shared<SelectedFormatResolver>(k_SelectedFormat.str()));

  auto firstFetch = cache.fetch(filePath);
  SIMPLNX_RESULT_REQUIRE_VALID(firstFetch);
  auto* sourceArray = firstFetch.value().getDataAs<UInt8Array>(k_LargeArrayPath);
  REQUIRE(sourceArray != nullptr);
  REQUIRE(sourceArray->getTupleShape() == ShapeType{static_cast<usize>(k_LogicalBytes)});
  REQUIRE(sourceArray->getComponentShape() == ShapeType{1});
  REQUIRE(sourceArray->getIDataStore()->getStoreType() == IDataStore::StoreType::Empty);
  REQUIRE(sourceArray->getIDataStore()->getPlannedStoreType() == IDataStore::StoreType::OutOfCore);
  REQUIRE(sourceArray->memoryUsage() == 0);

  DataStructure destination;
  auto recordingResolver = std::make_shared<RecordingFormatResolver>();
  destination.setFormatResolver(recordingResolver);
  ImportH5ObjectPathsAction action(filePath, {k_LargeArrayPath});
  auto actionApplyResult = action.apply(destination, IDataAction::Mode::Preflight);
  SIMPLNX_RESULT_REQUIRE_VALID(actionApplyResult);

  REQUIRE(cache.missCount() == 1);
  REQUIRE(cache.hitCount() == 1);
  REQUIRE(recordingResolver->paths == std::vector<DataPath>{k_LargeArrayPath});
  REQUIRE(recordingResolver->logicalBytes == std::vector<uint64>{k_LogicalBytes});
}
