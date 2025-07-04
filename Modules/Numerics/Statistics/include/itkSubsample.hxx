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
#ifndef itkSubsample_hxx
#define itkSubsample_hxx


#include "itkObject.h"

namespace itk::Statistics
{
template <typename TSample>
Subsample<TSample>::Subsample()
  : m_Sample(nullptr)
  , m_TotalFrequency(AbsoluteFrequencyType{})
{}

template <typename TSample>
void
Subsample<TSample>::PrintSelf(std::ostream & os, Indent indent) const
{
  Superclass::PrintSelf(os, indent);

  os << indent << "Sample: ";
  if (m_Sample != nullptr)
  {
    os << m_Sample << std::endl;
  }
  else
  {
    os << "not set." << std::endl;
  }

  os << indent << "TotalFrequency: " << m_TotalFrequency << std::endl;
  os << indent << "ActiveDimension: " << m_ActiveDimension << std::endl;
  os << indent << "InstanceIdentifierHolder : " << &m_IdHolder << std::endl;
}

template <typename TSample>
void
Subsample<TSample>::SetSample(const TSample * sample)
{
  m_Sample = sample;
  this->SetMeasurementVectorSize(m_Sample->GetMeasurementVectorSize());
  this->Modified();
}

template <typename TSample>
const TSample *
Subsample<TSample>::GetSample() const
{
  return m_Sample;
}

template <typename TSample>
void
Subsample<TSample>::InitializeWithAllInstances()
{
  m_IdHolder.resize(m_Sample->Size());
  auto                                  idIter = m_IdHolder.begin();
  typename TSample::ConstIterator       iter = m_Sample->Begin();
  const typename TSample::ConstIterator last = m_Sample->End();
  m_TotalFrequency = AbsoluteFrequencyType{};
  while (iter != last)
  {
    *idIter++ = iter.GetInstanceIdentifier();
    m_TotalFrequency += iter.GetFrequency();
    ++iter;
  }
  this->Modified();
}

template <typename TSample>
void
Subsample<TSample>::AddInstance(InstanceIdentifier id)
{
  if (id > m_Sample->Size())
  {
    itkExceptionMacro("MeasurementVector " << id << " does not exist in the Sample");
  }

  m_IdHolder.push_back(id);
  m_TotalFrequency += m_Sample->GetFrequency(id);
  this->Modified();
}

template <typename TSample>
auto
Subsample<TSample>::Size() const -> InstanceIdentifier
{
  return static_cast<unsigned int>(m_IdHolder.size());
}

template <typename TSample>
void
Subsample<TSample>::Clear()
{
  m_IdHolder.clear();
  m_TotalFrequency = AbsoluteFrequencyType{};
  this->Modified();
}

template <typename TSample>
auto
Subsample<TSample>::GetMeasurementVector(InstanceIdentifier id) const -> const MeasurementVectorType &
{
  if (id >= m_IdHolder.size())
  {
    itkExceptionMacro("MeasurementVector " << id << " does not exist");
  }

  // translate the id to its Sample container id
  const InstanceIdentifier idInTheSample = m_IdHolder[id];
  return m_Sample->GetMeasurementVector(idInTheSample);
}

template <typename TSample>
inline auto
Subsample<TSample>::GetFrequency(InstanceIdentifier id) const -> AbsoluteFrequencyType
{
  if (id >= m_IdHolder.size())
  {
    itkExceptionMacro("MeasurementVector " << id << " does not exist");
  }

  // translate the id to its Sample container id
  const InstanceIdentifier idInTheSample = m_IdHolder[id];
  return m_Sample->GetFrequency(idInTheSample);
}

template <typename TSample>
inline auto
Subsample<TSample>::GetTotalFrequency() const -> TotalAbsoluteFrequencyType
{
  return m_TotalFrequency;
}

template <typename TSample>
inline void
Subsample<TSample>::Swap(unsigned int index1, unsigned int index2)
{
  if (index1 >= m_IdHolder.size() || index2 >= m_IdHolder.size())
  {
    itkExceptionMacro("Index out of range");
  }

  const InstanceIdentifier temp = m_IdHolder[index1];
  m_IdHolder[index1] = m_IdHolder[index2];
  m_IdHolder[index2] = temp;
  this->Modified();
}

template <typename TSample>
inline const typename Subsample<TSample>::MeasurementVectorType &
Subsample<TSample>::GetMeasurementVectorByIndex(unsigned int index) const
{
  if (index >= m_IdHolder.size())
  {
    itkExceptionMacro("Index out of range");
  }
  return m_Sample->GetMeasurementVector(m_IdHolder[index]);
}

template <typename TSample>
inline auto
Subsample<TSample>::GetFrequencyByIndex(unsigned int index) const -> AbsoluteFrequencyType
{
  if (index >= m_IdHolder.size())
  {
    itkExceptionMacro("Index out of range");
  }

  return m_Sample->GetFrequency(m_IdHolder[index]);
}

template <typename TSample>
auto
Subsample<TSample>::GetInstanceIdentifier(unsigned int index) -> InstanceIdentifier
{
  if (index >= m_IdHolder.size())
  {
    itkExceptionMacro("Index out of range");
  }
  return m_IdHolder[index];
}

template <typename TSample>
void
Subsample<TSample>::Graft(const DataObject * thatObject)
{
  this->Superclass::Graft(thatObject);

  // Most of what follows is really a deep copy, rather than grafting of
  // output. Wish it were managed by pointers to bulk data. Sigh !

  const auto * thatConst = dynamic_cast<const Self *>(thatObject);
  if (thatConst)
  {
    auto * that = const_cast<Self *>(thatConst);
    this->SetSample(that->GetSample());
    this->m_IdHolder = that->m_IdHolder;
    this->m_ActiveDimension = that->m_ActiveDimension;
    this->m_TotalFrequency = that->m_TotalFrequency;
  }
}
} // namespace itk::Statistics

#endif
