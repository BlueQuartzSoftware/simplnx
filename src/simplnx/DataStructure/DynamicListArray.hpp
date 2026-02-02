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
   * @param dataStructure The DataStructure to insert the new DynamicListArray into
   * @param name The name of the new DynamicListArray
   * @param parentId Optional ID of the parent DataObject
   * @return DynamicListArray* Pointer to the created DynamicListArray, or nullptr if creation failed
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
   * @param dataStructure The DataStructure to insert the new DynamicListArray into
   * @param name The name of the new DynamicListArray
   * @param importId The ID to use for the imported DynamicListArray
   * @param parentId Optional ID of the parent DataObject
   * @return DynamicListArray* Pointer to the created DynamicListArray, or nullptr if import failed
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
   * @param other The DynamicListArray to copy from
   */
  DynamicListArray(const DynamicListArray& other)
  : DataObject(other)
  , m_Size(other.m_Size)
  {
    allocate(other.m_Size);

    for(usize i = 0; i < other.m_Size; i++)
    {
      setElementList(i, other.m_Array[i]);
    }
  }

  /**
   * @brief Creates a new DynamicListArray and moves values from the target
   * object. The caller is responsible for deleting the created DynamicListArray.
   * @param other The DynamicListArray to move from
   */
  DynamicListArray(DynamicListArray&& other)
  : DataObject(std::move(other))
  , m_Array(std::move(other.m_Array))
  , m_Size(std::move(other.m_Size))
  {
  }

  /**
   * @brief Destroys the DynamicListArray and deallocates all memory for element lists.
   */
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

  /**
   * @brief Returns the DataObject type as an enum value.
   * @return DataObject::Type The type enum value for DynamicListArray
   */
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
   * @param copyPath The path where the deep copy should be placed
   * @return std::shared_ptr<DataObject> Shared pointer to the deep copied DynamicListArray, or nullptr if copy failed
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
   * @return DataObject* A new DynamicListArray that is a deep copy of this object
   */
  DataObject* shallowCopy() override
  {
    return new DynamicListArray(*this);
  }

  /**
   * @brief Inserts a cell reference at the specified position in the element list.
   * @param pointId The point identifier
   * @param pos The position in the element list
   * @param cellId The cell identifier to insert
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
   * @brief Sets the element list for a given point, allocating memory and copying data.
   * @param pointId The point identifier
   * @param numCells The number of cells in the element list
   * @param data Pointer to the cell data to copy
   * @return bool True if the element list was set successfully, false otherwise
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
   * @brief Sets the element list for a given point from an ElementList structure.
   * @param pointId The point identifier
   * @param list Reference to the ElementList containing cell data
   * @return bool True if the element list was set successfully, false otherwise
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
   * @brief Deserializes element list links from a buffer.
   * @param buffer Vector containing the serialized link data
   * @param numElements The number of elements to deserialize
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
   * @brief Allocates element lists based on the provided link counts.
   * @param linkCounts Container holding the number of cells for each element list
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
   * @brief Constructs a DynamicListArray with the given DataStructure and name.
   * @param dataStructure The DataStructure that will contain this DynamicListArray
   * @param name The name of the DynamicListArray
   */
  DynamicListArray(DataStructure& dataStructure, std::string name)
  : DataObject(dataStructure, std::move(name))
  {
  }

  /**
   * @brief Constructs a DynamicListArray with the given DataStructure, name, and import ID.
   * @param dataStructure The DataStructure that will contain this DynamicListArray
   * @param name The name of the DynamicListArray
   * @param importId The ID to use when importing this DynamicListArray
   */
  DynamicListArray(DataStructure& dataStructure, std::string name, IdType importId)
  : DataObject(dataStructure, std::move(name), importId)
  {
  }

  /**
   * @brief Allocates memory to hold all the ElementList structures where each
   * structure is initialized to zero entries and a nullptr pointer.
   * @param size The number of ElementList structures to allocate
   */
  void allocate(usize size)
  {
    static typename DynamicListArray<T, K>::ElementList linkInit = {0, nullptr};

    if(this->m_Array != nullptr)
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
