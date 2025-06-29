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
#ifndef itkDistanceMetric_hxx
#define itkDistanceMetric_hxx

namespace itk::Statistics
{
template <typename TVector>
DistanceMetric<TVector>::DistanceMetric()
{
  // If the measurement vector type is non-resizable type,
  // initialize the vector size to it.
  if (!MeasurementVectorTraits::IsResizable<MeasurementVectorType>({}))
  {
    const MeasurementVectorSizeType defaultLength = NumericTraits<MeasurementVectorType>::GetLength({});

    this->m_MeasurementVectorSize = defaultLength;
    this->m_Origin.SetSize(this->m_MeasurementVectorSize);
  }
  else
  {
    // otherwise initialize it to zero
    this->m_MeasurementVectorSize = 0;
  }
  m_Origin.Fill(0.0);
}

template <typename TVector>
void
DistanceMetric<TVector>::SetOrigin(const OriginType & x)
{
  if (this->m_MeasurementVectorSize != 0)
  {
    if (x.Size() != this->m_MeasurementVectorSize)
    {
      itkExceptionMacro("Size of the origin must be same as the length of each measurement vector.");
    }
  }

  this->m_MeasurementVectorSize = x.Size();
  m_Origin.SetSize(this->m_MeasurementVectorSize);
  m_Origin = x;
  this->Modified();
}

template <typename TVector>
void
DistanceMetric<TVector>::PrintSelf(std::ostream & os, Indent indent) const
{
  Superclass::PrintSelf(os, indent);
  os << indent << "Origin: " << this->GetOrigin() << std::endl;
  os << indent << "MeasurementVectorSize: " << this->GetMeasurementVectorSize() << std::endl;
}
} // namespace itk::Statistics

#endif
