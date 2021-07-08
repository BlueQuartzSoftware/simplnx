#pragma once

#include "SIMPL/Utilities/TooltipGenerator.h"

namespace SIMPL
{
class AbstractGeometry;
class AbstractMontage;

/**
 * class AbstractTileIndex
 *
 */

class AbstractTileIndex
{
public:
  // Constructors/Destructors
  //

  /**
   * @brief
   */
  AbstractTileIndex();

  /**
   * @brief
   * @param other
   */
  AbstractTileIndex(const AbstractTileIndex& other);

  /**
   * @brief
   * @param other
   */
  AbstractTileIndex(AbstractTileIndex&& other) noexcept;

  /**
   * Empty Destructor
   */
  virtual ~AbstractTileIndex();

  /**
   * @return AbstractMontage*
   */
  AbstractMontage* getMontage() const;

  /**
   * @return AbstractGeometry*
   */
  virtual AbstractGeometry* getGeometry() const = 0;

  /**
   * @return bool
   */
  virtual bool isValid() const = 0;

  /**
   * @return SIMPL::TooltipGenerator
   */
  virtual SIMPL::TooltipGenerator getToolTipGenerator() const = 0;

protected:
  /**
   * @param montage
   */
  AbstractTileIndex(AbstractMontage* montage);

private:
  AbstractMontage* m_Montage = nullptr;
};
} // namespace SIMPL
