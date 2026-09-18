#pragma once

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/ImageProcessing/GaussianTemporaryStore.hpp"
#include "simplnx/Utilities/ImageProcessing/PointwiseEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/WorkingMemory.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <fmt/core.h>

#include <nonstd/span.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <limits>
#include <optional>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace nx::core::ImageProcessing
{
struct RecursiveGaussianPassOptions
{
  static constexpr usize k_FallbackStagingBytes = 64ULL * 1024ULL * 1024ULL;
  static constexpr uint32 k_AdaptiveStagingNumerator = 1;
  static constexpr uint32 k_AdaptiveStagingDenominator = 8;
  static constexpr usize k_Default2DResidentLimit = 64ULL * 1024ULL * 1024ULL;
  usize stagingByteBudget = k_FallbackStagingBytes;
  bool useInMemoryFastPath = true;
};

struct RecursiveGaussianPlaneAxis
{
  uint32 axis = 0;
  double sigma = 1.0;
  double spacing = 1.0;
  int order = 0;
  bool normalizeAcrossScale = false;
};

namespace detail
{
template <class T, class StoreT>
const DataStore<T>* GetRecursiveGaussianDataStore(const StoreT&)
{
  return nullptr;
}

template <class T>
const DataStore<T>* GetRecursiveGaussianDataStore(const AbstractDataStore<T>& store)
{
  return dynamic_cast<const DataStore<T>*>(&store);
}

template <class T, class StoreT>
DataStore<T>* GetRecursiveGaussianDataStore(StoreT&)
{
  return nullptr;
}

template <class T>
DataStore<T>* GetRecursiveGaussianDataStore(AbstractDataStore<T>& store)
{
  return dynamic_cast<DataStore<T>*>(&store);
}

struct RecursiveGaussianStoragePlan
{
  bool useTemporaryRecordStore = false;
  std::string workingDataFormat;
};

struct RecursiveGaussian2DBufferPlan
{
  usize blockRows = 0;
  usize coreCols = 0;
  usize blockCount = 0;
  usize residentBytes = 0;
  bool valid = false;
  bool overflow = false;
};

struct RecursiveGaussian2DCheckpoint
{
  double input1 = 0.0;
  double input2 = 0.0;
  double input3 = 0.0;
  double output1 = 0.0;
  double output2 = 0.0;
  double output3 = 0.0;
  double output4 = 0.0;
};

/** @brief Selects raw fixed-record scratch for any actual OOC endpoint and preserves input-format precedence for the resident fallback. */
inline RecursiveGaussianStoragePlan SelectRecursiveGaussianStoragePlan(IDataStore::StoreType inputStoreType, std::string inputDataFormat, IDataStore::StoreType outputStoreType,
                                                                       std::string outputDataFormat)
{
  if(inputStoreType == IDataStore::StoreType::OutOfCore)
  {
    return {true, std::move(inputDataFormat)};
  }
  if(outputStoreType == IDataStore::StoreType::OutOfCore)
  {
    return {true, std::move(outputDataFormat)};
  }
  return {false, std::move(inputDataFormat)};
}

/**
 * @brief Selects the data format for a full-volume recursive-Gaussian working store.
 *
 * OOC input takes precedence, then OOC output; resident endpoints preserve the input format.
 */
inline std::string SelectRecursiveGaussianWorkingDataFormat(IDataStore::StoreType inputStoreType, std::string inputDataFormat, IDataStore::StoreType outputStoreType, std::string outputDataFormat)
{
  return SelectRecursiveGaussianStoragePlan(inputStoreType, std::move(inputDataFormat), outputStoreType, std::move(outputDataFormat)).workingDataFormat;
}

template <class SrcT>
usize ComputeRecursiveGaussianZBatchRowCount(const SizeVec3& dims, usize stagingByteBudget)
{
  if(dims[0] == 0 || dims[1] == 0 || dims[2] == 0)
  {
    return 1;
  }
  const usize maxStagedValues = std::max<usize>(1, stagingByteBudget / (sizeof(SrcT) + sizeof(float32)));
  const usize maximumRows = std::max<usize>(1, std::min<usize>(dims[1], maxStagedValues / dims[0] / dims[2]));
  const usize batchCount = 1 + (dims[1] - 1) / maximumRows;
  return 1 + (dims[1] - 1) / batchCount;
}

// Deriche 4th-order recursive-Gaussian coefficients, held in double (ITK's ScalarRealType == double for every scalar
// input type). Faithful port of itkRecursiveGaussianImageFilter.hxx (v5.4.4).
struct RecursiveGaussianCoefficients
{
  double n0{}, n1{}, n2{}, n3{};
  double d1{}, d2{}, d3{}, d4{};
  double m1{}, m2{}, m3{}, m4{};
  double bn1{}, bn2{}, bn3{}, bn4{};
  double bm1{}, bm2{}, bm3{}, bm4{};
};

// Port of RecursiveGaussianImageFilter::ComputeNCoefficients.
inline void ComputeNCoefficients(double sigmad, double A1, double B1, double W1, double L1, double A2, double B2, double W2, double L2, double& N0, double& N1, double& N2, double& N3, double& SN,
                                 double& DN, double& EN)
{
  const double Sin1 = std::sin(W1 / sigmad);
  const double Sin2 = std::sin(W2 / sigmad);
  const double Cos1 = std::cos(W1 / sigmad);
  const double Cos2 = std::cos(W2 / sigmad);
  const double Exp1 = std::exp(L1 / sigmad);
  const double Exp2 = std::exp(L2 / sigmad);

  N0 = A1 + A2;
  N1 = Exp2 * (B2 * Sin2 - (A2 + 2 * A1) * Cos2);
  N1 += Exp1 * (B1 * Sin1 - (A1 + 2 * A2) * Cos1);
  N2 = (A1 + A2) * Cos2 * Cos1;
  N2 -= B1 * Cos2 * Sin1 + B2 * Cos1 * Sin2;
  N2 *= 2 * Exp1 * Exp2;
  N2 += A2 * Exp1 * Exp1 + A1 * Exp2 * Exp2;
  N3 = Exp2 * Exp1 * Exp1 * (B2 * Sin2 - A2 * Cos2);
  N3 += Exp1 * Exp2 * Exp2 * (B1 * Sin1 - A1 * Cos1);

  SN = N0 + N1 + N2 + N3;
  DN = N1 + 2 * N2 + 3 * N3;
  EN = N1 + 4 * N2 + 9 * N3;
}

// Port of RecursiveGaussianImageFilter::ComputeDCoefficients (writes d1..d4 into c).
inline void ComputeDCoefficients(double sigmad, double W1, double L1, double W2, double L2, RecursiveGaussianCoefficients& c, double& SD, double& DD, double& ED)
{
  const double Cos1 = std::cos(W1 / sigmad);
  const double Cos2 = std::cos(W2 / sigmad);
  const double Exp1 = std::exp(L1 / sigmad);
  const double Exp2 = std::exp(L2 / sigmad);

  c.d4 = Exp1 * Exp1 * Exp2 * Exp2;
  c.d3 = -2 * Cos1 * Exp1 * Exp2 * Exp2;
  c.d3 += -2 * Cos2 * Exp2 * Exp1 * Exp1;
  c.d2 = 4 * Cos2 * Cos1 * Exp1 * Exp2;
  c.d2 += Exp1 * Exp1 + Exp2 * Exp2;
  c.d1 = -2 * (Exp2 * Cos2 + Exp1 * Cos1);

  SD = 1.0 + c.d1 + c.d2 + c.d3 + c.d4;
  DD = c.d1 + 2 * c.d2 + 3 * c.d3 + 4 * c.d4;
  ED = c.d1 + 4 * c.d2 + 9 * c.d3 + 16 * c.d4;
}

// Port of RecursiveGaussianImageFilter::ComputeRemainingCoefficients (m* + boundary bn*/bm*).
inline void ComputeRemainingCoefficients(bool symmetric, RecursiveGaussianCoefficients& c)
{
  if(symmetric)
  {
    c.m1 = c.n1 - c.d1 * c.n0;
    c.m2 = c.n2 - c.d2 * c.n0;
    c.m3 = c.n3 - c.d3 * c.n0;
    c.m4 = -c.d4 * c.n0;
  }
  else
  {
    c.m1 = -(c.n1 - c.d1 * c.n0);
    c.m2 = -(c.n2 - c.d2 * c.n0);
    c.m3 = -(c.n3 - c.d3 * c.n0);
    c.m4 = c.d4 * c.n0;
  }

  const double SN = c.n0 + c.n1 + c.n2 + c.n3;
  const double SM = c.m1 + c.m2 + c.m3 + c.m4;
  const double SD = 1.0 + c.d1 + c.d2 + c.d3 + c.d4;

  c.bn1 = c.d1 * SN / SD;
  c.bn2 = c.d2 * SN / SD;
  c.bn3 = c.d3 * SN / SD;
  c.bn4 = c.d4 * SN / SD;

  c.bm1 = c.d1 * SM / SD;
  c.bm2 = c.d2 * SM / SD;
  c.bm3 = c.d3 * SM / SD;
  c.bm4 = c.d4 * SM / SD;
}

// Port of RecursiveGaussianImageFilter::SetUp. sigma and spacing are physical; order in {0,1,2}.
inline RecursiveGaussianCoefficients ComputeRecursiveGaussianCoefficients(double sigma, double spacing, int order, bool normalizeAcrossScale)
{
  RecursiveGaussianCoefficients c;

  double direction = 1.0;
  if(spacing < 0.0)
  {
    direction = -1.0;
    spacing = -spacing;
  }

  const double sigmad = sigma / spacing;
  double acrossScaleNormalization = 1.0;

  double A1[3];
  double B1[3];
  double A2[3];
  double B2[3];
  const double W1 = 0.6681;
  const double L1 = -1.3932;
  const double W2 = 2.0787;
  const double L2 = -1.3732;
  A1[0] = 1.3530;
  B1[0] = 1.8151;
  A2[0] = -0.3531;
  B2[0] = 0.0902;
  A1[1] = -0.6724;
  B1[1] = -3.4327;
  A2[1] = 0.6724;
  B2[1] = 0.6100;
  A1[2] = -1.3563;
  B1[2] = 5.2318;
  A2[2] = 0.3446;
  B2[2] = -2.2355;

  double SD = 0.0;
  double DD = 0.0;
  double ED = 0.0;
  ComputeDCoefficients(sigmad, W1, L1, W2, L2, c, SD, DD, ED);
  double SN = 0.0;
  double DN = 0.0;
  double EN = 0.0;

  if(order == 0)
  {
    ComputeNCoefficients(sigmad, A1[0], B1[0], W1, L1, A2[0], B2[0], W2, L2, c.n0, c.n1, c.n2, c.n3, SN, DN, EN);
    const double alpha0 = 2 * SN / SD - c.n0;
    c.n0 *= acrossScaleNormalization / alpha0;
    c.n1 *= acrossScaleNormalization / alpha0;
    c.n2 *= acrossScaleNormalization / alpha0;
    c.n3 *= acrossScaleNormalization / alpha0;
    ComputeRemainingCoefficients(true, c);
  }
  else if(order == 1)
  {
    if(normalizeAcrossScale)
    {
      acrossScaleNormalization = sigma;
    }
    ComputeNCoefficients(sigmad, A1[1], B1[1], W1, L1, A2[1], B2[1], W2, L2, c.n0, c.n1, c.n2, c.n3, SN, DN, EN);
    double alpha1 = 2 * (SN * DD - DN * SD) / (SD * SD);
    alpha1 *= direction;
    c.n0 *= acrossScaleNormalization / alpha1;
    c.n1 *= acrossScaleNormalization / alpha1;
    c.n2 *= acrossScaleNormalization / alpha1;
    c.n3 *= acrossScaleNormalization / alpha1;
    ComputeRemainingCoefficients(false, c);
  }
  else // order == 2
  {
    if(normalizeAcrossScale)
    {
      acrossScaleNormalization = sigma * sigma;
    }
    double N0_0 = 0.0, N1_0 = 0.0, N2_0 = 0.0, N3_0 = 0.0;
    double N0_2 = 0.0, N1_2 = 0.0, N2_2 = 0.0, N3_2 = 0.0;
    double SN0 = 0.0, DN0 = 0.0, EN0 = 0.0;
    double SN2 = 0.0, DN2 = 0.0, EN2 = 0.0;
    ComputeNCoefficients(sigmad, A1[0], B1[0], W1, L1, A2[0], B2[0], W2, L2, N0_0, N1_0, N2_0, N3_0, SN0, DN0, EN0);
    ComputeNCoefficients(sigmad, A1[2], B1[2], W1, L1, A2[2], B2[2], W2, L2, N0_2, N1_2, N2_2, N3_2, SN2, DN2, EN2);

    const double beta = -(2 * SN2 - SD * N0_2) / (2 * SN0 - SD * N0_0);
    c.n0 = N0_2 + beta * N0_0;
    c.n1 = N1_2 + beta * N1_0;
    c.n2 = N2_2 + beta * N2_0;
    c.n3 = N3_2 + beta * N3_0;
    SN = SN2 + beta * SN0;
    DN = DN2 + beta * DN0;
    EN = EN2 + beta * EN0;

    double alpha2 = EN * SD * SD - ED * SN * SD - 2 * DN * DD * SD + 2 * DD * DD * SN;
    alpha2 /= SD * SD * SD;
    c.n0 *= acrossScaleNormalization / alpha2;
    c.n1 *= acrossScaleNormalization / alpha2;
    c.n2 *= acrossScaleNormalization / alpha2;
    c.n3 *= acrossScaleNormalization / alpha2;
    ComputeRemainingCoefficients(true, c);
  }

  return c;
}

// Port of RecursiveSeparableImageFilter::FilterDataArray. `data` and `outs` are length-`ln` double buffers
// (ln >= 4). `outs` holds the causal result, then the anti-causal recurrence is accumulated directly using a
// four-value rolling window. ITK materializes that second response in another full line buffer; the rolling form
// preserves the recurrence and arithmetic order while removing that allocation and the final summation pass.
inline void FilterDataArray(double* outs, const double* data, usize ln, const RecursiveGaussianCoefficients& c)
{
  double* scratch1 = outs;

  // ---- Causal (forward) pass ----
  const double outV1 = data[0];
  scratch1[0] = outV1 * c.n0 + outV1 * c.n1 + outV1 * c.n2 + outV1 * c.n3;
  scratch1[1] = data[1] * c.n0 + outV1 * c.n1 + outV1 * c.n2 + outV1 * c.n3;
  scratch1[2] = data[2] * c.n0 + data[1] * c.n1 + outV1 * c.n2 + outV1 * c.n3;
  scratch1[3] = data[3] * c.n0 + data[2] * c.n1 + data[1] * c.n2 + outV1 * c.n3;

  scratch1[0] -= outV1 * c.bn1 + outV1 * c.bn2 + outV1 * c.bn3 + outV1 * c.bn4;
  scratch1[1] -= scratch1[0] * c.d1 + outV1 * c.bn2 + outV1 * c.bn3 + outV1 * c.bn4;
  scratch1[2] -= scratch1[1] * c.d1 + scratch1[0] * c.d2 + outV1 * c.bn3 + outV1 * c.bn4;
  scratch1[3] -= scratch1[2] * c.d1 + scratch1[1] * c.d2 + scratch1[0] * c.d3 + outV1 * c.bn4;

  for(usize i = 4; i < ln; ++i)
  {
    scratch1[i] = data[i] * c.n0 + data[i - 1] * c.n1 + data[i - 2] * c.n2 + data[i - 3] * c.n3;
    scratch1[i] -= scratch1[i - 1] * c.d1 + scratch1[i - 2] * c.d2 + scratch1[i - 3] * c.d3 + scratch1[i - 4] * c.d4;
  }

  // ---- Anti-causal (backward) pass ----
  const double outV2 = data[ln - 1];
  double next4 = outV2 * c.m1 + outV2 * c.m2 + outV2 * c.m3 + outV2 * c.m4;
  double next3 = data[ln - 1] * c.m1 + outV2 * c.m2 + outV2 * c.m3 + outV2 * c.m4;
  double next2 = data[ln - 2] * c.m1 + data[ln - 1] * c.m2 + outV2 * c.m3 + outV2 * c.m4;
  double next1 = data[ln - 3] * c.m1 + data[ln - 2] * c.m2 + data[ln - 1] * c.m3 + outV2 * c.m4;

  next4 -= outV2 * c.bm1 + outV2 * c.bm2 + outV2 * c.bm3 + outV2 * c.bm4;
  next3 -= next4 * c.d1 + outV2 * c.bm2 + outV2 * c.bm3 + outV2 * c.bm4;
  next2 -= next3 * c.d1 + next4 * c.d2 + outV2 * c.bm3 + outV2 * c.bm4;
  next1 -= next2 * c.d1 + next3 * c.d2 + next4 * c.d3 + outV2 * c.bm4;

  outs[ln - 1] += next4;
  outs[ln - 2] += next3;
  outs[ln - 3] += next2;
  outs[ln - 4] += next1;

  for(usize i = ln - 4; i > 0; --i)
  {
    double current = data[i] * c.m1 + data[i + 1] * c.m2 + data[i + 2] * c.m3 + data[i + 3] * c.m4;
    current -= next1 * c.d1 + next2 * c.d2 + next3 * c.d3 + next4 * c.d4;
    outs[i - 1] += current;
    next4 = next3;
    next3 = next2;
    next2 = next1;
    next1 = current;
  }
}

// Four independent lines in longitudinal-major layout: data[i * 4 + lane]. Keeping the lane loop innermost
// exposes the recurrence's line-level parallelism to the compiler while preserving each lane's scalar operation
// order exactly. Callers use the scalar routine for a final partial group.
inline void FilterFourDataArrays(double* outs, const double* data, usize ln, const RecursiveGaussianCoefficients& c)
{
  for(usize lane = 0; lane < 4; ++lane)
  {
    const double outV1 = data[lane];
    outs[lane] = outV1 * c.n0 + outV1 * c.n1 + outV1 * c.n2 + outV1 * c.n3;
    outs[4 + lane] = data[4 + lane] * c.n0 + outV1 * c.n1 + outV1 * c.n2 + outV1 * c.n3;
    outs[8 + lane] = data[8 + lane] * c.n0 + data[4 + lane] * c.n1 + outV1 * c.n2 + outV1 * c.n3;
    outs[12 + lane] = data[12 + lane] * c.n0 + data[8 + lane] * c.n1 + data[4 + lane] * c.n2 + outV1 * c.n3;

    outs[lane] -= outV1 * c.bn1 + outV1 * c.bn2 + outV1 * c.bn3 + outV1 * c.bn4;
    outs[4 + lane] -= outs[lane] * c.d1 + outV1 * c.bn2 + outV1 * c.bn3 + outV1 * c.bn4;
    outs[8 + lane] -= outs[4 + lane] * c.d1 + outs[lane] * c.d2 + outV1 * c.bn3 + outV1 * c.bn4;
    outs[12 + lane] -= outs[8 + lane] * c.d1 + outs[4 + lane] * c.d2 + outs[lane] * c.d3 + outV1 * c.bn4;
  }

  for(usize i = 4; i < ln; ++i)
  {
    const usize current = i * 4;
    for(usize lane = 0; lane < 4; ++lane)
    {
      outs[current + lane] = data[current + lane] * c.n0 + data[current - 4 + lane] * c.n1 + data[current - 8 + lane] * c.n2 + data[current - 12 + lane] * c.n3;
      outs[current + lane] -= outs[current - 4 + lane] * c.d1 + outs[current - 8 + lane] * c.d2 + outs[current - 12 + lane] * c.d3 + outs[current - 16 + lane] * c.d4;
    }
  }

  std::array<double, 4> next1{};
  std::array<double, 4> next2{};
  std::array<double, 4> next3{};
  std::array<double, 4> next4{};
  const usize last = (ln - 1) * 4;
  for(usize lane = 0; lane < 4; ++lane)
  {
    const double outV2 = data[last + lane];
    next4[lane] = outV2 * c.m1 + outV2 * c.m2 + outV2 * c.m3 + outV2 * c.m4;
    next3[lane] = data[last + lane] * c.m1 + outV2 * c.m2 + outV2 * c.m3 + outV2 * c.m4;
    next2[lane] = data[last - 4 + lane] * c.m1 + data[last + lane] * c.m2 + outV2 * c.m3 + outV2 * c.m4;
    next1[lane] = data[last - 8 + lane] * c.m1 + data[last - 4 + lane] * c.m2 + data[last + lane] * c.m3 + outV2 * c.m4;

    next4[lane] -= outV2 * c.bm1 + outV2 * c.bm2 + outV2 * c.bm3 + outV2 * c.bm4;
    next3[lane] -= next4[lane] * c.d1 + outV2 * c.bm2 + outV2 * c.bm3 + outV2 * c.bm4;
    next2[lane] -= next3[lane] * c.d1 + next4[lane] * c.d2 + outV2 * c.bm3 + outV2 * c.bm4;
    next1[lane] -= next2[lane] * c.d1 + next3[lane] * c.d2 + next4[lane] * c.d3 + outV2 * c.bm4;

    outs[last + lane] += next4[lane];
    outs[last - 4 + lane] += next3[lane];
    outs[last - 8 + lane] += next2[lane];
    outs[last - 12 + lane] += next1[lane];
  }

  for(usize i = ln - 4; i > 0; --i)
  {
    const usize input = i * 4;
    const usize output = (i - 1) * 4;
    for(usize lane = 0; lane < 4; ++lane)
    {
      double current = data[input + lane] * c.m1 + data[input + 4 + lane] * c.m2 + data[input + 8 + lane] * c.m3 + data[input + 12 + lane] * c.m4;
      current -= next1[lane] * c.d1 + next2[lane] * c.d2 + next3[lane] * c.d3 + next4[lane] * c.d4;
      outs[output + lane] += current;
      next4[lane] = next3[lane];
      next3[lane] = next2[lane];
      next2[lane] = next1[lane];
      next1[lane] = current;
    }
  }
}

inline bool RecursiveGaussianCheckedMultiply(usize left, usize right, usize& product) noexcept
{
  if(left != 0 && right > std::numeric_limits<usize>::max() / left)
  {
    return false;
  }
  product = left * right;
  return true;
}

inline bool RecursiveGaussianCheckedAdd(usize left, usize right, usize& sum) noexcept
{
  if(right > std::numeric_limits<usize>::max() - left)
  {
    return false;
  }
  sum = left + right;
  return true;
}

struct LaplacianRecursiveGaussianResidentMemoryAllocation
{
  CacheMemoryBudgetManager::WorkingMemoryReservation reservation;
  usize requiredBytes = 0;

  [[nodiscard]] bool holdsCompleteState() const noexcept
  {
    return requiredBytes > 0 && reservation.sizeBytes() == requiredBytes;
  }
};

inline bool ShouldUseLaplacianRecursiveGaussianResidentState(const SizeVec3& dims)
{
  return dims[2] > 1;
}

template <class T>
Result<usize> CalculateLaplacianRecursiveGaussianResidentWorkingMemoryBytes(const SizeVec3& dims)
{
  if(dims[0] == 0 || dims[1] == 0 || dims[2] == 0)
  {
    return {usize{0}};
  }

  usize sliceValues = 0;
  usize volumeValues = 0;
  usize bytesPerValue = 0;
  usize requiredBytes = 0;
  if(!RecursiveGaussianCheckedMultiply(dims[0], dims[1], sliceValues) || !RecursiveGaussianCheckedMultiply(sliceValues, dims[2], volumeValues) ||
     !RecursiveGaussianCheckedAdd(sizeof(T), 2 * sizeof(float32), bytesPerValue) || !RecursiveGaussianCheckedMultiply(volumeValues, bytesPerValue, requiredBytes))
  {
    return MakeErrorResult<usize>(-8768, fmt::format("Laplacian recursive Gaussian dimensions ({}) and {}-byte input values overflow while sizing the resident input, accumulator, and work images.",
                                                     StringUtilities::formatDimensions3D(dims), sizeof(T)));
  }
  return {requiredBytes};
}

template <class T>
Result<LaplacianRecursiveGaussianResidentMemoryAllocation> ReserveLaplacianRecursiveGaussianResidentWorkingMemory(const SizeVec3& dims)
{
  auto requiredResult = CalculateLaplacianRecursiveGaussianResidentWorkingMemoryBytes<T>(dims);
  if(requiredResult.invalid())
  {
    return ConvertInvalidResult<LaplacianRecursiveGaussianResidentMemoryAllocation>(std::move(requiredResult));
  }
  auto reservation = ReserveWorkingMemory(requiredResult.value(), requiredResult.value());
  return {LaplacianRecursiveGaussianResidentMemoryAllocation{std::move(reservation), requiredResult.value()}};
}

inline RecursiveGaussian2DBufferPlan BuildRecursiveGaussian2DBufferPlan(usize dimX, usize dimY, usize residentLimit, usize blockBytesPerValue = 32)
{
  constexpr usize k_FixedBytes = 4096;
  constexpr usize k_StateBytesPerColumn = sizeof(RecursiveGaussian2DCheckpoint) + 4 * sizeof(double);
  constexpr usize k_BackwardHaloRows = 4;
  RecursiveGaussian2DBufferPlan plan;
  if(dimX == 0 || dimY < 4 || residentLimit <= k_FixedBytes || blockBytesPerValue == 0)
  {
    return plan;
  }
  const usize minimumRows = std::min<usize>(dimY, 8);
  usize stateBytes = 0;
  usize rowValues = 0;
  usize minimumBlockBytes = 0;
  usize fullWidthBytes = 0;
  usize workerScratchValues = 0;
  usize workerScratchBytes = 0;
  usize minimumRowsWithHalo = 0;
  const usize workerCount = std::max<usize>(1, std::thread::hardware_concurrency());
  if(!RecursiveGaussianCheckedAdd(minimumRows, k_BackwardHaloRows, minimumRowsWithHalo) || !RecursiveGaussianCheckedMultiply(dimX, k_StateBytesPerColumn, stateBytes) ||
     !RecursiveGaussianCheckedMultiply(dimX, minimumRowsWithHalo, rowValues) || !RecursiveGaussianCheckedMultiply(rowValues, blockBytesPerValue, minimumBlockBytes) ||
     !RecursiveGaussianCheckedMultiply(dimX, 10 * workerCount, workerScratchValues) || !RecursiveGaussianCheckedMultiply(workerScratchValues, sizeof(double), workerScratchBytes) ||
     !RecursiveGaussianCheckedAdd(k_FixedBytes, stateBytes, fullWidthBytes) || !RecursiveGaussianCheckedAdd(fullWidthBytes, workerScratchBytes, fullWidthBytes) ||
     !RecursiveGaussianCheckedAdd(fullWidthBytes, minimumBlockBytes, fullWidthBytes))
  {
    plan.overflow = true;
    return plan;
  }

  if(fullWidthBytes <= residentLimit)
  {
    usize bytesPerRow = 0;
    usize fixedAndState = 0;
    usize haloValues = 0;
    usize haloBytes = 0;
    if(!RecursiveGaussianCheckedMultiply(dimX, blockBytesPerValue, bytesPerRow) || !RecursiveGaussianCheckedMultiply(dimX, k_BackwardHaloRows, haloValues) ||
       !RecursiveGaussianCheckedMultiply(haloValues, blockBytesPerValue, haloBytes) || !RecursiveGaussianCheckedAdd(k_FixedBytes, stateBytes, fixedAndState) ||
       !RecursiveGaussianCheckedAdd(fixedAndState, workerScratchBytes, fixedAndState) || !RecursiveGaussianCheckedAdd(fixedAndState, haloBytes, fixedAndState))
    {
      plan.overflow = true;
      return plan;
    }
    plan.blockRows = std::min(dimY, (residentLimit - fixedAndState) / bytesPerRow);
    if(plan.blockRows < minimumRows)
    {
      return {};
    }
    plan.coreCols = dimX;
  }
  else
  {
    usize blockBytesPerColumn = 0;
    usize bytesPerColumn = 0;
    if(!RecursiveGaussianCheckedMultiply(minimumRowsWithHalo, blockBytesPerValue, blockBytesPerColumn) || !RecursiveGaussianCheckedAdd(k_StateBytesPerColumn, blockBytesPerColumn, bytesPerColumn))
    {
      plan.overflow = true;
      return plan;
    }
    plan.blockRows = minimumRows;
    plan.coreCols = std::min(dimX, (residentLimit - k_FixedBytes) / bytesPerColumn);
    if(plan.coreCols == 0)
    {
      return {};
    }
  }

  while(plan.blockRows > 4)
  {
    const usize remainder = dimY % plan.blockRows;
    if(remainder == 0 || remainder >= 4)
    {
      break;
    }
    --plan.blockRows;
  }
  plan.blockCount = 1 + (dimY - 1) / plan.blockRows;
  if(dimY - (plan.blockCount - 1) * plan.blockRows < 4)
  {
    return {};
  }
  usize residentColumnsBytes = 0;
  usize residentBlockValues = 0;
  usize residentBlockBytes = 0;
  usize residentRowsWithHalo = 0;
  if(!RecursiveGaussianCheckedAdd(plan.blockRows, k_BackwardHaloRows, residentRowsWithHalo) || !RecursiveGaussianCheckedMultiply(plan.coreCols, k_StateBytesPerColumn, residentColumnsBytes) ||
     !RecursiveGaussianCheckedMultiply(plan.coreCols, residentRowsWithHalo, residentBlockValues) || !RecursiveGaussianCheckedMultiply(residentBlockValues, blockBytesPerValue, residentBlockBytes) ||
     !RecursiveGaussianCheckedAdd(k_FixedBytes, residentColumnsBytes, plan.residentBytes) ||
     (plan.coreCols == dimX && !RecursiveGaussianCheckedAdd(plan.residentBytes, workerScratchBytes, plan.residentBytes)) ||
     !RecursiveGaussianCheckedAdd(plan.residentBytes, residentBlockBytes, plan.residentBytes))
  {
    plan = {};
    plan.overflow = true;
    return plan;
  }
  plan.valid = plan.residentBytes <= residentLimit;
  return plan;
}

inline RecursiveGaussian2DBufferPlan AlignRecursiveGaussian2DBufferPlanToChunks(RecursiveGaussian2DBufferPlan plan, usize dimX, usize dimY, const std::optional<ShapeType>& chunkShape)
{
  if(!plan.valid || plan.coreCols != dimX || !chunkShape.has_value() || chunkShape->size() < 3 || (*chunkShape)[0] != 1 || (*chunkShape)[1] == 0 || (*chunkShape)[1] > plan.blockRows ||
     (*chunkShape)[2] != dimX)
  {
    return plan;
  }
  const usize chunkRows = (*chunkShape)[1];
  usize alignedRows = (plan.blockRows / chunkRows) * chunkRows;
  while(alignedRows >= chunkRows && alignedRows >= 4)
  {
    const usize remainder = dimY % alignedRows;
    if(remainder == 0 || remainder >= 4)
    {
      plan.blockRows = alignedRows;
      plan.blockCount = 1 + (dimY - 1) / plan.blockRows;
      return plan;
    }
    alignedRows -= chunkRows;
  }
  return plan;
}

inline usize RecursiveGaussian2DBlockBegin(usize blockIndex, const RecursiveGaussian2DBufferPlan& plan)
{
  return blockIndex * plan.blockRows;
}

inline usize RecursiveGaussian2DBlockRows(usize blockIndex, const RecursiveGaussian2DBufferPlan& plan, usize dimY)
{
  return std::min(plan.blockRows, dimY - RecursiveGaussian2DBlockBegin(blockIndex, plan));
}

inline usize RecursiveGaussian2DXBlockValues(usize dimX, usize coreCols)
{
  usize blockValues = std::min(dimX, std::max<usize>(4, coreCols));
  while(blockValues > 4)
  {
    const usize remainder = dimX % blockValues;
    if(remainder == 0 || remainder >= 4)
    {
      break;
    }
    --blockValues;
  }
  return blockValues;
}

inline usize RecursiveGaussian2DXBlockCount(usize dimX, usize coreCols)
{
  const usize blockValues = RecursiveGaussian2DXBlockValues(dimX, coreCols);
  return 1 + (dimX - 1) / blockValues;
}

template <class T, class StoreT>
Result<> ReadRecursiveGaussian2DBlock(const StoreT& store, usize dimX, usize yBegin, usize rowCount, usize xBegin, usize columnCount, nonstd::span<T> values)
{
  if(columnCount == dimX)
  {
    return store.copyIntoBuffer(yBegin * dimX, values.subspan(0, rowCount * columnCount));
  }
  for(usize row = 0; row < rowCount; ++row)
  {
    if(Result<> result = store.copyIntoBuffer((yBegin + row) * dimX + xBegin, values.subspan(row * columnCount, columnCount)); result.invalid())
    {
      return result;
    }
  }
  return {};
}

template <class SrcT>
void ComputeRecursiveGaussianCausalBlock(nonstd::span<const SrcT> source, usize rowCount, usize columnCount, usize column, bool firstBlock, const RecursiveGaussianCoefficients& coefficients,
                                         RecursiveGaussian2DCheckpoint& state, nonstd::span<double> causal)
{
  usize row = 0;
  if(firstBlock)
  {
    const double input0 = static_cast<double>(source[column]);
    const double input1 = static_cast<double>(source[columnCount + column]);
    const double input2 = static_cast<double>(source[2 * columnCount + column]);
    const double input3 = static_cast<double>(source[3 * columnCount + column]);
    causal[column] = input0 * coefficients.n0 + input0 * coefficients.n1 + input0 * coefficients.n2 + input0 * coefficients.n3;
    causal[columnCount + column] = input1 * coefficients.n0 + input0 * coefficients.n1 + input0 * coefficients.n2 + input0 * coefficients.n3;
    causal[2 * columnCount + column] = input2 * coefficients.n0 + input1 * coefficients.n1 + input0 * coefficients.n2 + input0 * coefficients.n3;
    causal[3 * columnCount + column] = input3 * coefficients.n0 + input2 * coefficients.n1 + input1 * coefficients.n2 + input0 * coefficients.n3;

    causal[column] -= input0 * coefficients.bn1 + input0 * coefficients.bn2 + input0 * coefficients.bn3 + input0 * coefficients.bn4;
    causal[columnCount + column] -= causal[column] * coefficients.d1 + input0 * coefficients.bn2 + input0 * coefficients.bn3 + input0 * coefficients.bn4;
    causal[2 * columnCount + column] -= causal[columnCount + column] * coefficients.d1 + causal[column] * coefficients.d2 + input0 * coefficients.bn3 + input0 * coefficients.bn4;
    causal[3 * columnCount + column] -=
        causal[2 * columnCount + column] * coefficients.d1 + causal[columnCount + column] * coefficients.d2 + causal[column] * coefficients.d3 + input0 * coefficients.bn4;
    state.input1 = input3;
    state.input2 = input2;
    state.input3 = input1;
    state.output1 = causal[3 * columnCount + column];
    state.output2 = causal[2 * columnCount + column];
    state.output3 = causal[columnCount + column];
    state.output4 = causal[column];
    row = 4;
  }

  for(; row < rowCount; ++row)
  {
    const double input = static_cast<double>(source[row * columnCount + column]);
    double output = input * coefficients.n0 + state.input1 * coefficients.n1 + state.input2 * coefficients.n2 + state.input3 * coefficients.n3;
    output -= state.output1 * coefficients.d1 + state.output2 * coefficients.d2 + state.output3 * coefficients.d3 + state.output4 * coefficients.d4;
    causal[row * columnCount + column] = output;
    state.input3 = state.input2;
    state.input2 = state.input1;
    state.input1 = input;
    state.output4 = state.output3;
    state.output3 = state.output2;
    state.output2 = state.output1;
    state.output1 = output;
  }
}

template <class SrcT>
void ComputeFourRecursiveGaussianCausalBlocks(nonstd::span<const SrcT> source, usize rowCount, usize columnCount, usize firstColumn, bool firstBlock, const RecursiveGaussianCoefficients& coefficients,
                                              nonstd::span<RecursiveGaussian2DCheckpoint> states, nonstd::span<double> causal)
{
  usize row = 0;
  if(firstBlock)
  {
    for(usize lane = 0; lane < 4; ++lane)
    {
      const usize column = firstColumn + lane;
      const double input0 = static_cast<double>(source[column]);
      const double input1 = static_cast<double>(source[columnCount + column]);
      const double input2 = static_cast<double>(source[2 * columnCount + column]);
      const double input3 = static_cast<double>(source[3 * columnCount + column]);
      causal[column] = input0 * coefficients.n0 + input0 * coefficients.n1 + input0 * coefficients.n2 + input0 * coefficients.n3;
      causal[columnCount + column] = input1 * coefficients.n0 + input0 * coefficients.n1 + input0 * coefficients.n2 + input0 * coefficients.n3;
      causal[2 * columnCount + column] = input2 * coefficients.n0 + input1 * coefficients.n1 + input0 * coefficients.n2 + input0 * coefficients.n3;
      causal[3 * columnCount + column] = input3 * coefficients.n0 + input2 * coefficients.n1 + input1 * coefficients.n2 + input0 * coefficients.n3;

      causal[column] -= input0 * coefficients.bn1 + input0 * coefficients.bn2 + input0 * coefficients.bn3 + input0 * coefficients.bn4;
      causal[columnCount + column] -= causal[column] * coefficients.d1 + input0 * coefficients.bn2 + input0 * coefficients.bn3 + input0 * coefficients.bn4;
      causal[2 * columnCount + column] -= causal[columnCount + column] * coefficients.d1 + causal[column] * coefficients.d2 + input0 * coefficients.bn3 + input0 * coefficients.bn4;
      causal[3 * columnCount + column] -=
          causal[2 * columnCount + column] * coefficients.d1 + causal[columnCount + column] * coefficients.d2 + causal[column] * coefficients.d3 + input0 * coefficients.bn4;
      auto& state = states[column];
      state.input1 = input3;
      state.input2 = input2;
      state.input3 = input1;
      state.output1 = causal[3 * columnCount + column];
      state.output2 = causal[2 * columnCount + column];
      state.output3 = causal[columnCount + column];
      state.output4 = causal[column];
    }
    row = 4;
  }

  for(; row < rowCount; ++row)
  {
    for(usize lane = 0; lane < 4; ++lane)
    {
      const usize column = firstColumn + lane;
      auto& state = states[column];
      const double input = static_cast<double>(source[row * columnCount + column]);
      double output = input * coefficients.n0 + state.input1 * coefficients.n1 + state.input2 * coefficients.n2 + state.input3 * coefficients.n3;
      output -= state.output1 * coefficients.d1 + state.output2 * coefficients.d2 + state.output3 * coefficients.d3 + state.output4 * coefficients.d4;
      causal[row * columnCount + column] = output;
      state.input3 = state.input2;
      state.input2 = state.input1;
      state.input1 = input;
      state.output4 = state.output3;
      state.output3 = state.output2;
      state.output2 = state.output1;
      state.output1 = output;
    }
  }
}

template <class SrcT>
void ComputeRecursiveGaussianAntiCausalBlock(nonstd::span<const SrcT> source, nonstd::span<const double> causal, usize rowCount, usize columnCount, usize column, bool lastBlock,
                                             const RecursiveGaussianCoefficients& coefficients, std::array<double, 4>& antiState, nonstd::span<float32> filtered)
{
  if(lastBlock)
  {
    const double outputValue = static_cast<double>(source[(rowCount - 1) * columnCount + column]);
    double next4 = outputValue * coefficients.m1 + outputValue * coefficients.m2 + outputValue * coefficients.m3 + outputValue * coefficients.m4;
    double next3 = static_cast<double>(source[(rowCount - 1) * columnCount + column]) * coefficients.m1 + outputValue * coefficients.m2 + outputValue * coefficients.m3 + outputValue * coefficients.m4;
    double next2 = static_cast<double>(source[(rowCount - 2) * columnCount + column]) * coefficients.m1 + static_cast<double>(source[(rowCount - 1) * columnCount + column]) * coefficients.m2 +
                   outputValue * coefficients.m3 + outputValue * coefficients.m4;
    double next1 = static_cast<double>(source[(rowCount - 3) * columnCount + column]) * coefficients.m1 + static_cast<double>(source[(rowCount - 2) * columnCount + column]) * coefficients.m2 +
                   static_cast<double>(source[(rowCount - 1) * columnCount + column]) * coefficients.m3 + outputValue * coefficients.m4;
    next4 -= outputValue * coefficients.bm1 + outputValue * coefficients.bm2 + outputValue * coefficients.bm3 + outputValue * coefficients.bm4;
    next3 -= next4 * coefficients.d1 + outputValue * coefficients.bm2 + outputValue * coefficients.bm3 + outputValue * coefficients.bm4;
    next2 -= next3 * coefficients.d1 + next4 * coefficients.d2 + outputValue * coefficients.bm3 + outputValue * coefficients.bm4;
    next1 -= next2 * coefficients.d1 + next3 * coefficients.d2 + next4 * coefficients.d3 + outputValue * coefficients.bm4;
    filtered[(rowCount - 1) * columnCount + column] = static_cast<float32>(causal[(rowCount - 1) * columnCount + column] + next4);
    filtered[(rowCount - 2) * columnCount + column] = static_cast<float32>(causal[(rowCount - 2) * columnCount + column] + next3);
    filtered[(rowCount - 3) * columnCount + column] = static_cast<float32>(causal[(rowCount - 3) * columnCount + column] + next2);
    filtered[(rowCount - 4) * columnCount + column] = static_cast<float32>(causal[(rowCount - 4) * columnCount + column] + next1);
    antiState = {next1, next2, next3, next4};
    for(usize inputRow = rowCount - 4; inputRow > 0; --inputRow)
    {
      double current = static_cast<double>(source[inputRow * columnCount + column]) * coefficients.m1 + static_cast<double>(source[(inputRow + 1) * columnCount + column]) * coefficients.m2 +
                       static_cast<double>(source[(inputRow + 2) * columnCount + column]) * coefficients.m3 + static_cast<double>(source[(inputRow + 3) * columnCount + column]) * coefficients.m4;
      current -= antiState[0] * coefficients.d1 + antiState[1] * coefficients.d2 + antiState[2] * coefficients.d3 + antiState[3] * coefficients.d4;
      filtered[(inputRow - 1) * columnCount + column] = static_cast<float32>(causal[(inputRow - 1) * columnCount + column] + current);
      antiState = {current, antiState[0], antiState[1], antiState[2]};
    }
    return;
  }

  for(usize outputReverse = rowCount; outputReverse > 0; --outputReverse)
  {
    const usize outputRow = outputReverse - 1;
    const usize inputRow = outputRow + 1;
    double current = static_cast<double>(source[inputRow * columnCount + column]) * coefficients.m1 + static_cast<double>(source[(inputRow + 1) * columnCount + column]) * coefficients.m2 +
                     static_cast<double>(source[(inputRow + 2) * columnCount + column]) * coefficients.m3 + static_cast<double>(source[(inputRow + 3) * columnCount + column]) * coefficients.m4;
    current -= antiState[0] * coefficients.d1 + antiState[1] * coefficients.d2 + antiState[2] * coefficients.d3 + antiState[3] * coefficients.d4;
    filtered[outputRow * columnCount + column] = static_cast<float32>(causal[outputRow * columnCount + column] + current);
    antiState = {current, antiState[0], antiState[1], antiState[2]};
  }
}

template <class SrcT>
void ComputeFourRecursiveGaussianAntiCausalBlocks(nonstd::span<const SrcT> source, nonstd::span<const double> causal, usize rowCount, usize columnCount, usize firstColumn, bool lastBlock,
                                                  const RecursiveGaussianCoefficients& coefficients, nonstd::span<std::array<double, 4>> antiStates, nonstd::span<float32> filtered)
{
  if(lastBlock)
  {
    for(usize lane = 0; lane < 4; ++lane)
    {
      const usize column = firstColumn + lane;
      const double outputValue = static_cast<double>(source[(rowCount - 1) * columnCount + column]);
      double next4 = outputValue * coefficients.m1 + outputValue * coefficients.m2 + outputValue * coefficients.m3 + outputValue * coefficients.m4;
      double next3 =
          static_cast<double>(source[(rowCount - 1) * columnCount + column]) * coefficients.m1 + outputValue * coefficients.m2 + outputValue * coefficients.m3 + outputValue * coefficients.m4;
      double next2 = static_cast<double>(source[(rowCount - 2) * columnCount + column]) * coefficients.m1 + static_cast<double>(source[(rowCount - 1) * columnCount + column]) * coefficients.m2 +
                     outputValue * coefficients.m3 + outputValue * coefficients.m4;
      double next1 = static_cast<double>(source[(rowCount - 3) * columnCount + column]) * coefficients.m1 + static_cast<double>(source[(rowCount - 2) * columnCount + column]) * coefficients.m2 +
                     static_cast<double>(source[(rowCount - 1) * columnCount + column]) * coefficients.m3 + outputValue * coefficients.m4;
      next4 -= outputValue * coefficients.bm1 + outputValue * coefficients.bm2 + outputValue * coefficients.bm3 + outputValue * coefficients.bm4;
      next3 -= next4 * coefficients.d1 + outputValue * coefficients.bm2 + outputValue * coefficients.bm3 + outputValue * coefficients.bm4;
      next2 -= next3 * coefficients.d1 + next4 * coefficients.d2 + outputValue * coefficients.bm3 + outputValue * coefficients.bm4;
      next1 -= next2 * coefficients.d1 + next3 * coefficients.d2 + next4 * coefficients.d3 + outputValue * coefficients.bm4;
      filtered[(rowCount - 1) * columnCount + column] = static_cast<float32>(causal[(rowCount - 1) * columnCount + column] + next4);
      filtered[(rowCount - 2) * columnCount + column] = static_cast<float32>(causal[(rowCount - 2) * columnCount + column] + next3);
      filtered[(rowCount - 3) * columnCount + column] = static_cast<float32>(causal[(rowCount - 3) * columnCount + column] + next2);
      filtered[(rowCount - 4) * columnCount + column] = static_cast<float32>(causal[(rowCount - 4) * columnCount + column] + next1);
      antiStates[column] = {next1, next2, next3, next4};
    }
    for(usize inputRow = rowCount - 4; inputRow > 0; --inputRow)
    {
      for(usize lane = 0; lane < 4; ++lane)
      {
        const usize column = firstColumn + lane;
        auto& antiState = antiStates[column];
        double current = static_cast<double>(source[inputRow * columnCount + column]) * coefficients.m1 + static_cast<double>(source[(inputRow + 1) * columnCount + column]) * coefficients.m2 +
                         static_cast<double>(source[(inputRow + 2) * columnCount + column]) * coefficients.m3 + static_cast<double>(source[(inputRow + 3) * columnCount + column]) * coefficients.m4;
        current -= antiState[0] * coefficients.d1 + antiState[1] * coefficients.d2 + antiState[2] * coefficients.d3 + antiState[3] * coefficients.d4;
        filtered[(inputRow - 1) * columnCount + column] = static_cast<float32>(causal[(inputRow - 1) * columnCount + column] + current);
        antiState = {current, antiState[0], antiState[1], antiState[2]};
      }
    }
    return;
  }
  for(usize outputReverse = rowCount; outputReverse > 0; --outputReverse)
  {
    const usize outputRow = outputReverse - 1;
    const usize inputRow = outputRow + 1;
    for(usize lane = 0; lane < 4; ++lane)
    {
      const usize column = firstColumn + lane;
      auto& antiState = antiStates[column];
      double current = static_cast<double>(source[inputRow * columnCount + column]) * coefficients.m1 + static_cast<double>(source[(inputRow + 1) * columnCount + column]) * coefficients.m2 +
                       static_cast<double>(source[(inputRow + 2) * columnCount + column]) * coefficients.m3 + static_cast<double>(source[(inputRow + 3) * columnCount + column]) * coefficients.m4;
      current -= antiState[0] * coefficients.d1 + antiState[1] * coefficients.d2 + antiState[2] * coefficients.d3 + antiState[3] * coefficients.d4;
      filtered[outputRow * columnCount + column] = static_cast<float32>(causal[outputRow * columnCount + column] + current);
      antiState = {current, antiState[0], antiState[1], antiState[2]};
    }
  }
}

template <class SrcT>
void FilterRecursiveGaussianXYPlane(nonstd::span<const SrcT> srcPlane, nonstd::span<float32> dstPlane, const SizeVec3& dims, const RecursiveGaussianPlaneAxis& pass,
                                    const RecursiveGaussianCoefficients& coefficients, const std::atomic_bool& shouldCancel)
{
  const usize dimX = dims[0];
  const usize dimY = dims[1];
  const usize lineLength = pass.axis == 0 ? dimX : dimY;
  const usize lineCount = pass.axis == 0 ? dimY : dimX;

  auto filterLines = [&](const Range& lineRange) {
    static thread_local std::vector<double> data;
    static thread_local std::vector<double> outs;
    data.resize(lineLength);
    outs.resize(lineLength);
    for(usize line = lineRange.min(); line < lineRange.max(); ++line)
    {
      if(shouldCancel)
      {
        return;
      }

      if(pass.axis == 0)
      {
        const usize rowOffset = line * dimX;
        for(usize i = 0; i < lineLength; ++i)
        {
          data[i] = static_cast<double>(srcPlane[rowOffset + i]);
        }
        detail::FilterDataArray(outs.data(), data.data(), lineLength, coefficients);
        for(usize i = 0; i < lineLength; ++i)
        {
          dstPlane[rowOffset + i] = static_cast<float32>(outs[i]);
        }
      }
      else
      {
        for(usize i = 0; i < lineLength; ++i)
        {
          data[i] = static_cast<double>(srcPlane[i * dimX + line]);
        }
        detail::FilterDataArray(outs.data(), data.data(), lineLength, coefficients);
        for(usize i = 0; i < lineLength; ++i)
        {
          dstPlane[i * dimX + line] = static_cast<float32>(outs[i]);
        }
      }
    }
  };

  ParallelDataAlgorithm parallelAlgorithm;
  parallelAlgorithm.setRange(0, lineCount);
  parallelAlgorithm.execute(filterLines);
}

template <class SrcT>
void FilterFourRecursiveGaussianXRows(nonstd::span<const SrcT> source, nonstd::span<float32> destination, usize dimX, usize dimY, const RecursiveGaussianCoefficients& coefficients,
                                      const std::atomic_bool& shouldCancel)
{
  const usize batchCount = (dimY + 3) / 4;
  auto filterBatches = [&](const Range& batchRange) {
    static thread_local std::vector<double> batchData;
    static thread_local std::vector<double> batchOutput;
    static thread_local std::vector<double> tailData;
    static thread_local std::vector<double> tailOutput;
    batchData.resize(dimX * 4);
    batchOutput.resize(dimX * 4);
    tailData.resize(dimX);
    tailOutput.resize(dimX);
    for(usize batchIndex = batchRange.min(); batchIndex < batchRange.max(); ++batchIndex)
    {
      if(shouldCancel)
      {
        return;
      }
      const usize firstRow = batchIndex * 4;
      const usize lanes = std::min<usize>(4, dimY - firstRow);
      if(lanes == 4)
      {
        for(usize x = 0; x < dimX; ++x)
        {
          for(usize lane = 0; lane < 4; ++lane)
          {
            batchData[x * 4 + lane] = static_cast<double>(source[(firstRow + lane) * dimX + x]);
          }
        }
        FilterFourDataArrays(batchOutput.data(), batchData.data(), dimX, coefficients);
        for(usize x = 0; x < dimX; ++x)
        {
          for(usize lane = 0; lane < 4; ++lane)
          {
            destination[(firstRow + lane) * dimX + x] = static_cast<float32>(batchOutput[x * 4 + lane]);
          }
        }
      }
      else
      {
        for(usize lane = 0; lane < lanes; ++lane)
        {
          const usize rowOffset = (firstRow + lane) * dimX;
          for(usize x = 0; x < dimX; ++x)
          {
            tailData[x] = static_cast<double>(source[rowOffset + x]);
          }
          FilterDataArray(tailOutput.data(), tailData.data(), dimX, coefficients);
          for(usize x = 0; x < dimX; ++x)
          {
            destination[rowOffset + x] = static_cast<float32>(tailOutput[x]);
          }
        }
      }
    }
  };
  ParallelDataAlgorithm parallelAlgorithm;
  parallelAlgorithm.setRange(0, batchCount);
  parallelAlgorithm.execute(filterBatches);
}
} // namespace detail

/**
 * @brief Runs one recursive-Gaussian Y-axis pass over a true-2-D row-major store using bounded row blocks.
 *
 * Forward recurrence writes only seven-double block-boundary checkpoints. The backward sweep rereads each source block,
 * reconstructs its causal values from the checkpoint, adds the anti-causal recurrence, and emits disjoint row-major
 * blocks to @p sink. Partial-X plans issue one bulk row-segment transfer per row and reuse the same checkpoint range for
 * each column tile.
 */
template <class SrcT, class SrcStoreT, class CheckpointStoreT, class SinkT>
Result<> RecursiveGaussian2DYAxisToSink(const SrcStoreT& sourceStore, const SizeVec3& dims, const RecursiveGaussianPlaneAxis& pass, const detail::RecursiveGaussian2DBufferPlan& plan,
                                        CheckpointStoreT& checkpointStore, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler, SinkT&& sink)
{
  if(pass.axis != 1 || dims[2] != 1 || dims[0] == 0 || dims[1] < 4 || !plan.valid || plan.coreCols == 0 || plan.blockRows < 4 || plan.blockCount == 0)
  {
    return MakeErrorResult(-23613,
                           fmt::format("Recursive Gaussian bounded 2D Y pass received an invalid axis, dimension, or buffer plan for dimensions ({}).", StringUtilities::formatDimensions3D(dims)));
  }
  usize totalValues = 0;
  usize checkpointValues = 0;
  if(!detail::RecursiveGaussianCheckedMultiply(dims[0], dims[1], totalValues) || !detail::RecursiveGaussianCheckedMultiply(plan.blockCount, plan.coreCols, checkpointValues))
  {
    return MakeErrorResult(-23614, fmt::format("Recursive Gaussian bounded 2D Y pass dimensions ({}) overflow addressable storage.", StringUtilities::formatDimensions3D(dims)));
  }
  if(sourceStore.getSize() != totalValues || checkpointStore.getSize() < checkpointValues)
  {
    return MakeErrorResult(-23615, fmt::format("Recursive Gaussian bounded 2D Y pass expected {} source values and at least {} checkpoint records, but received {} and {}.", totalValues,
                                               checkpointValues, sourceStore.getSize(), checkpointStore.getSize()));
  }

  const detail::RecursiveGaussianCoefficients coefficients = detail::ComputeRecursiveGaussianCoefficients(pass.sigma, pass.spacing, pass.order, pass.normalizeAcrossScale);
  std::vector<SrcT> source;
  std::vector<double> causal;
  std::vector<float32> filtered;
  std::vector<detail::RecursiveGaussian2DCheckpoint> causalStates;
  std::vector<std::array<double, 4>> antiCausalStates;

  MessageHelper messageHelper(messageHandler);
  auto progressHelper = messageHelper.createProgressMessageHelper();
  progressHelper.setMaxProgresss((plan.blockCount * 2) * (1 + (dims[0] - 1) / plan.coreCols));
  progressHelper.setProgressMessageTemplate("Recursive Gaussian: bounded 2D Y pass ({:.1f}%)");
  auto progressMessenger = progressHelper.createProgressMessenger(std::chrono::milliseconds(1000));

  for(usize xBegin = 0; xBegin < dims[0]; xBegin += plan.coreCols)
  {
    const usize columnCount = std::min(plan.coreCols, dims[0] - xBegin);
    causalStates.assign(columnCount, {});

    // Forward recurrence: retain only the state immediately before every non-first block.
    for(usize blockIndex = 0; blockIndex < plan.blockCount; ++blockIndex)
    {
      if(shouldCancel)
      {
        return {};
      }
      if(blockIndex > 0)
      {
        if(Result<> result = checkpointStore.copyFromBuffer(blockIndex * plan.coreCols, nonstd::span<const detail::RecursiveGaussian2DCheckpoint>(causalStates.data(), columnCount)); result.invalid())
        {
          return result;
        }
      }
      const usize yBegin = detail::RecursiveGaussian2DBlockBegin(blockIndex, plan);
      const usize rowCount = detail::RecursiveGaussian2DBlockRows(blockIndex, plan, dims[1]);
      source.resize(rowCount * columnCount);
      causal.resize(rowCount * columnCount);
      if(Result<> result = detail::ReadRecursiveGaussian2DBlock<SrcT>(sourceStore, dims[0], yBegin, rowCount, xBegin, columnCount, nonstd::span<SrcT>(source.data(), source.size())); result.invalid())
      {
        return result;
      }
      const usize columnBatchCount = (columnCount + 3) / 4;
      auto computeForward = [&](const Range& batchRange) {
        for(usize batchIndex = batchRange.min(); batchIndex < batchRange.max(); ++batchIndex)
        {
          const usize firstColumn = batchIndex * 4;
          const usize lanes = std::min<usize>(4, columnCount - firstColumn);
          if(lanes == 4)
          {
            detail::ComputeFourRecursiveGaussianCausalBlocks<SrcT>(nonstd::span<const SrcT>(source.data(), source.size()), rowCount, columnCount, firstColumn, blockIndex == 0, coefficients,
                                                                   nonstd::span<detail::RecursiveGaussian2DCheckpoint>(causalStates.data(), causalStates.size()),
                                                                   nonstd::span<double>(causal.data(), causal.size()));
          }
          else
          {
            for(usize lane = 0; lane < lanes; ++lane)
            {
              const usize column = firstColumn + lane;
              detail::ComputeRecursiveGaussianCausalBlock<SrcT>(nonstd::span<const SrcT>(source.data(), source.size()), rowCount, columnCount, column, blockIndex == 0, coefficients,
                                                                causalStates[column], nonstd::span<double>(causal.data(), causal.size()));
            }
          }
        }
      };
      ParallelDataAlgorithm parallelAlgorithm;
      parallelAlgorithm.setRange(0, columnBatchCount);
      parallelAlgorithm.execute(computeForward);
      progressMessenger.sendProgressMessage(1);
    }

    antiCausalStates.assign(columnCount, {});
    for(usize reverseIndex = plan.blockCount; reverseIndex > 0; --reverseIndex)
    {
      if(shouldCancel)
      {
        return {};
      }
      const usize blockIndex = reverseIndex - 1;
      const usize yBegin = detail::RecursiveGaussian2DBlockBegin(blockIndex, plan);
      const usize rowCount = detail::RecursiveGaussian2DBlockRows(blockIndex, plan, dims[1]);
      const usize yEnd = yBegin + rowCount;
      const usize haloRows = std::min<usize>(4, dims[1] - yEnd);
      source.resize((rowCount + haloRows) * columnCount);
      causal.resize(rowCount * columnCount);
      filtered.resize(rowCount * columnCount);
      if(Result<> result = detail::ReadRecursiveGaussian2DBlock<SrcT>(sourceStore, dims[0], yBegin, rowCount, xBegin, columnCount, nonstd::span<SrcT>(source.data(), rowCount * columnCount));
         result.invalid())
      {
        return result;
      }
      if(haloRows > 0)
      {
        if(Result<> result = detail::ReadRecursiveGaussian2DBlock<SrcT>(sourceStore, dims[0], yEnd, haloRows, xBegin, columnCount,
                                                                        nonstd::span<SrcT>(source.data() + rowCount * columnCount, haloRows * columnCount));
           result.invalid())
        {
          return result;
        }
      }
      if(blockIndex == 0)
      {
        std::fill(causalStates.begin(), causalStates.end(), detail::RecursiveGaussian2DCheckpoint{});
      }
      else if(Result<> result = checkpointStore.copyIntoBuffer(blockIndex * plan.coreCols, nonstd::span<detail::RecursiveGaussian2DCheckpoint>(causalStates.data(), columnCount)); result.invalid())
      {
        return result;
      }
      const usize columnBatchCount = (columnCount + 3) / 4;
      auto computeBackward = [&](const Range& batchRange) {
        for(usize batchIndex = batchRange.min(); batchIndex < batchRange.max(); ++batchIndex)
        {
          const usize firstColumn = batchIndex * 4;
          const usize lanes = std::min<usize>(4, columnCount - firstColumn);
          if(lanes == 4)
          {
            detail::ComputeFourRecursiveGaussianCausalBlocks<SrcT>(nonstd::span<const SrcT>(source.data(), source.size()), rowCount, columnCount, firstColumn, blockIndex == 0, coefficients,
                                                                   nonstd::span<detail::RecursiveGaussian2DCheckpoint>(causalStates.data(), causalStates.size()),
                                                                   nonstd::span<double>(causal.data(), causal.size()));
            detail::ComputeFourRecursiveGaussianAntiCausalBlocks<SrcT>(
                nonstd::span<const SrcT>(source.data(), source.size()), nonstd::span<const double>(causal.data(), causal.size()), rowCount, columnCount, firstColumn, blockIndex + 1 == plan.blockCount,
                coefficients, nonstd::span<std::array<double, 4>>(antiCausalStates.data(), antiCausalStates.size()), nonstd::span<float32>(filtered.data(), filtered.size()));
          }
          else
          {
            for(usize lane = 0; lane < lanes; ++lane)
            {
              const usize column = firstColumn + lane;
              detail::ComputeRecursiveGaussianCausalBlock<SrcT>(nonstd::span<const SrcT>(source.data(), source.size()), rowCount, columnCount, column, blockIndex == 0, coefficients,
                                                                causalStates[column], nonstd::span<double>(causal.data(), causal.size()));
              detail::ComputeRecursiveGaussianAntiCausalBlock<SrcT>(nonstd::span<const SrcT>(source.data(), source.size()), nonstd::span<const double>(causal.data(), causal.size()), rowCount,
                                                                    columnCount, column, blockIndex + 1 == plan.blockCount, coefficients, antiCausalStates[column],
                                                                    nonstd::span<float32>(filtered.data(), filtered.size()));
            }
          }
        }
      };
      ParallelDataAlgorithm parallelAlgorithm;
      parallelAlgorithm.setRange(0, columnBatchCount);
      parallelAlgorithm.execute(computeBackward);
      if(shouldCancel)
      {
        return {};
      }
      if(Result<> result = sink(yBegin, xBegin, rowCount, columnCount, nonstd::span<float32>(filtered.data(), filtered.size())); result.invalid())
      {
        return result;
      }
      progressMessenger.sendProgressMessage(1);
    }
  }
  return {};
}

template <class T, class StoreT>
Result<> WriteRecursiveGaussian2DBlock(StoreT& store, usize dimX, usize yBegin, usize xBegin, usize rowCount, usize columnCount, nonstd::span<const T> values)
{
  if(columnCount == dimX)
  {
    return store.copyFromBuffer(yBegin * dimX, values.subspan(0, rowCount * columnCount));
  }
  for(usize row = 0; row < rowCount; ++row)
  {
    if(Result<> result = store.copyFromBuffer((yBegin + row) * dimX + xBegin, values.subspan(row * columnCount, columnCount)); result.invalid())
    {
      return result;
    }
  }
  return {};
}

template <class SrcT, class SrcStoreT, class CheckpointStoreT, class SinkT>
Result<> RecursiveGaussian2DTiledXAxisToSink(const SrcStoreT& sourceStore, CheckpointStoreT& checkpointStore, const SizeVec3& dims, const RecursiveGaussianPlaneAxis& pass,
                                             const detail::RecursiveGaussian2DBufferPlan& plan, const std::atomic_bool& shouldCancel, SinkT&& sink)
{
  const usize blockValues = detail::RecursiveGaussian2DXBlockValues(dims[0], plan.coreCols);
  const usize blockCount = detail::RecursiveGaussian2DXBlockCount(dims[0], plan.coreCols);
  if(dims[0] - (blockCount - 1) * blockValues < 4 || checkpointStore.getSize() < blockCount)
  {
    return MakeErrorResult(
        -23623, fmt::format("Recursive Gaussian bounded 2D X tiles cannot represent a four-value recurrence block for width {} with {} checkpoint records.", dims[0], checkpointStore.getSize()));
  }

  const detail::RecursiveGaussianCoefficients coefficients = detail::ComputeRecursiveGaussianCoefficients(pass.sigma, pass.spacing, pass.order, pass.normalizeAcrossScale);
  std::vector<SrcT> source;
  std::vector<double> causal;
  std::vector<float32> filtered;
  for(usize y = 0; y < dims[1]; ++y)
  {
    detail::RecursiveGaussian2DCheckpoint causalState;
    for(usize blockIndex = 0; blockIndex < blockCount; ++blockIndex)
    {
      if(shouldCancel)
      {
        return {};
      }
      if(blockIndex > 0)
      {
        if(Result<> result = checkpointStore.copyFromBuffer(blockIndex, nonstd::span<const detail::RecursiveGaussian2DCheckpoint>(&causalState, 1)); result.invalid())
        {
          return result;
        }
      }
      const usize xBegin = blockIndex * blockValues;
      const usize valueCount = std::min(blockValues, dims[0] - xBegin);
      source.resize(valueCount);
      causal.resize(valueCount);
      if(Result<> result = sourceStore.copyIntoBuffer(y * dims[0] + xBegin, nonstd::span<SrcT>(source.data(), source.size())); result.invalid())
      {
        return result;
      }
      detail::ComputeRecursiveGaussianCausalBlock<SrcT>(nonstd::span<const SrcT>(source.data(), source.size()), valueCount, 1, 0, blockIndex == 0, coefficients, causalState,
                                                        nonstd::span<double>(causal.data(), causal.size()));
    }

    std::array<double, 4> antiState{};
    for(usize reverseIndex = blockCount; reverseIndex > 0; --reverseIndex)
    {
      if(shouldCancel)
      {
        return {};
      }
      const usize blockIndex = reverseIndex - 1;
      const usize xBegin = blockIndex * blockValues;
      const usize valueCount = std::min(blockValues, dims[0] - xBegin);
      const usize haloValues = std::min<usize>(4, dims[0] - (xBegin + valueCount));
      source.resize(valueCount + haloValues);
      causal.resize(valueCount);
      filtered.resize(valueCount);
      if(Result<> result = sourceStore.copyIntoBuffer(y * dims[0] + xBegin, nonstd::span<SrcT>(source.data(), source.size())); result.invalid())
      {
        return result;
      }
      if(blockIndex == 0)
      {
        causalState = {};
      }
      else if(Result<> result = checkpointStore.copyIntoBuffer(blockIndex, nonstd::span<detail::RecursiveGaussian2DCheckpoint>(&causalState, 1)); result.invalid())
      {
        return result;
      }
      detail::ComputeRecursiveGaussianCausalBlock<SrcT>(nonstd::span<const SrcT>(source.data(), source.size()), valueCount, 1, 0, blockIndex == 0, coefficients, causalState,
                                                        nonstd::span<double>(causal.data(), causal.size()));
      detail::ComputeRecursiveGaussianAntiCausalBlock<SrcT>(nonstd::span<const SrcT>(source.data(), source.size()), nonstd::span<const double>(causal.data(), causal.size()), valueCount, 1, 0,
                                                            blockIndex + 1 == blockCount, coefficients, antiState, nonstd::span<float32>(filtered.data(), filtered.size()));
      if(Result<> result = sink(y, xBegin, 1, valueCount, nonstd::span<float32>(filtered.data(), filtered.size())); result.invalid())
      {
        return result;
      }
    }
  }
  return {};
}

template <class SrcT, class SrcStoreT, class CheckpointStoreT, class SinkT>
Result<> RecursiveGaussian2DXAxisToSink(const SrcStoreT& sourceStore, const SizeVec3& dims, const RecursiveGaussianPlaneAxis& pass, const detail::RecursiveGaussian2DBufferPlan& plan,
                                        CheckpointStoreT& checkpointStore, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler, SinkT&& sink)
{
  if(pass.axis != 0 || dims[2] != 1 || dims[0] < 4 || dims[1] == 0 || !plan.valid || plan.blockRows == 0)
  {
    return MakeErrorResult(-23616,
                           fmt::format("Recursive Gaussian bounded 2D X pass received an invalid axis, dimension, or buffer plan for dimensions ({}).", StringUtilities::formatDimensions3D(dims)));
  }
  usize totalValues = 0;
  if(!detail::RecursiveGaussianCheckedMultiply(dims[0], dims[1], totalValues) || sourceStore.getSize() != totalValues)
  {
    return MakeErrorResult(-23617, fmt::format("Recursive Gaussian bounded 2D X pass expected {} values for dimensions ({}), but received {}.", totalValues, StringUtilities::formatDimensions3D(dims),
                                               sourceStore.getSize()));
  }
  if(plan.coreCols != dims[0])
  {
    return RecursiveGaussian2DTiledXAxisToSink<SrcT>(sourceStore, checkpointStore, dims, pass, plan, shouldCancel, std::forward<SinkT>(sink));
  }

  usize rowsPerBlock = plan.blockRows;
  const std::optional<ShapeType> sourceChunkShape = sourceStore.getChunkShape();
  if(sourceChunkShape.has_value() && sourceChunkShape->size() >= 3 && (*sourceChunkShape)[0] == 1 && (*sourceChunkShape)[1] > 0 && (*sourceChunkShape)[1] <= rowsPerBlock &&
     (*sourceChunkShape)[2] == dims[0])
  {
    rowsPerBlock = (rowsPerBlock / (*sourceChunkShape)[1]) * (*sourceChunkShape)[1];
  }
  std::vector<SrcT> source;
  std::vector<float32> filtered;
  const detail::RecursiveGaussianCoefficients coefficients = detail::ComputeRecursiveGaussianCoefficients(pass.sigma, pass.spacing, pass.order, pass.normalizeAcrossScale);

  MessageHelper messageHelper(messageHandler);
  auto progressHelper = messageHelper.createProgressMessageHelper();
  progressHelper.setMaxProgresss(1 + (dims[1] - 1) / rowsPerBlock);
  progressHelper.setProgressMessageTemplate("Recursive Gaussian: bounded 2D X pass ({:.1f}%)");
  auto progressMessenger = progressHelper.createProgressMessenger(std::chrono::milliseconds(1000));

  for(usize yBegin = 0; yBegin < dims[1]; yBegin += rowsPerBlock)
  {
    if(shouldCancel)
    {
      return {};
    }
    const usize rowCount = std::min(rowsPerBlock, dims[1] - yBegin);
    source.resize(rowCount * dims[0]);
    filtered.resize(rowCount * dims[0]);
    if(Result<> result = sourceStore.copyIntoBuffer(yBegin * dims[0], nonstd::span<SrcT>(source.data(), source.size())); result.invalid())
    {
      return result;
    }
    const SizeVec3 blockDims{dims[0], rowCount, 1};
    detail::FilterFourRecursiveGaussianXRows<SrcT>(nonstd::span<const SrcT>(source.data(), source.size()), nonstd::span<float32>(filtered.data(), filtered.size()), blockDims[0], blockDims[1],
                                                   coefficients, shouldCancel);
    if(shouldCancel)
    {
      return {};
    }
    if(Result<> result = sink(yBegin, 0, rowCount, dims[0], nonstd::span<float32>(filtered.data(), filtered.size())); result.invalid())
    {
      return result;
    }
    progressMessenger.sendProgressMessage(1);
  }
  return {};
}

template <class SrcT, class SrcStoreT, class WorkStoreT, class CheckpointStoreT, class SinkT>
Result<> RecursiveGaussian2DXYCascadeToSink(const SrcStoreT& sourceStore, WorkStoreT& workStore, CheckpointStoreT& checkpointStore, const SizeVec3& dims, const RecursiveGaussianPlaneAxis& firstPass,
                                            const RecursiveGaussianPlaneAxis& secondPass, const detail::RecursiveGaussian2DBufferPlan& plan, const std::atomic_bool& shouldCancel,
                                            const IFilter::MessageHandler& messageHandler, SinkT&& sink)
{
  if(firstPass.axis != 0 || secondPass.axis != 1)
  {
    return MakeErrorResult(-23618, fmt::format("Recursive Gaussian bounded 2D XY cascade requires axes 0 then 1, but received {} then {}.", firstPass.axis, secondPass.axis));
  }
  auto workSink = [&](usize yBegin, usize xBegin, usize rowCount, usize columnCount, nonstd::span<float32> values) {
    return WriteRecursiveGaussian2DBlock<float32>(workStore, dims[0], yBegin, xBegin, rowCount, columnCount, nonstd::span<const float32>(values.data(), values.size()));
  };
  if(Result<> result = RecursiveGaussian2DXAxisToSink<SrcT>(sourceStore, dims, firstPass, plan, checkpointStore, shouldCancel, messageHandler, workSink); result.invalid())
  {
    return result;
  }
  return RecursiveGaussian2DYAxisToSink<float32>(workStore, dims, secondPass, plan, checkpointStore, shouldCancel, messageHandler, std::forward<SinkT>(sink));
}

template <class SrcT, class SrcStoreT, class WorkStoreT, class CheckpointStoreT, class SinkT>
Result<> RecursiveGaussian2DYXCascadeToSink(const SrcStoreT& sourceStore, WorkStoreT& workStore, CheckpointStoreT& checkpointStore, const SizeVec3& dims, const RecursiveGaussianPlaneAxis& firstPass,
                                            const RecursiveGaussianPlaneAxis& secondPass, const detail::RecursiveGaussian2DBufferPlan& plan, const std::atomic_bool& shouldCancel,
                                            const IFilter::MessageHandler& messageHandler, SinkT&& sink)
{
  if(firstPass.axis != 1 || secondPass.axis != 0)
  {
    return MakeErrorResult(-23619, fmt::format("Recursive Gaussian bounded 2D YX cascade requires axes 1 then 0, but received axes {} then {}.", firstPass.axis, secondPass.axis));
  }
  if(plan.coreCols != dims[0])
  {
    auto workSink = [&](usize yBegin, usize xBegin, usize rowCount, usize columnCount, nonstd::span<float32> values) {
      return WriteRecursiveGaussian2DBlock<float32>(workStore, dims[0], yBegin, xBegin, rowCount, columnCount, nonstd::span<const float32>(values.data(), values.size()));
    };
    if(Result<> result = RecursiveGaussian2DYAxisToSink<SrcT>(sourceStore, dims, firstPass, plan, checkpointStore, shouldCancel, messageHandler, workSink); result.invalid())
    {
      return result;
    }
    std::vector<float32> terminalScratch;
    auto terminalSink = [&](usize yBegin, usize xBegin, usize rowCount, usize columnCount, nonstd::span<float32> filtered) -> Result<> {
      terminalScratch.resize(filtered.size());
      return sink(yBegin, xBegin, rowCount, columnCount, filtered, nonstd::span<float32>(terminalScratch.data(), terminalScratch.size()));
    };
    return RecursiveGaussian2DXAxisToSink<float32>(workStore, dims, secondPass, plan, checkpointStore, shouldCancel, messageHandler, terminalSink);
  }
  const detail::RecursiveGaussianCoefficients secondCoefficients =
      detail::ComputeRecursiveGaussianCoefficients(secondPass.sigma, secondPass.spacing, secondPass.order, secondPass.normalizeAcrossScale);
  std::vector<float32> secondOutput;
  auto firstSink = [&](usize yBegin, usize xBegin, usize rowCount, usize columnCount, nonstd::span<float32> firstOutput) -> Result<> {
    secondOutput.resize(firstOutput.size());
    const SizeVec3 blockDims{dims[0], rowCount, 1};
    detail::FilterFourRecursiveGaussianXRows<float32>(nonstd::span<const float32>(firstOutput.data(), firstOutput.size()), nonstd::span<float32>(secondOutput.data(), secondOutput.size()),
                                                      blockDims[0], blockDims[1], secondCoefficients, shouldCancel);
    if(shouldCancel)
    {
      return {};
    }
    return sink(yBegin, xBegin, rowCount, columnCount, nonstd::span<float32>(secondOutput.data(), secondOutput.size()), firstOutput);
  };
  return RecursiveGaussian2DYAxisToSink<SrcT>(sourceStore, dims, firstPass, plan, checkpointStore, shouldCancel, messageHandler, firstSink);
}

/**
 * @brief Streams each XY plane through two recursive-Gaussian passes and forwards the final float32 plane to a sink.
 *
 * Both pass axes must be distinct members of {0, 1}. Store access is
 * serial, while independent lines within each
 * staged plane are filtered in parallel. The float32 first-pass plane is deliberately materialized before the second
 * recurrence to preserve ITK's
 * inter-pass rounding. The sink runs only after both worker groups have completed and a
 * cancellation check has succeeded, so cancellation never exposes a partially computed plane.
 */
template <class SrcT, class SrcStoreT, class SinkT>
Result<> RecursiveGaussianXYPlaneCascadeToSink(const SrcStoreT& src, const SizeVec3& dims, const RecursiveGaussianPlaneAxis& firstPass, const RecursiveGaussianPlaneAxis& secondPass,
                                               const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler, SinkT&& sink)
{
  if(firstPass.axis > 1 || secondPass.axis > 1 || firstPass.axis == secondPass.axis)
  {
    return MakeErrorResult(-23600, fmt::format("Recursive Gaussian XY plane cascade requires two distinct X/Y axes, but received axes {} and {}.", firstPass.axis, secondPass.axis));
  }

  const usize firstLineLength = firstPass.axis == 0 ? dims[0] : dims[1];
  const usize secondLineLength = secondPass.axis == 0 ? dims[0] : dims[1];
  if(firstLineLength < 4 || secondLineLength < 4)
  {
    return MakeErrorResult(-23603,
                           fmt::format("Recursive Gaussian XY plane cascade requires at least 4 pixels along each filtered axis. Dimensions are ({}); axis {} has {} pixels and axis {} has {} pixels.",
                                       StringUtilities::formatDimensions3D(dims), firstPass.axis, firstLineLength, secondPass.axis, secondLineLength));
  }

  usize planeValues = 0;
  usize totalValues = 0;
  if(!detail::RecursiveGaussianCheckedMultiply(dims[0], dims[1], planeValues) || !detail::RecursiveGaussianCheckedMultiply(planeValues, dims[2], totalValues))
  {
    return MakeErrorResult(-23601, fmt::format("Recursive Gaussian XY plane cascade dimensions ({}) overflow the addressable value count.", StringUtilities::formatDimensions3D(dims)));
  }
  if(planeValues == 0 || dims[2] == 0 || src.getSize() != totalValues)
  {
    return MakeErrorResult(-23602,
                           fmt::format("Recursive Gaussian XY plane cascade expected {} scalar values for dimensions ({}), but the source store contains {}. Ensure the source is a single-component "
                                       "array whose tuple shape matches the Image Geometry.",
                                       totalValues, StringUtilities::formatDimensions3D(dims), src.getSize()));
  }

  std::vector<SrcT> srcPlane(planeValues);
  std::vector<float32> firstPlane(planeValues);
  std::vector<float32> secondPlane(planeValues);
  const detail::RecursiveGaussianCoefficients firstCoefficients = detail::ComputeRecursiveGaussianCoefficients(firstPass.sigma, firstPass.spacing, firstPass.order, firstPass.normalizeAcrossScale);
  const detail::RecursiveGaussianCoefficients secondCoefficients =
      detail::ComputeRecursiveGaussianCoefficients(secondPass.sigma, secondPass.spacing, secondPass.order, secondPass.normalizeAcrossScale);

  MessageHelper messageHelper(messageHandler);
  auto progressHelper = messageHelper.createProgressMessageHelper();
  progressHelper.setMaxProgresss(dims[2]);
  progressHelper.setProgressMessageTemplate("Recursive Gaussian: filtering XY planes ({:.1f}%)");
  auto progressMessenger = progressHelper.createProgressMessenger(std::chrono::milliseconds(1000));

  for(usize z = 0; z < dims[2]; ++z)
  {
    if(shouldCancel)
    {
      return {};
    }
    const usize planeOffset = z * planeValues;
    if(Result<> result = src.copyIntoBuffer(planeOffset, nonstd::span<SrcT>(srcPlane.data(), planeValues)); result.invalid())
    {
      return result;
    }

    detail::FilterRecursiveGaussianXYPlane<SrcT>(nonstd::span<const SrcT>(srcPlane.data(), planeValues), nonstd::span<float32>(firstPlane.data(), planeValues), dims, firstPass, firstCoefficients,
                                                 shouldCancel);
    if(shouldCancel)
    {
      return {};
    }
    detail::FilterRecursiveGaussianXYPlane<float32>(nonstd::span<const float32>(firstPlane.data(), planeValues), nonstd::span<float32>(secondPlane.data(), planeValues), dims, secondPass,
                                                    secondCoefficients, shouldCancel);
    if(shouldCancel)
    {
      return {};
    }
    if(Result<> result = sink(planeOffset, nonstd::span<float32>(secondPlane.data(), planeValues)); result.invalid())
    {
      return result;
    }
    progressMessenger.sendProgressMessage(1);
  }
  return {};
}

// One separable recursive-Gaussian pass along `axis` (0=x,1=y,2=z). Reads `src` (any scalar SrcT), promotes each line
// to double, runs the Deriche IIR in double, casts to float32 into `dst` (matching ITK's float32 InternalRealType
// intermediate). `src` may alias `dst` (both float32) for in-place later passes — each line/plane/slab is fully read
// before it is written. The filtered axis must have >= 4 pixels (caller-validated).
template <class SrcT, class SrcStoreT, class DstStoreT>
Result<> RecursiveGaussianAxisPass(const SrcStoreT& src, DstStoreT& dst, const SizeVec3& dims, uint32 axis, double sigma, double spacing, int order, bool normalizeAcrossScale,
                                   const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler, const RecursiveGaussianPassOptions& options = {})
{
  const int64 nX = static_cast<int64>(dims[0]);
  const int64 nY = static_cast<int64>(dims[1]);
  const int64 nZ = static_cast<int64>(dims[2]);
  const usize slice = static_cast<usize>(nX) * static_cast<usize>(nY);

  const detail::RecursiveGaussianCoefficients coeffs = detail::ComputeRecursiveGaussianCoefficients(sigma, spacing, order, normalizeAcrossScale);

  const int64 ln = (axis == 0) ? nX : ((axis == 1) ? nY : nZ);
  assert(ln >= 4); // the >= 4 contract is enforced by preflight ValidateSeparableImageDims; assert guards the engine boundary

  const auto* inMemorySrc = detail::GetRecursiveGaussianDataStore<SrcT>(src);
  auto* inMemoryDst = detail::GetRecursiveGaussianDataStore<float32>(dst);
  if(options.useInMemoryFastPath && inMemorySrc != nullptr && inMemoryDst != nullptr && axis == 0)
  {
    if(shouldCancel)
    {
      return {};
    }
    const nonstd::span<const SrcT> srcValues = inMemorySrc->createSpan();
    nonstd::span<float32> dstValues = inMemoryDst->createSpan();
    detail::FilterFourRecursiveGaussianXRows(srcValues, dstValues, static_cast<usize>(nX), static_cast<usize>(nY * nZ), coeffs, shouldCancel);
    return {};
  }
  if(options.useInMemoryFastPath && inMemorySrc != nullptr && inMemoryDst != nullptr && axis != 0)
  {
    if(shouldCancel)
    {
      return {};
    }

    const nonstd::span<const SrcT> srcValues = inMemorySrc->createSpan();
    nonstd::span<float32> dstValues = inMemoryDst->createSpan();
    const usize lineLength = static_cast<usize>(ln);
    const usize linesPerPlane = axis == 1 ? static_cast<usize>(nX) : slice;
    const usize batchesPerPlane = (linesPerPlane + 3) / 4;
    const usize planeCount = axis == 1 ? static_cast<usize>(nZ) : 1;
    const usize stride = axis == 1 ? static_cast<usize>(nX) : slice;

    auto filterLineBatches = [&](const Range& batchRange) {
      static thread_local std::vector<double> batchData;
      static thread_local std::vector<double> batchOuts;
      static thread_local std::vector<double> tailData;
      static thread_local std::vector<double> tailOuts;
      batchData.resize(lineLength * 4);
      batchOuts.resize(lineLength * 4);
      tailData.resize(lineLength);
      tailOuts.resize(lineLength);

      for(usize batchIndex = batchRange.min(); batchIndex < batchRange.max(); ++batchIndex)
      {
        if(shouldCancel)
        {
          return;
        }
        const usize plane = batchIndex / batchesPerPlane;
        const usize firstLine = (batchIndex % batchesPerPlane) * 4;
        const usize lanes = std::min<usize>(4, linesPerPlane - firstLine);
        const usize base = plane * slice + firstLine;

        if(lanes == 4)
        {
          for(usize i = 0; i < lineLength; ++i)
          {
            const usize source = base + i * stride;
            for(usize lane = 0; lane < 4; ++lane)
            {
              batchData[i * 4 + lane] = static_cast<double>(srcValues[source + lane]);
            }
          }
          detail::FilterFourDataArrays(batchOuts.data(), batchData.data(), lineLength, coeffs);
          for(usize i = 0; i < lineLength; ++i)
          {
            const usize destination = base + i * stride;
            for(usize lane = 0; lane < 4; ++lane)
            {
              dstValues[destination + lane] = static_cast<float32>(batchOuts[i * 4 + lane]);
            }
          }
        }
        else
        {
          for(usize lane = 0; lane < lanes; ++lane)
          {
            for(usize i = 0; i < lineLength; ++i)
            {
              tailData[i] = static_cast<double>(srcValues[base + lane + i * stride]);
            }
            detail::FilterDataArray(tailOuts.data(), tailData.data(), lineLength, coeffs);
            for(usize i = 0; i < lineLength; ++i)
            {
              dstValues[base + lane + i * stride] = static_cast<float32>(tailOuts[i]);
            }
          }
        }
      }
    };

    ParallelDataAlgorithm parallelAlgorithm;
    parallelAlgorithm.setRange(0, batchesPerPlane * planeCount);
    parallelAlgorithm.execute(filterLineBatches);
    return {};
  }

  // NOTE: the per-line IIR scratch (`data`/`outs`/`scratch`) is NOT allocated here. Each axis pass stages one
  // plane/slab into a LOCAL buffer (serial, one resident at a time -> bounded memory) and then filters its
  // independent 1-D lines with a ParallelDataAlgorithm; every worker allocates its OWN thread-local scratch so
  // there is no shared mutable state. The parallel region touches only the staged local buffers -- never the
  // store -- so it is byte-exact and safe for BOTH in-core and OOC with no store-thread-safety concern (each
  // output line depends only on its own input line + read-only coeffs, so reordering lines changes nothing).

  // Throttled progress over the pass's OUTER loop (per-z for the X/Y passes, per-y for the Z pass). Emits ONLY when a
  // new ~10% boundary is crossed -- never per iteration, which would spam thousands of messages on a large volume.
  // Progress is messages ONLY; it never touches store contents, so numerical output is unchanged.
  int lastReportedTenth = -1;
  auto reportProgress = [&](int64 done, int64 outerTotal) {
    if(outerTotal <= 0)
    {
      return;
    }
    const int tenth = static_cast<int>((done * 10) / outerTotal); // 0..9 as `done` runs 0..outerTotal-1
    if(tenth != lastReportedTenth)
    {
      lastReportedTenth = tenth;
      messageHandler(fmt::format("Recursive Gaussian: filtering along axis {} ({}%)", axis, tenth * 10));
    }
  };

  if(axis == 0)
  {
    // X pass: stage a whole z-plane, filter each contiguous nX-row in parallel, write the plane back. Reading the
    // full plane before writing it keeps in-place aliasing (src may alias dst) safe -- and makes this pass memory
    // O(slice), the same bound the Y pass already uses. X-pass lines never cross a plane, so each plane is fully
    // independent of the others.
    std::vector<SrcT> srcPlane(slice);
    std::vector<float32> dstPlane(slice);
    for(int64 z = 0; z < nZ; ++z)
    {
      if(shouldCancel)
      {
        return {};
      }
      reportProgress(z, nZ);
      const usize planeOff = static_cast<usize>(z) * slice;
      if(Result<> r = src.copyIntoBuffer(planeOff, nonstd::span<SrcT>(srcPlane.data(), slice)); r.invalid())
      {
        return r;
      }
      // Parallel over rows [0, nY): each worker filters a disjoint set of contiguous nX-rows with its OWN
      // thread-local scratch, gathering from srcPlane and scattering into dstPlane at disjoint indices. Pure
      // local-buffer compute -- no store access -> no synchronization needed.
      auto filterRows = [&](const Range& rowRange) {
        static thread_local std::vector<double> data;
        static thread_local std::vector<double> outs;
        data.resize(static_cast<usize>(nX));
        outs.resize(static_cast<usize>(nX));
        for(usize y = rowRange.min(); y < rowRange.max(); ++y)
        {
          const usize rowOff = y * static_cast<usize>(nX);
          for(int64 i = 0; i < nX; ++i)
          {
            data[static_cast<usize>(i)] = static_cast<double>(srcPlane[rowOff + static_cast<usize>(i)]);
          }
          detail::FilterDataArray(outs.data(), data.data(), static_cast<usize>(nX), coeffs);
          for(int64 i = 0; i < nX; ++i)
          {
            dstPlane[rowOff + static_cast<usize>(i)] = static_cast<float32>(outs[static_cast<usize>(i)]);
          }
        }
      };
      ParallelDataAlgorithm parallelAlgorithm;
      parallelAlgorithm.setRange(0, static_cast<usize>(nY));
      parallelAlgorithm.execute(filterRows);
      if(Result<> r = dst.copyFromBuffer(planeOff, nonstd::span<const float32>(dstPlane.data(), slice)); r.invalid())
      {
        return r;
      }
    }
    return {};
  }

  if(axis == 1)
  {
    // Y pass: read a whole z-plane, gather each column (stride nX, count nY), write the plane back.
    std::vector<SrcT> srcPlane(slice);
    std::vector<float32> dstPlane(slice);
    for(int64 z = 0; z < nZ; ++z)
    {
      if(shouldCancel)
      {
        return {};
      }
      reportProgress(z, nZ);
      const usize planeOff = static_cast<usize>(z) * slice;
      if(Result<> r = src.copyIntoBuffer(planeOff, nonstd::span<SrcT>(srcPlane.data(), slice)); r.invalid())
      {
        return r;
      }
      // Parallel over columns [0, nX): each worker filters a disjoint set of columns (stride nX, count nY) with
      // its OWN thread-local scratch, gathering from srcPlane and scattering into dstPlane at disjoint indices.
      // Pure local-buffer compute -- no store access -> no synchronization needed.
      auto filterColumns = [&](const Range& colRange) {
        static thread_local std::vector<double> data;
        static thread_local std::vector<double> outs;
        data.resize(static_cast<usize>(nY));
        outs.resize(static_cast<usize>(nY));
        for(usize x = colRange.min(); x < colRange.max(); ++x)
        {
          for(int64 j = 0; j < nY; ++j)
          {
            data[static_cast<usize>(j)] = static_cast<double>(srcPlane[static_cast<usize>(j) * static_cast<usize>(nX) + x]);
          }
          detail::FilterDataArray(outs.data(), data.data(), static_cast<usize>(nY), coeffs);
          for(int64 j = 0; j < nY; ++j)
          {
            dstPlane[static_cast<usize>(j) * static_cast<usize>(nX) + x] = static_cast<float32>(outs[static_cast<usize>(j)]);
          }
        }
      };
      ParallelDataAlgorithm parallelAlgorithm;
      parallelAlgorithm.setRange(0, static_cast<usize>(nX));
      parallelAlgorithm.execute(filterColumns);
      if(Result<> r = dst.copyFromBuffer(planeOff, nonstd::span<const float32>(dstPlane.data(), slice)); r.invalid())
      {
        return r;
      }
    }
    return {};
  }

  // Z pass (3D only): stage consecutive Y rows across every Z plane. A batch converts many small per-row HDF5
  // operations into one contiguous read and write per Z plane while retaining a fixed memory budget. The one-row
  // minimum is an indivisible XZ staging unit and may exceed the requested budget for exceptionally wide/deep data.
  usize stagingByteBudget = options.stagingByteBudget;
  CacheMemoryBudgetManager::WorkingMemoryReservation stagingReservation;
  if(stagingByteBudget == RecursiveGaussianPassOptions::k_FallbackStagingBytes)
  {
    auto saturatingMultiply = [](uint64 left, uint64 right) { return left != 0 && right > std::numeric_limits<uint64>::max() / left ? std::numeric_limits<uint64>::max() : left * right; };
    uint64 usefulBytes = saturatingMultiply(static_cast<uint64>(dims[0]), static_cast<uint64>(dims[1]));
    usefulBytes = saturatingMultiply(usefulBytes, static_cast<uint64>(dims[2]));
    usefulBytes = saturatingMultiply(usefulBytes, static_cast<uint64>(sizeof(SrcT) + sizeof(float32)));
    stagingReservation = ReserveWorkingMemoryFraction(usefulBytes, RecursiveGaussianPassOptions::k_AdaptiveStagingNumerator, RecursiveGaussianPassOptions::k_AdaptiveStagingDenominator,
                                                      RecursiveGaussianPassOptions::k_FallbackStagingBytes);
    if(stagingReservation.sizeBytes() > 0)
    {
      stagingByteBudget = static_cast<usize>(std::min<uint64>(stagingReservation.sizeBytes(), std::numeric_limits<usize>::max()));
    }
  }
  const usize maxYRows = detail::ComputeRecursiveGaussianZBatchRowCount<SrcT>(dims, stagingByteBudget);
  if(stagingReservation.sizeBytes() > 0 && dims[0] <= std::numeric_limits<usize>::max() / dims[2])
  {
    const usize valuesPerRow = dims[0] * dims[2];
    constexpr usize k_BytesPerStagedValue = sizeof(SrcT) + sizeof(float32);
    if(valuesPerRow <= std::numeric_limits<usize>::max() / k_BytesPerStagedValue && maxYRows <= std::numeric_limits<usize>::max() / (valuesPerRow * k_BytesPerStagedValue))
    {
      stagingReservation.shrinkTo(static_cast<uint64>(maxYRows * valuesPerRow * k_BytesPerStagedValue));
    }
  }
  std::vector<SrcT> srcSlab;
  std::vector<float32> dstSlab;
  for(usize yBegin = 0; yBegin < dims[1]; yBegin += maxYRows)
  {
    if(shouldCancel)
    {
      return {};
    }
    reportProgress(static_cast<int64>(yBegin), nY);
    const usize yCount = std::min(maxYRows, dims[1] - yBegin);
    const usize valuesPerPlaneBlock = yCount * dims[0];
    const usize batchValues = valuesPerPlaneBlock * dims[2];
    srcSlab.resize(batchValues);
    dstSlab.resize(batchValues);
    for(usize z = 0; z < dims[2]; ++z)
    {
      if(shouldCancel)
      {
        return {};
      }
      const usize blockOffset = (z * dims[1] + yBegin) * dims[0];
      if(Result<> r = src.copyIntoBuffer(blockOffset, nonstd::span<SrcT>(srcSlab.data() + z * valuesPerPlaneBlock, valuesPerPlaneBlock)); r.invalid())
      {
        return r;
      }
    }
    // Adjacent X/Y lines are independent and occupy adjacent lanes in every staged Z plane. Workers touch only
    // disjoint local-buffer lines; all store access remains outside the parallel region.
    const usize lineCount = valuesPerPlaneBlock;
    const usize lineBatchCount = (lineCount + 3) / 4;
    auto filterZLineBatches = [&](const Range& batchRange) {
      static thread_local std::vector<double> batchData;
      static thread_local std::vector<double> batchOuts;
      static thread_local std::vector<double> tailData;
      static thread_local std::vector<double> tailOuts;
      batchData.resize(dims[2] * 4);
      batchOuts.resize(dims[2] * 4);
      tailData.resize(dims[2]);
      tailOuts.resize(dims[2]);
      for(usize batchIndex = batchRange.min(); batchIndex < batchRange.max(); ++batchIndex)
      {
        if(shouldCancel)
        {
          return;
        }
        const usize firstLine = batchIndex * 4;
        const usize lanes = std::min<usize>(4, lineCount - firstLine);
        if(lanes == 4)
        {
          for(usize z = 0; z < dims[2]; ++z)
          {
            const usize source = z * valuesPerPlaneBlock + firstLine;
            for(usize lane = 0; lane < 4; ++lane)
            {
              batchData[z * 4 + lane] = static_cast<double>(srcSlab[source + lane]);
            }
          }
          detail::FilterFourDataArrays(batchOuts.data(), batchData.data(), dims[2], coeffs);
          for(usize z = 0; z < dims[2]; ++z)
          {
            const usize destination = z * valuesPerPlaneBlock + firstLine;
            for(usize lane = 0; lane < 4; ++lane)
            {
              dstSlab[destination + lane] = static_cast<float32>(batchOuts[z * 4 + lane]);
            }
          }
        }
        else
        {
          for(usize lane = 0; lane < lanes; ++lane)
          {
            for(usize z = 0; z < dims[2]; ++z)
            {
              tailData[z] = static_cast<double>(srcSlab[z * valuesPerPlaneBlock + firstLine + lane]);
            }
            detail::FilterDataArray(tailOuts.data(), tailData.data(), dims[2], coeffs);
            for(usize z = 0; z < dims[2]; ++z)
            {
              dstSlab[z * valuesPerPlaneBlock + firstLine + lane] = static_cast<float32>(tailOuts[z]);
            }
          }
        }
      }
    };
    ParallelDataAlgorithm parallelAlgorithm;
    parallelAlgorithm.setRange(0, lineBatchCount);
    parallelAlgorithm.execute(filterZLineBatches);
    if(shouldCancel)
    {
      return {};
    }
    for(usize z = 0; z < dims[2]; ++z)
    {
      if(shouldCancel)
      {
        return {};
      }
      const usize blockOffset = (z * dims[1] + yBegin) * dims[0];
      if(Result<> r = dst.copyFromBuffer(blockOffset, nonstd::span<const float32>(dstSlab.data() + z * valuesPerPlaneBlock, valuesPerPlaneBlock)); r.invalid())
      {
        return r;
      }
    }
  }
  return {};
}

// ---- Element-wise helpers over float32 stores. Streamed paths use the shared shape-aware pointwise planner so every
// non-final OOC transfer contains complete trailing-dimensional slabs while combined scratch targets at most 64 MiB. ----

namespace detail
{
inline Result<PointwiseBatchPlan> MakeRecursiveGaussianPostprocessingPlan(usize totalValues, usize bytesPerValue, usize requestedScratchBytes, std::initializer_list<PointwiseEndpointLayout> endpoints)
{
  const usize targetScratchBytes = std::min(requestedScratchBytes, k_PointwiseTargetScratchBytes);
  const usize targetValues = std::max<usize>(1, targetScratchBytes / bytesPerValue);
  return MakePointwiseBatchPlan(totalValues, targetValues, endpoints);
}

template <class TransformT>
bool TransformRecursiveGaussianBatch(usize count, const std::atomic_bool& shouldCancel, TransformT&& transform)
{
  constexpr usize k_CancelCheckValues = 65536;
  usize blockStart = 0;
  while(blockStart < count)
  {
    if(shouldCancel)
    {
      return false;
    }
    const usize blockEnd = blockStart + std::min(k_CancelCheckValues, count - blockStart);
    for(usize i = blockStart; i < blockEnd; ++i)
    {
      transform(i);
    }
    blockStart = blockEnd;
  }
  return true;
}

// @p DstStoreT is generalized (rather than fixed to AbstractDataStore<float32>) so this same batched transform serves
// both a real output store and a raw GaussianTemporaryStore<float32> accumulator -- either endpoint exposes the same
// getSize()/copyIntoBuffer()/copyFromBuffer() bulk-I/O surface.
template <class DstStoreT, class TransformT>
Result<> TransformTemporaryGaussianIntoStore(DstStoreT& destination, const GaussianTemporaryStore<float32>& source, bool readDestination, const std::atomic_bool& shouldCancel, TransformT&& transform)
{
  const usize total = destination.getSize();
  if(source.getSize() != total)
  {
    return MakeErrorResult(-23612, fmt::format("Gaussian fixed-record transform source contains {} values, but its destination contains {} values.", source.getSize(), total));
  }
  const usize bytesPerValue = sizeof(float32) * (readDestination ? 2 : 1);
  const usize batchValues = std::max<usize>(1, k_PointwiseTargetScratchBytes / bytesPerValue);
  std::vector<float32> sourceBuffer(std::min(batchValues, total));
  std::vector<float32> destinationBuffer(readDestination ? std::min(batchValues, total) : 0);
  for(usize start = 0; start < total;)
  {
    if(shouldCancel)
    {
      return {};
    }
    const usize count = std::min(batchValues, total - start);
    if(Result<> result = source.copyIntoBuffer(start, nonstd::span<float32>(sourceBuffer.data(), count)); result.invalid())
    {
      return result;
    }
    float32* output = sourceBuffer.data();
    if(readDestination)
    {
      if(Result<> result = destination.copyIntoBuffer(start, nonstd::span<float32>(destinationBuffer.data(), count)); result.invalid())
      {
        return result;
      }
      output = destinationBuffer.data();
    }
    if(!TransformRecursiveGaussianBatch(count, shouldCancel, [&](usize index) { output[index] = transform(output[index], sourceBuffer[index]); }))
    {
      return {};
    }
    if(Result<> result = destination.copyFromBuffer(start, nonstd::span<const float32>(output, count)); result.invalid())
    {
      return result;
    }
    start += count;
  }
  return {};
}
} // namespace detail

// dst[i] = (float32)sqr((double)src[i] / spacing) — GradientMagnitude FIRST-axis OVERWRITE (does NOT read dst). Writing
// the first axis's contribution directly is bit-identical to FillFloat32Store(dst, 0) then AccumulateSquaredOverSpacing:
// the Accumulate adds (double)0.0f, and adding 0.0 in double is exact, so the float32 result is unchanged. This lets the
// first accumulation axis skip a full-volume fill AND the first full-volume dst read.
inline Result<> SetSquaredOverSpacing(AbstractDataStore<float32>& dst, const AbstractDataStore<float32>& src, double spacing, const std::atomic_bool& shouldCancel,
                                      const detail::PointwiseExecutionOptions& options = {})
{
  const usize total = dst.getSize();
  if(src.getSize() != total)
  {
    return MakeErrorResult(-23604, fmt::format("Recursive Gaussian squared-derivative source contains {} values, but its destination contains {} values.", src.getSize(), total));
  }
  const auto* inMemorySrc = dynamic_cast<const DataStore<float32>*>(&src);
  auto* inMemoryDst = dynamic_cast<DataStore<float32>*>(&dst);
  if(!options.forceBuffered && inMemorySrc != nullptr && inMemoryDst != nullptr)
  {
    const nonstd::span<const float32> srcValues = inMemorySrc->createSpan();
    nonstd::span<float32> dstValues = inMemoryDst->createSpan();
    ParallelDataAlgorithm parallelAlgorithm;
    parallelAlgorithm.setRange(0, total);
    parallelAlgorithm.execute([&](const Range& range) {
      detail::TransformRecursiveGaussianBatch(range.max() - range.min(), shouldCancel, [&](usize localIndex) {
        const usize i = range.min() + localIndex;
        const double value = static_cast<double>(srcValues[i]) / spacing;
        dstValues[i] = static_cast<float32>(value * value);
      });
    });
    return {};
  }
  Result<detail::PointwiseBatchPlan> planResult = detail::MakeRecursiveGaussianPostprocessingPlan(
      total, sizeof(float32), options.targetScratchBytes,
      {{src.getStoreType(), src.getTupleShape(), src.getNumberOfComponents(), "squared-derivative input"},
       {dst.getStoreType(), dst.getTupleShape(), dst.getNumberOfComponents(), "squared-derivative output", detail::PointwiseEndpointLayout::Access::Write}});
  if(planResult.invalid())
  {
    return ConvertResult(std::move(planResult));
  }
  const usize batchValues = planResult.value().batchValues;
  std::vector<float32> buf(std::min(batchValues, total));
  for(usize start = 0; start < total;)
  {
    if(shouldCancel)
    {
      return {};
    }
    const usize n = std::min(batchValues, total - start);
    if(Result<> r = src.copyIntoBuffer(start, nonstd::span<float32>(buf.data(), n)); r.invalid())
    {
      return r;
    }
    if(!detail::TransformRecursiveGaussianBatch(n, shouldCancel, [&](usize i) {
         const double bd = static_cast<double>(buf[i]) / spacing;
         buf[i] = static_cast<float32>(bd * bd);
       }))
    {
      return {};
    }
    if(Result<> r = dst.copyFromBuffer(start, nonstd::span<const float32>(buf.data(), n)); r.invalid())
    {
      return r;
    }
    start += n;
  }
  return {};
}

// acc[i] = (float32)((double)acc[i] + sqr((double)deriv[i] / spacing)) — GradientMagnitude accumulation.
inline Result<> AccumulateSquaredOverSpacing(AbstractDataStore<float32>& acc, const AbstractDataStore<float32>& deriv, double spacing, const std::atomic_bool& shouldCancel,
                                             const detail::PointwiseExecutionOptions& options = {})
{
  const usize total = acc.getSize();
  if(deriv.getSize() != total)
  {
    return MakeErrorResult(-23605, fmt::format("Recursive Gaussian derivative source contains {} values, but its accumulator contains {} values.", deriv.getSize(), total));
  }
  auto* inMemoryAcc = dynamic_cast<DataStore<float32>*>(&acc);
  const auto* inMemoryDeriv = dynamic_cast<const DataStore<float32>*>(&deriv);
  if(!options.forceBuffered && inMemoryAcc != nullptr && inMemoryDeriv != nullptr)
  {
    nonstd::span<float32> accValues = inMemoryAcc->createSpan();
    const nonstd::span<const float32> derivValues = inMemoryDeriv->createSpan();
    ParallelDataAlgorithm parallelAlgorithm;
    parallelAlgorithm.setRange(0, total);
    parallelAlgorithm.execute([&](const Range& range) {
      detail::TransformRecursiveGaussianBatch(range.max() - range.min(), shouldCancel, [&](usize localIndex) {
        const usize i = range.min() + localIndex;
        const double value = static_cast<double>(derivValues[i]) / spacing;
        accValues[i] = static_cast<float32>(static_cast<double>(accValues[i]) + value * value);
      });
    });
    return {};
  }
  Result<detail::PointwiseBatchPlan> planResult =
      detail::MakeRecursiveGaussianPostprocessingPlan(total, sizeof(float32) * 2, options.targetScratchBytes,
                                                      {{acc.getStoreType(), acc.getTupleShape(), acc.getNumberOfComponents(), "gradient accumulator", detail::PointwiseEndpointLayout::Access::Write},
                                                       {deriv.getStoreType(), deriv.getTupleShape(), deriv.getNumberOfComponents(), "gradient derivative"}});
  if(planResult.invalid())
  {
    return ConvertResult(std::move(planResult));
  }
  const usize batchValues = planResult.value().batchValues;
  std::vector<float32> a(std::min(batchValues, total));
  std::vector<float32> b(std::min(batchValues, total));
  for(usize start = 0; start < total;)
  {
    if(shouldCancel)
    {
      return {};
    }
    const usize n = std::min(batchValues, total - start);
    if(Result<> r = acc.copyIntoBuffer(start, nonstd::span<float32>(a.data(), n)); r.invalid())
    {
      return r;
    }
    if(Result<> r = deriv.copyIntoBuffer(start, nonstd::span<float32>(b.data(), n)); r.invalid())
    {
      return r;
    }
    if(!detail::TransformRecursiveGaussianBatch(n, shouldCancel, [&](usize i) {
         const double bd = static_cast<double>(b[i]) / spacing;
         a[i] = static_cast<float32>(static_cast<double>(a[i]) + bd * bd);
       }))
    {
      return {};
    }
    if(Result<> r = acc.copyFromBuffer(start, nonstd::span<const float32>(a.data(), n)); r.invalid())
    {
      return r;
    }
    start += n;
  }
  return {};
}

// dst[i] = (float32)((double)src[i] * invSpacingSq) — Laplacian FIRST-axis OVERWRITE (does NOT read dst). Writing the
// first axis's contribution directly is bit-identical to FillFloat32Store(dst, 0) then AccumulateScaledDerivative: the
// Accumulate adds (double)0.0f, and adding 0.0 in double is exact, so the float32 result is unchanged. This lets the
// first accumulation axis skip a full-volume fill AND the first full-volume dst read.
inline Result<> SetScaledDerivative(AbstractDataStore<float32>& dst, const AbstractDataStore<float32>& src, double invSpacingSq, const std::atomic_bool& shouldCancel,
                                    const detail::PointwiseExecutionOptions& options = {})
{
  const usize total = dst.getSize();
  if(src.getSize() != total)
  {
    return MakeErrorResult(-23606, fmt::format("Recursive Gaussian scaled-derivative source contains {} values, but its destination contains {} values.", src.getSize(), total));
  }
  const auto* inMemorySrc = dynamic_cast<const DataStore<float32>*>(&src);
  auto* inMemoryDst = dynamic_cast<DataStore<float32>*>(&dst);
  if(!options.forceBuffered && inMemorySrc != nullptr && inMemoryDst != nullptr)
  {
    const nonstd::span<const float32> srcValues = inMemorySrc->createSpan();
    nonstd::span<float32> dstValues = inMemoryDst->createSpan();
    ParallelDataAlgorithm parallelAlgorithm;
    parallelAlgorithm.setRange(0, total);
    if(invSpacingSq == 1.0)
    {
      parallelAlgorithm.execute([&](const Range& range) {
        if(!shouldCancel)
        {
          std::copy(srcValues.begin() + range.min(), srcValues.begin() + range.max(), dstValues.begin() + range.min());
        }
      });
    }
    else
    {
      parallelAlgorithm.execute([&](const Range& range) {
        detail::TransformRecursiveGaussianBatch(range.max() - range.min(), shouldCancel, [&](usize localIndex) {
          const usize i = range.min() + localIndex;
          dstValues[i] = static_cast<float32>(static_cast<double>(srcValues[i]) * invSpacingSq);
        });
      });
    }
    return {};
  }
  Result<detail::PointwiseBatchPlan> planResult = detail::MakeRecursiveGaussianPostprocessingPlan(
      total, sizeof(float32), options.targetScratchBytes,
      {{src.getStoreType(), src.getTupleShape(), src.getNumberOfComponents(), "scaled-derivative input"},
       {dst.getStoreType(), dst.getTupleShape(), dst.getNumberOfComponents(), "scaled-derivative output", detail::PointwiseEndpointLayout::Access::Write}});
  if(planResult.invalid())
  {
    return ConvertResult(std::move(planResult));
  }
  const usize batchValues = planResult.value().batchValues;
  std::vector<float32> buf(std::min(batchValues, total));
  for(usize start = 0; start < total;)
  {
    if(shouldCancel)
    {
      return {};
    }
    const usize n = std::min(batchValues, total - start);
    if(Result<> r = src.copyIntoBuffer(start, nonstd::span<float32>(buf.data(), n)); r.invalid())
    {
      return r;
    }
    if(invSpacingSq != 1.0 && !detail::TransformRecursiveGaussianBatch(n, shouldCancel, [&](usize i) { buf[i] = static_cast<float32>(static_cast<double>(buf[i]) * invSpacingSq); }))
    {
      return {};
    }
    if(Result<> r = dst.copyFromBuffer(start, nonstd::span<const float32>(buf.data(), n)); r.invalid())
    {
      return r;
    }
    start += n;
  }
  return {};
}

// acc[i] = (float32)((double)acc[i] + (double)deriv[i] * invSpacingSq) — Laplacian accumulation.
inline Result<> AccumulateScaledDerivative(AbstractDataStore<float32>& acc, const AbstractDataStore<float32>& deriv, double invSpacingSq, const std::atomic_bool& shouldCancel,
                                           const detail::PointwiseExecutionOptions& options = {})
{
  const usize total = acc.getSize();
  if(deriv.getSize() != total)
  {
    return MakeErrorResult(-23607, fmt::format("Recursive Gaussian scaled-derivative source contains {} values, but its accumulator contains {} values.", deriv.getSize(), total));
  }
  auto* inMemoryAcc = dynamic_cast<DataStore<float32>*>(&acc);
  const auto* inMemoryDeriv = dynamic_cast<const DataStore<float32>*>(&deriv);
  if(!options.forceBuffered && inMemoryAcc != nullptr && inMemoryDeriv != nullptr)
  {
    nonstd::span<float32> accValues = inMemoryAcc->createSpan();
    const nonstd::span<const float32> derivValues = inMemoryDeriv->createSpan();
    ParallelDataAlgorithm parallelAlgorithm;
    parallelAlgorithm.setRange(0, total);
    if(invSpacingSq == 1.0)
    {
      parallelAlgorithm.execute([&](const Range& range) {
        detail::TransformRecursiveGaussianBatch(range.max() - range.min(), shouldCancel, [&](usize localIndex) {
          const usize i = range.min() + localIndex;
          accValues[i] = static_cast<float32>(static_cast<double>(accValues[i]) + static_cast<double>(derivValues[i]));
        });
      });
    }
    else
    {
      parallelAlgorithm.execute([&](const Range& range) {
        detail::TransformRecursiveGaussianBatch(range.max() - range.min(), shouldCancel, [&](usize localIndex) {
          const usize i = range.min() + localIndex;
          accValues[i] = static_cast<float32>(static_cast<double>(accValues[i]) + static_cast<double>(derivValues[i]) * invSpacingSq);
        });
      });
    }
    return {};
  }
  Result<detail::PointwiseBatchPlan> planResult =
      detail::MakeRecursiveGaussianPostprocessingPlan(total, sizeof(float32) * 2, options.targetScratchBytes,
                                                      {{acc.getStoreType(), acc.getTupleShape(), acc.getNumberOfComponents(), "Laplacian accumulator", detail::PointwiseEndpointLayout::Access::Write},
                                                       {deriv.getStoreType(), deriv.getTupleShape(), deriv.getNumberOfComponents(), "Laplacian derivative"}});
  if(planResult.invalid())
  {
    return ConvertResult(std::move(planResult));
  }
  const usize batchValues = planResult.value().batchValues;
  std::vector<float32> a(std::min(batchValues, total));
  std::vector<float32> b(std::min(batchValues, total));
  for(usize start = 0; start < total;)
  {
    if(shouldCancel)
    {
      return {};
    }
    const usize n = std::min(batchValues, total - start);
    if(Result<> r = acc.copyIntoBuffer(start, nonstd::span<float32>(a.data(), n)); r.invalid())
    {
      return r;
    }
    if(Result<> r = deriv.copyIntoBuffer(start, nonstd::span<float32>(b.data(), n)); r.invalid())
    {
      return r;
    }
    const bool transformed =
        invSpacingSq == 1.0 ?
            detail::TransformRecursiveGaussianBatch(n, shouldCancel, [&](usize i) { a[i] = static_cast<float32>(static_cast<double>(a[i]) + static_cast<double>(b[i])); }) :
            detail::TransformRecursiveGaussianBatch(n, shouldCancel, [&](usize i) { a[i] = static_cast<float32>(static_cast<double>(a[i]) + static_cast<double>(b[i]) * invSpacingSq); });
    if(!transformed)
    {
      return {};
    }
    if(Result<> r = acc.copyFromBuffer(start, nonstd::span<const float32>(a.data(), n)); r.invalid())
    {
      return r;
    }
    start += n;
  }
  return {};
}

// @p DstStoreT is generalized so the destination may be either the real output store (AbstractDataStore<float32>, the
// working-data-format 3-D route) or a raw GaussianTemporaryStore<float32> accumulator (the fixed-record-scratch 3-D
// route) -- both expose the bulk-I/O surface TransformTemporaryGaussianIntoStore needs.
template <class DstStoreT>
Result<> SetSquaredOverSpacing(DstStoreT& destination, const detail::GaussianTemporaryStore<float32>& source, double spacing, const std::atomic_bool& shouldCancel)
{
  return detail::TransformTemporaryGaussianIntoStore(destination, source, false, shouldCancel, [spacing](float32, float32 sourceValue) {
    const double derivative = static_cast<double>(sourceValue) / spacing;
    return static_cast<float32>(derivative * derivative);
  });
}

template <class DstStoreT>
Result<> AccumulateSquaredOverSpacing(DstStoreT& destination, const detail::GaussianTemporaryStore<float32>& source, double spacing, const std::atomic_bool& shouldCancel)
{
  return detail::TransformTemporaryGaussianIntoStore(destination, source, true, shouldCancel, [spacing](float32 destinationValue, float32 sourceValue) {
    const double derivative = static_cast<double>(sourceValue) / spacing;
    return static_cast<float32>(static_cast<double>(destinationValue) + derivative * derivative);
  });
}

template <class DstStoreT>
Result<> SetScaledDerivative(DstStoreT& destination, const detail::GaussianTemporaryStore<float32>& source, double invSpacingSq, const std::atomic_bool& shouldCancel)
{
  return detail::TransformTemporaryGaussianIntoStore(destination, source, false, shouldCancel,
                                                     [invSpacingSq](float32, float32 sourceValue) { return static_cast<float32>(static_cast<double>(sourceValue) * invSpacingSq); });
}

template <class DstStoreT>
Result<> AccumulateScaledDerivative(DstStoreT& destination, const detail::GaussianTemporaryStore<float32>& source, double invSpacingSq, const std::atomic_bool& shouldCancel)
{
  return detail::TransformTemporaryGaussianIntoStore(destination, source, true, shouldCancel, [invSpacingSq](float32 destinationValue, float32 sourceValue) {
    return static_cast<float32>(static_cast<double>(destinationValue) + static_cast<double>(sourceValue) * invSpacingSq);
  });
}

// Streams a finished raw GaussianTemporaryStore<float32> accumulator into @p destination with bounded batched
// transfers -- a plain copy, since the raw accumulator already holds the fully-accumulated (and, for
// GradientMagnitudeRecursiveGaussian, square-rooted) float32 values. This is the single point where the 3-D
// fixed-record-scratch route materializes results into the real (possibly deflate-backed) output store.
template <class DstStoreT>
Result<> MaterializeGaussianTemporaryStore(DstStoreT& destination, const detail::GaussianTemporaryStore<float32>& source, const std::atomic_bool& shouldCancel)
{
  return detail::TransformTemporaryGaussianIntoStore(destination, source, false, shouldCancel, [](float32, float32 sourceValue) { return sourceValue; });
}

// store[i] = (float32)std::sqrt((double)store[i]) — GradientMagnitude final step (ITK SqrtImageFilter casts to double).
inline Result<> SqrtStoreInPlace(AbstractDataStore<float32>& store, const std::atomic_bool& shouldCancel, const detail::PointwiseExecutionOptions& options = {})
{
  const usize total = store.getSize();
  if(auto* inMemoryStore = dynamic_cast<DataStore<float32>*>(&store); !options.forceBuffered && inMemoryStore != nullptr)
  {
    nonstd::span<float32> values = inMemoryStore->createSpan();
    ParallelDataAlgorithm parallelAlgorithm;
    parallelAlgorithm.setRange(0, total);
    parallelAlgorithm.execute([&](const Range& range) {
      detail::TransformRecursiveGaussianBatch(range.max() - range.min(), shouldCancel, [&](usize localIndex) {
        const usize i = range.min() + localIndex;
        values[i] = static_cast<float32>(std::sqrt(static_cast<double>(values[i])));
      });
    });
    return {};
  }
  Result<detail::PointwiseBatchPlan> planResult = detail::MakeRecursiveGaussianPostprocessingPlan(
      total, sizeof(float32), options.targetScratchBytes,
      {{store.getStoreType(), store.getTupleShape(), store.getNumberOfComponents(), "gradient square-root output", detail::PointwiseEndpointLayout::Access::Write}});
  if(planResult.invalid())
  {
    return ConvertResult(std::move(planResult));
  }
  const usize batchValues = planResult.value().batchValues;
  std::vector<float32> buf(std::min(batchValues, total));
  for(usize start = 0; start < total;)
  {
    if(shouldCancel)
    {
      return {};
    }
    const usize n = std::min(batchValues, total - start);
    if(Result<> r = store.copyIntoBuffer(start, nonstd::span<float32>(buf.data(), n)); r.invalid())
    {
      return r;
    }
    if(!detail::TransformRecursiveGaussianBatch(n, shouldCancel, [&](usize i) { buf[i] = static_cast<float32>(std::sqrt(static_cast<double>(buf[i]))); }))
    {
      return {};
    }
    if(Result<> r = store.copyFromBuffer(start, nonstd::span<const float32>(buf.data(), n)); r.invalid())
    {
      return r;
    }
    start += n;
  }
  return {};
}

// dst[i] = static_cast<T>(src[i]) — Smoothing final cast float32 -> output type (ITK CastImageFilter).
template <class T>
Result<> CastFloat32StoreTo(const AbstractDataStore<float32>& src, AbstractDataStore<T>& dst, const std::atomic_bool& shouldCancel, const detail::PointwiseExecutionOptions& options = {})
{
  const usize total = src.getSize();
  if(dst.getSize() != total)
  {
    return MakeErrorResult(-23608, fmt::format("Recursive Gaussian cast source contains {} values, but its destination contains {} values.", total, dst.getSize()));
  }
  const auto* inMemorySrc = dynamic_cast<const DataStore<float32>*>(&src);
  auto* inMemoryDst = dynamic_cast<DataStore<T>*>(&dst);
  if(!options.forceBuffered && inMemorySrc != nullptr && inMemoryDst != nullptr)
  {
    const nonstd::span<const float32> srcValues = inMemorySrc->createSpan();
    nonstd::span<T> dstValues = inMemoryDst->createSpan();
    ParallelDataAlgorithm parallelAlgorithm;
    parallelAlgorithm.setRange(0, total);
    parallelAlgorithm.execute([&](const Range& range) {
      detail::TransformRecursiveGaussianBatch(range.max() - range.min(), shouldCancel, [&](usize localIndex) {
        const usize i = range.min() + localIndex;
        dstValues[i] = static_cast<T>(srcValues[i]);
      });
    });
    return {};
  }
  Result<detail::PointwiseBatchPlan> planResult = detail::MakeRecursiveGaussianPostprocessingPlan(
      total, sizeof(float32) + sizeof(T), options.targetScratchBytes,
      {{src.getStoreType(), src.getTupleShape(), src.getNumberOfComponents(), "smoothing cast input"},
       {dst.getStoreType(), dst.getTupleShape(), dst.getNumberOfComponents(), "smoothing cast output", detail::PointwiseEndpointLayout::Access::Write}});
  if(planResult.invalid())
  {
    return ConvertResult(std::move(planResult));
  }
  const usize batchValues = planResult.value().batchValues;
  std::vector<float32> s(std::min(batchValues, total));
  std::vector<T> d(std::min(batchValues, total));
  for(usize start = 0; start < total;)
  {
    if(shouldCancel)
    {
      return {};
    }
    const usize n = std::min(batchValues, total - start);
    if(Result<> r = src.copyIntoBuffer(start, nonstd::span<float32>(s.data(), n)); r.invalid())
    {
      return r;
    }
    if(!detail::TransformRecursiveGaussianBatch(n, shouldCancel, [&](usize i) { d[i] = static_cast<T>(s[i]); }))
    {
      return {};
    }
    if(Result<> r = dst.copyFromBuffer(start, nonstd::span<const T>(d.data(), n)); r.invalid())
    {
      return r;
    }
    start += n;
  }
  return {};
}
} // namespace nx::core::ImageProcessing
