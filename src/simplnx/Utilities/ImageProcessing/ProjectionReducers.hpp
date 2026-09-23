#pragma once

#include "simplnx/Common/Types.hpp"

#include <nonstd/span.hpp>

#include <algorithm>
#include <cmath>

namespace nx::core::ImageProcessing
{
/**
 * @file ProjectionReducers.hpp
 * @brief Reduce functors consumed by ApplyAxisProjection (see AxisProjectionEngine.hpp).
 *
 * Two functor shapes exist, selected by the engine at compile time on `k_IsAssociative`:
 *
 * - Associative reducers (`k_IsAssociative == true`) fold each staged pencil one value at a time into a small local
 *   accumulator. Independent pencils can therefore be reduced in parallel without
 * concurrent DataStore access.
 *   Contract:
 *     - `static constexpr bool k_IsAssociative = true;`
 *     - `template <class TOut> struct State { ... };` a per-slot accumulator whose default-constructed
 *       value is the "empty" init state (a sentinel/flag distinguishing "no value seen yet").
 *     - `template <class TIn, class TOut> void accumulate(State<TOut>& state, TIn value) const;`
 *       folds one input value into the slot's state.
 *     - `template <class TOut> TOut finalize(const State<TOut>& state) const;` produces the slot's
 *       output value once every input value has been folded in.
 *
 * - Non-associative reducers (`k_IsAssociative == false`) need the whole pencil at once. Contract:
 *     - `static constexpr bool k_IsAssociative = false;`
 *     - `template <class TIn, class TOut> TOut reducePencil(nonstd::span<TIn> pencil) const;`
 *       reduces a full pencil of input values (the engine reads one pencil per output slot). The
 *       span is mutable so the functor may reorder it in place (e.g. std::nth_element).
 *
 * State/accumulate/finalize are intentionally minimal so additional reducers (e.g. Min/Sum/StdDev/
 * Binary) can be added without touching the engine.
 */

/**
 * @brief Associative maximum reducer. Keeps the running maximum per output slot.
 */
struct MaxReduce
{
  static constexpr bool k_IsAssociative = true;

  template <class TOut>
  struct State
  {
    TOut value{};
    bool initialized = false; // init sentinel: false until the first value is folded in
  };

  template <class TIn, class TOut>
  void accumulate(State<TOut>& state, TIn value) const
  {
    const TOut v = static_cast<TOut>(value);
    if(!state.initialized || v > state.value)
    {
      state.value = v;
      state.initialized = true;
    }
  }

  template <class TOut>
  TOut finalize(const State<TOut>& state) const
  {
    return state.value;
  }
};

/**
 * @brief Associative minimum reducer. Keeps the running minimum per output slot. Mirrors MaxReduce with
 *        the comparison flipped, matching the legacy ITK MinimumProjectionImageFilter (std::min fold).
 */
struct MinReduce
{
  static constexpr bool k_IsAssociative = true;

  template <class TOut>
  struct State
  {
    TOut value{};
    bool initialized = false; // init sentinel: false until the first value is folded in
  };

  template <class TIn, class TOut>
  void accumulate(State<TOut>& state, TIn value) const
  {
    const TOut v = static_cast<TOut>(value);
    if(!state.initialized || v < state.value)
    {
      state.value = v;
      state.initialized = true;
    }
  }

  template <class TOut>
  TOut finalize(const State<TOut>& state) const
  {
    return state.value;
  }
};

/**
 * @brief Associative mean reducer. Accumulates a plain Float64 sum and a count per output slot, matching ITK's
 *        MeanAccumulator (`m_Sum = m_Sum + input`); finalize() returns sum / count (empty slot -> 0).
 */
struct MeanReduce
{
  static constexpr bool k_IsAssociative = true;

  template <class TOut>
  struct State
  {
    float64 sum = 0.0;
    usize count = 0;
  };

  template <class TIn, class TOut>
  void accumulate(State<TOut>& state, TIn value) const
  {
    state.sum += static_cast<float64>(value);
    ++state.count;
  }

  template <class TOut>
  TOut finalize(const State<TOut>& state) const
  {
    if(state.count == 0)
    {
      return TOut{};
    }
    return static_cast<TOut>(state.sum / static_cast<float64>(state.count));
  }
};

/**
 * @brief Associative sum reducer. Accumulates a Kahan-compensated running sum per output slot;
 *        finalize() returns it (empty slot -> 0). Reproduces the legacy ITK SumProjectionImageFilter
 *        accumulator (itkSumProjectionImageFilter.h, Functor::SumAccumulator: m_Sum = m_Sum + input;
 *        GetValue() returns m_Sum), but the running sum is Kahan-compensated (as in MeanReduce /
 *        StreamingStatistics.hpp) so long pencils do not lose precision. The output type is Float64
 *        (the façade's AlwaysFloat64 OutTypeMap), matching the legacy filter's double FilterOutputType.
 */
struct SumReduce
{
  static constexpr bool k_IsAssociative = true;

  template <class TOut>
  struct State
  {
    float64 sum = 0.0;
    float64 comp = 0.0; // Kahan compensation term
  };

  template <class TIn, class TOut>
  void accumulate(State<TOut>& state, TIn value) const
  {
    const float64 dv = static_cast<float64>(value);
    const float64 y = dv - state.comp;
    const float64 t = state.sum + y;
    state.comp = (t - state.sum) - y;
    state.sum = t;
  }

  template <class TOut>
  TOut finalize(const State<TOut>& state) const
  {
    return static_cast<TOut>(state.sum);
  }
};

/**
 * @brief Associative standard-deviation reducer. Accumulates a Kahan-compensated running sum and a
 *        Kahan-compensated running sum-of-squares plus a count per output slot; finalize() returns the
 *        SAMPLE standard deviation, i.e. the one-pass algebraic form of the legacy ITK
 *        StandardDeviationProjectionImageFilter accumulator
 *        (itkStandardDeviationProjectionImageFilter.h, Functor::StandardDeviationAccumulator::GetValue():
 *        mean = sum / N; squaredSum = sum_i (x_i - mean)^2; return sqrt(squaredSum / (N - 1))). ITK forms
 *        the mean with the POPULATION divisor N but divides the squared-deviation total by (N - 1)
 *        (SAMPLE variance), and guards N <= 1 by returning 0 -- both replicated here. The output type is
 *        Float64 (the façade's AlwaysFloat64 OutTypeMap), matching the legacy filter's double
 *        FilterOutputType. Parity against ITK's two-pass form is algebraic (see the identity below), not
 *        bit-identical, so the projection parity tests compare within a tolerance.
 */
struct StdDevReduce
{
  static constexpr bool k_IsAssociative = true;

  template <class TOut>
  struct State
  {
    float64 sum = 0.0;
    float64 sumComp = 0.0;   // Kahan compensation for the running sum
    float64 sumSq = 0.0;     // running sum of squares
    float64 sumSqComp = 0.0; // Kahan compensation for the running sum of squares
    usize count = 0;
  };

  template <class TIn, class TOut>
  void accumulate(State<TOut>& state, TIn value) const
  {
    const float64 dv = static_cast<float64>(value);
    // Kahan-compensated running sum.
    {
      const float64 y = dv - state.sumComp;
      const float64 t = state.sum + y;
      state.sumComp = (t - state.sum) - y;
      state.sum = t;
    }
    // Kahan-compensated running sum of squares.
    {
      const float64 sq = dv * dv;
      const float64 y = sq - state.sumSqComp;
      const float64 t = state.sumSq + y;
      state.sumSqComp = (t - state.sumSq) - y;
      state.sumSq = t;
    }
    ++state.count;
  }

  template <class TOut>
  TOut finalize(const State<TOut>& state) const
  {
    // ITK returns 0 for a projected extent of N <= 1 (also avoids the (N - 1) division by zero).
    if(state.count <= 1)
    {
      return TOut{};
    }
    const float64 n = static_cast<float64>(state.count);
    // One-pass algebraic identity for ITK's two-pass sum_i (x_i - mean)^2 with mean = sum / N:
    //   sum_i (x_i - mean)^2 == sumSq - (sum * sum) / N
    // then the SAMPLE variance divides by (N - 1), exactly as ITK does.
    float64 variance = (state.sumSq - (state.sum * state.sum) / n) / (n - 1.0);
    // Guard the tiny negative that catastrophic cancellation can produce when every value along the
    // pencil is (nearly) equal, so std::sqrt never sees a negative argument.
    if(variance < 0.0)
    {
      variance = 0.0;
    }
    return static_cast<TOut>(std::sqrt(variance));
  }
};

/**
 * @brief Non-associative median reducer. Reduces a full pencil with std::nth_element at size()/2,
 *        matching the Phase-2 MedianImageFilter convention (upper-middle for even-length pencils).
 */
struct MedianReduce
{
  static constexpr bool k_IsAssociative = false;

  template <class TIn, class TOut>
  TOut reducePencil(nonstd::span<TIn> pencil) const
  {
    const auto middle = pencil.begin() + pencil.size() / 2;
    std::nth_element(pencil.begin(), middle, pencil.end());
    return static_cast<TOut>(*middle);
  }
};

/**
 * @brief Associative binary reducer. Reproduces the ITK BinaryProjectionImageFilter accumulator
 *        (itkBinaryProjectionImageFilter.h, Functor::BinaryAccumulator): each output slot starts as
 *        "background" and becomes "foreground" if ANY input value along the projected axis equals the
 *        foreground value; finalize() then emits the foreground value if the slot saw foreground, else the
 *        background value. The foreground/background values are held as passed in (float64, matching the
 *        legacy filter's Float64 parameters) and cast to the working element type at fold/finalize time --
 *        the equality test is performed in the INPUT element type, exactly as ITK compares
 *        `input == m_ForegroundValue` (m_ForegroundValue being the input pixel type).
 */
struct BinaryReduce
{
  static constexpr bool k_IsAssociative = true;

  float64 foregroundValue = 1.0;
  float64 backgroundValue = 0.0;

  template <class TOut>
  struct State
  {
    bool seenForeground = false; // init state: no foreground pixel folded in yet (slot starts as background)
  };

  template <class TIn, class TOut>
  void accumulate(State<TOut>& state, TIn value) const
  {
    // ITK's BinaryAccumulator compares each input pixel against the foreground value in the INPUT pixel
    // type: `if(input == m_ForegroundValue) m_IsForeground = true;`. Match that exactly.
    if(value == static_cast<TIn>(foregroundValue))
    {
      state.seenForeground = true;
    }
  }

  template <class TOut>
  TOut finalize(const State<TOut>& state) const
  {
    // ITK returns (TOutputPixel)m_ForegroundValue when any foreground pixel was seen, else m_BackgroundValue.
    return state.seenForeground ? static_cast<TOut>(foregroundValue) : static_cast<TOut>(backgroundValue);
  }
};
} // namespace nx::core::ImageProcessing
