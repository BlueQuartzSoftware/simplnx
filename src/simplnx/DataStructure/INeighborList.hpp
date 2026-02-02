#pragma once

#include "simplnx/Common/Aliases.hpp"
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
   * @tparam StoreT The target IListStore-derived type to cast to
   * @return const StoreT* Pointer to the ListStore cast to the specified type, or nullptr if the cast fails
   */
  template <class StoreT>
  const StoreT* getIListStoreAs() const
  {
    static_assert(std::is_base_of_v<IListStore, StoreT>);
    return dynamic_cast<const StoreT*>(getIListStore());
  }

  /**
   * @brief Returns a pointer to the DataStore cast as type StoreT.
   * @tparam StoreT The target IListStore-derived type to cast to
   * @return StoreT* Pointer to the ListStore cast to the specified type, or nullptr if the cast fails
   */
  template <class StoreT>
  StoreT* getIListStoreAs()
  {
    static_assert(std::is_base_of_v<IListStore, StoreT>);
    return dynamic_cast<StoreT*>(getIListStore());
  }

  /**
   * @brief Returns a reference to the DataStore cast as type StoreT.
   * @tparam StoreT The target IListStore-derived type to cast to
   * @return const StoreT& Reference to the ListStore cast to the specified type
   * @throws std::bad_cast if the cast fails
   */
  template <class StoreT>
  const StoreT& getIListStoreRefAs() const
  {
    static_assert(std::is_base_of_v<IListStore, StoreT>);
    return dynamic_cast<const StoreT&>(getIListStoreRef());
  }

  /**
   * @brief Returns a reference to the DataStore cast as type StoreT.
   * @tparam StoreT The target IListStore-derived type to cast to
   * @return StoreT& Reference to the ListStore cast to the specified type
   * @throws std::bad_cast if the cast fails
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
   * @brief Sets the name of the NumNeighbors array for use in HDF5 I/O.
   * @param name The name to assign to the NumNeighbors array
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
   * @return ShapeType The shape of the tuples in the neighbor list
   */
  ShapeType getTupleShape() const override;

  /**
   * @brief Returns the component shape.
   * @return ShapeType The shape of the components in the neighbor list
   */
  ShapeType getComponentShape() const override;

  /**
   * @brief Copies values from one tuple to another.
   * @param currentPos The index of the source tuple to copy from
   * @param newPos The index of the destination tuple to copy to
   */
  virtual void copyTuple(usize currentPos, usize newPos) = 0;

  /**
   * @brief Returns the DataType of the underlying data.
   * @return DataType
   */
  virtual DataType getDataType() const = 0;

  /**
   * @brief Returns an enumeration of the class or subclass. Used for quick comparison or type deduction
   * @return DataObject::Type The type enumeration for this neighbor list
   */
  DataObject::Type getDataObjectType() const override;

  /**
   * @brief Resizes the internal array to accommodate the specified tuple shape.
   * @param tupleShape The new shape for the tuples
   */
  void resizeTuples(const ShapeType& tupleShape) override;

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
