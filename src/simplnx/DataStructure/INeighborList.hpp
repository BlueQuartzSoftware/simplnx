#pragma once

#include "simplnx/DataStructure/IArray.hpp"
#include "simplnx/DataStructure/IListStore.hpp"

namespace nx::core
{
namespace NeighborListConstants
{
inline constexpr StringLiteral k_TypeName = "NeighborList<T>";
}

/**
 * @brief Non-templated base class for NeighborList class.
 */
class SIMPLNX_EXPORT INeighborList : public IArray
{
public:
  static inline constexpr StringLiteral k_TypeName = "INeighborList";

  ~INeighborList() noexcept override;

  /**
   * @brief Returns a pointer to the array's IListStore.
   * @return IListStore*
   */
  virtual IListStore* getIListStore() = 0;

  /**
   * @brief Returns a pointer to the array's IListStore.
   * @return const IListStore*
   */
  virtual const IListStore* getIListStore() const = 0;

  /**
   * @brief Returns a reference to the array's IListStore.
   * @return IListStore&
   */
  IListStore& getIListStoreRef();

  /**
   * @brief Returns a reference to the array's IListStore.
   * @return const IListStore&
   */
  const IListStore& getIListStoreRef() const;

  /**
   * @brief Returns a pointer to the DataStore cast as type StoreT.
   * @return const StoreT*
   */
  template <class StoreT>
  const StoreT* getIListStoreAs() const
  {
    static_assert(std::is_base_of_v<IListStore, StoreT>);
    return dynamic_cast<const StoreT*>(getIListStore());
  }

  /**
   * @brief Returns a pointer to the DataStore cast as type StoreT.
   * @return StoreT*
   */
  template <class StoreT>
  StoreT* getIListStoreAs()
  {
    static_assert(std::is_base_of_v<IListStore, StoreT>);
    return dynamic_cast<StoreT*>(getIListStore());
  }

  /**
   * @brief Returns a reference to the DataStore cast as type StoreT.
   * @return const StoreT&
   */
  template <class StoreT>
  const StoreT& getIListStoreRefAs() const
  {
    static_assert(std::is_base_of_v<IListStore, StoreT>);
    return dynamic_cast<const StoreT&>(getIListStoreRef());
  }

  /**
   * @brief Returns a reference to the DataStore cast as type StoreT.
   * @return StoreT&
   */
  template <class StoreT>
  StoreT& getIListStoreRefAs()
  {
    static_assert(std::is_base_of_v<IListStore, StoreT>);
    return dynamic_cast<StoreT&>(getIListStoreRef());
  }

  /**
   * @brief Returns typename of the DataObject as a std::string.
   * @return std::string
   */
  std::string getTypeName() const override;

  /**
   * @brief setNumNeighborsArrayName
   * @param name
   */
  void setNumNeighborsArrayName(const std::string& name);

  /**
   * @brief Returns the Num Neighbors array name for use in HDF5.
   * @return std::string
   */
  std::string getNumNeighborsArrayName() const;

  /**
   * @brief Returns the number of elements in the internal array.
   * @return usize
   */
  usize getNumberOfTuples() const override;

  /**
   * @brief Returns the number of components per tuple.
   * @return usize
   */
  usize getNumberOfComponents() const override;

  /**
   * @brief Returns the tuple shape.
   * @return
   */
  ShapeType getTupleShape() const override;

  /**
   * @brief Returns the component shape.
   * @return
   */
  ShapeType getComponentShape() const override;

  /**
   * @brief copyTuple
   * @param currentPos
   * @param newPos
   */
  virtual void copyTuple(usize currentPos, usize newPos) = 0;

  /**
   * @brief Returns the DataType of the underlying data.
   * @return DataType
   */
  virtual DataType getDataType() const = 0;

  /**
   * @brief Returns an enumeration of the class or subclass. Used for quick comparison or type deduction
   * @return
   */
  DataObject::Type getDataObjectType() const override;

  /**
   * @brief Resizes the internal array to accommodate
   */
  void resizeTuples(const std::vector<usize>& tupleShape) override;

protected:
  /**
   * @brief Constructs a new INeighborList
   * @param dataStructure
   * @param name
   * @param numTuples
   */
  INeighborList(DataStructure& dataStructure, const std::string& name);

  /**
   * @brief Constructor for use when importing INeighborLists
   * @param dataStructure
   * @param name
   * @param numTuples
   * @param importId
   */
  INeighborList(DataStructure& dataStructure, const std::string& name, IdType importId);

private:
  std::string m_NumNeighborsArrayName;
};
} // namespace nx::core
