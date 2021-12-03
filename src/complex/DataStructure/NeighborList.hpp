#pragma once

#include "complex/DataStructure/IDataArray.hpp"

namespace complex
{
/**
 * @class NeighborList
 * @brief
 * @tparam T
 */
template <typename T>
class NeighborList : public IDataArray
{
public:
  using value_type = T;
  using VectorType = std::vector<T>;
  using SharedVectorType = std::shared_ptr<VectorType>;

  /**
   * @brief
   * @param ds
   * @param name
   * @param numTuples
   * @param parentId = {}
   * @tparam T
   */
  static NeighborList* Create(DataStructure& ds, const std::string& name, usize numTuples, const std::optional<IdType>& parentId = {});

  ~NeighborList() override = default;

  /**
   * @brief Returns a shallow copy of the NeighborList without copying data.
   * THE CALLING CODE MUST DISPOSE OF THE RETURNED OBJECT.
   * @return DataObject*
   */
  DataObject* shallowCopy() override;

  /**
   * @brief Returns a deep copy of the NeighborList including a deep copy of the
   * data.
   * @return DataObject*
   */
  DataObject* deepCopy() override;

  /**
   * @brief Returns typename of the DataObject as a std::string.
   * @return std::string
   */
  std::string getTypeName() const override;

  void setNumNeighborsArrayName(const std::string& name);

  std::string getNumNeighborsArrayName();

  /**
   * @brief Gives this array a human readable name
   * @param name The name of this array
   */
  virtual void setInitValue(value_type initValue);

  /**
   * @brief Removes Tuples from the Array. If the size of the vector is Zero nothing is done. If the size of the
   * vector is greater than or Equal to the number of Tuples then the Array is Resized to Zero. If there are
   * indices that are larger than the size of the original (before erasing operations) then an error code (-100) is
   * returned from the program.
   * @param idxs The indices to remove
   * @return error code.
   */
  int32 eraseTuples(const std::vector<usize>& idxs);

  /**
   * @brief copyTuple
   * @param currentPos
   * @param newPos
   */
  void copyTuple(usize currentPos, usize newPos) override;

  /**
   * @brief Returns the number of elements in the internal array.
   */
  usize getNumberOfTuples() const override;

  /**
   * @brief getSize Returns the total number of data items that are being stored. This is the sum of all the sizes
   * of the internal storage arrays for this class.
   * @return
   */
  usize getSize() const override;

  /**
   * @brief setNumberOfComponents
   * @param nc
   */
  void setNumberOfComponents(int32 nc);

  /**
   * @brief getNumberOfComponents
   * @return
   */
  usize getNumberOfComponents() const override;

  /**
   * @brief getComponentDimensions
   * @return std::vector<usize>
   */
  std::vector<usize> getComponentDimensions() const;

  /**
   * @brief getTypeSize
   * @return
   */
  usize getTypeSize() const;

  /**
   * @brief initializeWithZeros
   */
  void initializeWithZeros();

  /**
   * @brief resizeTotalElements
   * @param size
   * @return
   */
  int32_t resizeTotalElements(usize size);

  /**
   * @brief Resizes the internal array to accomondate numTuples
   * @param numTuples
   */
  void resizeTuples(usize numTuples);

  /**
   * @brief addEntry
   * @param grainId
   * @param value
   */
  void addEntry(int32 grainId, value_type value);

  /**
   * @brief clearAllLists
   */
  void clearAllLists();

  /**
   * @brief setList
   * @param grainId
   * @param neighborList
   */
  void setList(int32 grainId, SharedVectorType neighborList);

  /**
   * @brief getValue
   * @param grainId
   * @param index
   * @param ok
   * @return
   */
  T getValue(int32 grainId, int32 index, bool& ok) const;

  /**
   * @brief getNumberOfLists
   * @return
   */
  int32 getNumberOfLists() const;

  /**
   * @brief getListSize
   * @param grainId
   * @return
   */
  int32 getListSize(int32 grainId) const;

  VectorType& getListReference(int32 grainId) const;

  /**
   * @brief getList
   * @param grainId
   * @return
   */
  SharedVectorType getList(int32 grainId) const;

  /**
   * @brief copyOfList
   * @param grainId
   * @return
   */
  VectorType copyOfList(int32 grainId) const;

  /**
   * @brief operator []
   * @param grainId
   * @return
   */
  VectorType& operator[](int32 grainId);

  /**
   * @brief operator []
   * @param grainId
   * @return
   */
  VectorType& operator[](usize grainId);

  /**
   * @brief Returns nullptr as there is no IDataStore.
   * @return const IDataStore*
   */
  IDataStore* getIDataStore() override;

  /**
   * @brief Returns nullptr as there is no IDataStore.
   * @return const IDataStore*
   */
  const IDataStore* getIDataStore() const override;

  /**
   * @brief Writes the DataArray to HDF5 using the provided group ID.
   *
   * This method will fail if no DataStore has been set.
   * @param dataStructureWriter
   * @param parentGroupWriter
   * @return H5::ErrorType
   */
  H5::ErrorType writeHdf5(H5::DataStructureWriter& dataStructureWriter, H5::GroupWriter& parentGroupWriter) const override;

protected:
  /**
   * @brief NeighborList
   */
  NeighborList(DataStructure& dataStructure, const std::string name, usize numTuples);

private:
  std::string m_NumNeighborsArrayName;
  std::vector<SharedVectorType> m_Array;
  usize m_NumTuples;
  bool m_IsAllocated;
  value_type m_InitValue;
};

using Int32NeighborListType = NeighborList<int32_t>;
using FloatNeighborListType = NeighborList<float>;

// -----------------------------------------------------------------------------
// Declare our extern templates

extern template class NeighborList<char>;
extern template class NeighborList<unsigned char>;

extern template class NeighborList<int8>;
extern template class NeighborList<uint8>;
extern template class NeighborList<int16>;
extern template class NeighborList<uint16>;
extern template class NeighborList<int32>;
extern template class NeighborList<uint32>;
extern template class NeighborList<int64>;
extern template class NeighborList<uint64>;

extern template class NeighborList<float32>;
extern template class NeighborList<float64>;

extern template class NeighborList<usize>;
} // namespace complex
