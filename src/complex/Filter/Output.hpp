#pragma once

#include <memory>
#include <vector>

#include "complex/Common/Result.hpp"
#include "complex/Common/Types.hpp"
#include "complex/DataStructure/DataStructure.hpp"

namespace complex
{
class IDataAction
{
public:
  enum class Mode : u8
  {
    Preflight = 0,
    Execute
  };

  virtual ~IDataAction() noexcept = default;

  IDataAction(const IDataAction&) = delete;
  IDataAction(IDataAction&&) noexcept = delete;
  IDataAction& operator=(const IDataAction&) = delete;
  IDataAction& operator=(IDataAction&&) noexcept = delete;

  /**
   * @brief
   * @param dataStructure
   * @return
   */
  virtual Result<> apply(DataStructure& dataStructure, Mode mode) const = 0;

protected:
  IDataAction() = default;
};

class CreateArrayAction : public IDataAction
{
public:
  CreateArrayAction() = delete;

  CreateArrayAction(NumericType type, const std::vector<usize>& dims, const DataPath& path);

  ~CreateArrayAction() noexcept override;

  CreateArrayAction(const CreateArrayAction&) = delete;
  CreateArrayAction(CreateArrayAction&&) noexcept = delete;
  CreateArrayAction& operator=(const CreateArrayAction&) = delete;
  CreateArrayAction& operator=(CreateArrayAction&&) noexcept = delete;

  /**
   * @brief
   * @param dataStructure
   * @return
   */
  Result<> apply(DataStructure& dataStructure, Mode mode) const override;

  /**
   * @brief
   * @return
   */
  [[nodiscard]] NumericType type() const;

  /**
   * @brief
   * @return
   */
  [[nodiscard]] std::vector<usize> dims() const;

  /**
   * @brief
   * @return
   */
  [[nodiscard]] DataPath path() const;

private:
  NumericType m_Type;
  std::vector<usize> m_Dims;
  DataPath m_Path;
};

struct OutputActions
{
  std::vector<std::unique_ptr<IDataAction>> actions;
};
} // namespace complex
