#pragma once

#include "simplnx/DataStructure/BaseGroup.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/simplnx_export.hpp"

namespace nx::core
{
/**
 * @class AttributeMatrix
 * @brief The AttributeMatrix class is an instantiable implementation of BaseGroup.
 * AttributeMatrix only accepts DataArrays of a certain tuple size.
 */
class SIMPLNX_EXPORT AttributeMatrix : public BaseGroup
{
public:
  static inline constexpr StringLiteral k_TypeName = "AttributeMatrix";

  /**
   * @brief Attempts to construct and insert a AttributeMatrix into the DataStructure.
   * If a parentId is provided, then the AttributeMatrix is created with the
   * corresponding BaseGroup as its parent. Otherwise, the DataStructure will be
   * used as the parent object. In either case, the DataStructure will take
   * ownership of the AttributeMatrix.
   *
   * Returns a pointer to the AttributeMatrix if the process succeeds. Returns
   * nullptr otherwise.
   * @param dataStructure The DataStructure to insert the AttributeMatrix into
   * @param name The name of the AttributeMatrix
   * @param tupleShape The Tuple Shape (Dimensions) of the AttributeMatrix
   * @param parentId Optional ID of the parent DataObject
   * @return AttributeMatrix* Pointer to the created AttributeMatrix, or nullptr if creation failed
   */
  static AttributeMatrix* Create(DataStructure& dataStructure, std::string name, ShapeType tupleShape, const std::optional<IdType>& parentId = {});

  /**
   * @brief Attempts to construct and insert a AttributeMatrix into the DataStructure.
   * If a parentId is provided, then the AttributeMatrix is created with the
   * corresponding BaseGroup as its parent. Otherwise, the DataStructure will be
   * used as the parent object. In either case, the DataStructure will take
   * ownership of the AttributeMatrix.
   *
   * Unlike Create, Import allows setting the DataObject ID for use in
   * importing data.
   *
   * Returns a pointer to the AttributeMatrix if the process succeeds. Returns
   * nullptr otherwise.
   * @param dataStructure The DataStructure to insert the AttributeMatrix into
   * @param name The name of the AttributeMatrix
   * @param tupleShape The Tuple Shape (Dimensions) of the AttributeMatrix
   * @param importId The ID to use for the imported AttributeMatrix
   * @param parentId Optional ID of the parent DataObject
   * @return AttributeMatrix* Pointer to the imported AttributeMatrix, or nullptr if import failed
   */
  static AttributeMatrix* Import(DataStructure& dataStructure, std::string name, ShapeType tupleShape, IdType importId, const std::optional<IdType>& parentId = {});

  /**
   * @brief Constructs a shallow copy of the AttributeMatrix. This copy is not added
   * to the DataStructure by default.
   * @param other The AttributeMatrix to copy from
   */
  AttributeMatrix(const AttributeMatrix& other);

  /**
   * @brief Constructs a AttributeMatrix and moves values from the specified target.
   * @param other The AttributeMatrix to move from
   */
  AttributeMatrix(AttributeMatrix&& other);

  /**
   * @brief Destructor.
   */
  ~AttributeMatrix() noexcept override;

  /**
   * @brief Returns an enumeration of the class or subclass. Used for quick comparison or type deduction
   * @return DataObject::Type The type enum value for AttributeMatrix
   */
  DataObject::Type getDataObjectType() const override;

  /**
   * @brief Returns an enumeration of the class or subclass GroupType. Used for quick comparison or type deduction
   * @return GroupType The group type enum value for AttributeMatrix
   */
  GroupType getGroupType() const override;

  /**
   * @brief Creates and returns a deep copy of the AttributeMatrix. The caller is
   * responsible for deleting the returned pointer when it is no longer needed.
   * @param copyPath The path where the deep copy should be placed
   * @return std::shared_ptr<DataObject> Shared pointer to the deep copied AttributeMatrix
   */
  std::shared_ptr<DataObject> deepCopy(const DataPath& copyPath) override;

  /**
   * @brief Creates and returns a shallow copy of the AttributeMatrix. The caller is
   * responsible for deleting the returned pointer when it is no longer needed
   * as a copy cannot be added to the DataStructure anywhere the original
   * exists without changing its name.
   * @return DataObject* Pointer to the shallow copied AttributeMatrix
   */
  DataObject* shallowCopy() override;

  /**
   * @brief Returns typename of the DataObject as a std::string.
   * @return std::string
   */
  std::string getTypeName() const override;

  /**
   * @brief Returns the tuple shape.
   * @return const ShapeType& Reference to the tuple shape
   */
  const ShapeType& getShape() const;

  /**
   * @brief Returns the total number of tuples.
   * @return usize The total number of tuples
   */
  usize getNumberOfTuples() const;

  /**
   * @brief Sets the tuple Shape and resizes all child arrays.
   *
   * <b>This may result in loss of data if the over all number of tuples is less
   * than the original number</b>. This action will <b>OVERWRITE</b> any existing
   * tuple shape that DataArrays contained in this AttributeMatrix currently have.
   *
   * For example: If an underlying array has a Tuple Shape of [4][5] and this
   * method is called with a Tuple Shape of [2][10], the underlying array will have
   * its Tuple Shape changed to be [4][5]. Since the total number of tuples remain
   * the same no data loss or memory allocation will take place.
   *
   * @param tupleShape The new tuple shape to set
   */
  void resizeTuples(ShapeType tupleShape);

  /**
   * @brief Validates that every IArray held by this attribute matrix have the same number
   * of tuples that the attribute matrix requires.
   * @return Result<> Result indicating success or containing validation errors
   */
  Result<> validate() const;

protected:
  /**
   * @brief Creates the AttributeMatrix for the target DataStructure and with the
   * specified name.
   * @param dataStructure The DataStructure that will contain this AttributeMatrix
   * @param name The name of the AttributeMatrix
   * @param tupleShape The tuple shape of the AttributeMatrix
   */
  AttributeMatrix(DataStructure& dataStructure, std::string name, ShapeType tupleShape);

  /**
   * @brief Creates the AttributeMatrix for the target DataStructure and with the
   * specified name.
   * @param dataStructure The DataStructure that will contain this AttributeMatrix
   * @param name The name of the AttributeMatrix
   * @param tupleShape The tuple shape of the AttributeMatrix
   * @param importId The ID to use when importing this AttributeMatrix
   */
  AttributeMatrix(DataStructure& dataStructure, std::string name, ShapeType tupleShape, IdType importId);

  /**
   * @brief Checks if the provided DataObject can be added to the container.
   * Returns true if the DataObject can be added to the container. Otherwise,
   * returns false.
   * @param obj Pointer to the DataObject to check
   * @return bool True if the DataObject can be inserted, false otherwise
   */
  bool canInsert(const DataObject* obj) const override;

private:
  ShapeType m_TupleShape;
};
} // namespace nx::core
