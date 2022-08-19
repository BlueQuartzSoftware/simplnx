#pragma once

#include <map>
#include <string>

namespace complex
{

class UpdatedUUIDMaps
{
public:
  UpdatedUUIDMaps()
  {
  }
  ~UpdatedUUIDMaps() = default;
  UpdatedUUIDMaps(const UpdatedUUIDMaps&) = default;
  UpdatedUUIDMaps(UpdatedUUIDMaps&&) noexcept = default;
  UpdatedUUIDMaps& operator=(const UpdatedUUIDMaps&) = delete;
  UpdatedUUIDMaps& operator=(UpdatedUUIDMaps&&) noexcept = delete;

private:
  static const std::map<std::string, std::string> k_SIMPL_to_Complex;
  static const std::map<std::string, std::string> k_Complex_to_SIMPL;
};

} // namespace complex
