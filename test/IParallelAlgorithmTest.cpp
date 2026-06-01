#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

#include <catch2/catch.hpp>

#include <map>
#include <memory>
#include <stdexcept>
#include <string>

using namespace nx::core;

namespace
{
/**
 * @brief Minimal mock data store that reports StoreType::OutOfCore.
 *
 * Only getStoreType() is meaningful; every other method throws because
 * the tests never access actual element data.
 */
template <typename T>
class MockOocDataStore : public AbstractDataStore<T>
{
public:
  using value_type = typename AbstractDataStore<T>::value_type;

  MockOocDataStore(const ShapeType& tupleShape, const ShapeType& componentShape)
  : m_TupleShape(tupleShape)
  , m_ComponentShape(componentShape)
  , m_NumTuples(std::accumulate(tupleShape.cbegin(), tupleShape.cend(), static_cast<usize>(1), std::multiplies<>()))
  , m_NumComponents(std::accumulate(componentShape.cbegin(), componentShape.cend(), static_cast<usize>(1), std::multiplies<>()))
  {
  }

  ~MockOocDataStore() override = default;

  IDataStore::StoreType getStoreType() const override
  {
    return IDataStore::StoreType::OutOfCore;
  }

  std::map<std::string, std::string> getRecoveryMetadata() const override
  {
    return {};
  }

  usize getNumberOfTuples() const override
  {
    return m_NumTuples;
  }

  usize getNumberOfComponents() const override
  {
    return m_NumComponents;
  }

  const ShapeType& getTupleShape() const override
  {
    return m_TupleShape;
  }

  const ShapeType& getComponentShape() const override
  {
    return m_ComponentShape;
  }

  DataType getDataType() const override
  {
    return GetDataType<T>();
  }

  void resizeTuples(const ShapeType& /*tupleShape*/) override
  {
    throw std::runtime_error("MockOocDataStore::resizeTuples not implemented");
  }

  value_type getValue(usize /*index*/) const override
  {
    throw std::runtime_error("MockOocDataStore::getValue not implemented");
  }

  void setValue(usize /*index*/, value_type /*value*/) override
  {
    throw std::runtime_error("MockOocDataStore::setValue not implemented");
  }

  Result<> copyIntoBuffer(usize /*startIndex*/, nonstd::span<T> /*buffer*/) const override
  {
    return MakeErrorResult(-9001, "MockOocDataStore::copyIntoBuffer not implemented");
  }

  Result<> copyFromBuffer(usize /*startIndex*/, nonstd::span<const T> /*buffer*/) override
  {
    return MakeErrorResult(-9002, "MockOocDataStore::copyFromBuffer not implemented");
  }

  std::vector<T> readExtent(const Extent& /*extent*/) const override
  {
    throw std::runtime_error("MockOocDataStore::readExtent not implemented");
  }

  void writeExtent(const Extent& /*extent*/, nonstd::span<const T> /*data*/) override
  {
    throw std::runtime_error("MockOocDataStore::writeExtent not implemented");
  }

  value_type at(usize /*index*/) const override
  {
    throw std::runtime_error("MockOocDataStore::at not implemented");
  }

  void add(usize /*index*/, value_type /*value*/) override
  {
    throw std::runtime_error("MockOocDataStore::add not implemented");
  }

  void sub(usize /*index*/, value_type /*value*/) override
  {
    throw std::runtime_error("MockOocDataStore::sub not implemented");
  }

  void mul(usize /*index*/, value_type /*value*/) override
  {
    throw std::runtime_error("MockOocDataStore::mul not implemented");
  }

  void div(usize /*index*/, value_type /*value*/) override
  {
    throw std::runtime_error("MockOocDataStore::div not implemented");
  }

  void rem(usize /*index*/, value_type /*value*/) override
  {
    throw std::runtime_error("MockOocDataStore::rem not implemented");
  }

  void bitwiseAND(usize /*index*/, value_type /*value*/) override
  {
    throw std::runtime_error("MockOocDataStore::bitwiseAND not implemented");
  }

  void bitwiseOR(usize /*index*/, value_type /*value*/) override
  {
    throw std::runtime_error("MockOocDataStore::bitwiseOR not implemented");
  }

  void bitwiseXOR(usize /*index*/, value_type /*value*/) override
  {
    throw std::runtime_error("MockOocDataStore::bitwiseXOR not implemented");
  }

  void bitwiseLShift(usize /*index*/, value_type /*value*/) override
  {
    throw std::runtime_error("MockOocDataStore::bitwiseLShift not implemented");
  }

  void bitwiseRShift(usize /*index*/, value_type /*value*/) override
  {
    throw std::runtime_error("MockOocDataStore::bitwiseRShift not implemented");
  }

  void byteSwap(usize /*index*/) override
  {
    throw std::runtime_error("MockOocDataStore::byteSwap not implemented");
  }

  void swap(usize /*index1*/, usize /*index2*/) override
  {
    throw std::runtime_error("MockOocDataStore::swap not implemented");
  }

  std::unique_ptr<IDataStore> deepCopy() const override
  {
    return std::make_unique<MockOocDataStore>(*this);
  }

  std::unique_ptr<IDataStore> createNewInstance() const override
  {
    return std::make_unique<MockOocDataStore>(m_TupleShape, m_ComponentShape);
  }

  std::pair<int32, std::string> writeBinaryFile(const std::string& /*absoluteFilePath*/) const override
  {
    return {-1, "MockOocDataStore cannot write files"};
  }

  std::pair<int32, std::string> writeBinaryFile(std::ostream& /*outputStream*/) const override
  {
    return {-1, "MockOocDataStore cannot write files"};
  }

  Result<> readHdf5(const HDF5::DatasetIO& /*dataset*/) override
  {
    return MakeErrorResult(-1, "MockOocDataStore cannot read HDF5");
  }

  Result<> writeHdf5(HDF5::DatasetIO& /*dataset*/) const override
  {
    return MakeErrorResult(-1, "MockOocDataStore cannot write HDF5");
  }

private:
  ShapeType m_TupleShape;
  ShapeType m_ComponentShape;
  usize m_NumTuples = 0;
  usize m_NumComponents = 0;
};
} // namespace

TEST_CASE("IParallelAlgorithm: TBB enabled by default", "[simplnx][IParallelAlgorithm]")
{
  ParallelDataAlgorithm algorithm;

#ifdef SIMPLNX_ENABLE_MULTICORE
  REQUIRE(algorithm.getParallelizationEnabled() == true);
#else
  REQUIRE(algorithm.getParallelizationEnabled() == false);
#endif
}

TEST_CASE("IParallelAlgorithm: requireArraysInMemory with in-memory arrays keeps TBB enabled", "[simplnx][IParallelAlgorithm]")
{
  DataStructure dataStructure;
  auto store = std::make_shared<DataStore<float32>>(ShapeType{10}, ShapeType{1}, 0.0f);
  auto* dataArray = DataArray<float32>::Create(dataStructure, "TestArray", store);
  REQUIRE(dataArray != nullptr);

  ParallelDataAlgorithm algorithm;
  algorithm.requireArraysInMemory({dataArray});

#ifdef SIMPLNX_ENABLE_MULTICORE
  REQUIRE(algorithm.getParallelizationEnabled() == true);
#else
  REQUIRE(algorithm.getParallelizationEnabled() == false);
#endif
}

TEST_CASE("IParallelAlgorithm: requireStoresInMemory with in-memory stores keeps TBB enabled", "[simplnx][IParallelAlgorithm]")
{
  DataStore<float32> store(ShapeType{10}, ShapeType{1}, 0.0f);

  ParallelDataAlgorithm algorithm;
  algorithm.requireStoresInMemory({&store});

#ifdef SIMPLNX_ENABLE_MULTICORE
  REQUIRE(algorithm.getParallelizationEnabled() == true);
#else
  REQUIRE(algorithm.getParallelizationEnabled() == false);
#endif
}

TEST_CASE("IParallelAlgorithm: requireArraysInMemory with OOC arrays disables TBB", "[simplnx][IParallelAlgorithm]")
{
  DataStructure dataStructure;
  auto oocStore = std::make_shared<MockOocDataStore<float32>>(ShapeType{10}, ShapeType{1});
  auto* dataArray = DataArray<float32>::Create(dataStructure, "OocArray", oocStore);
  REQUIRE(dataArray != nullptr);

  ParallelDataAlgorithm algorithm;
  algorithm.requireArraysInMemory({dataArray});

  // OOC arrays should disable parallelization regardless of SIMPLNX_ENABLE_MULTICORE
  REQUIRE(algorithm.getParallelizationEnabled() == false);
}

TEST_CASE("IParallelAlgorithm: requireStoresInMemory with OOC stores disables TBB", "[simplnx][IParallelAlgorithm]")
{
  MockOocDataStore<float32> oocStore(ShapeType{10}, ShapeType{1});

  ParallelDataAlgorithm algorithm;
  algorithm.requireStoresInMemory({&oocStore});

  // OOC stores should disable parallelization regardless of SIMPLNX_ENABLE_MULTICORE
  REQUIRE(algorithm.getParallelizationEnabled() == false);
}

TEST_CASE("IParallelAlgorithm: requireStoresInMemory with mixed stores disables TBB", "[simplnx][IParallelAlgorithm]")
{
  DataStore<float32> inMemoryStore(ShapeType{10}, ShapeType{1}, 0.0f);
  MockOocDataStore<float32> oocStore(ShapeType{10}, ShapeType{1});

  ParallelDataAlgorithm algorithm;
  algorithm.requireStoresInMemory({&inMemoryStore, &oocStore});

  // A single OOC store in the mix should disable parallelization
  REQUIRE(algorithm.getParallelizationEnabled() == false);
}

TEST_CASE("IParallelAlgorithm: requireArraysInMemory with empty array list keeps TBB enabled", "[simplnx][IParallelAlgorithm]")
{
  ParallelDataAlgorithm algorithm;
  algorithm.requireArraysInMemory({});

#ifdef SIMPLNX_ENABLE_MULTICORE
  REQUIRE(algorithm.getParallelizationEnabled() == true);
#else
  REQUIRE(algorithm.getParallelizationEnabled() == false);
#endif
}

TEST_CASE("IParallelAlgorithm: requireStoresInMemory with nullptr entries keeps TBB enabled", "[simplnx][IParallelAlgorithm]")
{
  ParallelDataAlgorithm algorithm;
  algorithm.requireStoresInMemory({nullptr, nullptr});

#ifdef SIMPLNX_ENABLE_MULTICORE
  REQUIRE(algorithm.getParallelizationEnabled() == true);
#else
  REQUIRE(algorithm.getParallelizationEnabled() == false);
#endif
}
