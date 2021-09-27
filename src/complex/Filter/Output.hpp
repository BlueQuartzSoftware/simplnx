#pragma once

#include <memory>
#include <vector>

#include "complex/Common/Result.hpp"
#include "complex/Common/Types.hpp"
#include "complex/DataStructure/DataStructure.hpp"

namespace complex
{
/**
 * @brief IDataAction is the interface for declaratively stating what changes to be made to a DataStructure.
 * Can then be applied in preflight or execute mode to actually make those changes.
 */
class IDataAction
{
public:
  using UniquePointer = std::unique_ptr<IDataAction>;

  enum class Mode : uint8
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
   * @brief Applies this action's change to the given DataStructure in the given mode.
   * Returns any warnings/errors. On error, DataStructure is not guaranteed to be consistent.
   * @param dataStructure
   * @return
   */
  virtual Result<> apply(DataStructure& dataStructure, Mode mode) const = 0;

protected:
  IDataAction() = default;
};

/**
 * @brief Action for creating DataArrays in a DataStructure
 */
class CreateArrayAction : public IDataAction
{
public:
  CreateArrayAction() = delete;

  CreateArrayAction(NumericType type, const std::vector<usize>& dims, uint64 nComp, const DataPath& path);

  ~CreateArrayAction() noexcept override;

  CreateArrayAction(const CreateArrayAction&) = delete;
  CreateArrayAction(CreateArrayAction&&) noexcept = delete;
  CreateArrayAction& operator=(const CreateArrayAction&) = delete;
  CreateArrayAction& operator=(CreateArrayAction&&) noexcept = delete;

  /**
   * @brief Applies this action's change to the given DataStructure in the given mode.
   * Returns any warnings/errors. On error, DataStructure is not guaranteed to be consistent.
   * @param dataStructure
   * @return
   */
  Result<> apply(DataStructure& dataStructure, Mode mode) const override;

  /**
   * @brief Returns the NumericType of the DataArray to be created.
   * @return
   */
  [[nodiscard]] NumericType type() const;

  /**
   * @brief Returns the dimensions of the DataArray to be created.
   * @return
   */
  [[nodiscard]] std::vector<usize> dims() const;

  /**
   * @brief Returns the number of components of the DataArray to be created.
   * @return
   */
  [[nodiscard]] uint64 numComponents() const;

  /**
   * @brief Returns the path of the DataArray to be created.
   * @return
   */
  [[nodiscard]] DataPath path() const;

private:
  NumericType m_Type;
  std::vector<usize> m_Dims;
  uint64 m_NComp;
  DataPath m_Path;
};

/**
 * @brief Container for IDataActions
 */
struct OutputActions
{
  std::vector<IDataAction::UniquePointer> actions;
};
} // namespace complex
