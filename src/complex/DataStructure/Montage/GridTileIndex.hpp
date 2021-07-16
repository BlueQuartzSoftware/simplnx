#pragma once

#include "complex/Common/Array.hpp"
#include "complex/DataStructure/Montage/AbstractTileIndex.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
class GridMontage;

/**
 * class GridTileIndex
 *
 */

class COMPLEX_EXPORT GridTileIndex : public AbstractTileIndex
{
public:
  friend class GridMontage;
  
  /**
   * @brief Empty Constructor
   */
  GridTileIndex();

  /**
   * @brief Copy constructor
   */
  GridTileIndex(const GridTileIndex& other);

  /**
   * @brief Move constructor
   */
  GridTileIndex(GridTileIndex&& other) noexcept;

  virtual ~GridTileIndex();

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
  const AbstractGeometry* getGeometry() const override;

  /**
   * @return TooltipGenerator
   */
  TooltipGenerator getToolTipGenerator() const override;

  /**
   * @return bool
   */
  bool isValid() const override;

protected:
  /**
   * @brief
   * @param montage
   * @param pos
   */
  GridTileIndex(const GridMontage* montage, const SizeVec3& pos);

private:
  SizeVec3 m_Pos;
};
} // namespace complex
