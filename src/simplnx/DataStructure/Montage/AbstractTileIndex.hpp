#pragma once

#include "simplnx/Utilities/TooltipGenerator.hpp"
#include "simplnx/simplnx_export.hpp"

namespace nx::core
{
class IGeometry;
class AbstractMontage;

/**
 * @class AbstractTileIndex
 * @brief The AbstractTileIndex class serves as a base class for all montage
 * tile index classes. Each type of AbstractMontage should have its own derived
 * type of AbstractTileIndex as well.
 */
class SIMPLNX_EXPORT AbstractTileIndex
{
public:
  /**
   * @brief Constructs an invalid tile index.
   */
  AbstractTileIndex();

  /**
   * @brief Constructs a new AbstractTileIndex and copies values from the
   * specified tile index.
   * @param other
   */
  AbstractTileIndex(const AbstractTileIndex& other);

  /**
   * @brief Constructs a new AbstractTileIndex and moves values from the
   * specified tile index.
   * @param other
   */
  AbstractTileIndex(AbstractTileIndex&& other) noexcept;

  virtual ~AbstractTileIndex();

  /**
   * @brief Returns the montage linked to this tile index.
   * @return AbstractMontage*
   */
  const AbstractMontage* getMontage() const;

  /**
   * @brief Returns the geometry specified by the tile index.
   * @return IGeometry*
   */
  virtual const IGeometry* getGeometry() const = 0;

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
   * @brief Constructor taking a pointer to the target AbstractMontage.
   * @param montage
   */
  AbstractTileIndex(const AbstractMontage* montage);

private:
  const AbstractMontage* m_Montage = nullptr;
};
} // namespace nx::core
