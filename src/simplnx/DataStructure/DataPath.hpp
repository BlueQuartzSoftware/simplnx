#pragma once

#include "simplnx/Common/Types.hpp"
#include "simplnx/simplnx_export.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace nx::core
{
/**
 * @class DataPath
 * @brief The DataPath class is designed to store a path through the
 * DataStructure to reach a specific DataObject. There may be multiple paths
 * to a DataObject, but by providing a path, it is possible to extrapolate
 * which set of siblings a user may be interested in or iterate over a data
 * group with common children names.
 */
class SIMPLNX_EXPORT DataPath
{
public:
  /**
   * @brief Attempts to create a DataPath from the given string by splitting it using the given delimiter.
   * @param string The string to parse into a DataPath
   * @param delimiter The character used to separate path components (default is '/')
   * @return Optional DataPath if parsing succeeds, std::nullopt otherwise
   */
  static std::optional<DataPath> FromString(std::string_view string, char delimiter = '/');

  /**
   * @brief Default constructor.
   */
  DataPath();

  /**
   * @brief Creates a DataPath using a vector of DataObject names.
   * @param path Vector of strings representing the path components
   */
  DataPath(std::vector<std::string> path);

  /**
   * @brief Copy constructor.
   * @param rhs The DataPath to copy from
   */
  DataPath(const DataPath& rhs);

  /**
   * @brief Move constructor.
   * @param rhs The DataPath to move from
   */
  DataPath(DataPath&& rhs) noexcept;

  /**
   * @brief Copy assignment operator.
   * @param rhs The DataPath to copy from
   * @return Reference to this DataPath after assignment
   */
  DataPath& operator=(const DataPath& rhs);

  /**
   * @brief Move assignment operator.
   * @param rhs The DataPath to move from
   * @return Reference to this DataPath after assignment
   */
  DataPath& operator=(DataPath&& rhs) noexcept;

  /**
   * @brief Destructor.
   */
  ~DataPath() noexcept;

  /**
   * @brief Returns the number of items in the DataPath.
   * @return Number of path components
   */
  usize getLength() const;

  /**
   * @brief Return true if the path is empty. Returns false otherwise.
   * @return True if the path contains no components, false otherwise
   */
  bool empty() const;

  /**
   * @brief Returns the last name on the path. Returns an empty string if the
   * path is empty.
   * @return String containing the last path component, or empty string if path is empty
   */
  std::string getTargetName() const;

  /**
   * @Brief Returns a newly constructed DataPath where the only change is the replacement of the last
   * element in the DataPath to the 'newName' value.
   *
   * @code
   *
   * DataPath foo({"A", "B", "C"});  // foo is now "A/B/C"
   * DataPath bar = foo.replaceName("D"); // bar is now "A/B/D"
   * @endcode
   *
   * @param newName The replacement value for the last element in the DataPath
   * @return DataPath
   */
  [[nodiscard]] DataPath replaceName(const std::string& newName) const;

  /**
   * @brief Returns the path as a vector of strings.
   * @return Vector containing all path components
   */
  std::vector<std::string> getPathVector() const;

  /**
   * @brief Returns a DataPath pointing to the target's parent.
   * @return DataPath with the last component removed
   */
  DataPath getParent() const;

  /**
   * @brief Creates and returns a DataPath pointing to a child with the specified name.
   * @param name The name of the child to append to the path
   * @return New DataPath with the child name appended
   */
  DataPath createChildPath(std::string name) const;

  /**
   * @brief Creates and returns a DataPath by replacing the specified symbol with
   * the target string value.
   * @param symbol The string symbol to search for and replace
   * @param targetName The replacement string value
   * @return New DataPath with the symbol replaced
   */
  DataPath replace(std::string_view symbol, std::string_view targetName) const;

  /**
   * @brief Attempts to handle renaming the DataPath. If the old path is a
   * subset or exact match of the current path, the matching section will be
   * replaced with the new path.
   * @param oldPath The old path to search for
   * @param newPath The new path to replace with
   * @return True if the rename was applied, false otherwise
   */
  bool attemptRename(const DataPath& oldPath, const DataPath& newPath);

  /**
   * @brief Checks equality between two DataPaths.
   * @param rhs The DataPath to compare against
   * @return True if both DataPaths are equal, false otherwise
   */
  bool operator==(const DataPath& rhs) const;

  /**
   * @brief Checks inequality between two DataPaths.
   * @param rhs The DataPath to compare against
   * @return True if DataPaths are not equal, false otherwise
   */
  bool operator!=(const DataPath& rhs) const;

  /**
   * @brief Checks if the string representation is less than the target DataPath's.
   * @param rhs The DataPath to compare against
   * @return True if this DataPath is lexicographically less than rhs, false otherwise
   */
  bool operator<(const DataPath& rhs) const;

  /**
   * @brief Checks if the string representation is greater than the target DataPath's.
   * @param rhs The DataPath to compare against
   * @return True if this DataPath is lexicographically greater than rhs, false otherwise
   */
  bool operator>(const DataPath& rhs) const;

  /**
   * @brief Returns the name at the target index.
   * @param index The index of the path component to retrieve
   * @return Const reference to the path component string at the specified index
   */
  const std::string& operator[](usize index) const;

  /**
   * @brief Returns a string representation using a target divider.
   * @param div The delimiter to use between path components (default is "/")
   * @return String representation of the DataPath
   */
  std::string toString(std::string_view div = "/") const;

private:
  std::vector<std::string> m_Path;
};
} // namespace nx::core
