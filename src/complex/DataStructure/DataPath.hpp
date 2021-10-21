#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "complex/Common/Types.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
/**
 * @class DataPath
 * @brief The DataPath class is designed to store a path through the
 * DataStructure to reach a specific DataObject. There may be multiple paths
 * to a DataObject, but by providing a path, it is possible to extrapolate
 * which set of siblings a user may be interested in or iterate over a data
 * group with common children names.
 */
class COMPLEX_EXPORT DataPath
{
public:
  /**
   * @brief Attempts to create a DataPath from the given string by splitting it using the given delimiter.
   * @param string
   * @param delimter
   * @return
   */
  static std::optional<DataPath> FromString(std::string_view string, char delimter = '/');

  /**
   * @brief Default constructor.
   */
  DataPath();

  /**
   * @brief Creates a DataPath using a vector of DataObject names.
   * @param path
   */
  DataPath(std::vector<std::string> path);

  /**
   * @brief Copy constructor.
   * @param rhs
   */
  DataPath(const DataPath& rhs);

  /**
   * @brief Move constructor.
   * @param rhs
   */
  DataPath(DataPath&& rhs) noexcept;

  /**
   * @brief Copy assignment.
   * @param other
   * @return
   */
  DataPath& operator=(const DataPath& rhs);

  /**
   * @brief Move assignment.
   * @param rhs
   * @return
   */
  DataPath& operator=(DataPath&& rhs) noexcept;

  /**
   * @brief Destructor.
   */
  ~DataPath() noexcept;

  /**
   * @brief Returns the number of items in the DataPath.
   * @return usize
   */
  usize getLength() const;

  /**
   * @brief Return true if the path is empty. Returns false otherwise.
   * @return bool
   */
  bool empty() const;

  /**
   * @brief Returns the last name on the path. Returns an empty string if the
   * path is empty.
   * @return std::string
   */
  std::string getTargetName() const;

  /**
   * @brief Returns the path as a vector of strings.
   * @return std::vector<std::string>
   */
  std::vector<std::string> getPathVector() const;

  /**
   * @brief Returns a DataPath pointing to the target's parent.
   * @return DataPath
   */
  DataPath getParent() const;

  /**
   * @brief Creates and returns a DataPath pointing to a child with the specified name.
   * @param name
   * @return DataPath
   */
  DataPath createChildPath(std::string name) const;

  /**
   * @brief Creates and returns a DataPath by replacing the specified symbol with
   * the target string value.
   * @param symbol
   * @param targetName
   * @return DataPath
   */
  DataPath replace(std::string_view symbol, std::string_view targetName) const;

  /**
   * @brief Checks equality between two DataPaths.
   * @param rhs
   * @return bool
   */
  bool operator==(const DataPath& rhs) const;

  /**
   * @brief Checks inequality between two DataPaths.
   * @param rhs
   * @return bool
   */
  bool operator!=(const DataPath& rhs) const;

  /**
   * @brief Returns the name at the target index.
   * @param index
   * @return std::string
   */
  const std::string& operator[](usize index) const;

  /**
   * @brief Returns a string representation using a target divider.
   * @param div = "/"
   * @return std::string
   */
  std::string toString(std::string_view div = "/") const;

private:
  std::vector<std::string> m_Path;
};
} // namespace complex
