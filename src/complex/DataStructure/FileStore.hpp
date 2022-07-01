#pragma once

#include "complex/DataStructure/AbstractDataStore.hpp"
#include "complex/Utilities/Parsing/HDF5/H5AttributeReader.hpp"
#include "complex/Utilities/Parsing/HDF5/H5DatasetReader.hpp"
#include "complex/Utilities/Parsing/HDF5/H5DatasetWriter.hpp"
#include "complex/Utilities/Parsing/HDF5/H5Support.hpp"

#include "FileVec/collection/Array.hpp"

#include <fmt/core.h>

#include <nonstd/span.hpp>

#include <algorithm>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <vector>

#include <cstring>

namespace complex
{
/**
 * @class FileStore
 * @brief The FileStore class handles the storing and retrieval of data for
 * use in DataArrays.
 * @tparam T
 */
template <typename T>
class FileStore : public AbstractDataStore<T>
{
public:
  using value_type = typename AbstractDataStore<T>::value_type;
  using reference = typename AbstractDataStore<T>::reference;
  using const_reference = typename AbstractDataStore<T>::const_reference;
  using ShapeType = typename IDataStore::ShapeType;

  static constexpr const char k_FileStore[] = "FileStore";
  static constexpr const char k_DataObjectId[] = "DataObjectId";
  static constexpr const char k_DataArrayTypeName[] = "DataArray";

  /**
   * @brief Creates a new FileStore with a single tuple dimensions of 'numTuples' and
   * a single component dimension of {1}
   * @param numTuples
   * @param initValue
   */
  FileStore(usize numTuples, std::optional<T> initValue)
  : FileStore({numTuples}, {1}, initValue)
  {
  }

  /**
   * @brief Constructs a FileStore with the specified tupleSize and tupleCount.
   * @param tupleShape The dimensions of the tuples
   * @param componentShape The dimensions of the component at each tuple
   * @param initValue
   */
  FileStore(const ShapeType& tupleShape, const ShapeType& componentShape, std::optional<T> initValue)
  : m_ComponentShape(componentShape)
  , m_TupleShape(tupleShape)
  {
    reshapeTuples(m_TupleShape);
    if(initValue.has_value())
    {
      m_Data->fill(*initValue);
    }
  }

  /**
   * @brief Constructs a FileStore from an existing buffer.
   * @param buffer
   * @param tupleShape
   * @param componentShape
   */
  FileStore(std::unique_ptr<File::Array<T>> buffer, ShapeType tupleShape, ShapeType componentShape)
  : m_ComponentShape(std::move(componentShape))
  , m_TupleShape(std::move(tupleShape))
  , m_Data(std::move(buffer))
  {
  }

  /**
   * @brief Copy constructor
   * @param other
   */
  FileStore(const FileStore& other)
  : m_ComponentShape(other.m_ComponentShape)
  , m_TupleShape(other.m_TupleShape)
  {
    const usize count = other.getSize();
    m_Data = File::Array<T>::CreatePtr({count}, {count});
    File::Array<T>& data = *m_Data.get();
    File::Array<T>& otherData = *other.m_Data.get();
    for(uint64 i = 0; i < count; ++i)
    {
      data[i] = otherData[i];
    }
  }

  /**
   * @brief Move constructor
   * @param other
   */
  FileStore(FileStore&& other) noexcept
  : m_TupleShape(std::move(other.m_TupleShape))
  , m_ComponentShape(std::move(other.m_ComponentShape))
  , m_Data(std::move(other.m_Data))
  {
  }

  /**
   * @brief Copy assignment.
   * @param rhs
   * @return
   */
  FileStore& operator=(const FileStore& rhs) = delete;

  /**
   * @brief Move assignment.
   * @param rhs
   * @return
   */
  FileStore& operator=(FileStore&& rhs) = default;

  ~FileStore() override = default;

  /**
   * @brief Returns the number of tuples in the FileStore.
   * @return usize
   */
  usize getNumberOfTuples() const override
  {
    return std::accumulate(m_TupleShape.cbegin(), m_TupleShape.cend(), static_cast<size_t>(1), std::multiplies<>());
  }

#if 0
  /**
   * @brief Returns the pointer to the allocated data. Const version
   * @return
   */
  const T* data() const
  {
    return m_Data.get();
  }

  /**
   * @brief Returns the pointer to the allocated data. Non-const version
   * @return
   */
  T* data()
  {
    return m_Data.get();
  }
#endif

  /**
   * @brief Returns the number of elements in each Tuple.
   * @return usize
   */
  usize getNumberOfComponents() const override
  {
    usize numComponents = 1;
    const usize count = m_ComponentShape.size();
    for(usize i = 0; i < count; i++)
    {
      numComponents *= m_ComponentShape[i];
    }
    return numComponents;
  }

  /**
   * @brief Returns the dimensions of the Tuples
   * @return
   */
  const ShapeType& getTupleShape() const override
  {
    return m_TupleShape;
  }

  /**
   * @brief Returns the dimensions of the Components
   * @return
   */
  const ShapeType& getComponentShape() const override
  {
    return m_ComponentShape;
  }

  /**
   * @brief Returns the store type e.g. in memory, out of core, etc.
   * @return StoreType
   */
  IDataStore::StoreType getStoreType() const override
  {
    return IDataStore::StoreType::OutOfCore;
  }

  /**
   * @brief
   * @param tupleShape
   */
  void reshapeTuples(const std::vector<usize>& tupleShape) override
  {
    auto oldSize = this->getSize();
    // Calculate the total number of values in the new array
    usize newSize = getNumberOfComponents() * std::accumulate(tupleShape.cbegin(), tupleShape.cend(), static_cast<size_t>(1), std::multiplies<>());
    m_TupleShape = tupleShape;

    if(m_Data.get() == nullptr) // Data was never allocated
    {
      m_Data = File::Array<T>::CreatePtr({newSize}, {newSize});
      return;
    }

    // The caller is reshaping the array without actually effecting its overall number
    // of elements. Old was 100 x 3 and the new was 300. Both with a {1} comp dim.
    if(newSize == oldSize)
    {
      return;
    }

    // We have now figured out that the old array and the new array are different sizes so
    // copy the old data into the newly allocated data array or as much or as little
    // as possible
    auto data = File::Array<T>::CreatePtr({newSize}, {newSize});
    for(usize i = 0; i < newSize && i < oldSize; i++)
    {
      data->operator[](i) = m_Data->operator[](i);
    }
    m_Data = std::move(data);
  }

  /**
   * @brief Returns the value found at the specified index of the FileStore.
   * This cannot be used to edit the value found at the specified index.
   * @param index
   * @return value_type
   */
  value_type getValue(usize index) const override
  {
    return m_Data->operator[](index);
  }

  /**
   * @brief Sets the value stored at the specified index.
   * @param index
   * @param value
   */
  void setValue(usize index, value_type value) override
  {
    m_Data->operator[](index) = value;
  }

  /**
   * @brief Returns the value found at the specified index of the FileStore.
   * This cannot be used to edit the value found at the specified index.
   * @param  index
   * @return const_reference
   */
  const_reference operator[](usize index) const override
  {
    return m_Data->operator[](index);
  }

  /**
   * @brief Returns the value found at the specified index of the FileStore.
   * This can be used to edit the value found at the specified index.
   * @param  index
   * @return reference
   */
  reference operator[](usize index) override
  {
    return m_Data->operator[](index);
  }

  /**
   * @brief Returns the value found at the specified index of the FileStore.
   * This cannot be used to edit the value found at the specified index.
   * @param index
   * @return const_reference
   */
  const_reference at(usize index) const override
  {
    if(index >= this->getSize())
    {
      throw std::runtime_error("");
    }
    return m_Data->operator[](index);
  }

  /**
   * @brief Returns a deep copy of the data store and all its data.
   * @return std::unique_ptr<IDataStore>
   */
  std::unique_ptr<IDataStore> deepCopy() const override
  {
    return std::make_unique<FileStore<T>>(*this);
  }

  /**
   * @brief Returns a data store of the same type as this but with default initialized data.
   * @return std::unique_ptr<IDataStore>
   */
  std::unique_ptr<IDataStore> createNewInstance() const override
  {
    return std::make_unique<FileStore<T>>(this->getTupleShape(), this->getComponentShape(), static_cast<T>(0));
  }

  nonstd::span<T> createSpan()
  {
    return {m_Data->begin(), m_Data->end()};
  }

  nonstd::span<const T> createSpan() const
  {
    return {m_Data->begin(), m_Data->end()};
  }

  /**
   * @brief Writes the data store to HDF5. Returns the HDF5 error code should
   * one be encountered. Otherwise, returns 0.
   * @param datasetWriter
   * @return H5::ErrorType
   */
  H5::ErrorType writeHdf5(H5::DatasetWriter& datasetWriter) const override
  {
    if(!datasetWriter.isValid())
    {
      return -1;
    }

    // Consolodate the Tuple and Component Dims into a single array which is used
    // to write the entire data array to HDF5
    std::vector<hsize_t> h5dims;
    for(const auto& value : m_TupleShape)
    {
      h5dims.push_back(static_cast<hsize_t>(value));
    }
    for(const auto& value : m_ComponentShape)
    {
      h5dims.push_back(static_cast<hsize_t>(value));
    }

    usize count = this->getSize();
    herr_t err = datasetWriter.writeSpan(h5dims, createSpan());
    if(err < 0)
    {
      return err;
    }

    // Write shape attributes to the dataset
    auto tupleAttribute = datasetWriter.createAttribute(IDataStore::k_TupleShape);
    err = tupleAttribute.writeVector({m_TupleShape.size()}, m_TupleShape);
    if(err < 0)
    {
      return err;
    }

    auto componentAttribute = datasetWriter.createAttribute(IDataStore::k_ComponentShape);
    err = componentAttribute.writeVector({m_ComponentShape.size()}, m_ComponentShape);

    return err;
  }

  static std::unique_ptr<FileStore> ReadHdf5(const H5::DatasetReader& datasetReader)
  {
    auto tupleShape = IDataStore::ReadTupleShape(datasetReader);
    auto componentShape = IDataStore::ReadComponentShape(datasetReader);

    // Create FileStore
    auto dataStore = std::make_unique<FileStore<T>>(tupleShape, componentShape, static_cast<T>(0));

    if(!datasetReader.readIntoSpan(dataStore->createSpan()))
    {
      throw std::runtime_error(fmt::format("Error reading data from FileStore from HDF5 at {}/{}", H5::Support::GetObjectPath(datasetReader.getParentId()), datasetReader.getName()));
    }

    return dataStore;
  }

private:
  ShapeType m_ComponentShape;
  ShapeType m_TupleShape;
  std::unique_ptr<File::Array<T>> m_Data = nullptr;
};

// Declare aliases
using UInt8FileStore = FileStore<uint8>;
using UInt16FileStore = FileStore<uint16>;
using UInt32FileStore = FileStore<uint32>;
using UInt64FileStore = FileStore<uint64>;

using Int8FileStore = FileStore<int8>;
using Int16FileStore = FileStore<int16>;
using Int32FileStore = FileStore<int32>;
using Int64FileStore = FileStore<int64>;

using USizeFileStore = FileStore<usize>;

using Float32FileStore = FileStore<float32>;
using Float64FileStore = FileStore<float64>;
} // namespace complex
