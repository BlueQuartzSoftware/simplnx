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
 * Copy, Move, and Reference validate supplied bounds before attaching them.
 * Create allocates a cell attribute matrix and three bound arrays with at
 * least two values each. In execute, new bounds start at zero and increase.
 */
class SIMPLNX_EXPORT CreateRectGridGeometryAction : public IDataCreationAction
{
public:
  using DimensionType = std::vector<size_t>;

  using OriginType = std::vector<float>;

  using SpacingType = std::vector<float>;

  CreateRectGridGeometryAction(const DataPath& path, usize xBoundsDim, usize yBoundsDim, usize zBoundsDim, std::string  cellAttributeMatrixName, std::string  xBoundsName,
                               std::string  yBoundsName, std::string  zBoundsName);

  CreateRectGridGeometryAction(const DataPath& path, const DataPath& inputXBoundsPath, const DataPath& inputYBoundsPath, const DataPath& inputZBoundsPath, std::string  cellAttributeMatrixName,
                               const ArrayHandlingType& arrayType);

  ~CreateRectGridGeometryAction() noexcept override;

  CreateRectGridGeometryAction(const CreateRectGridGeometryAction&) = delete;
  CreateRectGridGeometryAction(CreateRectGridGeometryAction&&) noexcept = delete;
  CreateRectGridGeometryAction& operator=(const CreateRectGridGeometryAction&) = delete;
  CreateRectGridGeometryAction& operator=(CreateRectGridGeometryAction&&) noexcept = delete;

  /**
   * @brief Creates and configures the RectGridGeom.
   * The action checks requested counts and supplied bounds before creating the geometry in either mode.
   * If these checks fail, the data structure is unchanged. A later creation or store error can leave a partial result.
   * @param dataStructure Destination data structure.
   * @param mode Preflight or execute action mode.
   * @return Success or a bounds error. Error -5914 indicates a requested count below two in Create mode.
   * RectGridGeom::ValidateBounds() can return -4006, -4048, -4008, -4007, or a store read error.
   * Other errors include path, allocation, copy, store write, and reparenting errors.
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
