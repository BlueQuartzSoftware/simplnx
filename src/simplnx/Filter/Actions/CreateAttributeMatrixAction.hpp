#pragma once

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/Filter/Output.hpp"
#include "simplnx/simplnx_export.hpp"

namespace nx::core
{

/**
 * @brief Action to create a DataGroup with the DataStructure
 */
class SIMPLNX_EXPORT CreateAttributeMatrixAction : public IDataCreationAction
{
public:
  CreateAttributeMatrixAction() = delete;

  /**
   * @brief Constructs a CreateAttributeMatrixAction.
   * @param path The path where the AttributeMatrix will be created
   * @param shape The tuple shape of the AttributeMatrix
   */
  CreateAttributeMatrixAction(const DataPath& path, const ShapeType& shape);

  ~CreateAttributeMatrixAction() noexcept override;

  CreateAttributeMatrixAction(const CreateAttributeMatrixAction&) = delete;
  CreateAttributeMatrixAction(CreateAttributeMatrixAction&&) noexcept = delete;
  CreateAttributeMatrixAction& operator=(const CreateAttributeMatrixAction&) = delete;
  CreateAttributeMatrixAction& operator=(CreateAttributeMatrixAction&&) noexcept = delete;

  /**
   * @brief Applies this action's change to the given DataStructure in the given mode.
   * Returns any warnings/errors. On error, DataStructure is not guaranteed to be consistent.
   * @param dataStructure The DataStructure to modify
   * @param mode The mode (Preflight or Execute)
   * @return Result<> Result with any errors or warnings
   */
  Result<> apply(DataStructure& dataStructure, Mode mode) const override;

  /**
   * @brief Returns a copy of the action.
   * @return UniquePointer A unique pointer to the cloned action
   */
  UniquePointer clone() const override;

  /**
   * @brief Returns all of the DataPaths to be created.
   * @return std::vector<DataPath>
   */
  std::vector<DataPath> getAllCreatedPaths() const override;

private:
  ShapeType m_TupleShape;
};
} // namespace nx::core
