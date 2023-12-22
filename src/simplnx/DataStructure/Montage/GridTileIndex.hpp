#pragma once

#include "simplnx/Common/Array.hpp"
#include "simplnx/DataStructure/Montage/AbstractTileIndex.hpp"
#include "simplnx/simplnx_export.hpp"

namespace nx::core
{
class GridMontage;

/**
 * @class GridTileIndex
 * @brief The GridTileIndex class is the corresponding tile index for
 * GridMontages. An index corresponds to a single 3D tile position in the
 * montage.
 */
class SIMPLNX_EXPORT GridTileIndex : public AbstractTileIndex
{
public:
  friend class GridMontage;

  /**
   * @brief Creates an invalid GridTileIndex.
   */
  GridTileIndex();

  /**
   * @brief Creates a copy of the specified GridTileIndex.
   * @param other
   */
  GridTileIndex(const GridTileIndex& other);

  /**
   * @brief Creates a new GridTileIndex and moves values from the specified
   * GridTileIndex.
   */
  GridTileIndex(GridTileIndex&& other) noexcept;

  ~GridTileIndex() override;

  /**
   * @brief Returns the index's row position.
   * @return usize
   */
  usize getRow() const;

  /**
   * @brief Returns the index's column position.
   * @return usize
   */
  usize getCol() const;

  /**
   * @brief Returns the index's depth position.
   * @return usize
   */
  usize getDepth() const;

  /**
   * @brief Returns the index's 3D tile position.
   * @return SizeVec3Type
   */
  SizeVec3 getTilePos() const;

  /**
   * @brief Returns a const pointer to the target geometry.
   * @return IGeometry*
   */
  const IGeometry* getGeometry() const override;

  /**
   * @brief Returns a TooltipGenerator containing information for generating an
   * HTML tooltip based on current values.
   * @return TooltipGenerator
   */
  TooltipGenerator getToolTipGenerator() const override;

  /**
   * @brief Returns true if the index is valid. Returns false otherwise.
   * A valid index has both a target GridMontage and 3D position.
   * @return bool
   */
  bool isValid() const override;

protected:
  /**
   * @brief Creates a new GridTileIndex for the target GridMontage and 3D tile
   * position.
   * @param montage
   * @param pos
   */
  GridTileIndex(const GridMontage* montage, const SizeVec3& pos);

private:
  SizeVec3 m_Pos;
};
} // namespace nx::core
