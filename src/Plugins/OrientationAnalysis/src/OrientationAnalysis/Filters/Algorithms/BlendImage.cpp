#include "BlendImage.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

#include <Eigen/Dense>

#include <cmath>
#include <limits>
#include <utility>
#include <vector>

using namespace nx::core;

namespace
{
/**
 * @class BlendImageImpl
 * @brief Blends an RGB color array by a normalized per-tuple weight.
 *
 * Each tuple's weight is mapped from the global [ciMin, ciMax] range into [0, 1]. In advanced
 * mode the weight is additionally quantized to 256 BYTSCL levels and the final channel value is
 * rounded (not truncated), matching the IDL makeebsdimage_ang numerics. When blackout is enabled,
 * tuples whose criterion value falls below the threshold are forced to black. Workers write
 * disjoint output tuples, so the pass is thread-safe. If @p products is non-null the floating-point
 * products are staged there (for a later global rescale) instead of being written to @p output.
 */
class BlendImageImpl
{
public:
  BlendImageImpl(const UInt8Array* ipfColors, const float32* weight, const Float32Array* criterion, UInt8Array* output, float32* products, bool quantize, bool roundHalf, bool doBlackout,
                     float32 blackoutThreshold, float32 ciMin, float32 ciRange, const std::atomic_bool* shouldCancel)
  : m_IPFColors(ipfColors)
  , m_Weight(weight)
  , m_Criterion(criterion)
  , m_Output(output)
  , m_Products(products)
  , m_Quantize(quantize)
  , m_RoundHalf(roundHalf)
  , m_DoBlackout(doBlackout)
  , m_BlackoutThreshold(blackoutThreshold)
  , m_CIMin(ciMin)
  , m_CIRange(ciRange)
  , m_ShouldCancel(shouldCancel)
  {
  }

  void operator()(const Range& range) const
  {
    for(usize i = range.min(); i < range.max(); ++i)
    {
      if(*m_ShouldCancel)
      {
        return;
      }

      float32 frac = (m_Weight[i] - m_CIMin) / m_CIRange;
      if(frac < 0.0f)
      {
        frac = 0.0f;
      }
      else if(frac > 1.0f)
      {
        frac = 1.0f;
      }

      // Advanced mode reproduces IDL BYTSCL(imageg)/255.: quantize the weight to 256 levels.
      const float32 weight = m_Quantize ? (std::floor(255.9999f * frac) / 255.0f) : frac;

      const bool black = m_DoBlackout && ((*m_Criterion)[i] < m_BlackoutThreshold);

      for(usize c = 0; c < 3; ++c)
      {
        const float32 product = black ? 0.0f : static_cast<float32>((*m_IPFColors)[i * 3 + c]) * weight;
        if(m_Products != nullptr)
        {
          m_Products[i * 3 + c] = product;
        }
        else
        {
          // Advanced mode rounds (IDL BYTE(ROUND(...))); basic mode truncates (static_cast).
          float32 value = m_RoundHalf ? std::floor(product + 0.5f) : std::floor(product);
          if(value > 255.0f)
          {
            value = 255.0f;
          }
          (*m_Output)[i * 3 + c] = static_cast<uint8>(value);
        }
      }
    }
  }

private:
  const UInt8Array* m_IPFColors;
  const float32* m_Weight;
  const Float32Array* m_Criterion;
  UInt8Array* m_Output;
  float32* m_Products;
  bool m_Quantize;
  bool m_RoundHalf;
  bool m_DoBlackout;
  float32 m_BlackoutThreshold;
  float32 m_CIMin;
  float32 m_CIRange;
  const std::atomic_bool* m_ShouldCancel;
};

/**
 * @brief Builds the (px, py) exponent pairs of every monomial whose total degree is <= degree.
 */
std::vector<std::pair<int32, int32>> generateMonomials(int32 degree)
{
  std::vector<std::pair<int32, int32>> terms;
  for(int32 total = 0; total <= degree; ++total)
  {
    for(int32 px = total; px >= 0; --px)
    {
      terms.emplace_back(px, total - px);
    }
  }
  return terms;
}

/**
 * @brief Subtracts a per-slice 2D polynomial background surface from the weight values.
 *
 * Mirrors the IDL /level_gray path: for each Z slice a total-degree-<=@p degree polynomial is
 * least-squares fit to the mean-subtracted weight image and subtracted from it. Grid coordinates
 * are normalized to [0, 1] purely for numerical conditioning; this does not change the fitted
 * surface. The fit uses the normal equations solved with a rank-revealing decomposition, so it is
 * robust to degenerate/underdetermined slices. Note: this is functionally equivalent to IDL SFIT
 * but not bit-identical (different linear-algebra internals).
 */
void applyBackgroundLeveling(std::vector<float32>& weight, const ImageGeom& imageGeom, int32 degree, const std::atomic_bool& shouldCancel)
{
  const SizeVec3 dims = imageGeom.getDimensions();
  const usize nX = dims[0];
  const usize nY = dims[1];
  const usize nZ = dims[2];
  const usize sliceSize = nX * nY;
  if(sliceSize == 0)
  {
    return;
  }

  const std::vector<std::pair<int32, int32>> terms = generateMonomials(degree);
  const usize nTerms = terms.size();

  // Normalize grid coordinates to [0, 1] for conditioning.
  const float64 xDen = (nX > 1) ? static_cast<float64>(nX - 1) : 1.0;
  const float64 yDen = (nY > 1) ? static_cast<float64>(nY - 1) : 1.0;

  std::vector<float64> xPow(static_cast<usize>(degree) + 1);
  std::vector<float64> yPow(static_cast<usize>(degree) + 1);
  std::vector<float64> basis(nTerms);

  for(usize z = 0; z < nZ; ++z)
  {
    if(shouldCancel)
    {
      return;
    }
    const usize base = z * sliceSize;

    // IDL fits the surface to the mean-subtracted slice.
    float64 mean = 0.0;
    for(usize i = 0; i < sliceSize; ++i)
    {
      mean += static_cast<float64>(weight[base + i]);
    }
    mean /= static_cast<float64>(sliceSize);

    // Accumulate the normal equations (A^T A) c = (A^T b), with b = value - mean.
    Eigen::MatrixXd ata = Eigen::MatrixXd::Zero(static_cast<Eigen::Index>(nTerms), static_cast<Eigen::Index>(nTerms));
    Eigen::VectorXd atb = Eigen::VectorXd::Zero(static_cast<Eigen::Index>(nTerms));
    for(usize y = 0; y < nY; ++y)
    {
      if(shouldCancel)
      {
        return;
      }
      const float64 yN = static_cast<float64>(y) / yDen;
      yPow[0] = 1.0;
      for(int32 p = 1; p <= degree; ++p)
      {
        yPow[static_cast<usize>(p)] = yPow[static_cast<usize>(p) - 1] * yN;
      }
      for(usize x = 0; x < nX; ++x)
      {
        const float64 xN = static_cast<float64>(x) / xDen;
        xPow[0] = 1.0;
        for(int32 p = 1; p <= degree; ++p)
        {
          xPow[static_cast<usize>(p)] = xPow[static_cast<usize>(p) - 1] * xN;
        }
        for(usize t = 0; t < nTerms; ++t)
        {
          basis[t] = xPow[static_cast<usize>(terms[t].first)] * yPow[static_cast<usize>(terms[t].second)];
        }
        const float64 bVal = static_cast<float64>(weight[base + y * nX + x]) - mean;
        for(usize r = 0; r < nTerms; ++r)
        {
          atb[static_cast<Eigen::Index>(r)] += basis[r] * bVal;
          for(usize c = 0; c < nTerms; ++c)
          {
            ata(static_cast<Eigen::Index>(r), static_cast<Eigen::Index>(c)) += basis[r] * basis[c];
          }
        }
      }
    }

    const Eigen::VectorXd coeffs = ata.completeOrthogonalDecomposition().solve(atb);

    // Subtract the fitted surface from the weight values.
    for(usize y = 0; y < nY; ++y)
    {
      if(shouldCancel)
      {
        return;
      }
      const float64 yN = static_cast<float64>(y) / yDen;
      yPow[0] = 1.0;
      for(int32 p = 1; p <= degree; ++p)
      {
        yPow[static_cast<usize>(p)] = yPow[static_cast<usize>(p) - 1] * yN;
      }
      for(usize x = 0; x < nX; ++x)
      {
        const float64 xN = static_cast<float64>(x) / xDen;
        xPow[0] = 1.0;
        for(int32 p = 1; p <= degree; ++p)
        {
          xPow[static_cast<usize>(p)] = xPow[static_cast<usize>(p) - 1] * xN;
        }
        float64 surface = 0.0;
        for(usize t = 0; t < nTerms; ++t)
        {
          surface += coeffs[static_cast<Eigen::Index>(t)] * xPow[static_cast<usize>(terms[t].first)] * yPow[static_cast<usize>(terms[t].second)];
        }
        weight[base + y * nX + x] -= static_cast<float32>(surface);
      }
    }
  }
}

/**
 * @brief Contrast-stretches the staged float products in the flat index range [start, end) to
 * [0, 255] (IDL BYTSCL of a single 2D image) and writes the result into the uint8 output array.
 *
 * The range is a half-open span of flat component indices (tuple * 3 + channel) so that one Z
 * slice can be rescaled independently, matching IDL's per-2D-scan BYTSCL.
 */
void finalRescaleRange(const std::vector<float32>& products, UInt8Array& output, usize start, usize end, const std::atomic_bool& shouldCancel)
{
  float32 pMin = std::numeric_limits<float32>::max();
  float32 pMax = std::numeric_limits<float32>::lowest();
  for(usize j = start; j < end; ++j)
  {
    const float32 p = products[j];
    if(p < pMin)
    {
      pMin = p;
    }
    if(p > pMax)
    {
      pMax = p;
    }
  }

  const float32 pRange = pMax - pMin;
  if(pRange <= 0.0f)
  {
    // IDL BYTSCL maps a constant image to all zeros.
    for(usize j = start; j < end; ++j)
    {
      output[j] = 0;
    }
    return;
  }

  for(usize j = start; j < end; ++j)
  {
    if(shouldCancel)
    {
      return;
    }
    float32 value = std::floor(255.9999f * (products[j] - pMin) / pRange);
    if(value > 255.0f)
    {
      value = 255.0f;
    }
    else if(value < 0.0f)
    {
      value = 0.0f;
    }
    output[j] = static_cast<uint8>(value);
  }
}
} // namespace

BlendImage::BlendImage(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel, BlendImageInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_MessageHandler(msgHandler)
, m_ShouldCancel(shouldCancel)
, m_InputValues(inputValues)
{
}

BlendImage::~BlendImage() noexcept = default;

const std::atomic_bool& BlendImage::getCancel()
{
  return m_ShouldCancel;
}

namespace
{
/**
 * @brief Computes the [min, max] of the weight buffer over the flat tuple range [start, end).
 * Returns the normalization (min, range), applying the degenerate all-equal handling that mirrors
 * IDL's "imageg + 255*(mx EQ 0)" (a range of 0 becomes a constant weight of 1.0 everywhere).
 */
std::pair<float32, float32> computeNormalization(const std::vector<float32>& weight, usize start, usize end)
{
  float32 ciMin = std::numeric_limits<float32>::max();
  float32 ciMax = std::numeric_limits<float32>::lowest();
  for(usize i = start; i < end; ++i)
  {
    const float32 w = weight[i];
    if(w < ciMin)
    {
      ciMin = w;
    }
    if(w > ciMax)
    {
      ciMax = w;
    }
  }

  float32 ciRange = ciMax - ciMin;
  if(ciRange == 0.0f)
  {
    ciRange = 1.0f;
    ciMin = ciMax - 1.0f;
  }
  return {ciMin, ciRange};
}
} // namespace

Result<> BlendImage::operator()()
{
  const auto& ipfColors = m_DataStructure.getDataRefAs<UInt8Array>(m_InputValues->cellIPFColorsArrayPath);
  const auto& ciArray = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->ciArrayPath);
  auto& output = m_DataStructure.getDataRefAs<UInt8Array>(m_InputValues->outputArrayPath);

  const usize numTuples = ciArray.getNumberOfTuples();

  const bool advanced = m_InputValues->advancedMode;
  const bool doBlackout = advanced && m_InputValues->useBlackout;
  const bool doLeveling = advanced && m_InputValues->useLeveling;
  const bool doRescale = advanced && m_InputValues->useFinalRescale;

  // Working copy of the modifier/weight values; background leveling mutates this buffer in place.
  std::vector<float32> weight(numTuples);
  for(usize i = 0; i < numTuples; ++i)
  {
    weight[i] = ciArray[i];
  }

  const Float32Array* criterion = doBlackout ? &m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->criterionArrayPath) : nullptr;

  if(!advanced)
  {
    // Basic mode: generic continuous blend normalized globally over all tuples (original behavior).
    m_MessageHandler.sendMessage(IFilter::Message::Type::Info, "Computing normalization range...");
    const auto [ciMin, ciRange] = computeNormalization(weight, 0, numTuples);

    m_MessageHandler.sendMessage(IFilter::Message::Type::Info, "Blending colors with normalized weights...");
    ParallelDataAlgorithm dataAlg;
    dataAlg.setRange(0, numTuples);
    dataAlg.execute(BlendImageImpl(&ipfColors, weight.data(), nullptr, &output, nullptr, false, false, false, 0.0f, ciMin, ciRange, &m_ShouldCancel));
    return {};
  }

  // Advanced mode. Without leveling the whole field is treated as a single image (global
  // normalization and rescale). With leveling, each Z slice of the image geometry is an independent
  // 2D image (matching IDL's per-2D-scan semantics): the surface fit, weight normalization and the
  // final BYTSCL rescale are all computed per slice. A single-slice geometry reduces to a global pass.
  usize sliceSize = numTuples;
  usize nZ = 1;

  if(doLeveling)
  {
    const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->imageGeometryPath);
    const SizeVec3 dims = imageGeom.getDimensions();
    sliceSize = dims[0] * dims[1];
    nZ = dims[2];

    m_MessageHandler.sendMessage(IFilter::Message::Type::Info, "Leveling modifier background (per-slice surface fit)...");
    applyBackgroundLeveling(weight, imageGeom, m_InputValues->levelingDegree, m_ShouldCancel);
    if(m_ShouldCancel)
    {
      return {};
    }
  }

  // When rescaling, the per-slice BYTSCL needs the full floating-point product image first, so stage
  // the products (3 * numTuples float32 = 12 bytes/tuple) and rescale each slice in place below.
  std::vector<float32> products;
  if(doRescale)
  {
    products.resize(numTuples * 3);
  }

  m_MessageHandler.sendMessage(IFilter::Message::Type::Info, "Blending colors with normalized weights...");
  for(usize z = 0; z < nZ; ++z)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    const usize base = z * sliceSize;
    const auto [ciMin, ciRange] = computeNormalization(weight, base, base + sliceSize);

    ParallelDataAlgorithm dataAlg;
    dataAlg.setRange(base, base + sliceSize);
    if(doRescale)
    {
      dataAlg.execute(BlendImageImpl(&ipfColors, weight.data(), criterion, nullptr, products.data(), true, true, doBlackout, m_InputValues->blackoutThreshold, ciMin, ciRange, &m_ShouldCancel));
      finalRescaleRange(products, output, base * 3, (base + sliceSize) * 3, m_ShouldCancel);
    }
    else
    {
      dataAlg.execute(BlendImageImpl(&ipfColors, weight.data(), criterion, &output, nullptr, true, true, doBlackout, m_InputValues->blackoutThreshold, ciMin, ciRange, &m_ShouldCancel));
    }
  }

  return {};
}
