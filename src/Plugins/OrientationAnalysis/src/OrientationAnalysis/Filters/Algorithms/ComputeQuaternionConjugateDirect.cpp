#include "ComputeQuaternionConjugateDirect.hpp"

#include "ComputeQuaternionConjugate.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

using namespace nx::core;

namespace
{
/**
 * @class ComputeQuaternionConjugateImpl
 * @brief Conjugates direct quaternion tuple ranges.
 *
 * ParallelDataAlgorithm can invoke copied workers for separate ranges. The
 * direct loop reads and writes Float32Array elements. This worker does not
 * establish generic DataArray or DataStore thread safety.
 */
class ComputeQuaternionConjugateImpl
{
private:
  const Float32Array* m_Input;
  Float32Array* m_Output;
  const std::atomic_bool* m_ShouldCancel;

public:
  /**
   * @brief Initializes a direct quaternion range worker.
   * @param inputQuat Provides input quaternion tuples.
   * @param outputQuat Receives conjugated quaternion tuples.
   * @param shouldCancel Signals cancellation.
   * @pre The arrays contain matching four-component tuples.
   * @pre Each argument remains valid while the parallel algorithm executes.
   */
  ComputeQuaternionConjugateImpl(const Float32Array* inputQuat, Float32Array* outputQuat, const std::atomic_bool* shouldCancel)
  : m_Input(inputQuat)
  , m_Output(outputQuat)
  , m_ShouldCancel(shouldCancel)
  {
  }

  /**
   * @brief Conjugates a half-open tuple range.
   * @param start Identifies the first tuple.
   * @param end Identifies the tuple after the last tuple.
   *
   * Cancellation stops this range before its next tuple write.
   */
  void convert(size_t start, size_t end) const
  {
    for(size_t i = start; i < end; i++)
    {
      if(*m_ShouldCancel)
      {
        return;
      }
      (*m_Output)[i * 4] = -1.0f * (*m_Input)[i * 4];
      (*m_Output)[i * 4 + 1] = -1.0f * (*m_Input)[i * 4 + 1];
      (*m_Output)[i * 4 + 2] = -1.0f * (*m_Input)[i * 4 + 2];
      (*m_Output)[i * 4 + 3] = (*m_Input)[i * 4 + 3];
    }
  }

  /**
   * @brief Conjugates a parallel tuple range.
   * @param range Identifies the half-open tuple range.
   */
  void operator()(const Range& range) const
  {
    convert(range.min(), range.max());
  }
};
} // namespace

ComputeQuaternionConjugateDirect::ComputeQuaternionConjugateDirect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                                   const ComputeQuaternionConjugateInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

ComputeQuaternionConjugateDirect::~ComputeQuaternionConjugateDirect() noexcept = default;

const std::atomic_bool& ComputeQuaternionConjugateDirect::getCancel()
{
  return m_ShouldCancel;
}

Result<> ComputeQuaternionConjugateDirect::operator()()
{
  const auto& input = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->QuaternionDataArrayPath);
  auto& output = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->OutputDataArrayPath);

  ParallelDataAlgorithm dataAlg;
  dataAlg.setRange(0, input.getNumberOfTuples());
  dataAlg.execute(ComputeQuaternionConjugateImpl(&input, &output, &m_ShouldCancel));

  return {};
}
