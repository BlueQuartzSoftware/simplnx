#pragma once

#include "complex/DataStructure/AttributeMatrix.hpp"
#include "complex/Filter/Output.hpp"
#include "complex/complex_export.hpp"

namespace complex
{

/**
 * @brief Action to create a DataGroup with the DataStructure
 */
class COMPLEX_EXPORT CreateAttributeMatrixAction : public IDataCreationAction
{
public:
  CreateAttributeMatrixAction() = delete;

  CreateAttributeMatrixAction(const DataPath& path, const AttributeMatrix::ShapeType& shape);

  ~CreateAttributeMatrixAction() noexcept override;

  CreateAttributeMatrixAction(const CreateAttributeMatrixAction&) = delete;
  CreateAttributeMatrixAction(CreateAttributeMatrixAction&&) noexcept = delete;
  CreateAttributeMatrixAction& operator=(const CreateAttributeMatrixAction&) = delete;
  CreateAttributeMatrixAction& operator=(CreateAttributeMatrixAction&&) noexcept = delete;

  /**
   * @brief Applies this action's change to the given DataStructure in the given mode.
   * Returns any warnings/errors. On error, DataStructure is not guaranteed to be consistent.
   * @param dataStructure
   * @return
   */
  Result<> apply(DataStructure& dataStructure, Mode mode) const override;

  /**
   * @brief Returns a copy of the action.
   * @return
   */
  UniquePointer clone() const override;

  /**
   * @brief Returns all of the DataPaths to be created.
   * @return std::vector<DataPath>
   */
  std::vector<DataPath> getAllCreatedPaths() const override;

private:
  AttributeMatrix::ShapeType m_TupleShape;
};
} // namespace complex
