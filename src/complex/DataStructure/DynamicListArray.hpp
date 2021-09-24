#pragma once

#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>

#include "complex/DataStructure/DataObject.hpp"

namespace complex
{
template <typename T, typename K>
class DynamicListArray : public DataObject
{
public:
  friend class DataStructure;

  using Self = DynamicListArray<T, K>;

  struct ElementList
  {
    T numcells;
    K* cells;
  };

  /**
   * @brief Attempts to create a new DynamicListArray and insert it into the
   * DataStructure. If a parentId is provided, the created DynamicListArray
   * will be nested under the target DataObject. Otherwise, it will be placed
   * directly under the DataStructure.
   *
   * Returns a pointer to the created DynamicListArray if the operation succeeded.
   * Returns nullptr otherwise.
   * @param ds
   * @param name
   * @param parentId = {}
   * @return DynamicListArray*
   */
  static DynamicListArray* Create(DataStructure& ds, const std::string& name, const std::optional<IdType>& parentId)
  {
    auto data = std::shared_ptr<DynamicListArray>(new DynamicListArray(ds, name));
    if(!AttemptToAddObject(ds, data, parentId))
    {
      return nullptr;
    }
    return data.get();
  }

  /**
   * @brief Creates a copy of the specified DynamicListArray. This copy is not
   * added to the DataStructure. The caller is responsible for deleting the
   * DynamicListArray.
   * @param other
   */
  DynamicListArray(const DynamicListArray& other)
  : DataObject(other)
  , m_Array(other.m_Array)
  , m_Size(other.m_Size)
  {
  }

  /**
   * @brief Creates a new DynamicListArray and moves values from the target
   * object. The caller is responsible for deleting the created DynamicListArray.
   * @param other
   */
  DynamicListArray(DynamicListArray&& other)
  : DataObject(std::move(other))
  , m_Array(std::move(other.m_Array))
  , m_Size(std::move(other.m_Size))
  {
  }

  // -----------------------------------------------------------------------------
  //
  // -----------------------------------------------------------------------------
  virtual ~DynamicListArray()
  {
    // This makes sure we deallocate any lists that have been created
    for(size_t i = 0; i < this->m_Size; i++)
    {
      if(this->m_Array[i].cells != nullptr)
      {
        delete[] this->m_Array[i].cells;
      }
    }
    // Now delete all the "ElementList" structures
    if(this->m_Array != nullptr)
    {
      delete[] this->m_Array;
    }
  }

  /**
   * @brief Returns the typename of the DataObject as a std::string.
   * @return std::string
   */
  std::string getTypeName() const override
  {
    return "DynamicListArray";
  }

  /**
   * @brief Returns the current array size.
   * @return size_t
   */
  size_t size() const
  {
    return m_Size;
  }

  /**
   * @brief Creates a copy of the object. The caller is responsible for
   * deleting the returned value.
   * @return DataObject*
   */
  DataObject* deepCopy() override
  {
    DynamicListArray* copy = new DynamicListArray(*this);
    std::vector<T> linkCounts(m_Size, 0);

    // Figure out how many entries, and for each entry, how many cells
    for(size_t pointId = 0; pointId < m_Size; pointId++)
    {
      linkCounts[pointId] = this->m_Array[pointId].numcells;
    }
    // Allocate all that in the copy
    copy->allocateLists(linkCounts);
    // Copy the data from the original to the new
    for(size_t ptId = 0; ptId < m_Size; ptId++)
    {
      ElementList& elementList = getElementList(ptId);
      copy->setElementList(ptId, elementList);
    }
    return copy;
  }

  /**
   * @brief The DynamicListArray cannot be shallow copied.
   * @return DataObject*
   */
  DataObject* shallowCopy() override
  {
    throw std::runtime_error("");
  }

  /**
   * @brief insertCellReference
   * @param pointId
   * @param pos
   * @param cellId
   */
  inline void insertCellReference(size_t pointId, size_t pos, size_t cellId)
  {
    this->m_Array[pointId].cells[pos] = cellId;
  }

  /**
   * @brief Get a link structure given a point id.
   * @param pointId
   * @return ElementList&
   */
  ElementList& getElementList(size_t pointId) const
  {
    return this->m_Array[pointId];
  }

  /**
   * @brief setElementList
   * @param pointId
   * @param numCells
   * @param data
   * @return bool
   */
  bool setElementList(size_t pointId, T numCells, K* data)
  {
    if(pointId >= m_Size)
    {
      return false;
    }
    if(m_Array[pointId].cells != nullptr && m_Array[pointId].numcells > 0)
    {
      delete[] this->m_Array[pointId].cells;
      m_Array[pointId].cells = nullptr;
      m_Array[pointId].numcells = 0;
    }
    m_Array[pointId].numcells = numCells;
    // If numCells is huge then there could be problems with this
    this->m_Array[pointId].cells = new K[numCells];
    std::memcpy(m_Array[pointId].cells, data, sizeof(K) * numCells);
    return true;
  }

  /**
   * @brief setElementList
   * @param pointId
   * @param list
   * @return bool
   */
  bool setElementList(size_t pointId, ElementList& list)
  {
    T nCells = list.numcells;
    K* data = list.cells;
    if(pointId >= m_Size)
    {
      return false;
    }
    if(m_Array[pointId].cells != nullptr && m_Array[pointId].numcells > 0)
    {
      delete[] this->m_Array[pointId].cells;
      m_Array[pointId].cells = nullptr;
      m_Array[pointId].numcells = 0;
    }
    m_Array[pointId].numcells = nCells;
    // If numCells is huge then there could be problems with this
    this->m_Array[pointId].cells = new K[nCells];
    std::memcpy(m_Array[pointId].cells, data, sizeof(K) * nCells);
    return true;
  }

  /**
   * @brief Get the number of cells using the point specified by pointId.
   * @param pointId
   * @return T
   */
  T getNumberOfElements(size_t pointId) const
  {
    return this->m_Array[pointId].numcells;
  }

  /**
   * @brief Return a list of cell ids using the point.
   * @param pointId
   * @return K*
   */
  K* getElementListPointer(size_t pointId) const
  {
    return this->m_Array[pointId].cells;
  }

  /**
   * @brief deserializeLinks
   * @param buffer
   * @param numElements
   */
  void deserializeLinks(std::vector<uint8_t>& buffer, size_t numElements)
  {
    size_t offset = 0;
    allocate(numElements); // Allocate all the links with 0 and nullptr;
    uint8_t* bufPtr = &(buffer.front());

    // Walk the array and allocate all the array links to Zero and nullptr
    T* numcells = nullptr;
    // int32_t* cells = nullptr;
    for(size_t i = 0; i < numElements; ++i)
    {
      numcells = reinterpret_cast<T*>(bufPtr + offset);
      this->m_Array[i].numcells = *numcells; // Set the number of cells in this link
      offset += 2;
      this->m_Array[i].cells = new K[(*numcells)];                                   // Allocate a new chunk of memory to store the list
      std::memcpy(this->m_Array[i].cells, bufPtr + offset, (*numcells) * sizeof(K)); // Copy from the buffer into the new list memory
      offset += (*numcells) * sizeof(K);                                             // Increment the offset
    }
  }

  /**
   * @brief
   * @param linkCounts
   */
  template <typename Container>
  void allocateLists(const Container& linkCounts)
  {
    allocate(linkCounts.size());
    for(typename std::vector<T>::size_type i = 0; i < linkCounts.size(); i++)
    {
      this->m_Array[i].numcells = linkCounts[i];
      if(linkCounts[i] > 0)
      {
        this->m_Array[i].cells = new K[this->m_Array[i].numcells];
      }
    }
  }

protected:
  /**
   * @brief
   * @param ds
   * @param name
   */
  DynamicListArray(DataStructure& ds, const std::string& name)
  : DataObject(ds, name)
  {
  }

  //----------------------------------------------------------------------------
  // This will allocate memory to hold all the NeighborList structures where each
  // structure is initialized to Zero Entries and a nullptr Pointer
  void allocate(size_t size)
  {
    static typename DynamicListArray<T, K>::ElementList linkInit = {0, nullptr};

    // This makes sure we deallocate any lists that have been created
    for(size_t i = 0; i < this->m_Size; i++)
    {
      if(this->m_Array[i].cells != nullptr)
      {
        delete[] this->m_Array[i].cells;
      }
    }
    // Now delete all the "ElementList" structures
    if(this->m_Array != nullptr)
    {
      delete[] this->m_Array;
    }

    this->m_Size = size;
    // Allocate a whole new set of structures
    this->m_Array = new typename DynamicListArray<T, K>::ElementList[size];

    // Initialize each structure to have 0 entries and nullptr pointer.
    for(size_t i = 0; i < size; i++)
    {
      this->m_Array[i] = linkInit;
    }
  }

  /**
   * @brief Writes the DataArray to HDF5 using the provided group ID.
   * @param parentId
   * @param dataId
   * @return H5::ErrorType
   */
  H5::ErrorType writeHdf5_impl(H5::IdType parentId, H5::IdType dataId) const override
  {
    throw std::runtime_error("");
  }

private:
  ElementList* m_Array = nullptr; // pointer to data
  size_t m_Size = 0;
};

typedef DynamicListArray<int32_t, int32_t> Int32Int32DynamicListArray;
typedef DynamicListArray<uint16_t, int64_t> UInt16Int64DynamicListArray;
typedef DynamicListArray<int64_t, int64_t> Int64Int64DynamicListArray;
} // namespace complex
