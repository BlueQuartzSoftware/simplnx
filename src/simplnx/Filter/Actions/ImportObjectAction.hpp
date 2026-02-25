#pragma once

#include "simplnx/Filter/Output.hpp"

namespace nx::core
{
/**
 * @brief Action for importing DataObjects into a DataStructure.
 */
class SIMPLNX_EXPORT ImportObjectAction : public IDataCreationAction
{
public:
  ImportObjectAction() = delete;

  /**
   * @brief Constructs an ImportObjectAction.
   * @param importObject The DataObject to import
   * @param path The path where the object will be inserted
   */
  ImportObjectAction(const std::shared_ptr<DataObject>& importObject, const DataPath& path);

  ~ImportObjectAction() noexcept override;

  ImportObjectAction(const ImportObjectAction&) = delete;
  ImportObjectAction(ImportObjectAction&&) noexcept = delete;
  ImportObjectAction& operator=(const ImportObjectAction&) = delete;
  ImportObjectAction& operator=(ImportObjectAction&&) noexcept = delete;

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
   * @brief Returns a shared_ptr to the DataObject being imported into the DataStructure.
   * @return std::shared_ptr<DataObject>
   */
  std::shared_ptr<DataObject> getImportObject() const;

  /**
   * @brief Returns the path used to insert the DataObject into the DataStructure.
   * @return DataPath
   */
  DataPath path() const;

  /**
   * @brief Returns all of the DataPaths to be created.
   * @return std::vector<DataPath>
   */
  std::vector<DataPath> getAllCreatedPaths() const override;

private:
  std::shared_ptr<DataObject> m_ImportData;
};
} // namespace nx::core