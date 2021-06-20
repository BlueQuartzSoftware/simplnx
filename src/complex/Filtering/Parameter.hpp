#pragma once

#include <any>

#include "complex/complex_export.hpp"

namespace complex
{
class COMPLEX_EXPORT Parameter
{
public:
  virtual ~Parameter() = default;

  /**
   * @brief Returns the type_info required by the matching Argument.
   * @return const type_info&
   */
  virtual const type_info& valueTypeInfo() = 0;

private:
  Parameter();
};
} // namespace complex
