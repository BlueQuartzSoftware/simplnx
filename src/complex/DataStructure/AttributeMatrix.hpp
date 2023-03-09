#pragma once

#include "complex/DataStructure/BaseGroup.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
/**
 * @class AttributeMatrix
 * @brief The AttributeMatrix class is an instantiable implementation of BaseGroup.
 * AttributeMatrix only accepts DataArrays of a certain tuple size.
 */
class COMPLEX_EXPORT AttributeMatrix : public BaseGroup
{
public:
  using ShapeType = std::vector<usize>;

  static inline constexpr StringLiteral k_TypeName = "AttributeMatrix";

  /**
   * @brief Attempts to construct and insert a AttributeMatrix into the DataStructure.
   * If a parentId is provided, then the AttributeMatrix is created with the
   * corresponding BaseGroup as its parent. Otherwise, the DataStucture will be
   * used as the parent object. In either case, the DataStructure will take
   * ownership of the AttributeMatrix.
   *
   * Returns a pointer to the AttributeMatrix if the process succeeds. Returns
   * nullptr otherwise.
   * @param dataStructure
   * @param name
   * @param parentId = {}
   * @return AttributeMatrix*
   */
  static AttributeMatrix* Create(DataStructure& dataStructure, std::string name, const std::optional<IdType>& parentId = {});

  /**
   * @brief Attempts to construct and insert a AttributeMatrix into the DataStructure.
   * If a parentId is provided, then the AttributeMatrix is created with the
   * corresponding BaseGroup as its parent. Otherwise, the DataStucture will be
   * used as the parent object. In either case, the DataStructure will take
   * ownership of the AttributeMatrix.
   *
   * Unlike Create, Import allows setting the DataObject ID for use in
   * importing data.
   *
   * Returns a pointer to the AttributeMatrix if the process succeeds. Returns
   * nullptr otherwise.
   * @param dataStructure
   * @param name
   * @param importId
   * @param parentId = {}
   * @return AttributeMatrix*
   */
  static AttributeMatrix* Import(DataStructure& dataStructure, std::string name, IdType importId, const std::optional<IdType>& parentId = {});

  /**
   * @brief Constructs a shallow copy of the AttributeMatrix. This copy is not added
   * to the DataStructure by default.
   * @param other
   */
  AttributeMatrix(const AttributeMatrix& other);

  /**
   * @brief Constructs a AttributeMatrix and moves values from the specified target.
   * @param other
   */
  AttributeMatrix(AttributeMatrix&& other);

  /**
   * @brief Destructor.
   */
  ~AttributeMatrix() noexcept override;

  /**
   * @brief Returns an enumeration of the class or subclass. Used for quick comparison or type deduction
   * @return
   */
  DataObject::Type getDataObjectType() const override;

  /**
   * @brief Returns an enumeration of the class or subclass GroupType. Used for quick comparison or type deduction
   * @return
   */
  GroupType getGroupType() const override;

  /**
   * @brief Creates and returns a deep copy of the AttributeMatrix. The caller is
   * responsible for deleting the returned pointer when it is no longer needed.
   * @return DataObject*
   */
  std::shared_ptr<DataObject> deepCopy(const DataPath& copyPath) override;

  /**
   * @brief Creates and returns a shallow copy of the AttributeMatrix. The caller is
   * responsible for deleting the returned pointer when it is no longer needed
   * as a copy cannot be added to the DataStructure anywhere the original
   * exists without changing its name.
   * @return DataObject*
   */
  DataObject* shallowCopy() override;

  /**
   * @brief Returns typename of the DataObject as a std::string.
   * @return std::string
   */
  std::string getTypeName() const override;

  /**
   * @brief Returns the tuple shape.
   * @return
   */
  const ShapeType& getShape() const;

  /**
   * @brief Sets the tuple shape.
   * @param shape
   */
  void setShape(ShapeType shape);

  /**
   * @brief Returns the total number of tuples.
   * @return
   */
  usize getNumTuples() const;

protected:
  /**
   * @brief Creates the AttributeMatrix for the target DataStructure and with the
   * specified name.
   * @param dataStructure
   * @param name
   */
  AttributeMatrix(DataStructure& dataStructure, std::string name);

  /**
   * @brief Creates the AttributeMatrix for the target DataStructure and with the
   * specified name.
   * @param dataStructure
   * @param name
   * @param importId
   */
  AttributeMatrix(DataStructure& dataStructure, std::string name, IdType importId);

  /**
   * @brief Checks if the provided DataObject can be added to the container.
   * Returns true if the DataObject can be added to the container. Otherwise,
   * returns false.
   * @param obj
   * @return bool
   */
  bool canInsert(const DataObject* obj) const override;

private:
  ShapeType m_TupleShape;
};
} // namespace complex
