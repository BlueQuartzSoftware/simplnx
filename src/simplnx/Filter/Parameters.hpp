#pragma once

#include "simplnx/Common/Types.hpp"
#include "simplnx/Filter/IParameter.hpp"
#include "simplnx/simplnx_export.hpp"

#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <variant>

namespace nx::core
{
namespace detail
{
using IsActiveFunc = bool (*)(const IParameter&, const std::any&, const std::any&);

template <class ParamT>
IsActiveFunc CreateIsActiveFunc()
{
  return [](const IParameter& param, const std::any& value, const std::any& associatedValue) { return dynamic_cast<const ParamT&>(param).checkActive(value, associatedValue); };
}
} // namespace detail

/**
 * @brief Parameters stores a map of strings to IParameter. Preserves insertion order.
 */
class SIMPLNX_EXPORT Parameters
{
public:
  struct SIMPLNX_EXPORT Separator
  {
    std::string name;
  };

  struct SIMPLNX_EXPORT ParameterKey
  {
    std::string key;
  };

  using LayoutObject = std::variant<ParameterKey, Separator>;
  using LayoutVector = std::vector<LayoutObject>;
  using GroupList = std::vector<std::pair<std::string, std::any>>;

  using IsActiveFunc = detail::IsActiveFunc;

  Parameters() = default;
  ~Parameters() noexcept = default;

  Parameters(const Parameters& rhs) = default;
  Parameters(Parameters&&) noexcept = default;

  Parameters& operator=(const Parameters& rhs) = default;
  Parameters& operator=(Parameters&&) noexcept = default;

  /**
   * @brief Returns true if contains a parameter with the given name.
   * @param name
   * @return
   */
  bool contains(std::string_view name) const;

  /**
   * @brief Returns the size of the collection.
   * @return
   */
  usize size() const;

  /**
   * @brief Returns true if empty, otherwise false.
   * @return
   */
  bool empty() const;

  /**
   * @brief Inserts the given parameter.
   * @param parameter
   */
  void insert(IParameter::UniquePointer parameter);

  /**
   * @brief Inserts a separator.
   * @param separator
   */
  void insert(Separator separator);

  /**
   * @brief Inserts a separator.
   * @param separator
   */
  void insertSeparator(Separator separator);

  /**
   * @brief Returns the parameter with the given key. Throws if not found.
   * @param key
   * @return
   */
  AnyParameter& at(std::string_view key);

  /**
   * @brief Returns the parameter with the given key. Throws if not found.
   * @param key
   * @return
   */
  const AnyParameter& at(std::string_view key) const;

  /**
   * @brief Inserts the given parameter and makes it available as a group for other parameters.
   * Requires the parameter to implement a member function bool checkActive(const std::any&, const std::any&) const.
   * @tparam ParameterT
   * @tparam Enable if ParameterT is derived from IParameter
   * @param parameter
   */
  template <class ParameterT, class = std::enable_if_t<std::is_base_of_v<IParameter, ParameterT>>>
  void insertLinkableParameter(std::unique_ptr<ParameterT> parameter)
  {
    insertLinkableParameter(std::move(parameter), detail::CreateIsActiveFunc<ParameterT>());
  }

  /**
   * @brief Inserts the given parameter and makes it available as a group for other parameters.
   * The function determines whether a group is active.
   * @param parameter
   * @param func
   */
  void insertLinkableParameter(IParameter::UniquePointer parameter, IsActiveFunc func);

  /**
   * @brief Adds child parameter to an existing group with the value that indicates it's active.
   * @param groupKey
   * @param childKey
   * @param associatedValue
   */
  void linkParameters(std::string groupKey, std::string_view childKey, std::any associatedValue);

  /**
   * @brief Returns true if the parameter with the given is active for the given value.
   * @param key
   * @param args
   * @return
   */
  bool isParameterActive(std::string_view key, const Arguments& args) const;

  /**
   * @brief Returns the number of groups that the given parameter belongs to.
   * @param key
   * @return
   */
  usize getNumberOfLinkedGroups(std::string_view key) const;

  /**
   * @brief Returns the list of groups that the parameter belongs to.
   * @param key
   * @return
   */
  const GroupList& getLinkedGroups(std::string_view key) const;

  /**
   * @brief Returns true if a group with the given key exists.
   * @param key
   * @return
   */
  bool containsGroup(std::string_view key) const;

  /**
   * @brief Returns the list of keys in the given group.
   * @param groupKey
   * @return
   */
  std::vector<std::string> getKeysInGroup(std::string_view groupKey) const;

  /**
   * @brief Returns a list of the keys (in insertion order) that represent the accepted keys for this Parameters object
   * @return A vector of std::string objects
   */
  std::vector<std::string> getKeys() const;

  /**
   * @brief Returns a list of the keys for parameters that are a group (i.e. control the active state of other parameters)
   * @return
   */
  std::vector<std::string> getGroupKeys() const;

  /**
   * @brief Returns the layout of stored parameters (e.g. for display in a GUI). Each element can be a parameter or a separator.
   * @return
   */
  const LayoutVector& getLayout() const;

  auto begin()
  {
    return m_Params.begin();
  }

  auto begin() const
  {
    return m_Params.begin();
  }

  auto end()
  {
    return m_Params.end();
  }

  auto end() const
  {
    return m_Params.end();
  }

private:
  std::map<std::string, AnyParameter, std::less<>> m_Params;
  std::vector<LayoutObject> m_LayoutVector;
  std::map<std::string, GroupList, std::less<>> m_ParamGroups;
  std::map<std::string, IsActiveFunc, std::less<>> m_Groups;
};
} // namespace nx::core
