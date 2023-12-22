#pragma once

#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/DataStructure/DataObject.hpp"

#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>

namespace nx::core
{

namespace DynamicListArrayConstants
{
inline constexpr StringLiteral k_TypeName = "DynamicListArray";
}

template <typename T, typename K>
class DynamicListArray : public DataObject
{
public:
  friend class DataStructure;

  using Self = DynamicListArray<T, K>;

  struct ElementList
  {
    T numCells;
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
   * @param dataStructure
   * @param name
   * @param parentId = {}
   * @return DynamicListArray*
   */
  static DynamicListArray* Create(DataStructure& dataStructure, std::string name, const std::optional<IdType>& parentId)
  {
    auto data = std::shared_ptr<DynamicListArray>(new DynamicListArray(dataStructure, std::move(name)));
    if(!AttemptToAddObject(dataStructure, data, parentId))
    {
      return nullptr;
    }
    return data.get();
  }

  /**
   * @brief Attempts to create a new DynamicListArray and insert it into the
   * DataStructure. If a parentId is provided, the created DynamicListArray
   * will be nested under the target DataObject. Otherwise, it will be placed
   * directly under the DataStructure.
   *
   * Returns a pointer to the created DynamicListArray if the operation succeeded.
   * Returns nullptr otherwise.
   * @param dataStructure
   * @param name
   * @param importId
   * @param parentId = {}
   * @return DynamicListArray*
   */
  static DynamicListArray* Import(DataStructure& dataStructure, std::string name, IdType importId, const std::optional<IdType>& parentId)
  {
    auto data = std::shared_ptr<DynamicListArray>(new DynamicListArray(dataStructure, std::move(name), importId));
    if(!AttemptToAddObject(dataStructure, data, parentId))
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
  ~DynamicListArray() override
  {
    // This makes sure we deallocate any lists that have been created
    for(usize i = 0; i < this->m_Size; i++)
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

  DataObject::Type getDataObjectType() const override
  {
    return Type::DynamicListArray;
  }

  /**
   * @brief Returns the typename of the DataObject as a std::string.
   * @return std::string
   */
  std::string getTypeName() const override
  {
    return DynamicListArrayConstants::k_TypeName;
  }

  /**
   * @brief Returns the current array size.
   * @return usize
   */
  usize size() const
  {
    return m_Size;
  }

  /**
   * @brief Creates a copy of the object. The caller is responsible for
   * deleting the returned value.
   * @return DataObject*
   */
  std::shared_ptr<DataObject> deepCopy(const DataPath& copyPath) override
  {
    auto& dataStruct = getDataStructureRef();
    if(dataStruct.containsData(copyPath))
    {
      return nullptr;
    }
    // Don't construct with identifier since it will get created when inserting into data structure
    std::shared_ptr<DynamicListArray<T, K>> copy = std::shared_ptr<DynamicListArray<T, K>>(new DynamicListArray<T, K>(dataStruct, copyPath.getTargetName()));
    std::vector<T> linkCounts(m_Size, 0);

    // Figure out how many entries, and for each entry, how many cells
    for(usize pointId = 0; pointId < m_Size; pointId++)
    {
      linkCounts[pointId] = this->m_Array[pointId].numCells;
    }
    // Allocate all that in the copy
    copy->allocateLists(linkCounts);
    // Copy the data from the original to the new
    for(usize ptId = 0; ptId < m_Size; ptId++)
    {
      ElementList& elementList = getElementList(ptId);
      copy->setElementList(ptId, elementList);
    }
    if(dataStruct.insert(copy, copyPath.getParent()))
    {
      return copy;
    }
    return nullptr;
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
  inline void insertCellReference(usize pointId, usize pos, usize cellId)
  {
    this->m_Array[pointId].cells[pos] = cellId;
  }

  /**
   * @brief Get a link structure given a point identifier.
   * @param pointId
   * @return ElementList&
   */
  ElementList& getElementList(usize pointId) const
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
  bool setElementList(usize pointId, T numCells, K* data)
  {
    if(pointId >= m_Size)
    {
      return false;
    }
    if(m_Array[pointId].cells != nullptr && m_Array[pointId].numCells > 0)
    {
      delete[] this->m_Array[pointId].cells;
      m_Array[pointId].cells = nullptr;
      m_Array[pointId].numCells = 0;
    }
    m_Array[pointId].numCells = numCells;
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
  bool setElementList(usize pointId, ElementList& list)
  {
    T nCells = list.numCells;
    K* data = list.cells;
    if(pointId >= m_Size)
    {
      return false;
    }
    if(m_Array[pointId].cells != nullptr && m_Array[pointId].numCells > 0)
    {
      delete[] this->m_Array[pointId].cells;
      m_Array[pointId].cells = nullptr;
      m_Array[pointId].numCells = 0;
    }
    m_Array[pointId].numCells = nCells;
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
  T getNumberOfElements(usize pointId) const
  {
    return this->m_Array[pointId].numCells;
  }

  /**
   * @brief Return a list of cell ids using the point.
   * @param pointId
   * @return K*
   */
  K* getElementListPointer(usize pointId) const
  {
    return this->m_Array[pointId].cells;
  }

  /**
   * @brief deserializeLinks
   * @param buffer
   * @param numElements
   */
  void deserializeLinks(std::vector<uint8>& buffer, usize numElements)
  {
    usize offset = 0;
    allocate(numElements); // Allocate all the links with 0 and nullptr;
    uint8* bufPtr = &(buffer.front());

    // Walk the array and allocate all the array links to Zero and nullptr
    T* numCells = nullptr;
    // int32* cells = nullptr;
    for(usize i = 0; i < numElements; ++i)
    {
      numCells = reinterpret_cast<T*>(bufPtr + offset);
      this->m_Array[i].numCells = *numCells; // Set the number of cells in this link
      offset += 2;
      this->m_Array[i].cells = new K[(*numCells)];                                   // Allocate a new chunk of memory to store the list
      std::memcpy(this->m_Array[i].cells, bufPtr + offset, (*numCells) * sizeof(K)); // Copy from the buffer into the new list memory
      offset += (*numCells) * sizeof(K);                                             // Increment the offset
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
      this->m_Array[i].numCells = linkCounts[i];
      if(linkCounts[i] > 0)
      {
        this->m_Array[i].cells = new K[this->m_Array[i].numCells];
      }
    }
  }

protected:
  /**
   * @brief
   * @param dataStructure
   * @param name
   */
  DynamicListArray(DataStructure& dataStructure, std::string name)
  : DataObject(dataStructure, std::move(name))
  {
  }

  /**
   * @brief
   * @param dataStructure
   * @param name
   * @param importId
   */
  DynamicListArray(DataStructure& dataStructure, std::string name, IdType importId)
  : DataObject(dataStructure, std::move(name), importId)
  {
  }

  //----------------------------------------------------------------------------
  // This will allocate memory to hold all the NeighborList structures where each
  // structure is initialized to Zero Entries and a nullptr Pointer
  void allocate(usize size)
  {
    static typename DynamicListArray<T, K>::ElementList linkInit = {0, nullptr};

    // This makes sure we deallocate any lists that have been created
    for(usize i = 0; i < this->m_Size; i++)
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
    for(usize i = 0; i < size; i++)
    {
      this->m_Array[i] = linkInit;
    }
  }

private:
  ElementList* m_Array = nullptr; // pointer to data
  usize m_Size = 0;
};

using Int32Int32DynamicListArray = DynamicListArray<int32, int32>;
using UInt16Int64DynamicListArray = DynamicListArray<uint16, int64>;
using Int64Int64DynamicListArray = DynamicListArray<int64, int64>;
} // namespace nx::core
