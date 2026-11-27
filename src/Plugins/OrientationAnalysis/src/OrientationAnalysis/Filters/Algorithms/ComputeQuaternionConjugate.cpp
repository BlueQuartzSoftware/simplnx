#include "ComputeQuaternionConjugate.hpp"

#include "ComputeQuaternionConjugateDirect.hpp"
#include "ComputeQuaternionConjugateScanline.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"

using namespace nx::core;

ComputeQuaternionConjugate::ComputeQuaternionConjugate(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                       ComputeQuaternionConjugateInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

const std::atomic_bool& ComputeQuaternionConjugate::getCancel()
{
  return m_ShouldCancel;
}

Result<> ComputeQuaternionConjugate::operator()()
{
  const auto& input = m_DataStructure.getDataRefAs<Float32ArrayType>(m_InputValues->quaternion_data_array_path);
  const auto& output = m_DataStructure.getDataRefAs<Float32ArrayType>(m_InputValues->output_data_array_path);

  return DispatchAlgorithm<ComputeQuaternionConjugateDirect, ComputeQuaternionConjugateScanline>({&input, &output}, m_DataStructure, m_MessageHandler, m_ShouldCancel, m_InputValues);
}
