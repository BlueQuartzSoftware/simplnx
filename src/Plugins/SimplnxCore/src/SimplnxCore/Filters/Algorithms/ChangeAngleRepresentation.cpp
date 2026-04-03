#include "ChangeAngleRepresentation.hpp"

#include "simplnx/Common/Numbers.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

using namespace nx::core;

namespace
{
namespace EulerAngleConversionType
{
constexpr uint64 DegreesToRadians = 0;
constexpr uint64 RadiansToDegrees = 1;
} // namespace EulerAngleConversionType

class ChangeAngleRepresentationImpl
{
public:
  ChangeAngleRepresentationImpl(Float32AbstractDataStore& angles, float factor)
  : m_Angles(angles)
  , m_ConvFactor(factor)
  {
  }
  ~ChangeAngleRepresentationImpl() noexcept = default;

  void convert(size_t start, size_t end) const
  {
    for(size_t i = start; i < end; i++)
    {
      m_Angles[i] = m_Angles[i] * m_ConvFactor;
    }
  }

  void operator()(const Range& range) const
  {
    convert(range.min(), range.max());
  }

private:
  Float32AbstractDataStore& m_Angles;
  float32 m_ConvFactor = 0.0F;
};
} // namespace

// -----------------------------------------------------------------------------
ChangeAngleRepresentation::ChangeAngleRepresentation(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                     ChangeAngleRepresentationInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ChangeAngleRepresentation::~ChangeAngleRepresentation() noexcept = default;

// -----------------------------------------------------------------------------
Result<> ChangeAngleRepresentation::operator()()
{
  MessageHelper messageHelper(m_MessageHandler);
  messageHelper.sendMessage("ChangeAngleRepresentation: Converting angle representation...");

  auto& angles = m_DataStructure.getDataAs<Float32Array>(m_InputValues->AnglesArrayPath)->getDataStoreRef();

  float conversionFactor = 1.0f;
  if(m_InputValues->ConversionTypeIndex == EulerAngleConversionType::DegreesToRadians)
  {
    conversionFactor = static_cast<float>(nx::core::numbers::pi / 180.0f);
  }
  else if(m_InputValues->ConversionTypeIndex == EulerAngleConversionType::RadiansToDegrees)
  {
    conversionFactor = static_cast<float>(180.0f / nx::core::numbers::pi);
  }

  // Parallel algorithm to find duplicate nodes
  ParallelDataAlgorithm dataAlg;
  dataAlg.setRange(0ULL, angles.getSize());
  dataAlg.execute(::ChangeAngleRepresentationImpl(angles, conversionFactor));

  return {};
}
