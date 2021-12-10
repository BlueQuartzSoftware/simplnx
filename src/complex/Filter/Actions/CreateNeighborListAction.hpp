#pragma once

#include "complex/Filter/Output.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
/**
 * @class CreateNeighborListAction
 * @brief Action for creating NeighborList arrays in a DataStructure
 */
class COMPLEX_EXPORT CreateNeighborListAction : public IDataAction
{
public:
  CreateNeighborListAction() = delete;

  CreateNeighborListAction(DataType type, usize tupleCount, const DataPath& path);

  ~CreateNeighborListAction() noexcept override;

  CreateNeighborListAction(const CreateNeighborListAction&) = delete;
  CreateNeighborListAction(CreateNeighborListAction&&) noexcept = delete;
  CreateNeighborListAction& operator=(const CreateNeighborListAction&) = delete;
  CreateNeighborListAction& operator=(CreateNeighborListAction&&) noexcept = delete;

  /**
   * @brief Applies this action's change to the given DataStructure in the given mode.
   * Returns any warnings/errors. On error, DataStructure is not guaranteed to be consistent.
   * @param dataStructure
   * @return Result<>
   */
  Result<> apply(DataStructure& dataStructure, Mode mode) const override;

  /**
   * @brief Returns the DataType of the DataArray to be created.
   * @return DataType
   */
  DataType type() const;

  /**
   * @brief Returns the number of tuples for the NeighborList to be created.
   * @return usize
   */
  usize tupleCount() const;

  /**
   * @brief Returns the path of the DataArray to be created.
   * @return DataPath
   */
  DataPath path() const;

private:
  DataType m_Type;
  usize m_TupleCount;
  DataPath m_Path;
};
} // namespace complex
