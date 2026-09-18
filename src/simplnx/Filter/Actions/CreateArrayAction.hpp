#pragma once

#include "simplnx/simplnx_export.hpp"

#include "simplnx/Common/Aliases.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Output.hpp"

#include <string>
#include <vector>

namespace nx::core
{

/**
 * @class CreateArrayAction
 * @brief Creates a numeric DataArray through an output action.
 *
 * An empty format defers storage selection to the data-structure resolver. A
 * non-empty format requests an override after the geometry compatibility gate.
 * Tuple, component, and byte-count products must fit their required types.
 */
class SIMPLNX_EXPORT CreateArrayAction : public IDataCreationAction
{
public:
  CreateArrayAction() = delete;

  CreateArrayAction(DataType type, const std::vector<usize>& tDims, const std::vector<usize>& cDims, const DataPath& path, std::string dataFormat = "", std::string fillValue = "");

  ~CreateArrayAction() noexcept override;

  CreateArrayAction(const CreateArrayAction&) = delete;
  CreateArrayAction(CreateArrayAction&&) noexcept = delete;
  CreateArrayAction& operator=(const CreateArrayAction&) = delete;
  CreateArrayAction& operator=(CreateArrayAction&&) noexcept = delete;

  /**
   * @brief Creates the configured array in a data structure.
   * @param dataStructure Destination data structure.
   * @param mode Preflight or execute action mode.
   * @return Creation warnings or errors.
   */
  Result<> apply(DataStructure& dataStructure, Mode mode) const override;

  UniquePointer clone() const override;

  DataType type() const;

  const std::vector<usize>& dims() const;

  const ShapeType& componentDims() const;

  DataPath path() const;

  std::vector<DataPath> getAllCreatedPaths() const override;

  std::string fillValue() const;

  /**
   * @brief Returns the requested storage-format override.
   *
   * An empty string uses automatic resolver selection. A non-empty value requests
   * the specified format after the geometry compatibility gate.
   * @return Requested format, or an empty string for automatic selection.
   */
  std::string dataFormat() const;

private:
  DataType m_Type;
  std::vector<usize> m_Dims;
  std::vector<usize> m_CDims;
  std::string m_DataFormat = "";
  std::string m_FillValue = "";
};
} // namespace nx::core
