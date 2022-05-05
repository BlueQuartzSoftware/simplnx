/* ============================================================================
 * Copyright (c) 2022-2022 BlueQuartz Software, LLC
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice, this
 * list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 *
 * Neither the name of BlueQuartz Software, the US Air Force, nor the names of its
 * contributors may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include "fmt/core.h"

#include <string>

#include "complex/Common/Result.hpp"
#include "complex/Common/Types.hpp"
#include "complex/Common/TypesUtility.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
template <typename T>
std::string generateOutOfRangeErrorMessage(std::string token)
{
  return fmt::format("Value '{}' is out of range for data type '{}'.", token, DataTypeToString(GetDataType<T>()));
}

// -----------------------------------------------------------------------------
template <typename T>
std::string generateInvalidArgumentErrorMessage(std::string token)
{
  return fmt::format("Value '{}' could not be converted to data type '{}'.", token, DataTypeToString(GetDataType<T>()));
}

// -----------------------------------------------------------------------------
template <typename T>
class SignedIntegerFunctor
{
public:
  ~SignedIntegerFunctor() = default;

  Result<T> operator()(const std::string& token)
  {
    if(!std::is_signed_v<T> || !std::numeric_limits<T>::is_integer)
    {
      return MakeErrorResult<T>(-1, fmt::format("Value '{}' is not a signed integer data type.", token));
    }

    int64 value = 0;
    try
    {
      value = std::stoll(token);
    } catch(std::invalid_argument e)
    {
      return MakeErrorResult<T>(-1, fmt::format("{}\n\nException message: {}", generateInvalidArgumentErrorMessage<T>(token), e.what()));
    } catch(std::out_of_range e)
    {
      return MakeErrorResult<T>(-1, fmt::format("{}\n\nException message: {}", generateOutOfRangeErrorMessage<T>(token), e.what()));
    }

    if(value > std::numeric_limits<T>().max() || value < std::numeric_limits<T>().min())
    {
      return MakeErrorResult<T>(-1, generateOutOfRangeErrorMessage<T>(token));
    }

    return {static_cast<T>(value)};
  }
};

// -----------------------------------------------------------------------------
template <typename T>
class UnsignedIntegerFunctor
{
public:
  ~UnsignedIntegerFunctor() = default;

  Result<T> operator()(const std::string& token)
  {
    if(!std::is_unsigned_v<T> || !std::numeric_limits<T>::is_integer)
    {
      return MakeErrorResult<T>(-1, fmt::format("Value '{}' is not an unsigned integer data type.", token));
    }

    uint64 value = 0;
    try
    {
      value = std::stoull(token);
    } catch(std::invalid_argument e)
    {
      return MakeErrorResult<T>(-1, fmt::format("{}\n\nException message: {}", generateInvalidArgumentErrorMessage<T>(token), e.what()));
    } catch(std::out_of_range e)
    {
      return MakeErrorResult<T>(-1, fmt::format("{}\n\nException message: {}", generateOutOfRangeErrorMessage<T>(token), e.what()));
    }

    if(value > std::numeric_limits<T>().max() || value < std::numeric_limits<T>().min())
    {
      return MakeErrorResult<T>(-1, generateOutOfRangeErrorMessage<T>(token));
    }

    return {static_cast<T>(value)};
  }
};

// -----------------------------------------------------------------------------
template <typename T>
class FloatingPointFunctor
{
public:
  ~FloatingPointFunctor() = default;

  Result<T> operator()(const std::string& token)
  {
    if(!std::is_floating_point_v<T>)
    {
      return MakeErrorResult<T>(-1, fmt::format("Value '{}' is not a floating point data type.", token));
    }

    float64 value = 0;
    try
    {
      value = std::stold(token);
    } catch(std::invalid_argument e)
    {
      return MakeErrorResult<T>(-1, fmt::format("{}\n\nException message: {}", generateInvalidArgumentErrorMessage<T>(token), e.what()));
    } catch(std::out_of_range e)
    {
      return MakeErrorResult<T>(-1, fmt::format("{}\n\nException message: {}", generateOutOfRangeErrorMessage<T>(token), e.what()));
    }

    if(value > std::numeric_limits<T>().max() || value < std::numeric_limits<T>().min())
    {
      return MakeErrorResult<T>(-1, generateOutOfRangeErrorMessage<T>(token));
    }

    return {static_cast<T>(value)};
  }
};

//// -----------------------------------------------------------------------------
// class StringFunctor
//{
// public:
//  ~StringFunctor() = default;

//  Result<std::string> operator()(const std::string& token)
//  {
//    return {token};
//  }
//};
