#pragma once

#include "SIMPL/Common/SIMPLArray.hpp"
#include "SIMPL/DataStructure/Montage/AbstractTileIndex.h"

namespace SIMPL
{
class GridMontage;

/**
 * class GridTileIndex
 *
 */

class GridTileIndex : virtual public AbstractTileIndex
{
public:
  // Constructors/Destructors
  //

  /**
   * Empty Constructor
   */
  GridTileIndex();
  GridTileIndex(const GridTileIndex& other);
  GridTileIndex(GridTileIndex&& other) noexcept;

  /**
   * Empty Destructor
   */
  virtual ~GridTileIndex();

  // Static Public attributes
  //

  // Public attributes
  //

  /**
   * @return size_t
   */
  size_t getRow() const;

  /**
   * @return size_t
   */
  size_t getCol() const;

  /**
   * @return size_t
   */
  size_t getDepth() const;

  /**
   * @return SizeVec3Type
   */
  SizeVec3 getTilePos() const;

  /**
   * @return AbstractGeometry*
   */
  AbstractGeometry* getGeometry() const override;

  /**
   * @return SIMPL::TooltipGenerator
   */
  SIMPL::TooltipGenerator getToolTipGenerator() const override;

  /**
   * @return bool
   */
  bool isValid() const override;

protected:
  // Static Protected attributes
  //

  // Protected attributes
  //

  /**
   * @brief
   * @param montage
   * @param pos
   */
  GridTileIndex(GridMontage* montage, const SizeVec3& pos);

private:
  SizeVec3 m_Pos;
};
} // namespace SIMPL
