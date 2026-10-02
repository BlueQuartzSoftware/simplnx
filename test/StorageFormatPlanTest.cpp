#include "simplnx/Common/ScopeGuard.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/Core/Preferences.hpp"
#include "simplnx/DataStructure/BaseGroup.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/EmptyDataStore.hpp"
#include "simplnx/DataStructure/EmptyListStore.hpp"
#include "simplnx/DataStructure/IO/Generic/DataIOCollection.hpp"
#include "simplnx/DataStructure/IO/Generic/IDataIOManager.hpp"
#include "simplnx/DataStructure/IO/Generic/IDataStoreFormatResolver.hpp"
#include "simplnx/DataStructure/IO/Generic/IOConstants.hpp"
#include "simplnx/DataStructure/IO/Generic/InMemoryFormatResolver.hpp"
#include "simplnx/DataStructure/IO/HDF5/DataStoreIO.hpp"
#include "simplnx/DataStructure/IO/HDF5/DataStructureReader.hpp"
#include "simplnx/DataStructure/Messaging/DataAddedMessage.hpp"
#include "simplnx/DataStructure/Messaging/DataRemovedMessage.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/ImportH5ObjectPathsAction.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dPreflightCache.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/FileIO.hpp"
#include "simplnx/Utilities/StoreCopyUtilities.hpp"
#include <iterator>

#if defined(SIMPLNX_STORE_COPY_STRICT_FORMAT) && SIMPLNX_STORE_COPY_STRICT_FORMAT
#include "SimplnxOoc/OocDataIOManager.hpp"
#endif

#include "simplnx/unit_test/simplnx_test_dirs.hpp"

#include <catch2/catch.hpp>

#include <H5Dpublic.h>
#include <H5Ppublic.h>
#include <H5Spublic.h>
#include <H5Tpublic.h>

#include <algorithm>
#include <any>
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
    addDataStoreCreationFnc(k_SelectedFormat.str(), [](DataType, const ShapeType&, const ShapeType&, const std::optional<ShapeType>&, DataStoreInitializationMode) -> std::unique_ptr<IDataStore> {
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

  // Ordinary and neutral metadata use independent masters.
  REQUIRE(cache.missCount() == 2);
  REQUIRE(cache.hitCount() == 0);
  REQUIRE(recordingResolver->paths == std::vector<DataPath>{k_LargeArrayPath});
  REQUIRE(recordingResolver->logicalBytes == std::vector<uint64>{k_LogicalBytes});
}

namespace
{
const DataPath k_ImportPolicyGroup({"SelectedGroup"});
const DataPath k_ImportPolicyFirst({"SelectedGroup", "First"});
const DataPath k_ImportPolicySecond({"SelectedGroup", "Second"});
const DataPath k_ImportPolicyExcluded({"SelectedGroup", "Excluded"});
const DataPath k_ImportPolicyMarker({"ExistingMarker"});
constexpr std::array<int32, 6> k_ImportPolicyFirstValues{3, -7, 11, 29, 0, 101};
constexpr std::array<int32, 4> k_ImportPolicySecondValues{-9, 42, 5, 18};
constexpr std::array<int32, 3> k_ImportPolicyExcludedValues{91, 92, 93};
constexpr std::array<int32, 2> k_ImportPolicyMarkerValues{731, -204};

template <usize Count>
Int32Array* CreateImportPolicyArray(DataStructure& dataStructure, const DataPath& path, const std::array<int32, Count>& values, DataObject::OptionalId parentId = {})
{
  auto store = DataStoreUtilities::CreateDataStore<int32>(dataStructure, path, ShapeType{Count}, ShapeType{1});
  REQUIRE(store != nullptr);
  auto* array = Int32Array::Create(dataStructure, path.getTargetName(), std::move(store), parentId);
  REQUIRE(array != nullptr);
  for(usize valueIdx = 0; valueIdx < Count; ++valueIdx)
  {
    (*array)[valueIdx] = values[valueIdx];
  }
  return array;
}

/**
 * @brief Checks the closed fixture through a fixed-size native HDF5 read.
 * @tparam Count Specifies the exact permitted dataset size.
 * @param fileReader Supplies the independently reopened source file.
 * @param path Identifies the numeric dataset below DataStructure.
 * @param expected Supplies literal values independent of import output.
 */
template <usize Count>
void CheckImportPolicyFileArray(const HDF5::FileIO& fileReader, const DataPath& path, const std::array<int32, Count>& expected)
{
  const auto fileId = fileReader.getId();
  const std::lock_guard<std::mutex> lock(HDF5::Support::ApiLock());
  const std::string datasetPath = "/DataStructure/" + path.toString();
  const hid_t datasetId = H5Dopen2(fileId, datasetPath.c_str(), H5P_DEFAULT);
  REQUIRE(datasetId >= 0);
  const auto datasetGuard = MakeScopeGuard([datasetId]() noexcept { H5Dclose(datasetId); });
  const hid_t spaceId = H5Dget_space(datasetId);
  REQUIRE(spaceId >= 0);
  const auto spaceGuard = MakeScopeGuard([spaceId]() noexcept { H5Sclose(spaceId); });
  REQUIRE(H5Sget_simple_extent_ndims(spaceId) == 2);
  REQUIRE(H5Sget_simple_extent_npoints(spaceId) == static_cast<hssize_t>(Count));
  std::array<hsize_t, 2> dimensions{};
  REQUIRE(H5Sget_simple_extent_dims(spaceId, dimensions.data(), nullptr) == 2);
  REQUIRE(dimensions == std::array<hsize_t, 2>{Count, 1});
  std::array<int32, Count> actual{};
  REQUIRE(H5Dread(datasetId, H5T_NATIVE_INT32, H5S_ALL, H5S_ALL, H5P_DEFAULT, actual.data()) >= 0);
  REQUIRE(actual == expected);
}

void WriteImportPolicyFile(const fs::path& path)
{
  DataStructure source;
  source.setFormatResolver(std::make_shared<InMemoryFormatResolver>());
  auto* group = DataGroup::Create(source, k_ImportPolicyGroup.getTargetName());
  REQUIRE(group != nullptr);
  CreateImportPolicyArray(source, k_ImportPolicyFirst, k_ImportPolicyFirstValues, group->getId());
  CreateImportPolicyArray(source, k_ImportPolicySecond, k_ImportPolicySecondValues, group->getId());
  CreateImportPolicyArray(source, k_ImportPolicyExcluded, k_ImportPolicyExcludedValues, group->getId());
  const auto writeResult = DREAM3D::WriteFile(path, source);
  SIMPLNX_RESULT_REQUIRE_VALID(writeResult);
  {
    const auto fileReader = HDF5::FileIO::ReadFile(path);
    REQUIRE(fileReader.isValid());
    CheckImportPolicyFileArray(fileReader, k_ImportPolicyFirst, k_ImportPolicyFirstValues);
    CheckImportPolicyFileArray(fileReader, k_ImportPolicySecond, k_ImportPolicySecondValues);
    CheckImportPolicyFileArray(fileReader, k_ImportPolicyExcluded, k_ImportPolicyExcludedValues);
  }
  fs::last_write_time(path, fs::file_time_type::clock::now() - std::chrono::seconds(10));
  REQUIRE(fs::file_time_type::clock::now() - fs::last_write_time(path) >= DREAM3D::Dream3dPreflightCache::k_MtimeTrustWindow);
}

template <usize Count>
void CheckImportPolicyArray(const DataStructure& dataStructure, const DataPath& path, const std::array<int32, Count>& expected, IDataAction::Mode mode)
{
  const auto* array = dataStructure.getDataAs<Int32Array>(path);
  REQUIRE(array != nullptr);
  CHECK(dataStructure.getData(array->getId()) == array);
  REQUIRE(array->getTupleShape() == ShapeType{Count});
  REQUIRE(array->getComponentShape() == ShapeType{1});
  REQUIRE(array->size() == Count);
  REQUIRE(array->getIDataStore() != nullptr);
  CHECK(array->getDataFormat().empty());
  CHECK(array->getIDataStore()->getPlannedStoreType() == IDataStore::StoreType::InMemory);
  if(mode == IDataAction::Mode::Preflight)
  {
    REQUIRE(array->getStoreType() == IDataStore::StoreType::Empty);
    CHECK(array->memoryUsage() == Count * sizeof(int32));
  }
  else
  {
    REQUIRE(array->getStoreType() == IDataStore::StoreType::InMemory);
    for(usize valueIdx = 0; valueIdx < Count; ++valueIdx)
    {
      CHECK((*array)[valueIdx] == expected[valueIdx]);
    }
  }
}

void CheckImportPolicyOutput(const DataStructure& dataStructure, IDataAction::Mode mode)
{
  const auto* group = dataStructure.getDataAs<DataGroup>(k_ImportPolicyGroup);
  REQUIRE(group != nullptr);
  CHECK(dataStructure.getData(group->getId()) == group);
  CheckImportPolicyArray(dataStructure, k_ImportPolicyFirst, k_ImportPolicyFirstValues, mode);
  CheckImportPolicyArray(dataStructure, k_ImportPolicySecond, k_ImportPolicySecondValues, mode);
  CHECK_FALSE(dataStructure.containsData(k_ImportPolicyExcluded));
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

void CheckImportPolicyRecords(const RecordingFormatResolver& resolver)
{
  REQUIRE(resolver.paths.size() == 2);
  REQUIRE(resolver.numericTypes.size() == 2);
  REQUIRE(resolver.logicalBytes.size() == 2);
  CHECK(std::count(resolver.paths.begin(), resolver.paths.end(), k_ImportPolicyFirst) == 1);
  CHECK(std::count(resolver.paths.begin(), resolver.paths.end(), k_ImportPolicySecond) == 1);
  for(usize recordIdx = 0; recordIdx < resolver.paths.size(); ++recordIdx)
  {
    CHECK(resolver.numericTypes[recordIdx] == DataType::int32);
    const uint64 expectedBytes = resolver.paths[recordIdx] == k_ImportPolicyFirst ? sizeof(int32) * k_ImportPolicyFirstValues.size() : sizeof(int32) * k_ImportPolicySecondValues.size();
    CHECK(resolver.logicalBytes[recordIdx] == expectedBytes);
  }
}

void CheckImportPolicyMarker(const DataStructure& destination, const std::shared_ptr<DataObject>& markerOwner, const IDataStore* originalStore)
{
  const auto* marker = destination.getDataAs<Int32Array>(k_ImportPolicyMarker);
  REQUIRE(marker != nullptr);
  CHECK(marker == markerOwner.get());
  CHECK(destination.getData(markerOwner->getId()) == markerOwner.get());
  CHECK(marker->getIDataStore() == originalStore);
  const auto tag = marker->getMetadata().getData("policy-marker");
  const auto* tagValue = std::any_cast<std::string>(&tag);
  REQUIRE(tagValue != nullptr);
  CHECK(*tagValue == "keep-existing-metadata");
  CheckImportPolicyArray(destination, k_ImportPolicyMarker, k_ImportPolicyMarkerValues, IDataAction::Mode::Execute);
}

class RejectingImportProcessResolver : public IDataStoreFormatResolver
{
public:
  mutable usize calls = 0;

  std::string resolveFormat(const DataStructure&, const DataPath&, DataType, uint64) const override
  {
    ++calls;
    throw std::runtime_error("selected-import process policy witness");
  }
};

/**
 * @class CompleteImportContextResolver
 * @brief Records complete selected metadata without changing the resident storage choice.
 */
class CompleteImportContextResolver : public RecordingFormatResolver
{
public:
  mutable usize completeContextCalls = 0;
  mutable usize placeholderContextCalls = 0;

  std::string resolveFormat(const DataStructure& dataStructure, const DataPath& path, DataType type, uint64 bytes) const override
  {
    const auto* first = dataStructure.getDataAs<Int32Array>(k_ImportPolicyFirst);
    const auto* second = dataStructure.getDataAs<Int32Array>(k_ImportPolicySecond);
    if(dataStructure.containsData(k_ImportPolicyMarker) && dataStructure.getDataAs<DataGroup>(k_ImportPolicyGroup) != nullptr && first != nullptr && second != nullptr &&
       !dataStructure.containsData(k_ImportPolicyExcluded))
    {
      ++completeContextCalls;
    }
    if(first != nullptr && second != nullptr && first->getStoreType() == IDataStore::StoreType::Empty && second->getStoreType() == IDataStore::StoreType::Empty)
    {
      ++placeholderContextCalls;
    }
    return RecordingFormatResolver::resolveFormat(dataStructure, path, type, bytes);
  }
};
} // namespace

TEST_CASE("StorageFormatPlan: selected import does not consult process policy", "[StorageFormatPlan][SelectedImportPolicy]")
{
  const auto application = Application::GetOrCreateInstance();
  const auto mode = GENERATE(IDataAction::Mode::Preflight, IDataAction::Mode::Execute);
  const bool warmCache = GENERATE(false, true);
  CAPTURE(static_cast<int>(mode), warmCache);
  const ScopedTempFile file(fs::path(unit_test::k_BinaryTestOutputDir.view()) / "selected_import_process_policy.dream3d");
  auto& cache = DREAM3D::Dream3dPreflightCache::Instance();
  const auto memoryResolver = std::make_shared<InMemoryFormatResolver>();
  const auto cleanup = MakeScopeGuard([&cache, memoryResolver]() noexcept {
    cache.clear();
    DataStructure::setDefaultFormatResolver(memoryResolver);
  });
  DataStructure::setDefaultFormatResolver(memoryResolver);
  cache.clear();
  cache.resetStats();
  WriteImportPolicyFile(file.path);
  const ImportH5ObjectPathsAction action(file.path, {k_ImportPolicySecond, k_ImportPolicyGroup, k_ImportPolicyFirst});
  DataStructure warmDestination;
  warmDestination.setFormatResolver(memoryResolver);
  if(warmCache)
  {
    // The action warms its own metadata mode. An ordinary fetch can use a different cache entry.
    const auto warmResult = action.apply(warmDestination, mode);
    SIMPLNX_RESULT_REQUIRE_VALID(warmResult);
    CheckImportPolicyOutput(warmDestination, mode);
    REQUIRE(cache.missCount() == 1);
    REQUIRE(cache.hitCount() == 0);
    cache.resetStats();
  }
  REQUIRE(cache.missCount() == 0);
  REQUIRE(cache.hitCount() == 0);

  DataStructure destination;
  auto* marker = CreateImportPolicyArray(destination, k_ImportPolicyMarker, k_ImportPolicyMarkerValues);
  marker->getMetadata().setData("policy-marker", std::string("keep-existing-metadata"));
  const auto markerOwner = destination.getSharedData(k_ImportPolicyMarker);
  const auto* markerStore = marker->getIDataStore();
  const auto nextId = destination.getNextId();
  const auto destinationResolver = std::make_shared<RecordingFormatResolver>();
  destination.setFormatResolver(destinationResolver);
  const auto processResolver = std::make_shared<RejectingImportProcessResolver>();
  DataStructure::setDefaultFormatResolver(processResolver);

  const auto result = action.apply(destination, mode);
  INFO("Process-policy callbacks: " << processResolver->calls);
  CHECK(processResolver->calls == 0);
  CHECK(cache.missCount() == (warmCache ? 0 : 1));
  CHECK(cache.hitCount() == (warmCache ? 1 : 0));
  CHECK(&destination.formatResolver() == destinationResolver.get());
  CheckImportPolicyMarker(destination, markerOwner, markerStore);
  if(result.invalid())
  {
    CHECK(destination.getAllDataPaths() == std::vector<DataPath>{k_ImportPolicyMarker});
    CHECK(destination.getNextId() == nextId);
  }
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  CheckImportPolicyRecords(*destinationResolver);
  CheckImportPolicyOutput(destination, mode);
  CHECK(destination.getAllDataPaths().size() == 4);
  if(warmCache)
  {
    for(const auto& path : {k_ImportPolicyFirst, k_ImportPolicySecond})
    {
      const auto* firstHandout = warmDestination.getDataAs<Int32Array>(path);
      const auto* secondHandout = destination.getDataAs<Int32Array>(path);
      REQUIRE(firstHandout != nullptr);
      REQUIRE(secondHandout != nullptr);
      CHECK(firstHandout != secondHandout);
      CHECK(firstHandout->getIDataStore() != secondHandout->getIDataStore());
    }
    CheckImportPolicyOutput(warmDestination, mode);
  }
}

TEST_CASE("StorageFormatPlan: destination policy sees all selected metadata before values", "[StorageFormatPlan][SelectedImportPolicy]")
{
  const auto application = Application::GetOrCreateInstance();
  const auto mode = GENERATE(IDataAction::Mode::Preflight, IDataAction::Mode::Execute);
  CAPTURE(static_cast<int>(mode));
  const ScopedTempFile file(fs::path(unit_test::k_BinaryTestOutputDir.view()) / "selected_import_complete_context.dream3d");
  auto& cache = DREAM3D::Dream3dPreflightCache::Instance();
  const auto memoryResolver = std::make_shared<InMemoryFormatResolver>();
  const auto cleanup = MakeScopeGuard([&cache, memoryResolver]() noexcept {
    cache.clear();
    DataStructure::setDefaultFormatResolver(memoryResolver);
  });
  DataStructure::setDefaultFormatResolver(memoryResolver);
  cache.clear();
  WriteImportPolicyFile(file.path);

  DataStructure destination;
  auto* marker = CreateImportPolicyArray(destination, k_ImportPolicyMarker, k_ImportPolicyMarkerValues);
  marker->getMetadata().setData("policy-marker", std::string("keep-existing-metadata"));
  const auto markerOwner = destination.getSharedData(k_ImportPolicyMarker);
  const auto* markerStore = marker->getIDataStore();
  const auto resolver = std::make_shared<CompleteImportContextResolver>();
  destination.setFormatResolver(resolver);
  const ImportH5ObjectPathsAction action(file.path, {k_ImportPolicySecond, k_ImportPolicyGroup, k_ImportPolicyFirst});
  const auto result = action.apply(destination, mode);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  CheckImportPolicyRecords(*resolver);
  CHECK(resolver->completeContextCalls == 2);
  CHECK(resolver->placeholderContextCalls == 2);
  CHECK(&destination.formatResolver() == resolver.get());
  CheckImportPolicyMarker(destination, markerOwner, markerStore);
  CheckImportPolicyOutput(destination, mode);
  CHECK(destination.getAllDataPaths().size() == 4);
}

namespace
{
struct SelectedImportTestContext
{
  std::shared_ptr<InMemoryFormatResolver> memoryResolver = std::make_shared<InMemoryFormatResolver>();
  ScopedTempFile file;
  DREAM3D::Dream3dPreflightCache& cache = DREAM3D::Dream3dPreflightCache::Instance();

  explicit SelectedImportTestContext(const std::string& name)
  : file(fs::path(unit_test::k_BinaryTestOutputDir.view()) / name)
  {
    DataStructure::setDefaultFormatResolver(memoryResolver);
    cache.clear();
    WriteImportPolicyFile(file.path);
  }

  ~SelectedImportTestContext()
  {
    cache.clear();
    DataStructure::setDefaultFormatResolver(memoryResolver);
  }
};

std::shared_ptr<DataObject> CreateSelectedImportMarker(DataStructure& destination)
{
  auto* marker = CreateImportPolicyArray(destination, k_ImportPolicyMarker, k_ImportPolicyMarkerValues);
  marker->getMetadata().setData("policy-marker", std::string("keep-existing-metadata"));
  return destination.getSharedData(k_ImportPolicyMarker);
}

class LaterImportPlanFailure : public CompleteImportContextResolver
{
public:
  bool allocation = false;

  std::string resolveFormat(const DataStructure& structure, const DataPath& path, DataType type, uint64 bytes) const override
  {
    auto format = CompleteImportContextResolver::resolveFormat(structure, path, type, bytes);
    if(paths.size() == 2)
    {
      if(allocation)
      {
        throw std::bad_alloc();
      }
      throw std::runtime_error("second selected plan witness");
    }
    return format;
  }
};
} // namespace

TEST_CASE("C8 later destination planning failure preserves destination and allocation identity", "[C8][SelectedImportPolicy]")
{
  SelectedImportTestContext context("selected_import_later_plan.dream3d");
  const bool allocation = GENERATE(false, true);
  const auto mode = GENERATE(IDataAction::Mode::Preflight, IDataAction::Mode::Execute);
  CAPTURE(allocation, static_cast<int>(mode));
  DataStructure destination;
  const auto marker = CreateSelectedImportMarker(destination);
  const auto* markerStore = dynamic_cast<const Int32Array*>(marker.get())->getIDataStore();
  const auto savedIds = destination.getAllDataObjectIds();
  const auto nextId = destination.getNextId();
  const auto resolver = std::make_shared<LaterImportPlanFailure>();
  resolver->allocation = allocation;
  destination.setFormatResolver(resolver);
  const ImportH5ObjectPathsAction action(context.file.path, {k_ImportPolicyGroup, k_ImportPolicyFirst, k_ImportPolicySecond});
  if(allocation)
  {
    REQUIRE_THROWS_AS(action.apply(destination, mode), std::bad_alloc);
  }
  else
  {
    const auto result = action.apply(destination, mode);
    REQUIRE(result.invalid());
    REQUIRE_FALSE(result.errors().empty());
    CHECK(result.errors().front().message.find("second selected plan witness") != std::string::npos);
  }
  CheckImportPolicyRecords(*resolver);
  CHECK(resolver->completeContextCalls == 2);
  CHECK(resolver->placeholderContextCalls == 2);
  CHECK(destination.getAllDataObjectIds() == savedIds);
  CHECK(destination.getAllDataPaths() == std::vector<DataPath>{k_ImportPolicyMarker});
  CHECK(destination.getNextId() == nextId);
  CheckImportPolicyMarker(destination, marker, markerStore);
}

#if defined(SIMPLNX_BUILD_TESTS) && SIMPLNX_BUILD_TESTS
TEST_CASE("C8 publication faults remove exact objects and permit immediate ID reuse", "[C8][SelectedImportPublication]")
{
  using Fault = ImportH5ObjectPathsAction::PublicationFault;
  const auto fault = GENERATE(Fault::BeforePublication, Fault::AfterSecondInsert, Fault::AfterHierarchy, Fault::BeforeTracking, Fault::AllocationAfterSecond, Fault::ReturnFalse);
  const auto mode = GENERATE(IDataAction::Mode::Preflight, IDataAction::Mode::Execute);
  CAPTURE(static_cast<int>(fault), static_cast<int>(mode));
  SelectedImportTestContext context("selected_import_publication_faults.dream3d");
  DataStructure destination;
  const auto marker = CreateSelectedImportMarker(destination);
  const auto* markerStore = dynamic_cast<const Int32Array*>(marker.get())->getIDataStore();
  destination.setFormatResolver(context.memoryResolver);
  const auto savedIds = destination.getAllDataObjectIds();
  const auto savedNextId = destination.getNextId();
  std::vector<DataObject::IdType> removals;
  usize additions = 0;
  nod::scoped_connection connection(destination.getSignal().connect([&](DataStructure*, const std::shared_ptr<AbstractDataStructureMessage>& message) {
    if(message->getMsgType() == DataRemovedMessage::MsgType)
    {
      removals.push_back(std::static_pointer_cast<DataRemovedMessage>(message)->getId());
    }
    else if(message->getMsgType() == DataAddedMessage::MsgType)
    {
      ++additions;
    }
  }));
  ImportH5ObjectPathsAction action(context.file.path, {k_ImportPolicyGroup, k_ImportPolicyFirst, k_ImportPolicySecond});
  action.setPublicationFaultForTesting(fault);
  if(fault == Fault::AllocationAfterSecond)
  {
    REQUIRE_THROWS_AS(action.apply(destination, mode), std::bad_alloc);
  }
  else
  {
    const auto result = action.apply(destination, mode);
    REQUIRE(result.invalid());
    REQUIRE_FALSE(result.errors().empty());
    CHECK(result.errors().front().code == -6214);
  }
  REQUIRE(action.publicationFaultConsumedForTesting());
  CHECK(destination.getAllDataObjectIds() == savedIds);
  CHECK(destination.getAllDataPaths() == std::vector<DataPath>{k_ImportPolicyMarker});
  CHECK(destination.getNextId() == savedNextId);
  CheckImportPolicyMarker(destination, marker, markerStore);
  const usize expectedRemovals = fault == Fault::BeforePublication ? 0 : (fault == Fault::AfterSecondInsert || fault == Fault::AllocationAfterSecond ? 2 : 1);
  CHECK(removals.size() == expectedRemovals);
  CHECK(additions == 0);
  auto retained = action.retainedOwnerForTesting();
  if(retained != nullptr)
  {
    CHECK(retained->getDataStructure() == nullptr);
  }
  const ImportH5ObjectPathsAction retry(context.file.path, {k_ImportPolicyGroup, k_ImportPolicyFirst, k_ImportPolicySecond});
  auto retried = retry.apply(destination, mode);
  SIMPLNX_RESULT_REQUIRE_VALID(retried);
  CheckImportPolicyOutput(destination, mode);
  CHECK(additions == 0);
  const auto nextId = destination.getNextId();
  auto* later = DataGroup::Create(destination, "AfterRetry");
  REQUIRE(later != nullptr);
  CHECK(later->getId() == nextId);
  CHECK(destination.getData(nextId) == later);
  CHECK(destination.getData(DataPath({"AfterRetry"})) == later);
  const auto beforeRelease = removals.size();
  retained.reset();
  action.setPublicationFaultForTesting(Fault::None);
  CHECK(removals.size() == beforeRelease);
  CHECK(additions == 1);
  REQUIRE(destination.removeData(nextId));
  CHECK(removals.size() == beforeRelease + 1);
  CHECK(removals.back() == nextId);
  const auto idsAfterOrdinaryRemoval = destination.getAllDataObjectIds();
  CHECK(std::find(idsAfterOrdinaryRemoval.begin(), idsAfterOrdinaryRemoval.end(), nextId) != idsAfterOrdinaryRemoval.end());
  UnitTest::CheckArraysInheritTupleDims(destination);
}

TEST_CASE("C8 refused partial publication retains ownership and reserves identifiers", "[C8][SelectedImportPublication]")
{
  using Fault = ImportH5ObjectPathsAction::PublicationFault;
  const auto fault = GENERATE(Fault::AfterHierarchy, Fault::BeforeTracking, Fault::AfterSecondInsert);
  CAPTURE(static_cast<int>(fault));
  SelectedImportTestContext context("selected_import_refused_cleanup.dream3d");
  DataStructure destination;
  const auto marker = CreateSelectedImportMarker(destination);
  const auto* markerStore = dynamic_cast<const Int32Array*>(marker.get())->getIDataStore();
  const auto savedNextId = destination.getNextId();
  ImportH5ObjectPathsAction action(context.file.path, {k_ImportPolicyGroup, k_ImportPolicyFirst, k_ImportPolicySecond});
  action.setPublicationFaultForTesting(fault, true);
  const auto result = action.apply(destination, IDataAction::Mode::Execute);
  REQUIRE(result.invalid());
  REQUIRE(result.errors().size() >= 2);
  CHECK(result.errors().front().code == -6214);
  CHECK(result.errors()[1].code == -6215);
  REQUIRE(action.publicationFaultConsumedForTesting());
  REQUIRE(action.cleanupFaultConsumedForTesting());
  const auto retained = action.retainedOwnerForTesting();
  REQUIRE(retained != nullptr);
  const DataPath retainedPath({k_ImportPolicyGroup.getTargetName(), retained->getName()});
  REQUIRE(destination.getData(retainedPath) == retained.get());
  REQUIRE(destination.getData(k_ImportPolicyGroup) != nullptr);
  CHECK(destination.getData(retained->getId()) == (fault == Fault::AfterSecondInsert ? retained.get() : nullptr));
  CHECK(destination.getNextId() > retained->getId());
  CHECK(destination.getNextId() > savedNextId);
  auto* unrelated = DataGroup::Create(destination, "AfterRefusal");
  REQUIRE(unrelated != nullptr);
  CHECK(unrelated->getId() > retained->getId());
  const auto advancedNextId = destination.getNextId();
  const auto cleaned = action.cleanupFailedImportForTesting(destination);
  SIMPLNX_RESULT_REQUIRE_VALID(cleaned);
  CHECK_FALSE(destination.containsData(k_ImportPolicyGroup));
  CHECK(destination.getData(retained->getId()) == nullptr);
  const auto remainingIds = destination.getAllDataObjectIds();
  CHECK(std::find(remainingIds.begin(), remainingIds.end(), retained->getId()) == remainingIds.end());
  CHECK(retained->getDataStructure() == nullptr);
  CHECK(destination.getData(DataPath({"AfterRefusal"})) == unrelated);
  CHECK(destination.getNextId() == advancedNextId);
  CheckImportPolicyMarker(destination, marker, markerStore);
  const ImportH5ObjectPathsAction retry(context.file.path, {k_ImportPolicyGroup, k_ImportPolicyFirst, k_ImportPolicySecond});
  auto retried = retry.apply(destination, IDataAction::Mode::Execute);
  SIMPLNX_RESULT_REQUIRE_VALID(retried);
  CheckImportPolicyOutput(destination, IDataAction::Mode::Execute);
}

TEST_CASE("C8 rollback settles all objects before callbacks and preserves callback IDs", "[C8][SelectedImportPublication]")
{
  const bool throwingObserver = GENERATE(false, true);
  CAPTURE(throwingObserver);
  SelectedImportTestContext context("selected_import_observer_cleanup.dream3d");
  DataStructure destination;
  const auto marker = CreateSelectedImportMarker(destination);
  const auto savedNextId = destination.getNextId();
  usize removed = 0;
  bool fullyDetachedAtEveryCallback = true;
  DataObject::IdType callbackId = 0;
  nod::scoped_connection connection(destination.getSignal().connect([&](DataStructure*, const std::shared_ptr<AbstractDataStructureMessage>& message) {
    if(message->getMsgType() != DataRemovedMessage::MsgType)
    {
      return;
    }
    ++removed;
    fullyDetachedAtEveryCallback = fullyDetachedAtEveryCallback && !destination.containsData(k_ImportPolicyGroup);
    if(throwingObserver)
    {
      throw std::runtime_error("checked removal observer witness");
    }
    if(callbackId == 0)
    {
      auto* added = DataGroup::Create(destination, "ObserverAdded");
      if(added != nullptr)
      {
        callbackId = added->getId();
      }
    }
  }));
  const ImportH5ObjectPathsAction action(context.file.path, {k_ImportPolicyGroup, k_ImportPolicyFirst, k_ImportPolicySecond});
  action.setPublicationFaultForTesting(ImportH5ObjectPathsAction::PublicationFault::AfterSecondInsert);
  const auto result = action.apply(destination, IDataAction::Mode::Execute);
  REQUIRE(result.invalid());
  REQUIRE_FALSE(result.errors().empty());
  CHECK(result.errors().front().code == -6214);
  CHECK(removed == 2);
  CHECK(fullyDetachedAtEveryCallback);
  CHECK_FALSE(destination.containsData(k_ImportPolicyGroup));
  if(throwingObserver)
  {
    REQUIRE(result.errors().size() == 3);
    CHECK(result.errors()[1].code == -6216);
    CHECK(result.errors()[2].code == -6216);
    CHECK(destination.getNextId() == savedNextId);
  }
  else
  {
    REQUIRE(callbackId != 0);
    CHECK(destination.getData(DataPath({"ObserverAdded"})) == destination.getData(callbackId));
    CHECK(destination.getNextId() == callbackId + 1);
    CHECK(destination.getAllDataObjectIds().size() == 2);
  }
  CHECK(destination.getData(marker->getId()) == marker.get());
}
#endif

TEST_CASE("C8 import rejects expired ID collisions and checked ID overflow before publication", "[C8][SelectedImportPublication]")
{
  const bool overflow = GENERATE(false, true);
  SelectedImportTestContext context("selected_import_id_precheck.dream3d");
  DataStructure destination;
  auto* old = DataGroup::Create(destination, "Expired");
  REQUIRE(old != nullptr);
  const auto expiredId = old->getId();
  REQUIRE(destination.removeData(expiredId));
  REQUIRE(destination.getData(expiredId) == nullptr);
  const auto beforeIds = destination.getAllDataObjectIds();
  REQUIRE(std::find(beforeIds.begin(), beforeIds.end(), expiredId) != beforeIds.end());
  const auto requestedNext = overflow ? std::numeric_limits<DataObject::IdType>::max() - 1 : expiredId;
  destination.setNextId(requestedNext);
  const ImportH5ObjectPathsAction action(context.file.path, {k_ImportPolicyGroup, k_ImportPolicyFirst});
  const auto result = action.apply(destination, IDataAction::Mode::Preflight);
  REQUIRE(result.invalid());
  REQUIRE_FALSE(result.errors().empty());
  CHECK(result.errors().front().code == -6213);
  CHECK(destination.getAllDataObjectIds() == beforeIds);
  CHECK(destination.getAllDataPaths().empty());
  CHECK(destination.getNextId() == requestedNext);
  const ImportH5ObjectPathsAction empty(context.file.path, {});
  REQUIRE(empty.apply(destination, IDataAction::Mode::Preflight).valid());
  const ImportH5ObjectPathsAction absent(context.file.path, {DataPath({"Absent"})});
  REQUIRE(absent.apply(destination, IDataAction::Mode::Preflight).valid());
  CHECK(destination.getAllDataObjectIds() == beforeIds);
  CHECK(destination.getNextId() == requestedNext);
}

#if defined(SIMPLNX_BUILD_TESTS) && SIMPLNX_BUILD_TESTS
TEST_CASE("C8 rollback preserves an existing destination parent and its other children", "[C8][SelectedImportPublication]")
{
  SelectedImportTestContext context("selected_import_existing_parent_rollback.dream3d");
  DataStructure destination;
  const auto marker = CreateSelectedImportMarker(destination);
  auto* parent = DataGroup::Create(destination, k_ImportPolicyGroup.getTargetName());
  REQUIRE(parent != nullptr);
  const DataPath otherPath({"SelectedGroup", "Other"});
  auto* other = CreateImportPolicyArray(destination, otherPath, k_ImportPolicyMarkerValues, parent->getId());
  const auto parentId = parent->getId();
  const auto otherId = other->getId();
  const auto nextId = destination.getNextId();
  const auto ids = destination.getAllDataObjectIds();
  const ImportH5ObjectPathsAction action(context.file.path, {k_ImportPolicyFirst, k_ImportPolicySecond});
  action.setPublicationFaultForTesting(ImportH5ObjectPathsAction::PublicationFault::AfterHierarchy);
  const auto failed = action.apply(destination, IDataAction::Mode::Execute);
  REQUIRE(failed.invalid());
  REQUIRE(action.publicationFaultConsumedForTesting());
  CHECK(destination.getData(parentId) == parent);
  CHECK(destination.getData(k_ImportPolicyGroup) == parent);
  CHECK(destination.getData(otherId) == other);
  CHECK(parent->getSize() == 1);
  CHECK(destination.getAllDataObjectIds() == ids);
  CHECK(destination.getNextId() == nextId);
  CheckImportPolicyArray(destination, otherPath, k_ImportPolicyMarkerValues, IDataAction::Mode::Execute);
  CHECK(destination.getData(marker->getId()) == marker.get());
  UnitTest::CheckArraysInheritTupleDims(destination);
}
#endif

#if !defined(SIMPLNX_STORE_COPY_STRICT_FORMAT) || !SIMPLNX_STORE_COPY_STRICT_FORMAT
namespace
{
const DataPath k_C8FinalizerLists({"SelectedGroup", "Lists"});
const DataPath k_C8FinalizerStrings({"SelectedGroup", "Strings"});
constexpr int32 k_C8FinalizerWarning = -89531;
constexpr int32 k_C8SecondLeafFailure = -89532;
constexpr int32 k_C8DuplicateFinalizer = -89533;
const DataPath k_C8LegacyContainer({"PlanContainer"});
const DataPath k_C8LegacyMatrix({"PlanContainer", "CellData"});
const DataPath k_C8LegacyFirst({"PlanContainer", "CellData", "First"});
const DataPath k_C8LegacySecond({"PlanContainer", "CellData", "Second"});

/**
 * @class C8SingleImportFinalizer
 * @brief Observes the actual finalizer callback and materializes each tiny selected leaf once.
 *
 * The in-core tests register this manager only when no finalizer is active. The manager is disabled before its test ends.
 */
class C8SingleImportFinalizer : public IDataIOManager
{
public:
  bool active = false;
  bool failOnSecondLeaf = false;
  bool failureConsumed = false;
  usize calls = 0;
  usize leafAttempts = 0;
  usize numericReads = 0;
  bool completeEmptyContext = false;
  std::vector<DataPath> expectedLeaves;
  std::vector<DataPath> numericPaths;
  std::vector<DataPath> ancestors;
  std::vector<DataPath> observedLeaves;
  std::vector<DataPath> completedLeaves;

  std::string formatName() const override
  {
    return "C8-single-import-finalizer";
  }

  bool finalizesImport() const override
  {
    return active;
  }

  /**
   * @brief Checks staged plans and completes each selected leaf through existing readers.
   * @param structure Supplies the prepared destination context.
   * @param paths Supplies only the selected numeric, list, and string leaves.
   * @param reader Supplies the closed-and-reopened source fixture.
   * @return Completed-read warning or the deliberate second-leaf or duplicate-call error.
   */
  Result<> onImportFinalize(DataStructure& structure, const std::vector<DataPath>& paths, const HDF5::FileIO& reader) override
  {
    ++calls;
    if(calls != 1)
    {
      return MakeErrorResult(k_C8DuplicateFinalizer, "duplicate selected-import finalizer witness");
    }
    observedLeaves = paths;
    auto actual = paths;
    auto expected = expectedLeaves;
    std::sort(actual.begin(), actual.end());
    std::sort(expected.begin(), expected.end());
    completeEmptyContext = actual == expected && structure.containsData(k_ImportPolicyMarker) && !structure.containsData(k_ImportPolicyExcluded);
    for(const auto& path : ancestors)
    {
      completeEmptyContext = completeEmptyContext && structure.getDataAs<BaseGroup>(path) != nullptr;
    }
    for(const auto& path : numericPaths)
    {
      const auto* array = structure.getDataAs<Int32Array>(path);
      completeEmptyContext = completeEmptyContext && array != nullptr && array->getStoreType() == IDataStore::StoreType::Empty && array->getDataFormat().empty() &&
                             array->getIDataStore()->getPlannedStoreType() == IDataStore::StoreType::InMemory;
    }
    if(std::find(paths.begin(), paths.end(), k_C8FinalizerLists) != paths.end())
    {
      const auto* lists = structure.getDataAs<NeighborList<int32>>(k_C8FinalizerLists);
      const auto* strings = structure.getDataAs<StringArray>(k_C8FinalizerStrings);
      completeEmptyContext = completeEmptyContext && lists != nullptr && dynamic_cast<EmptyListStore<int32>*>(lists->getStore().get()) != nullptr && strings != nullptr && strings->isPlaceholder();
    }
    if(!completeEmptyContext)
    {
      return MakeErrorResult(-89534, "selected-import finalizer received incomplete or materialized metadata");
    }

    Result<> result;
    const std::string root = DREAM3D::GetFileVersion(reader) == DREAM3D::k_LegacyFileVersion ? "/DataContainers/" : "/DataStructure/";
    for(const auto& path : expectedLeaves)
    {
      ++leafAttempts;
      if(failOnSecondLeaf && leafAttempts == 2)
      {
        failureConsumed = true;
        auto failure = MakeErrorResult(k_C8SecondLeafFailure, "second selected leaf materialization witness");
        failure.warnings() = std::move(result.warnings());
        return failure;
      }
      Result<> step;
      if(auto* array = structure.getDataAs<Int32Array>(path); array != nullptr)
      {
        const auto dataset = reader.openDataset(root + path.toString());
        ++numericReads;
        auto read = HDF5::DataStoreIO::ReadDataStoreIntoMemory<int32>(dataset);
        if(read.invalid())
        {
          return MergeResults(std::move(result), ConvertResult(std::move(read)));
        }
        if(read.value() == nullptr)
        {
          return MergeResults(std::move(result), MakeErrorResult(-89535, "finalizer fixture unexpectedly returned a recovery placeholder"));
        }
        auto warnings = std::move(read.warnings());
        step = array->setDataStore(std::move(read.value()));
        step.warnings().insert(step.warnings().begin(), std::make_move_iterator(warnings.begin()), std::make_move_iterator(warnings.end()));
      }
      else
      {
        step = HDF5::DataStructureReader::FinishImportingObject(structure, reader, path);
      }
      result = MergeResults(std::move(result), std::move(step));
      if(result.invalid())
      {
        return result;
      }
      completedLeaves.push_back(path);
      if(completedLeaves.size() == 1)
      {
        result.warnings().push_back({k_C8FinalizerWarning, "first selected leaf materialized"});
      }
    }
    return result;
  }
};

/**
 * @brief Writes a mixed NX fixture and validates its numeric values before the finalizer is active.
 * @param path Identifies the closed source file.
 */
void WriteC8FinalizerFile(const fs::path& path)
{
  DataStructure source;
  source.setFormatResolver(std::make_shared<InMemoryFormatResolver>());
  auto* group = DataGroup::Create(source, k_ImportPolicyGroup.getTargetName());
  REQUIRE(group != nullptr);
  CreateImportPolicyArray(source, k_ImportPolicyFirst, k_ImportPolicyFirstValues, group->getId());
  CreateImportPolicyArray(source, k_ImportPolicySecond, k_ImportPolicySecondValues, group->getId());
  CreateImportPolicyArray(source, k_ImportPolicyExcluded, k_ImportPolicyExcludedValues, group->getId());
  auto* lists = NeighborList<int32>::Create(source, "Lists", ShapeType{3}, group->getId());
  REQUIRE(lists != nullptr);
  lists->setList(0, std::vector<int32>{31, -7});
  lists->setList(1, std::vector<int32>{});
  lists->setList(2, std::vector<int32>{9});
  REQUIRE(StringArray::CreateWithValues(source, "Strings", ShapeType{2}, {"first", "last"}, group->getId()) != nullptr);
  auto write = DREAM3D::WriteFile(path, source);
  SIMPLNX_RESULT_REQUIRE_VALID(write);
  {
    const auto reader = HDF5::FileIO::ReadFile(path);
    REQUIRE(reader.isValid());
    CheckImportPolicyFileArray(reader, k_ImportPolicyFirst, k_ImportPolicyFirstValues);
    CheckImportPolicyFileArray(reader, k_ImportPolicySecond, k_ImportPolicySecondValues);
    CheckImportPolicyFileArray(reader, k_C8FinalizerLists, std::array<int32, 3>{31, -7, 9});
    CheckImportPolicyFileArray(reader, k_ImportPolicyGroup.createChildPath(lists->getNumNeighborsArrayName()), std::array<int32, 3>{2, 0, 1});
  }
  fs::last_write_time(path, fs::file_time_type::clock::now() - std::chrono::seconds(10));
}

/**
 * @brief Creates two bounded legacy arrays and one real unsupported-object warning.
 * @param path Identifies the closed fixture file.
 */
void WriteC8LegacyPlanWarningFile(const fs::path& path)
{
  {
    auto file = HDF5::FileIO::WriteFile(path);
    REQUIRE(file.isValid());
    REQUIRE(file.writeStringAttribute("FileVersion", DREAM3D::k_LegacyFileVersion.str()).valid());
    auto containers = file.createGroup("DataContainers");
    auto container = containers.createGroup("PlanContainer");
    auto matrix = container.createGroup("CellData");
    REQUIRE(matrix.writeVectorAttribute("TupleDimensions", ShapeType{3}).valid());
    REQUIRE(matrix.writeScalarAttribute<uint32>("AttributeMatrixType", 13).valid());
    for(const std::string name : {"First", "Second"})
    {
      auto dataset = matrix.createDataset(name);
      const std::array<int32, 3> values = name == "First" ? std::array<int32, 3>{11, -4, 6} : std::array<int32, 3>{27, 0, -19};
      REQUIRE(dataset.writeSpan<int32>({3, 1}, nonstd::span<const int32>(values.data(), values.size())).valid());
      REQUIRE(dataset.writeStringAttribute(Constants::k_ObjectTypeTag, Int32Array::GetTypeName()).valid());
      REQUIRE(dataset.writeVectorAttribute("TupleDimensions", ShapeType{3}).valid());
      REQUIRE(dataset.writeVectorAttribute("ComponentDimensions", ShapeType{1}).valid());
    }
    auto unsupported = matrix.createGroup("WarningSource");
    REQUIRE(unsupported.writeStringAttribute(Constants::k_ObjectTypeTag, "UnsupportedLegacyObject").valid());
  }
  {
    const auto reader = HDF5::FileIO::ReadFile(path);
    REQUIRE(reader.isValid());
    const auto fileId = reader.getId();
    const std::lock_guard lock(HDF5::Support::ApiLock());
    for(const std::string name : {"First", "Second"})
    {
      const std::string datasetPath = "/DataContainers/PlanContainer/CellData/" + name;
      const hid_t dataset = H5Dopen2(fileId, datasetPath.c_str(), H5P_DEFAULT);
      REQUIRE(dataset >= 0);
      const auto datasetGuard = MakeScopeGuard([dataset]() noexcept { H5Dclose(dataset); });
      const hid_t space = H5Dget_space(dataset);
      REQUIRE(space >= 0);
      const auto spaceGuard = MakeScopeGuard([space]() noexcept { H5Sclose(space); });
      REQUIRE(H5Sget_simple_extent_ndims(space) == 2);
      REQUIRE(H5Sget_simple_extent_npoints(space) == 3);
      std::array<hsize_t, 2> shape{};
      REQUIRE(H5Sget_simple_extent_dims(space, shape.data(), nullptr) == 2);
      REQUIRE(shape == std::array<hsize_t, 2>{3, 1});
      std::array<int32, 3> values{};
      REQUIRE(H5Dread(dataset, H5T_NATIVE_INT32, H5S_ALL, H5S_ALL, H5P_DEFAULT, values.data()) >= 0);
      const std::array<int32, 3> expected = name == "First" ? std::array<int32, 3>{11, -4, 6} : std::array<int32, 3>{27, 0, -19};
      REQUIRE(values == expected);
    }
  }
  fs::last_write_time(path, fs::file_time_type::clock::now() - std::chrono::seconds(10));
}

/**
 * @class C8WarningPlanResolver
 * @brief Records complete legacy plans and can reject the second plan after disk warnings exist.
 */
class C8WarningPlanResolver : public RecordingFormatResolver
{
public:
  bool failOnSecond = false;
  mutable usize completeEmptyCalls = 0;

  std::string resolveFormat(const DataStructure& structure, const DataPath& path, DataType type, uint64 bytes) const override
  {
    const auto* first = structure.getDataAs<Int32Array>(k_C8LegacyFirst);
    const auto* second = structure.getDataAs<Int32Array>(k_C8LegacySecond);
    if(first != nullptr && second != nullptr && first->getStoreType() == IDataStore::StoreType::Empty && second->getStoreType() == IDataStore::StoreType::Empty &&
       structure.containsData(k_ImportPolicyMarker))
    {
      ++completeEmptyCalls;
    }
    auto format = RecordingFormatResolver::resolveFormat(structure, path, type, bytes);
    if(failOnSecond && paths.size() == 2)
    {
      throw std::runtime_error("second selected plan after disk warning witness");
    }
    return format;
  }
};
} // namespace

TEST_CASE("C8 single active finalizer observes complete plans and exact leaves once", "[C8][SelectedImportFinalizer]")
{
  const auto application = Application::GetOrCreateInstance();
  SelectedImportTestContext context("c8_single_finalizer.dream3d");
  WriteC8FinalizerFile(context.file.path);
  auto& collection = application->getIOCollection();
  REQUIRE_FALSE(collection.anyManagerFinalizesImport());
  const auto manager = std::make_shared<C8SingleImportFinalizer>();
  const auto disable = MakeScopeGuard([manager]() noexcept { manager->active = false; });
  auto registration = collection.addIOManager(manager);
  SIMPLNX_RESULT_REQUIRE_VALID(registration);
  manager->expectedLeaves = {k_ImportPolicyFirst, k_ImportPolicySecond, k_C8FinalizerLists, k_C8FinalizerStrings};
  manager->numericPaths = {k_ImportPolicyFirst, k_ImportPolicySecond};
  manager->ancestors = {k_ImportPolicyGroup};
  manager->active = true;
  REQUIRE(collection.anyManagerFinalizesImport());
  REQUIRE(collection.getManager(manager->formatName()) == manager);
  DataStructure destination;
  const auto marker = CreateSelectedImportMarker(destination);
  const auto* markerStore = dynamic_cast<Int32Array*>(marker.get())->getIDataStore();
  const auto resolver = std::make_shared<CompleteImportContextResolver>();
  destination.setFormatResolver(resolver);
  const ImportH5ObjectPathsAction action(context.file.path, {k_C8FinalizerStrings, k_ImportPolicySecond, k_ImportPolicyGroup, k_C8FinalizerLists, k_ImportPolicyFirst});
  auto result = action.apply(destination, IDataAction::Mode::Execute);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  CHECK(manager->calls == 1);
  CHECK(manager->completeEmptyContext);
  CHECK(manager->leafAttempts == 4);
  CHECK(manager->numericReads == 2);
  CHECK(manager->completedLeaves == manager->expectedLeaves);
  REQUIRE(result.warnings().size() == 1);
  CHECK(result.warnings()[0].code == k_C8FinalizerWarning);
  CheckImportPolicyRecords(*resolver);
  CHECK(resolver->completeContextCalls == 2);
  CheckImportPolicyOutput(destination, IDataAction::Mode::Execute);
  CheckImportPolicyMarker(destination, marker, markerStore);
  const auto* lists = destination.getDataAs<NeighborList<int32>>(k_C8FinalizerLists);
  const auto* strings = destination.getDataAs<StringArray>(k_C8FinalizerStrings);
  REQUIRE(lists != nullptr);
  REQUIRE(strings != nullptr);
  CHECK(lists->getList(0) == std::vector<int32>{31, -7});
  CHECK(lists->getList(1).empty());
  CHECK(lists->getList(2) == std::vector<int32>{9});
  CHECK(strings->values() == std::vector<std::string>{"first", "last"});
  CHECK(destination.getAllDataPaths().size() == 6);
  const auto idsBeforeDuplicate = destination.getAllDataObjectIds();
  const auto reader = HDF5::FileIO::ReadFile(context.file.path);
  auto duplicate = manager->onImportFinalize(destination, manager->expectedLeaves, reader);
  REQUIRE(duplicate.invalid());
  REQUIRE(duplicate.errors().size() == 1);
  CHECK(duplicate.errors()[0].code == k_C8DuplicateFinalizer);
  CHECK(manager->calls == 2);
  CHECK(manager->numericReads == 2);
  CHECK(manager->completedLeaves.size() == 4);
  CHECK(destination.getAllDataObjectIds() == idsBeforeDuplicate);
  UnitTest::CheckArraysInheritTupleDims(destination);
}

TEST_CASE("C8 planning and second leaf failures retain earlier warnings without publication", "[C8][SelectedImportFinalizer]")
{
  const bool planningFailure = GENERATE(false, true);
  const bool warm = GENERATE(false, true);
  CAPTURE(planningFailure, warm);
  const auto application = Application::GetOrCreateInstance();
  SelectedImportTestContext context("c8_failure_warning_order.dream3d");
  WriteC8LegacyPlanWarningFile(context.file.path);
  auto& collection = application->getIOCollection();
  REQUIRE_FALSE(collection.anyManagerFinalizesImport());
  const auto manager = std::make_shared<C8SingleImportFinalizer>();
  const auto disable = MakeScopeGuard([manager]() noexcept { manager->active = false; });
  auto registration = collection.addIOManager(manager);
  SIMPLNX_RESULT_REQUIRE_VALID(registration);
  manager->expectedLeaves = {k_C8LegacyFirst, k_C8LegacySecond};
  manager->numericPaths = manager->expectedLeaves;
  manager->ancestors = {k_C8LegacyContainer, k_C8LegacyMatrix};
  manager->failOnSecondLeaf = true;
  manager->active = true;
  if(warm)
  {
    auto metadata = context.cache.fetchNeutralMetadata(context.file.path);
    SIMPLNX_RESULT_REQUIRE_VALID(metadata);
    REQUIRE(metadata.warnings().size() == 1);
    REQUIRE(metadata.warnings()[0].code == -298012);
  }
  context.cache.resetStats();
  DataStructure destination;
  const auto marker = CreateSelectedImportMarker(destination);
  const auto* markerStore = dynamic_cast<Int32Array*>(marker.get())->getIDataStore();
  const auto beforeIds = destination.getAllDataObjectIds();
  const auto beforeNextId = destination.getNextId();
  const auto resolver = std::make_shared<C8WarningPlanResolver>();
  resolver->failOnSecond = planningFailure;
  destination.setFormatResolver(resolver);
  const ImportH5ObjectPathsAction action(context.file.path, {k_C8LegacySecond, k_C8LegacyContainer, k_C8LegacyFirst, k_C8LegacyMatrix});
  auto result = action.apply(destination, IDataAction::Mode::Execute);
  REQUIRE(result.invalid());
  REQUIRE(result.errors().size() == 1);
  REQUIRE(result.warnings().size() == (planningFailure ? 1 : 2));
  CHECK(result.warnings()[0].code == -298012);
  CHECK(result.warnings()[0].message.find("WarningSource") != std::string::npos);
  CHECK(resolver->paths.size() == 2);
  CHECK(std::count(resolver->paths.begin(), resolver->paths.end(), k_C8LegacyFirst) == 1);
  CHECK(std::count(resolver->paths.begin(), resolver->paths.end(), k_C8LegacySecond) == 1);
  CHECK(resolver->numericTypes == std::vector<DataType>{DataType::int32, DataType::int32});
  CHECK(resolver->logicalBytes == std::vector<uint64>{12, 12});
  CHECK(resolver->completeEmptyCalls == 2);
  CHECK(context.cache.hitCount() == (warm ? 1 : 0));
  CHECK(context.cache.missCount() == (warm ? 0 : 1));
  if(planningFailure)
  {
    CHECK(result.errors()[0].code == -10601);
    CHECK(result.errors()[0].message.find("second selected plan after disk warning witness") != std::string::npos);
    CHECK(manager->calls == 0);
    CHECK(manager->leafAttempts == 0);
  }
  else
  {
    CHECK(result.errors()[0].code == k_C8SecondLeafFailure);
    CHECK(result.errors()[0].message == "second selected leaf materialization witness");
    CHECK(result.warnings()[1].code == k_C8FinalizerWarning);
    CHECK(result.warnings()[1].message == "first selected leaf materialized");
    CHECK(manager->calls == 1);
    CHECK(manager->completeEmptyContext);
    CHECK(manager->leafAttempts == 2);
    CHECK(manager->numericReads == 1);
    CHECK(manager->completedLeaves == std::vector<DataPath>{k_C8LegacyFirst});
    CHECK(manager->failureConsumed);
  }
  CHECK(destination.getAllDataPaths() == std::vector<DataPath>{k_ImportPolicyMarker});
  CHECK(destination.getAllDataObjectIds() == beforeIds);
  CHECK(destination.getNextId() == beforeNextId);
  CHECK(&destination.formatResolver() == resolver.get());
  CheckImportPolicyMarker(destination, marker, markerStore);
}
#endif
