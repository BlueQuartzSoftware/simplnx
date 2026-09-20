#pragma once

#include "simplnx/Common/Array.hpp"
#include "simplnx/DataStructure/Geometry/IGridGeometry.hpp"
#include "simplnx/Filter/Output.hpp"
#include "simplnx/simplnx_export.hpp"

namespace nx::core
{

/**
 * @class CreateRectGridGeometryAction
 * @brief Creates a RectGridGeom with axis-bound arrays.
 *
 * Copy, Move, and Reference attach supplied bounds. Create allocates three
 * bound arrays and a cell attribute matrix. Each bound count must be greater
 * than zero.
 */
class SIMPLNX_EXPORT CreateRectGridGeometryAction : public IDataCreationAction
{
public:
  using DimensionType = std::vector<size_t>;

  using OriginType = std::vector<float>;

  using SpacingType = std::vector<float>;

  CreateRectGridGeometryAction(const DataPath& path, usize xBoundsDim, usize yBoundsDim, usize zBoundsDim, const std::string& cellAttributeMatrixName, const std::string& xBoundsName,
                               const std::string& yBoundsName, const std::string& zBoundsName);

  CreateRectGridGeometryAction(const DataPath& path, const DataPath& inputXBoundsPath, const DataPath& inputYBoundsPath, const DataPath& inputZBoundsPath, const std::string& cellAttributeMatrixName,
                               const ArrayHandlingType& arrayType);

  ~CreateRectGridGeometryAction() noexcept override;

  CreateRectGridGeometryAction(const CreateRectGridGeometryAction&) = delete;
  CreateRectGridGeometryAction(CreateRectGridGeometryAction&&) noexcept = delete;
  CreateRectGridGeometryAction& operator=(const CreateRectGridGeometryAction&) = delete;
  CreateRectGridGeometryAction& operator=(CreateRectGridGeometryAction&&) noexcept = delete;

  /**
   * @brief Creates and configures the RectGridGeom.
   * @param dataStructure Destination data structure.
   * @param mode Preflight or execute action mode.
   * @return Validation, allocation, or reparenting errors.
   * @pre Existing bound arrays have at least one tuple.
   */
  Result<> apply(DataStructure& dataStructure, Mode mode) const override;

  UniquePointer clone() const override;

  DataPath path() const;

  const usize& xDims() const;

  const usize& yDims() const;

  const usize& zDims() const;

  /**
   * @brief Returns paths created by this action.
   * @return Geometry and cell-data paths. Create and Copy also return bound-array paths.
   */
  std::vector<DataPath> getAllCreatedPaths() const override;

protected:
  CreateRectGridGeometryAction() = default;

private:
  usize m_NumXBoundTuples = 2;
  usize m_NumYBoundTuples = 2;
  usize m_NumZBoundTuples = 2;
  std::string m_CellDataName;
  std::string m_XBoundsArrayName;
  std::string m_YBoundsArrayName;
  std::string m_ZBoundsArrayName;
  DataPath m_InputXBounds;
  DataPath m_InputYBounds;
  DataPath m_InputZBounds;
  ArrayHandlingType m_ArrayHandlingType = ArrayHandlingType::Create;

  Float32Array* createBoundArray(DataStructure& dataStructure, Mode mode, const std::string& arrayName, usize numTuples, std::vector<Error>& errors) const;
};
} // namespace nx::core
