#include "RodriguesConvertor.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

using namespace nx::core;

namespace
{
class RodriguesConvertorImpl
{
private:
  const Float32Array* m_Input;
  Float32Array* m_Output;
  const std::atomic_bool* m_ShouldCancel;

public:
  RodriguesConvertorImpl(const Float32Array* inputQuat, Float32Array* outputQuat, const std::atomic_bool* shouldCancel)
  : m_Input(inputQuat)
  , m_Output(outputQuat)
  , m_ShouldCancel(shouldCancel)
  {
  }

  void convert(size_t start, size_t end) const
  {
    for(size_t i = start; i < end; i++)
    {
      if(*m_ShouldCancel)
      {
        return;
      }
      const float r0 = (*m_Input)[i * 3];
      const float r1 = (*m_Input)[i * 3 + 1];
      const float r2 = (*m_Input)[i * 3 + 2];
      const float length = sqrtf(r0 * r0 + r1 * r1 + r2 * r2);

      (*m_Output)[i * 4] = r0 / length;
      (*m_Output)[i * 4 + 1] = r1 / length;
      (*m_Output)[i * 4 + 2] = r2 / length;
      (*m_Output)[i * 4 + 3] = length;
    }
  }

  void operator()(const Range& range) const
  {
    convert(range.min(), range.max());
  }
};

} // namespace

// -----------------------------------------------------------------------------
RodriguesConvertor::RodriguesConvertor(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, RodriguesConvertorInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
RodriguesConvertor::~RodriguesConvertor() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& RodriguesConvertor::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> RodriguesConvertor::operator()()
{
  const auto& input = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->RodriguesDataArrayPath);
  auto& output = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->OutputDataArrayPath);

  ParallelDataAlgorithm dataAlg;
  dataAlg.setRange(0, input.getNumberOfTuples());
  dataAlg.execute(RodriguesConvertorImpl(&input, &output, &m_ShouldCancel));

  return {};
}
