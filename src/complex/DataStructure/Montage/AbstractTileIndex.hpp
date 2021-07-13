#pragma once

#include "complex/Utilities/TooltipGenerator.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
class AbstractGeometry;
class AbstractMontage;

/**
 * @class AbstractTileIndex
 * @brief
 */
class COMPLEX_EXPORT AbstractTileIndex
{
public:
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
   * @brief Returns the montage linked to this tile index.
   * @return AbstractMontage*
   */
  AbstractMontage* getMontage() const;

  /**
   * @brief Returns the geometry specified by the tile index.
   * @return AbstractGeometry*
   */
  virtual AbstractGeometry* getGeometry() const = 0;

  /**
   * @brief Checks if the index contains matches the conditions to be
   * a valid tile index.
   * @return bool
   */
  virtual bool isValid() const = 0;

  /**
   * @brief Returns a TooltipGenerator containing information about the
   * tile index and it specified montage.
   * @return TooltipGenerator
   */
  virtual TooltipGenerator getToolTipGenerator() const = 0;

protected:
  /**
   * @param montage
   */
  AbstractTileIndex(AbstractMontage* montage);

private:
  AbstractMontage* m_Montage = nullptr;
};
} // namespace complex
