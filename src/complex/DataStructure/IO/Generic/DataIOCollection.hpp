#pragma once

#include "complex/complex_export.hpp"

#include <map>
#include <memory>
#include <string>

namespace complex
{
class IDataIOManager;

class COMPLEX_EXPORT DataIOCollection
{
public:
  using map_type = std::map<std::string, std::shared_ptr<IDataIOManager>>;
  using iterator = typename map_type::iterator;
  using const_iterator = typename map_type::const_iterator;

  DataIOCollection();
  ~DataIOCollection() noexcept;

  void addIOManager(std::shared_ptr<IDataIOManager> manager);
  std::shared_ptr<IDataIOManager> getManager(const std::string& formatName) const;

  iterator begin();
  iterator end();

  const_iterator begin() const;
  const_iterator end() const;

protected:
  void initialize();

private:
  map_type m_ManagerMap;
};
} // namespace complex
