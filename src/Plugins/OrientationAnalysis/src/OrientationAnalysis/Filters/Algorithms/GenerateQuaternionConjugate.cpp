#include "GenerateQuaternionConjugate.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

using namespace nx::core;

namespace
{
class GenerateQuaternionConjugateImpl
{
private:
  const Float32Array* m_Input;
  Float32Array* m_Output;
  const std::atomic_bool* m_ShouldCancel;

public:
  GenerateQuaternionConjugateImpl(const Float32Array* inputQuat, Float32Array* outputQuat, const std::atomic_bool* shouldCancel)
  : m_Input(inputQuat)
  , m_Output(outputQuat)
  , m_ShouldCancel(shouldCancel)
  {
  }
  GenerateQuaternionConjugateImpl(const GenerateQuaternionConjugateImpl&) = default;           // Copy Constructor
  GenerateQuaternionConjugateImpl(GenerateQuaternionConjugateImpl&&) = delete;                 // Move Constructor Not Implemented
  GenerateQuaternionConjugateImpl& operator=(const GenerateQuaternionConjugateImpl&) = delete; // Copy Assignment Not Implemented
  GenerateQuaternionConjugateImpl& operator=(GenerateQuaternionConjugateImpl&&) = delete;      // Move Assignment Not Implemented

  virtual ~GenerateQuaternionConjugateImpl() = default;

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

  void operator()(const Range& range) const
  {
    convert(range.min(), range.max());
  }
};
} // namespace

// -----------------------------------------------------------------------------
GenerateQuaternionConjugate::GenerateQuaternionConjugate(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                         GenerateQuaternionConjugateInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
GenerateQuaternionConjugate::~GenerateQuaternionConjugate() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& GenerateQuaternionConjugate::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> GenerateQuaternionConjugate::operator()()
{
  const auto& input = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->QuaternionDataArrayPath);
  auto& output = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->OutputDataArrayPath);

  ParallelDataAlgorithm dataAlg;
  dataAlg.setRange(0, input.getNumberOfTuples());
  dataAlg.execute(GenerateQuaternionConjugateImpl(&input, &output, &m_ShouldCancel));

  return {};
}
