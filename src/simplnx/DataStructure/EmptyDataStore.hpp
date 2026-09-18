#pragma once

#include "simplnx/Utilities/StoreCopyUtilities.hpp"

#include "simplnx/DataStructure/AbstractDataStore.hpp"

#include <fmt/format.h>
#include <fmt/ranges.h>

#include <algorithm>
#include <limits>
#include <map>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

/**
 * @namespace nx::core
 * @brief Contains simplnx core types and functions.
 */
namespace nx::core
{
/**
 * @class EmptyDataStore
 * @brief Preserves data-store metadata without allocating values.
 * @tparam T Planned value type.
 */
template <typename T>
class EmptyDataStore : public AbstractDataStore<T>
{
public:
  /**
   * @brief Names the planned value type.
   */
  using value_type = typename AbstractDataStore<T>::value_type;

  /**
   * @brief Names the mutable value-proxy type.
   */
  using reference = typename AbstractDataStore<T>::reference;

  /**
   * @brief Creates a metadata store from a validated storage selection.
   * @param tupleShape Planned tuple dimensions in slowest-to-fastest order.
   * @param componentShape Planned component dimensions in slowest-to-fastest order.
   * @param selectedFormat Selected format. Empty and the canonical in-memory name select memory.
   * @return A metadata-only store, or a validation error.
   * @throws std::bad_alloc If metadata allocation fails.
   *
   * An empty shape vector has an identity product of one. A zero dimension gives zero elements. The function checks each product and byte count before construction. It does not create a value store.
   */
  static Result<std::unique_ptr<EmptyDataStore<T>>> Create(const ShapeType& tupleShape, const ShapeType& componentShape, const std::string& selectedFormat)
  {
    try
    {
      auto formatResult = ValidateNumericStorageFormat(selectedFormat);
      if(formatResult.invalid())
      {
        return ConvertInvalidResult<std::unique_ptr<EmptyDataStore<T>>>(std::move(formatResult));
      }

      const usize tupleCount = CheckedPlaceholderProduct(tupleShape, "tuple");
      const usize componentCount = CheckedPlaceholderProduct(componentShape, "component");
      if(componentCount != 0 && tupleCount > std::numeric_limits<usize>::max() / componentCount)
      {
        throw std::runtime_error("The numeric placeholder value count exceeds the supported size");
      }
      const usize valueCount = tupleCount * componentCount;
      if(valueCount > std::numeric_limits<usize>::max() / sizeof(T) || valueCount > std::numeric_limits<uint64>::max() / sizeof(T))
      {
        throw std::runtime_error("The numeric placeholder byte count exceeds the supported size");
      }

      auto store = std::unique_ptr<EmptyDataStore<T>>(new EmptyDataStore<T>(ValidatedConstruction{}, tupleShape, componentShape, tupleCount, componentCount, formatResult.value()));
      Result<std::unique_ptr<EmptyDataStore<T>>> result{std::move(store)};
      result.warnings() = std::move(formatResult.warnings());
      return result;
    } catch(const std::bad_alloc&)
    {
      throw;
    } catch(const std::exception& error)
    {
      return MakeErrorResult<std::unique_ptr<EmptyDataStore<T>>>(k_CreationError,
                                                                 fmt::format("Cannot create numeric placeholder with tuple shape [{}], component shape [{}], and selected format '{}': {}",
                                                                             fmt::join(tupleShape, ", "), fmt::join(componentShape, ", "), selectedFormat, error.what()));
    }
  }

  /**
   * @brief Copies metadata-store state.
   * @param other Source metadata store.
   */
  EmptyDataStore(const EmptyDataStore& other)
  : m_ComponentShape(other.m_ComponentShape)
  , m_TupleShape(other.m_TupleShape)
  , m_NumComponents(other.m_NumComponents)
  , m_NumTuples(other.m_NumTuples)
  , m_DataFormat(other.m_DataFormat)
  {
  }

  /**
   * @brief Moves metadata-store state.
   * @param other Source metadata store.
   */
  EmptyDataStore(EmptyDataStore&& other) noexcept
  : m_ComponentShape(std::move(other.m_ComponentShape))
  , m_TupleShape(std::move(other.m_TupleShape))
  , m_NumComponents(other.m_NumComponents)
  , m_NumTuples(other.m_NumTuples)
  , m_DataFormat(std::move(other.m_DataFormat))
  {
    other.m_ComponentShape.clear();
    other.m_TupleShape.clear();
    other.m_NumComponents = 1;
    other.m_NumTuples = 1;
    other.m_DataFormat.clear();
  }

  /**
   * @brief Destroys the metadata store.
   */
  ~EmptyDataStore() override = default;

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

  IDataStore::StoreType getStoreType() const override
  {
    return IDataStore::StoreType::Empty;
  }

  /**
   * @brief Returns the store type that materializes after preflight.
   *
   * An empty planned format selects in-memory storage. A non-empty format
   * selects out-of-core storage without allocating values.
   * @return Planned in-memory or out-of-core store type.
   */
  IDataStore::StoreType getPlannedStoreType() const override
  {
    return m_DataFormat.empty() ? IDataStore::StoreType::InMemory : IDataStore::StoreType::OutOfCore;
  }

  /**
   * @brief Rejects recovery metadata access.
   * @return Does not return.
   * @throws std::runtime_error Always, because this store has no backing data.
   */
  std::map<std::string, std::string> getRecoveryMetadata() const override
  {
    throw std::runtime_error("EmptyDataStore::getRecoveryMetadata: cannot query recovery metadata on a placeholder store");
  }

  /**
   * @brief Returns the recorded storage selection without refreshing policy.
   * @return Empty string for memory, or the validated out-of-core format name.
   *
   * The factory records one normalized selection. Preference changes do not alter this value.
   */
  std::string getDataFormat() const override
  {
    return m_DataFormat;
  }

  /**
   * @brief Returns planned in-memory usage in bytes.
   * @return Logical byte size for in-memory storage, or zero for out-of-core storage.
   *
   * The recorded format determines whether materialization uses memory or an out-of-core store.
   */
  uint64 memoryUsage() const override
  {
    return m_DataFormat.empty() ? (sizeof(T) * this->getSize()) : 0;
  }

  /**
   * @brief Changes the placeholder tuple shape without accessing values.
   * @param tupleShape New tuple dimensions in slowest-to-fastest order.
   * @return Valid on success, or an error if the tuple count exceeds the supported size.
   *
   * Preflight must resize metadata before execution materializes the data store.
   */
  [[nodiscard]] Result<> resizeTuples(const ShapeType& tupleShape) override
  {
    try
    {
      const usize tupleCount = CheckedPlaceholderProduct(tupleShape, "tuple");
      if(m_NumComponents != 0 && tupleCount > std::numeric_limits<usize>::max() / m_NumComponents)
      {
        throw std::runtime_error("The numeric placeholder value count exceeds the supported size");
      }
      const usize valueCount = tupleCount * m_NumComponents;
      if(valueCount > std::numeric_limits<usize>::max() / sizeof(T) || valueCount > std::numeric_limits<uint64>::max() / sizeof(T))
      {
        throw std::runtime_error("The numeric placeholder byte count exceeds the supported size");
      }
      m_TupleShape = tupleShape;
      m_NumTuples = tupleCount;
      return {};
    } catch(const std::bad_alloc&)
    {
      throw;
    } catch(const std::exception& error)
    {
      return MakeErrorResult(k_ResizeError, fmt::format("Cannot resize numeric placeholder to tuple shape [{}]: {}", fmt::join(tupleShape, ", "), error.what()));
    }
  }

  /**
   * @brief Rejects value access.
   * @param index Flat value index.
   * @return Does not return.
   * @throws std::runtime_error Always, because this store has no values.
   */
  value_type getValue(usize index) const override
  {
    throw std::runtime_error("EmptyDataStore::getValue() is not implemented");
  }

  /**
   * @brief Rejects value writes.
   * @param index Flat value index.
   * @param value Value to store.
   * @throws std::runtime_error Always, because this store has no values.
   */
  void setValue(usize index, value_type value) override
  {
    throw std::runtime_error("EmptyDataStore::setValue() is not implemented");
  }

  /**
   * @brief Rejects bulk reads.
   * @param startIndex First requested flat value index.
   * @param buffer Destination buffer.
   * @return Error -6038 with the requested range because this store has no values.
   */
  [[nodiscard]] Result<> copyIntoBuffer(usize startIndex, nonstd::span<T> buffer) const override
  {
    return MakeErrorResult(-6038, fmt::format("EmptyDataStore bulk read [{}..{}) failed: the metadata-only preflight store has no values.", startIndex, startIndex + buffer.size()));
  }

  /**
   * @brief Rejects bulk writes.
   * @param startIndex First requested flat value index.
   * @param buffer Source buffer.
   * @return Error -6038 with the requested range because this store has no values.
   */
  [[nodiscard]] Result<> copyFromBuffer(usize startIndex, nonstd::span<const T> buffer) override
  {
    return MakeErrorResult(-6038, fmt::format("EmptyDataStore bulk write [{}..{}) failed: the metadata-only preflight store has no values.", startIndex, startIndex + buffer.size()));
  }

  /**
   * @brief Rejects extent reads.
   * @param extent Requested tuple-space extent.
   * @return Error -6038 with the requested extent because this store has no values.
   */
  [[nodiscard]] Result<std::vector<T>> readExtent(const Extent& extent) const override
  {
    return MakeErrorResult<std::vector<T>>(-6038, fmt::format("EmptyDataStore extent read min [{}], max [{}], stride [{}] failed: the metadata-only preflight store has no values.",
                                                              fmt::join(extent.min, ", "), fmt::join(extent.max, ", "), fmt::join(extent.stride, ", ")));
  }

  /**
   * @brief Rejects caller-buffer extent reads.
   * @param extent Requested tuple-space extent.
   * @param destination Destination buffer.
   * @return Error -6038 with the requested extent because this store has no values.
   */
  [[nodiscard]] Result<> readExtentIntoBuffer(const Extent& extent, nonstd::span<T> destination) const override
  {
    return MakeErrorResult(-6038, fmt::format("EmptyDataStore extent read min [{}], max [{}], stride [{}] into {} values failed: the metadata-only preflight store has no values.",
                                              fmt::join(extent.min, ", "), fmt::join(extent.max, ", "), fmt::join(extent.stride, ", "), destination.size()));
  }

  /**
   * @brief Rejects extent writes.
   * @param extent Requested tuple-space extent.
   * @param data Source values.
   * @return Error -6038 with the requested extent because this store has no values.
   */
  [[nodiscard]] Result<> writeExtent(const Extent& extent, nonstd::span<const T> data) override
  {
    return MakeErrorResult(-6038, fmt::format("EmptyDataStore extent write min [{}], max [{}], stride [{}] from {} values failed: the metadata-only preflight store has no values.",
                                              fmt::join(extent.min, ", "), fmt::join(extent.max, ", "), fmt::join(extent.stride, ", "), data.size()));
  }

  /**
   * @brief Rejects bounds-checked value access.
   * @param index Flat value index.
   * @return Does not return.
   * @throws std::runtime_error Always, because this store has no values.
   */
  value_type at(usize index) const override
  {
    throw std::runtime_error("EmptyDataStore::at() is not implemented");
  }

  /**
   * @brief Rejects value addition.
   * @param index Flat value index.
   * @param value Value to add.
   * @throws std::runtime_error Always, because this store has no values.
   */
  void add(usize index, value_type value) override
  {
    throw std::runtime_error("EmptyDataStore::add() is not implemented");
  }

  /**
   * @brief Rejects value subtraction.
   * @param index Flat value index.
   * @param value Value to subtract.
   * @throws std::runtime_error Always, because this store has no values.
   */
  void sub(usize index, value_type value) override
  {
    throw std::runtime_error("EmptyDataStore::sub() is not implemented");
  }

  /**
   * @brief Rejects value multiplication.
   * @param index Flat value index.
   * @param value Multiplier.
   * @throws std::runtime_error Always, because this store has no values.
   */
  void mul(usize index, value_type value) override
  {
    throw std::runtime_error("EmptyDataStore::mul() is not implemented");
  }

  /**
   * @brief Rejects value division.
   * @param index Flat value index.
   * @param value Divisor.
   * @throws std::runtime_error Always, because this store has no values.
   */
  void div(usize index, value_type value) override
  {
    throw std::runtime_error("EmptyDataStore::div() is not implemented");
  }

  /**
   * @brief Rejects remainder operations.
   * @param index Flat value index.
   * @param value Divisor.
   * @throws std::runtime_error Always, because this store has no values.
   */
  void rem(usize index, value_type value) override
  {
    throw std::runtime_error("EmptyDataStore::rem() is not implemented");
  }

  /**
   * @brief Rejects bitwise AND operations.
   * @param index Flat value index.
   * @param value Operand.
   * @throws std::runtime_error Always, because this store has no values.
   */
  void bitwiseAND(usize index, value_type value) override
  {
    throw std::runtime_error("EmptyDataStore::bitwiseAND() is not implemented");
  }

  /**
   * @brief Rejects bitwise OR operations.
   * @param index Flat value index.
   * @param value Operand.
   * @throws std::runtime_error Always, because this store has no values.
   */
  void bitwiseOR(usize index, value_type value) override
  {
    throw std::runtime_error("EmptyDataStore::bitwiseOR() is not implemented");
  }

  /**
   * @brief Rejects bitwise XOR operations.
   * @param index Flat value index.
   * @param value Operand.
   * @throws std::runtime_error Always, because this store has no values.
   */
  void bitwiseXOR(usize index, value_type value) override
  {
    throw std::runtime_error("EmptyDataStore::bitwiseXOR() is not implemented");
  }

  /**
   * @brief Rejects left-shift operations.
   * @param index Flat value index.
   * @param value Shift count.
   * @throws std::runtime_error Always, because this store has no values.
   */
  void bitwiseLShift(usize index, value_type value) override
  {
    throw std::runtime_error("EmptyDataStore::bitwiseLShift() is not implemented");
  }

  /**
   * @brief Rejects right-shift operations.
   * @param index Flat value index.
   * @param value Shift count.
   * @throws std::runtime_error Always, because this store has no values.
   */
  void bitwiseRShift(usize index, value_type value) override
  {
    throw std::runtime_error("EmptyDataStore::bitwiseRShift() is not implemented");
  }

  /**
   * @brief Rejects byte-order changes.
   * @param index Flat value index.
   * @throws std::runtime_error Always, because this store has no values.
   */
  void byteSwap(usize index) override
  {
    throw std::runtime_error("EmptyDataStore::byteSwap() is not implemented");
  }

  /**
   * @brief Rejects value swaps.
   * @param index1 First flat value index.
   * @param index2 Second flat value index.
   * @throws std::runtime_error Always, because this store has no values.
   */
  void swap(usize index1, usize index2) override
  {
    throw std::runtime_error("EmptyDataStore::swap() is not implemented");
  }

  /**
   * @brief Copies numeric shape metadata without allocating values.
   * @param destinationFormat Resolved destination format; empty and the canonical in-memory name select memory.
   * @return Independent metadata placeholder with the same shape and no values.
   * @throws std::runtime_error If metadata validation fails or an OOC build rejects an unavailable format.
   * @throws std::bad_alloc If a metadata allocation fails.
   *
   * In-core builds use the in-memory selection for unavailable formats. OOC builds reject unavailable formats.
   * In-memory selection and in-core fallback store an empty planned-format name. Other supported formats retain their planned-format name.
   * This store validates destinationFormat and does not resolve storage policy itself.
   * Resolve destination policy before this call. Use DataArray::deepCopy for automatic policy selection.
   * @see DataArray::deepCopy
   */
  std::unique_ptr<IDataStore> deepCopy(const std::string& destinationFormat) const override
  {
    return CopyDataStore(*this, destinationFormat);
  }

  /**
   * @brief Creates a metadata store with the same shapes.
   * @return Owning metadata store.
   */
  std::unique_ptr<IDataStore> createNewInstance() const override
  {
    return std::make_unique<EmptyDataStore<T>>(*this);
  }

  /**
   * @brief Rejects binary-file writes.
   * @param absoluteFilePath Destination file path.
   * @return Error code and message because this store has no values.
   */
  std::pair<int32, std::string> writeBinaryFile(const std::string& absoluteFilePath) const override
  {
    return {-10175, fmt::format("EmptyDataStore cannot read or write files", absoluteFilePath)};
  }

  /**
   * @brief Rejects binary-stream writes.
   * @param outputStream Destination stream.
   * @return Error code and message because this store has no values.
   */
  std::pair<int32, std::string> writeBinaryFile(std::ostream& outputStream) const override
  {
    return {-10175, fmt::format("EmptyDataStore cannot read or write files")};
  }

  /**
   * @brief Rejects HDF5 reads.
   * @param dataset HDF5 dataset to read.
   * @return Error because this store has no values.
   */
  Result<> readHdf5(const HDF5::DatasetIO& dataset) override
  {
    return MakeErrorResult(-42350, "Cannot read data into an EmptyDataStore");
  }

  /**
   * @brief Rejects HDF5 writes.
   * @param dataset HDF5 dataset to write.
   * @return Error because this store has no values.
   */
  Result<> writeHdf5(HDF5::DatasetIO& dataset) const override
  {
    return MakeErrorResult(-42350, "Cannot write data from an EmptyDataStore");
  }

private:
  static inline constexpr int32 k_CreationError = -10602;
  static inline constexpr int32 k_ResizeError = -10603;

  struct ValidatedConstruction
  {
  };

  EmptyDataStore(ValidatedConstruction, const ShapeType& tupleShape, const ShapeType& componentShape, usize tupleCount, usize componentCount, std::string dataFormat)
  : m_ComponentShape(componentShape)
  , m_TupleShape(tupleShape)
  , m_NumComponents(componentCount)
  , m_NumTuples(tupleCount)
  , m_DataFormat(std::move(dataFormat))
  {
  }

  static usize CheckedPlaceholderProduct(const ShapeType& shape, const std::string& shapeName)
  {
    if(std::find(shape.begin(), shape.end(), 0) != shape.end())
    {
      return 0;
    }
    usize count = 1;
    for(const usize extent : shape)
    {
      if(extent > std::numeric_limits<usize>::max() / count)
      {
        throw std::runtime_error(fmt::format("The numeric placeholder {} shape product exceeds the supported size", shapeName));
      }
      count *= extent;
    }
    return count;
  }

  ShapeType m_ComponentShape;
  ShapeType m_TupleShape;
  usize m_NumComponents = {0};
  usize m_NumTuples = {0};
  std::string m_DataFormat = "";
};
} // namespace nx::core
