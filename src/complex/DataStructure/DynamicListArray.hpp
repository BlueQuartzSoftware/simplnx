#pragma once

#include <iostream>
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
    T ncells;
    K* cells;
  };

  DynamicListArray(const DynamicListArray& other)
  : DataObject(other)
  , m_Array(other.m_Array)
  , m_Size(other.m_Size)
  {
  }

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
   * @brief size
   * @return
   */
  size_t size() const
  {
    return m_Size;
  }

  /**
   * @brief deepCopy
   * @return DataObject*
   */
  DataObject* deepCopy() override
  {
    DynamicListArray* copy = new DynamicListArray(*this);
    std::vector<T> linkCounts(m_Size, 0);
    //if(forceNoAllocate)
    //{
    //  copy->allocateLists(linkCounts);
    //  return copy;
    //}

    // Figure out how many entries, and for each entry, how many cells
    for(size_t ptId = 0; ptId < m_Size; ptId++)
    {
      linkCounts[ptId] = this->m_Array[ptId].ncells;
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
   * @brief Returns a shallow copy of the DataObject.
   * @return DataObject*
   */
  DataObject* shallowCopy() override
  {
    throw std::exception();
  }

  /**
   * @brief insertCellReference
   * @param ptId
   * @param pos
   * @param cellId
   */
  inline void insertCellReference(size_t ptId, size_t pos, size_t cellId)
  {
    this->m_Array[ptId].cells[pos] = cellId;
  }

  /**
   * @brief Get a link structure given a point id.
   * @param ptId
   * @return
   */
  ElementList& getElementList(size_t ptId) const
  {
    return this->m_Array[ptId];
  }

  /**
   * @brief setElementList
   * @param ptId
   * @param nCells
   * @param data
   * @return
   */
  bool setElementList(size_t ptId, T nCells, K* data)
  {
    if(ptId >= m_Size)
    {
      return false;
    }
    if(nullptr != m_Array[ptId].cells && m_Array[ptId].ncells > 0)
    {
      delete[] this->m_Array[ptId].cells;
      m_Array[ptId].cells = nullptr;
      m_Array[ptId].ncells = 0;
    }
    m_Array[ptId].ncells = nCells;
    // If nCells is huge then there could be problems with this
    this->m_Array[ptId].cells = new K[nCells];
    ::memcpy(m_Array[ptId].cells, data, sizeof(K) * nCells);
    return true;
  }

  /**
   * @brief setElementList
   * @param ptId
   * @param list
   * @return
   */
  bool setElementList(size_t ptId, ElementList& list)
  {
    T nCells = list.ncells;
    K* data = list.cells;
    if(ptId >= m_Size)
    {
      return false;
    }
    if(nullptr != m_Array[ptId].cells && m_Array[ptId].ncells > 0)
    {
      delete[] this->m_Array[ptId].cells;
      m_Array[ptId].cells = nullptr;
      m_Array[ptId].ncells = 0;
    }
    m_Array[ptId].ncells = nCells;
    // If nCells is huge then there could be problems with this
    this->m_Array[ptId].cells = new K[nCells];
    ::memcpy(m_Array[ptId].cells, data, sizeof(K) * nCells);
    return true;
  }

  /**
   * @brief Get the number of cells using the point specified by ptId.
   * @param ptId
   * @return
   */
  T getNumberOfElements(size_t ptId) const
  {
    return this->m_Array[ptId].ncells;
  }

  /**
   * @brief Return a list of cell ids using the point.
   * @param ptId
   * @return
   */
  K* getElementListPointer(size_t ptId) const
  {
    return this->m_Array[ptId].cells;
  }

  /**
   * @brief deserializeLinks
   * @param buffer
   * @param nElements
   */
  void deserializeLinks(std::vector<uint8_t>& buffer, size_t nElements)
  {
    size_t offset = 0;
    allocate(nElements); // Allocate all the links with 0 and nullptr;
    uint8_t* bufPtr = &(buffer.front());

    // Walk the array and allocate all the array links to Zero and nullptr
    T* ncells = nullptr;
    // int32_t* cells = nullptr;
    for(size_t i = 0; i < nElements; ++i)
    {
      ncells = reinterpret_cast<T*>(bufPtr + offset);
      this->m_Array[i].ncells = *ncells; // Set the number of cells in this link
      offset += 2;
      this->m_Array[i].cells = new K[(*ncells)];                                // Allocate a new chunk of memory to store the list
      ::memcpy(this->m_Array[i].cells, bufPtr + offset, (*ncells) * sizeof(K)); // Copy from the buffer into the new list memory
      offset += (*ncells) * sizeof(K);                                          // Increment the offset
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
      this->m_Array[i].ncells = linkCounts[i];
      if(linkCounts[i] > 0)
      {
        this->m_Array[i].cells = new K[this->m_Array[i].ncells];
      }
    }
  }

  /**
   * @brief Writes the DataObject to the specified XDMF file.
   * @param out
   * @param hdfFileName
   * @return H5::ErrorType
   */
  H5::ErrorType generateXdmfText(std::ostream& out, const std::string& hdfFileName) const override
  {
    throw std::exception();
  }

  /**
   * @brief Reads and replaces values from the provided input stream.
   * @param in
   * @param hdfFileName
   * @return H5::ErrorType
   */
  H5::ErrorType readFromXdmfText(std::istream& in, const std::string& hdfFileName) override
  {
    throw std::exception();
  }

protected:
  /**
   * @brief 
   * @param ds 
   * @param name 
  */
  DynamicListArray(DataStructure* ds, const std::string& name)
  : DataObject(ds, name)
  {
  }

  //----------------------------------------------------------------------------
  // This will allocate memory to hold all the NeighborList structures where each
  // structure is initialized to Zero Entries and a nullptr Pointer
  void allocate(size_t sz)
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

    this->m_Size = sz;
    // Allocate a whole new set of structures
    this->m_Array = new typename DynamicListArray<T, K>::ElementList[sz];

    // Initialize each structure to have 0 entries and nullptr pointer.
    for(size_t i = 0; i < sz; i++)
    {
      this->m_Array[i] = linkInit;
    }
  }

private:
  ElementList* m_Array = nullptr; // pointer to data
  size_t m_Size = 0;
};

typedef DynamicListArray<int32_t, int32_t> Int32Int32DynamicListArray;
typedef DynamicListArray<uint16_t, int64_t> UInt16Int64DynamicListArray;
typedef DynamicListArray<int64_t, int64_t> Int64Int64DynamicListArray;
} // namespace complex
