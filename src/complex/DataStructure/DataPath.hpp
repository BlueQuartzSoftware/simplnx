#pragma once

#include <string>
#include <vector>

#include "complex/complex_export.hpp"

namespace complex
{
class COMPLEX_EXPORT DataPath
{
public:
  /**
   * @brief
   * @param path
   */
  DataPath(const std::vector<std::string>& path = {});

  /**
   * @brief
   * @param other
   */
  DataPath(const DataPath& other);

  /**
   * @brief
   * @param other
   */
  DataPath(DataPath&& other) noexcept;

  /**
   * @brief
   */
  virtual ~DataPath();

  /**
   * @brief
   * @return size_t
   */
  size_t getLength() const;

  /**
   * @brief
   * @return std::vector<std::string>
   */
  std::vector<std::string> getPathVector() const;

  /**
   * @brief
   * @return DataPath
   */
  DataPath getParent() const;

  /**
   * @brief
   * @param name
   * @return DataPath
   */
  DataPath createChildPath(const std::string& name) const;

  /**
   * @brief
   * @param symbol
   * @param targetName
   * @return DataPath;
   */
  DataPath replace(const std::string& symbol, const std::string& targetName);

  /**
   * @brief
   * @param rhs
   * @return
   */
  bool operator==(const DataPath& rhs) const;

  /**
   * @brief
   * @param rhs
   * @return
   */
  bool operator!=(const DataPath& rhs) const;

  /**
   * @brief
   * @param index
   * @return std::string
   */
  const std::string& operator[](size_t index) const;

  /**
   * @brief
   * @param div = " / "
   * @return
   */
  std::string toString(const std::string& div = " / ") const;

protected:
  // Static Protected attributes
  //

  // Protected attributes
  //

  /**
   * @brief
   * @param path
   * @param child
   */
  DataPath(const std::vector<std::string>& path, const std::string& child);

private:
  // Static Private attributes
  //

  // Private attributes
  //

  std::vector<std::string> m_Path;
};
} // namespace complex
