#pragma once

#include "simplnx/Filter/Output.hpp"

namespace nx::core
{
/**
 * @brief Action for importing DataObjects into a DataStructure.
 */
class SIMPLNX_EXPORT ImportObjectAction : public AbstractDataCreationAction
{
public:
  ImportObjectAction() = delete;

  ImportObjectAction(const std::shared_ptr<AbstractDataObject>& importObject, const DataPath& path);

  ~ImportObjectAction() noexcept override;

  ImportObjectAction(const ImportObjectAction&) = delete;
  ImportObjectAction(ImportObjectAction&&) noexcept = delete;
  ImportObjectAction& operator=(const ImportObjectAction&) = delete;
  ImportObjectAction& operator=(ImportObjectAction&&) noexcept = delete;

  /**
   * @brief Applies this action's change to the given DataStructure in the given mode.
   * Returns any warnings/errors. On error, DataStructure is not guaranteed to be consistent.
   * @param dataStructure
   * @param mode
   * @return Result<>
   */
  Result<> apply(DataStructure& dataStructure, Mode mode) const override;

  /**
   * @brief Returns a copy of the action.
   * @return
   */
  UniquePointer clone() const override;

  /**
   * @brief Returns a shared_ptr to the AbstractDataObject being imported into the DataStructure.
   * @return std::shared_ptr<AbstractDataObject>
   */
  std::shared_ptr<AbstractDataObject> getImportObject() const;

  /**
   * @brief Returns the path used to insert the AbstractDataObject into the DataStructure.
   * @return DataPath
   */
  DataPath path() const;

  /**
   * @brief Returns all of the DataPaths to be created.
   * @return std::vector<DataPath>
   */
  std::vector<DataPath> getAllCreatedPaths() const override;

private:
  std::shared_ptr<AbstractDataObject> m_ImportData;
};
} // namespace nx::core