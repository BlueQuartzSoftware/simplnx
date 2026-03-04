#pragma once

#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/simplnx_export.hpp"

namespace nx::core
{
/**
 * @class INodeGeometry0D
 * @brief Pure virtual interface for node-based geometries with vertices.
 *
 * Contains the string constants that form the public contract for 0D node geometries.
 */
class SIMPLNX_EXPORT INodeGeometry0D
{
public:
  static constexpr StringLiteral k_SharedVertexListName = "Shared Vertex List";
  static constexpr StringLiteral k_VertexAttributeMatrixName = "Vertex Data";

  virtual ~INodeGeometry0D() noexcept = default;

protected:
  INodeGeometry0D() = default;
  INodeGeometry0D(const INodeGeometry0D&) = default;
  INodeGeometry0D(INodeGeometry0D&&) = default;
  INodeGeometry0D& operator=(const INodeGeometry0D&) = default;
  INodeGeometry0D& operator=(INodeGeometry0D&&) noexcept = default;
};
} // namespace nx::core
