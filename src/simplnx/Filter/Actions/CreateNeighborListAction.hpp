#pragma once

#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/Filter/Output.hpp"

#include "simplnx/simplnx_export.hpp"

#include <string>

namespace nx::core
{

/**
 * @class CreateNeighborListAction
 * @brief Creates a typed NeighborList through an output action.
 *
 * An empty format defers storage selection to the data-structure resolver. A
 * non-empty format requests an override after the geometry compatibility gate.
 * The tuple-count product must fit usize.
 */
class SIMPLNX_EXPORT CreateNeighborListAction : public IDataCreationAction
{
public:
  CreateNeighborListAction() = delete;

  CreateNeighborListAction(DataType type, const ShapeType& tupleShape, const DataPath& path, std::string dataFormat = "");

  ~CreateNeighborListAction() noexcept override;

  CreateNeighborListAction(const CreateNeighborListAction&) = delete;
  CreateNeighborListAction(CreateNeighborListAction&&) noexcept = delete;
  CreateNeighborListAction& operator=(const CreateNeighborListAction&) = delete;
  CreateNeighborListAction& operator=(CreateNeighborListAction&&) noexcept = delete;

  /**
   * @brief Creates the configured NeighborList in a data structure.
   * @param dataStructure Destination data structure.
   * @param mode Preflight or execute action mode.
   * @return Creation warnings or errors.
   */
  Result<> apply(DataStructure& dataStructure, Mode mode) const override;

  UniquePointer clone() const override;

  DataType type() const;

  const ShapeType& tupleShape() const;

  DataPath path() const;

  std::vector<DataPath> getAllCreatedPaths() const override;

  /**
   * @brief Returns the requested storage-format override.
   *
   * An empty string uses automatic resolver selection. A non-empty value remains
   * subject to the unstructured-geometry in-core gate.
   * @return Requested format, or an empty string for automatic selection.
   */
  std::string dataFormat() const;

private:
  DataType m_Type;
  ShapeType m_TupleShape;
  std::string m_DataFormat = "";
};
} // namespace nx::core
