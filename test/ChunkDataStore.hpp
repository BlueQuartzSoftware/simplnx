#pragma once

#include "complex/DataStructure/AbstractDataStore.hpp"

#include <algorithm>

namespace complex
{
template <typename T>
class ChunkDataStore : public AbstractDataStore<T>
{
public:
  using parent_type = AbstractDataStore<T>;
  using value_type = T;
  using reference = T&;
  using const_reference = const T&;
  using ShapeType = typename parent_type::ShapeType;
  using index_type = typename parent_type::index_type;
  using chunk_container = std::vector<value_type>;

  ChunkDataStore(const ShapeType& tupleShape, const ShapeType& componentShape, const ShapeType& chunkShape, std::optional<T> initValue = {})
  : parent_type()
  , m_ComponentShape(componentShape)
  , m_TupleShape(tupleShape)
  , m_ChunkShape(chunkShape)
  , m_NumChunks(chunkShape.size())
  , m_NumTuples(std::accumulate(m_TupleShape.cbegin(), m_TupleShape.cend(), static_cast<usize>(1), std::multiplies<>()))
  , m_NumComponents(std::accumulate(m_ComponentShape.cbegin(), m_ComponentShape.cend(), static_cast<usize>(1), std::multiplies<>()))
  {
    m_ArrayShape.clear();
    m_ArrayShape.insert(m_ArrayShape.end(), tupleShape.begin(), tupleShape.end());
    m_ArrayShape.insert(m_ArrayShape.end(), componentShape.begin(), componentShape.end());

    for(uint64 i = 0; i < chunkShape.size(); i++)
    {
      m_NumChunks[i] = std::max(static_cast<index_type>(m_ArrayShape[i] / chunkShape[i]), static_cast<index_type>(1));
    }

    populateChunks();

    if(initValue.has_value())
    {
      T value = initValue.value();
      uint64 size = getSize();
      for(uint64 i = 0; i < size; i++)
      {
        setValue(i, value);
      }
    }
  }

  ChunkDataStore(const ChunkDataStore& other)
  : parent_type()
  , m_ArrayShape(other.m_ArrayShape)
  , m_ComponentShape(other.m_ComponentShape)
  , m_Chunks(other.m_Chunks)
  , m_TupleShape(other.m_TupleShape)
  , m_ChunkShape(other.m_ChunkShape)
  , m_NumChunks(other.m_NumChunks)
  , m_NumTuples(other.m_NumTuples)
  , m_NumComponents(other.m_NumComponents)
  {
  }

  ChunkDataStore(ChunkDataStore&& other)
  : parent_type()
  , m_ArrayShape(std::move(other.m_ArrayShape))
  , m_ComponentShape(std::move(other.m_ComponentShape))
  , m_Chunks(std::move(other.m_Chunks))
  , m_TupleShape(std::move(other.m_TupleShape))
  , m_ChunkShape(std::move(other.m_ChunkShape))
  , m_NumChunks(other.m_NumChunks)
  , m_NumTuples(other.m_NumTuples)
  , m_NumComponents(other.m_NumComponents)
  {
  }

  virtual ~ChunkDataStore() = default;

  /**
   * @brief Returns the value found at the specified index of the DataStore.
   * This cannot be used to edit the value found at the specified index.
   * @param index
   * @return value_type
   */
  value_type getValue(usize index) const override
  {
    return at(index);
  }

  /**
   * @brief Sets the value stored at the specified index.
   * @param index
   * @param value
   */
  void setValue(usize index, value_type value) override
  {
    operator[](index) = value;
  }

  /**
   * @brief Returns the value found at the specified index of the DataStore.
   * This cannot be used to edit the value found at the specified index.
   * @param index
   * @return const_reference
   */
  const_reference operator[](usize index) const override
  {
    const auto& shape = m_ArrayShape;
    const auto& chunkShapeVec = m_ChunkShape;
    const ShapeType position(FindPosition(index, shape));
    const ShapeType chunkIndex(std::move(FindChunkId(position, chunkShapeVec)));
    const index_type offsetIndex = FindChunkIndexFromPosition(position, chunkShapeVec, chunkIndex);
    auto& block = getBlockAtChunkIndex(chunkIndex);
    return (block)[offsetIndex];
  }

  /**
   * @brief Returns the value found at the specified index of the DataStore.
   * This cannot be used to edit the value found at the specified index.
   * @param index
   * @return const_reference
   */
  const_reference at(usize index) const override
  {
    if(index >= getSize())
    {
      throw std::out_of_range("Target FileCore::Array has size: " + std::to_string(getSize()));
    }
    return operator[](index);
  }

  /**
   * @brief Returns the value found at the specified index of the DataStore.
   * This can be used to edit the value found at the specified index.
   * @param  index
   * @return T&
   */
  reference operator[](usize index) override
  {
    const ShapeType& shapeValue = m_ArrayShape;
    const ShapeType& chunkShapeVec = m_ChunkShape;
    const ShapeType position(FindPosition(index, shapeValue));
    const ShapeType chunkIndex(std::move(FindChunkId(position, chunkShapeVec)));
    const index_type offsetIndex = FindChunkIndexFromPosition(position, chunkShapeVec, chunkIndex);
    auto& block = getBlockAtChunkIndex(chunkIndex);
    return (block)[offsetIndex];
  }

  /**
   * @brief Returns the number of tuples in the DataStore.
   * @return usize
   */
  usize getNumberOfTuples() const override
  {
    return m_NumTuples;
  }

  /**
   * @brief Returns the number of elements in each Tuple.
   * @return usize
   */
  usize getNumberOfComponents() const override
  {
    return m_NumComponents;
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

  std::optional<ShapeType> getChunkShape() const override
  {
    return {m_ChunkShape};
  }

  /**
   * @brief Returns the data for a particular data chunk. Returns an empty span if the data is not chunked.
   * @param chunkPosition
   * @return chunk data as span
   */
  std::vector<T> getChunkValues(const ShapeType& chunkPosition) const override
  {
    const uint64 chunkIndex = Flatten(chunkPosition, m_NumChunks);
    return m_Chunks[chunkIndex];
  }

  /**
   * @brief Returns the store type e.g. in memory, out of core, etc.
   * @return StoreType
   */
  IDataStore::StoreType getStoreType() const override
  {
    return IDataStore::StoreType::InMemory;
  }

  void resizeTuples(const std::vector<usize>& tupleShape) override
  {
    throw std::runtime_error("ChunkDataStore<T>::resizeTuples not implemented");
  }

  /**
   * @brief Returns a deep copy of the data store and all its data.
   * @return std::unique_ptr<IDataStore>
   */
  std::unique_ptr<IDataStore> deepCopy() const override
  {
    return std::make_unique<ChunkDataStore<T>>(*this);
  }

  /**
   * @brief Returns a data store of the same type as this but with default initialized data.
   * @return std::unique_ptr<IDataStore>
   */
  std::unique_ptr<IDataStore> createNewInstance() const override
  {
    return std::make_unique<ChunkDataStore<T>>(this->getTupleShape(), this->getComponentShape(), m_ChunkShape);
  }

  std::pair<int32, std::string> writeBinaryFile(const std::string& absoluteFilePath) const override
  {
    throw std::runtime_error("ChunkDataStore<T>::writeBinaryFile not implemented");
  }

protected:
  void populateChunks()
  {
    const uint64 numChunks = std::accumulate(m_NumChunks.cbegin(), m_NumChunks.cend(), static_cast<usize>(1), std::multiplies<>());
    const uint64 chunkSize = std::accumulate(m_ChunkShape.cbegin(), m_ChunkShape.cend(), static_cast<usize>(1), std::multiplies<>());

    m_Chunks = std::vector<chunk_container>(numChunks);
    for(uint64 i = 0; i < numChunks; i++)
    {
      m_Chunks[i] = std::vector<T>(chunkSize);
    }
  }

  chunk_container& getBlockAtChunkIndex(const ShapeType& chunkPosition)
  {
    const uint64 chunkIndex = Flatten(chunkPosition, m_NumChunks);
    return m_Chunks[chunkIndex];
  }
  const chunk_container& getBlockAtChunkIndex(const ShapeType& chunkPosition) const
  {
    const uint64 chunkIndex = Flatten(chunkPosition, m_NumChunks);
    return m_Chunks[chunkIndex];
  }

  static uint64 Flatten(const ShapeType& position, const ShapeType& shape)
  {
    using index_type = uint64_t;
    const size_t dimensions = position.size();

    if(shape.size() != dimensions)
    {
      throw std::runtime_error("Could not flatten position due to mismatched dimensions");
    }

    index_type index = 0;
    index_type mult = 1;
    for(index_type i = 0; i < dimensions; i++)
    {
      const index_type offset = dimensions - i - 1;
      index += position[offset] * mult;
      mult *= shape[offset];
    }

    return index;
  }

  static ShapeType FindPosition(uint64 index, const ShapeType& shape)
  {
    using index_type = uint64_t;

    const size_t dimensions = shape.size();
    ShapeType position(dimensions);
    for(index_type i = 0; i < dimensions; i++)
    {
      const index_type offset = dimensions - i - 1;
      position[offset] = index % shape[offset];
      index /= shape[offset];
    }
    return position;
  }

  static ShapeType FindChunkId(const ShapeType& position, const ShapeType& chunk)
  {
    using index_type = uint64_t;
    using shape_type = std::vector<index_type>;

    const size_t dimensions = position.size();
    if(dimensions != chunk.size())
    {
      throw std::runtime_error("Could not find chunk ID due to mismatched dimensions");
    }

    shape_type chunkIndex(dimensions);
    std::transform(position.begin(), position.end(), chunk.begin(), chunkIndex.begin(), std::divides<index_type>());
    return std::move(chunkIndex);
  }

  // static ShapeType FindChunkId(const uint64& index, const ShapeType& chunkIndex, const ShapeType& chunkShape)
  //{
  //   const ShapeType position(std::move(FindPosition(index, shape)));
  //   return std::move(FindChunkId(position, chunkShape));
  // }

  static ShapeType FindChunkPosition(const ShapeType& position, const ShapeType& chunkIndex, const ShapeType& chunkShape)
  {
    using index_type = uint64_t;

    const size_t dimensions = position.size();
    if(dimensions != chunkIndex.size() || dimensions != chunkShape.size())
    {
      throw std::runtime_error("Could not find chunk position due to mismatched dimensions");
    }

    ShapeType chunkOffset(dimensions);
    for(index_type i = 0; i < dimensions; i++)
    {
      const index_type offset = i;

      const index_type chunkPos = chunkIndex[offset] * chunkShape[offset];
      const uint64_t value = position[offset];
      if(value < chunkPos)
      {
        throw std::out_of_range("The provided position is not within the specified chunk");
      }
      chunkOffset[offset] = value - chunkPos;
    }
    return std::move(chunkOffset);
  }

  static uint64 FindChunkIndexFromPosition(const ShapeType& position, const ShapeType& chunkShape, const ShapeType& chunkIndex)
  {
    const ShapeType chunkPosition(std::move(FindChunkPosition(position, chunkIndex, chunkShape)));
    return Flatten(chunkPosition, chunkShape);
  }

  static uint64 FindChunkIndex(uint64 index, const ShapeType& shape, const ShapeType& chunkShape)
  {
    const ShapeType position(std::move(FindPosition(index, shape)));
    const ShapeType chunkIndex(std::move(FindChunkId(position, chunkShape)));
    const ShapeType chunkPosition(std::move(FindChunkPosition(position, chunkIndex, chunkShape, order)));
    return Flatten(chunkPosition, chunkShape);
  }

private:
  ShapeType m_TupleShape;
  ShapeType m_ComponentShape;
  ShapeType m_ArrayShape;
  ShapeType m_ChunkShape;
  ShapeType m_NumChunks;
  std::vector<chunk_container> m_Chunks;
  usize m_NumTuples = 0;
  usize m_NumComponents = 0;
};
} // namespace complex
