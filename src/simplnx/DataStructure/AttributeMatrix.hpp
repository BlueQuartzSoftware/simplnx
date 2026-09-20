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
   * @brief Creates a deep copy of the AttributeMatrix at a new path.
   * @param copyPath Path for the copy.
   * @return Shared pointer to the copy, or null when insertion fails.
   */
  std::shared_ptr<DataObject> deepCopy(const DataPath& copyPath) override;

  /**
   * @brief Creates a shallow copy of the AttributeMatrix.
   * @return Raw pointer that the caller owns.
   *
   * The copy must use a different name before insertion into the source DataStructure.
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
   * @brief Sets the tuple shape and resizes each child array in order.
   *
   * Shrinking can discard trailing values. A new shape with the same tuple count changes only shape metadata.
   *
   * @param tupleShape New tuple dimensions in slowest-to-fastest order.
   * @return Valid on success. The first child failure includes that child's path and propagates its error code.
   *
   * The method stops at the first failure. The matrix and each earlier child retain the new shape. Later children retain their prior shapes.
   */
  [[nodiscard]] Result<> resizeTuples(ShapeType tupleShape);

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
