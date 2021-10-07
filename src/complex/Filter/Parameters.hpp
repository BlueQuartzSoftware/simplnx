#pragma once

#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <variant>

#include "complex/Common/Types.hpp"
#include "complex/Filter/AnyParameter.hpp"
#include "complex/Filter/IParameter.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
/**
 * @brief Parameters stores a map of strings to IParameter. Preserves insertion order.
 */
class COMPLEX_EXPORT Parameters
{
public:
  struct COMPLEX_EXPORT Separator
  {
    std::string name;
  };

  struct COMPLEX_EXPORT ParameterKey
  {
    std::string key;
  };

  using LayoutObject = std::variant<ParameterKey, Separator>;
  using LayoutVector = std::vector<LayoutObject>;

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
   * @param name
   */
  void insert(Separator name);

  /**
   * @brief Returns the parameter with the given key. Throws if not found.
   * @param key
   * @return
   */
  IParameter* at(std::string_view key);

  /**
   * @brief Returns the parameter with the given key. Throws if not found.
   * @param key
   * @return
   */
  const IParameter* at(std::string_view key) const;

  /**
   * @brief Returns a list of the keys (in insertion order) that represent the accepted keys for this Parameters object
   * @return A vector of std::string objects
   */
  std::vector<std::string> getKeys() const;

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
};
} // namespace complex
