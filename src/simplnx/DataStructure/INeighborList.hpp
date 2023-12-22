#pragma once

#include "simplnx/DataStructure/IArray.hpp"

namespace nx::core
{
/**
 * @brief Non-templated base class for NeighborList class.
 */
class SIMPLNX_EXPORT INeighborList : public IArray
{
public:
  static inline constexpr StringLiteral k_TypeName = "INeighborList";

  ~INeighborList() noexcept override;

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
  DataObject::Type getDataObjectType() const override
  {
    return Type::INeighborList;
  }

protected:
  /**
   * @brief Constructs a new INeighborList
   * @param dataStructure
   * @param name
   * @param numTuples
   */
  INeighborList(DataStructure& dataStructure, const std::string& name, usize numTuples);

  /**
   * @brief Constructor for use when importing INeighborLists
   * @param dataStructure
   * @param name
   * @param numTuples
   * @param importId
   */
  INeighborList(DataStructure& dataStructure, const std::string& name, usize numTuples, IdType importId);

  /**
   * @brief Sets the number of tuples.
   * @param numTuples
   */
  void setNumberOfTuples(usize numTuples);

private:
  std::string m_NumNeighborsArrayName;
  usize m_NumTuples;
};
} // namespace nx::core
