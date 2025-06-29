/*=========================================================================
 *
 *  Copyright NumFOCUS
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         https://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/
#ifndef itkVariableLengthVector_hxx
#define itkVariableLengthVector_hxx

#include "itkNumericTraitsVariableLengthVectorPixel.h"
#include "itkMath.h"
#include <cstring>
#include <cstdlib>

namespace itk
{

template <typename TValue>
VariableLengthVector<TValue>::VariableLengthVector(unsigned int length)
  : m_Data(nullptr)
{
  Reserve(length);
  // postcondition(s)
  itkAssertInDebugAndIgnoreInReleaseMacro(m_Data != nullptr);
}

template <typename TValue>
VariableLengthVector<TValue>::VariableLengthVector(ValueType * datain, unsigned int sz, bool LetArrayManageMemory)
  : m_LetArrayManageMemory(LetArrayManageMemory)
  , m_Data(datain)
  , m_NumElements(sz)
{}

template <typename TValue>
VariableLengthVector<TValue>::VariableLengthVector(const ValueType * datain, unsigned int sz, bool LetArrayManageMemory)
  : m_LetArrayManageMemory(LetArrayManageMemory)
  , m_Data(const_cast<ValueType *>(datain))
  , m_NumElements(sz)
{}

template <typename TValue>
VariableLengthVector<TValue>::VariableLengthVector(const VariableLengthVector<TValue> & v)
  : m_NumElements(v.Size())
{
  if (m_NumElements != 0)
  {
    m_Data = this->AllocateElements(m_NumElements);
    itkAssertInDebugAndIgnoreInReleaseMacro(m_Data != nullptr);
    itkAssertInDebugAndIgnoreInReleaseMacro(v.m_Data != nullptr);
    std::copy_n(&v.m_Data[0], m_NumElements, &this->m_Data[0]);
  }
  else
  {
    m_Data = nullptr;
  }
}

template <typename TValue>
VariableLengthVector<TValue>::VariableLengthVector(Self && v) noexcept
  : m_LetArrayManageMemory(v.m_LetArrayManageMemory)
  , m_Data(v.m_Data)
  , m_NumElements(v.m_NumElements)
{
  v.m_LetArrayManageMemory = true;
  v.m_Data = nullptr;
  v.m_NumElements = 0;
}

template <typename TValue>
VariableLengthVector<TValue> &
VariableLengthVector<TValue>::operator=(Self && v) noexcept
{
  itkAssertInDebugAndIgnoreInReleaseMacro(&v != this);

  // Possible cases:
  // - both are proxy
  //   => a shallow assignment is enough
  // - none are proxy
  //   => this->m_Data is released, v content is stolen by *this
  // - v is a proxy, but not *this
  //   => Fall back to usual copy-assignment
  // - *this is a proxy, but not v
  //   => v content is stolen by *this, nothing to delete[]

  if (!IsAProxy() && v.IsAProxy())
  { // Fall back to usual copy-assignment
    return *this = v;
  }

  // Delete old data, when data is stolen
  if (!IsAProxy() && !v.IsAProxy())
  {
    delete[] m_Data;
  }

  // Shallow copy of the information
  m_LetArrayManageMemory = v.m_LetArrayManageMemory;
  m_Data = v.m_Data;
  m_NumElements = v.m_NumElements;

  // Reset v to something assignable and destructible
  // NB: It's not necessary to always reset v. The choice made is to avoid a
  // test
  v.m_LetArrayManageMemory = true;
  v.m_Data = nullptr;
  v.m_NumElements = 0;

  return *this;
}

template <typename TValue>
template <typename VariableLengthVectorExpression1, typename VariableLengthVectorExpression2, typename TBinaryOp>
VariableLengthVector<TValue>::VariableLengthVector(
  const VariableLengthVectorExpression<VariableLengthVectorExpression1, VariableLengthVectorExpression2, TBinaryOp> &
    rhs)
  : m_NumElements(rhs.Size())
{
  m_Data = this->AllocateElements(m_NumElements);
  // allocate Elements post-condition
  itkAssertInDebugAndIgnoreInReleaseMacro(m_Data != nullptr);
  for (ElementIdentifier i = 0; i < m_NumElements; ++i)
  {
    this->m_Data[i] = static_cast<TValue>(rhs[i]);
  }
}

template <typename TValue>
template <typename VariableLengthVectorExpression1, typename VariableLengthVectorExpression2, typename TBinaryOp>
VariableLengthVector<TValue> &
VariableLengthVector<TValue>::operator=(
  const VariableLengthVectorExpression<VariableLengthVectorExpression1, VariableLengthVectorExpression2, TBinaryOp> &
    rhs)
{
  const ElementIdentifier N = rhs.Size();
  this->SetSize(N, DontShrinkToFit(), DumpOldValues());
  for (ElementIdentifier i = 0; i < N; ++i)
  {
    this->m_Data[i] = static_cast<TValue>(rhs[i]);
  }
  return *this;
}

template <typename TValue>
VariableLengthVector<TValue>::~VariableLengthVector()
{
  // if data exists and we are responsible for its memory, get rid of it..
  if (m_LetArrayManageMemory)
  {
    delete[] m_Data;
  }
}

template <typename TValue>
void
VariableLengthVector<TValue>::Reserve(ElementIdentifier size)
{
  if (m_Data)
  {
    if (size > m_NumElements)
    {
      TValue * temp = this->AllocateElements(size);
      itkAssertInDebugAndIgnoreInReleaseMacro(temp);
      itkAssertInDebugAndIgnoreInReleaseMacro(m_NumElements == 0 || (m_NumElements > 0 && m_Data != nullptr));
      // only copy the portion of the data used in the old buffer
      std::copy_n(m_Data, m_NumElements, temp);
      if (m_LetArrayManageMemory)
      {
        delete[] m_Data;
      }
      m_Data = temp;
      m_LetArrayManageMemory = true;
      m_NumElements = size;
    }
  }
  else
  {
    m_Data = this->AllocateElements(size);
    m_NumElements = size;
    m_LetArrayManageMemory = true;
  }
  itkAssertInDebugAndIgnoreInReleaseMacro(m_Data != nullptr);
}

template <typename TValue>
TValue *
VariableLengthVector<TValue>::AllocateElements(ElementIdentifier size) const
{
  try
  {
    return new TValue[size];
  }
  catch (...)
  {
    // Intercept std::bad_alloc and any exception thrown from TValue
    // default constructor.
    itkGenericExceptionMacro("Failed to allocate memory of length " << size << " for VariableLengthVector.");
  }
}

template <typename TValue>
void
VariableLengthVector<TValue>::SetData(TValue * datain, bool LetArrayManageMemory)
{
  // Free any existing data if we manage its memory
  if (m_LetArrayManageMemory)
  {
    delete[] m_Data;
  }

  m_LetArrayManageMemory = LetArrayManageMemory;
  m_Data = datain;
}

template <typename TValue>
void
VariableLengthVector<TValue>::SetData(TValue * datain, unsigned int sz, bool LetArrayManageMemory)
{
  // Free any existing data if we manage its memory
  if (m_LetArrayManageMemory)
  {
    delete[] m_Data;
  }

  m_LetArrayManageMemory = LetArrayManageMemory;
  m_Data = datain;
  m_NumElements = sz;
}


template <typename TValue>
void
VariableLengthVector<TValue>::DestroyExistingData()
{
  // Free any existing data if we manage its memory.
  if (m_LetArrayManageMemory)
  {
    delete[] m_Data;
  }

  m_Data = nullptr;
  m_NumElements = 0;
}

template <typename TValue>
template <typename TReallocatePolicy, typename TKeepValuesPolicy>
void
VariableLengthVector<TValue>::SetSize(unsigned int sz, TReallocatePolicy reallocatePolicy, TKeepValuesPolicy keepValues)
{
  static_assert(
    std::is_base_of_v<AllocateRootPolicy, TReallocatePolicy>,
    "The allocation policy does not inherit from itk::VariableLengthVector::AllocateRootPolicy as expected");
  static_assert(
    std::is_base_of_v<KeepValuesRootPolicy, TKeepValuesPolicy>,
    "The old values keeping policy does not inherit from itk::VariableLengthVector::KeepValuesRootPolicy as expected");

  if (reallocatePolicy(sz, m_NumElements) || !m_LetArrayManageMemory)
  {
    TValue * temp = this->AllocateElements(sz); // may throw
    itkAssertInDebugAndIgnoreInReleaseMacro(temp);
    itkAssertInDebugAndIgnoreInReleaseMacro(m_NumElements == 0 || (m_NumElements > 0 && m_Data != nullptr));
    keepValues(sz, m_NumElements, m_Data, temp); // possible leak if TValue copy may throw
    // commit changes
    if (m_LetArrayManageMemory)
    {
      delete[] m_Data;
    }
    m_Data = temp;
    m_LetArrayManageMemory = true;
  }
  m_NumElements = sz;
}

template <typename TValue>
void
VariableLengthVector<TValue>::Fill(const TValue & v)
{
  itkAssertInDebugAndIgnoreInReleaseMacro(m_NumElements == 0 || (m_NumElements > 0 && m_Data != nullptr));
  // VC++ version of std::fill_n() expects the output iterator to be valid
  // instead of expecting the range [OutIt, OutIt+n) to be valid.
  std::fill(&this->m_Data[0], &this->m_Data[m_NumElements], v);
}

template <typename TValue>
VariableLengthVector<TValue> &
VariableLengthVector<TValue>::operator=(const Self & v)
{
  // No self assignment test is done. Indeed:
  // - the operator already resists self assignment through a strong exception
  // guarantee
  // - the test becomes a pessimization as we never write "v = v;".
  const ElementIdentifier N = v.Size();
  this->SetSize(N, DontShrinkToFit(), DumpOldValues());

  // VC++ version of std::copy expects the input range to be valid, and the
  // output iterator as well (as it's a pointer, it's expected non null)
  // Hence the manual loop instead
  itkAssertInDebugAndIgnoreInReleaseMacro(N == 0 || this->m_Data != nullptr);
  itkAssertInDebugAndIgnoreInReleaseMacro(N == 0 || v.m_Data != nullptr);
  for (ElementIdentifier i = 0; i != N; ++i)
  {
    this->m_Data[i] = v.m_Data[i];
  }

  itkAssertInDebugAndIgnoreInReleaseMacro(m_LetArrayManageMemory);
  return *this;
}

template <typename TValue>
inline VariableLengthVector<TValue> &
VariableLengthVector<TValue>::FastAssign(const Self & v)
{
  itkAssertInDebugAndIgnoreInReleaseMacro(this->m_LetArrayManageMemory);
  const ElementIdentifier N = v.Size();
  itkAssertInDebugAndIgnoreInReleaseMacro(N > 0);
  itkAssertInDebugAndIgnoreInReleaseMacro(N == this->Size());
  // Redundant precondition checks
  itkAssertInDebugAndIgnoreInReleaseMacro(v.m_Data != nullptr);
  itkAssertInDebugAndIgnoreInReleaseMacro(this->m_Data != nullptr);

  std::copy_n(&v.m_Data[0], N, &this->m_Data[0]);

  return *this;
}

template <typename TValue>
VariableLengthVector<TValue> &
VariableLengthVector<TValue>::operator=(const TValue & v)
{
  this->Fill(v);
  return *this;
}

template <typename TValue>
VariableLengthVector<TValue> &
VariableLengthVector<TValue>::operator-()
{
  for (ElementIdentifier i = 0; i < m_NumElements; ++i)
  {
    m_Data[i] *= -1;
  }
  return *this;
}

template <typename TValue>
bool
VariableLengthVector<TValue>::operator==(const Self & v) const
{
  if (m_NumElements != v.Size())
  {
    return false;
  }
  for (ElementIdentifier i = 0; i < m_NumElements; ++i)
  {
    if (Math::NotExactlyEquals(m_Data[i], v[i]))
    {
      return false;
    }
  }
  return true;
}

template <typename TValue>
auto
VariableLengthVector<TValue>::GetNorm() const -> RealValueType
{
  using std::sqrt;
  return static_cast<RealValueType>(sqrt(this->GetSquaredNorm()));
}

template <typename TValue>
auto
VariableLengthVector<TValue>::GetSquaredNorm() const -> RealValueType
{
  RealValueType sum = 0.0;

  for (ElementIdentifier i = 0; i < this->m_NumElements; ++i)
  {
    const RealValueType value = (*this)[i];
    sum += value * value;
  }
  return sum;
}

template <typename TExpr1, typename TExpr2, typename TBinaryOp>
auto
VariableLengthVectorExpression<TExpr1, TExpr2, TBinaryOp>::GetNorm() const -> RealValueType
{
  return itk::GetNorm(*this);
}

template <typename TExpr1, typename TExpr2, typename TBinaryOp>
auto
VariableLengthVectorExpression<TExpr1, TExpr2, TBinaryOp>::GetSquaredNorm() const -> RealValueType
{
  return itk::GetSquaredNorm(*this);
}

} // namespace itk

#endif
