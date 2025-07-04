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
#ifndef itkImagePCAShapeModelEstimator_hxx
#define itkImagePCAShapeModelEstimator_hxx

#include "itkPrintHelper.h"

namespace itk
{
template <typename TInputImage, typename TOutputImage>
ImagePCAShapeModelEstimator<TInputImage, TOutputImage>::ImagePCAShapeModelEstimator()

{
  m_EigenVectors.set_size(0, 0);
  m_EigenValues.set_size(0);
  this->SetNumberOfPrincipalComponentsRequired(1);
}

template <typename TInputImage, typename TOutputImage>
void
ImagePCAShapeModelEstimator<TInputImage, TOutputImage>::PrintSelf(std::ostream & os, Indent indent) const
{
  using namespace print_helper;

  Superclass::PrintSelf(os, indent);

  os << indent << "InputImageIteratorArray size: " << m_InputImageIteratorArray.size() << std::endl;
  os << indent << "Means: " << m_Means << std::endl;
  os << indent << "InnerProduct: " << m_InnerProduct << std::endl;
  os << indent << "EigenVectors: " << m_EigenVectors << std::endl;
  os << indent << "EigenValues: " << m_EigenValues << std::endl;
  os << indent << "EigenVectorNormalizedEnergy: " << m_EigenVectorNormalizedEnergy << std::endl;
  os << indent << "InputImageSize: " << static_cast<typename NumericTraits<ImageSizeType>::PrintType>(m_InputImageSize)
     << std::endl;
  os << indent << "NumberOfPixels: " << m_NumberOfPixels << std::endl;
  os << indent << "NumberOfTrainingImages: " << m_NumberOfTrainingImages << std::endl;
  os << indent << "NumberOfPrincipalComponentsRequired: " << m_NumberOfPrincipalComponentsRequired << std::endl;
}

template <typename TInputImage, typename TOutputImage>
void
ImagePCAShapeModelEstimator<TInputImage, TOutputImage>::EnlargeOutputRequestedRegion(DataObject * itkNotUsed(output))
{
  // Require all of the output images to be in the buffer
  for (unsigned int idx = 0; idx < this->GetNumberOfIndexedOutputs(); ++idx)
  {
    if (this->GetOutput(idx))
    {
      this->GetOutput(idx)->SetRequestedRegionToLargestPossibleRegion();
    }
  }
}

template <typename TInputImage, typename TOutputImage>
void
ImagePCAShapeModelEstimator<TInputImage, TOutputImage>::GenerateInputRequestedRegion()
{
  Superclass::GenerateInputRequestedRegion();

  // Require all of the inputs to be in the buffer up to the
  // LargestPossibleRegion of the first input.
  if (this->GetInput(0))
  {
    // Set the requested region of the first input to largest possible region
    const InputImagePointer input = const_cast<TInputImage *>(this->GetInput(0));
    input->SetRequestedRegionToLargestPossibleRegion();

    // Set the requested region of the remaining input to the largest possible
    // region of the first input
    for (unsigned int idx = 1; idx < this->GetNumberOfIndexedInputs(); ++idx)
    {
      if (this->GetInput(idx))
      {
        const typename TInputImage::RegionType requestedRegion = this->GetInput(0)->GetLargestPossibleRegion();

        const typename TInputImage::RegionType largestRegion = this->GetInput(idx)->GetLargestPossibleRegion();

        if (!largestRegion.IsInside(requestedRegion))
        {
          itkExceptionMacro("LargestPossibleRegion of input "
                            << idx << " is not a superset of the LargestPossibleRegion of input 0");
        }

        const InputImagePointer ptr = const_cast<TInputImage *>(this->GetInput(idx));
        ptr->SetRequestedRegion(requestedRegion);
      }
    }
  }
}

template <typename TInputImage, typename TOutputImage>
void
ImagePCAShapeModelEstimator<TInputImage, TOutputImage>::GenerateData()
{
  this->EstimateShapeModels();

  // Allocate memory for each output.
  auto numberOfOutputs = static_cast<unsigned int>(this->GetNumberOfIndexedOutputs());

  const InputImagePointer input = const_cast<TInputImage *>(this->GetInput(0));
  for (unsigned int j = 0; j < numberOfOutputs; ++j)
  {
    const OutputImagePointer output = this->GetOutput(j);
    output->SetBufferedRegion(output->GetRequestedRegion());
    output->Allocate();
  }

  // Fill the output images.
  VectorOfDoubleType m_OneEigenVector;
  using OutputIterator = ImageRegionIterator<OutputImageType>;

  // Fill the mean image first

  typename OutputImageType::RegionType region = this->GetOutput(0)->GetRequestedRegion();
  OutputIterator                       outIter(this->GetOutput(0), region);

  unsigned int i = 0;
  while (!outIter.IsAtEnd())
  {
    outIter.Set(static_cast<typename OutputImageType::PixelType>(m_Means[i]));
    ++outIter;
    ++i;
  }

  // Now fill the principal component outputs
  unsigned int       kthLargestPrincipalComp = m_NumberOfTrainingImages;
  const unsigned int numberOfValidOutputs = std::min(numberOfOutputs, m_NumberOfTrainingImages + 1);
  unsigned int       j = 1;
  for (; j < numberOfValidOutputs; ++j)
  {
    // Extract one column vector at a time
    m_OneEigenVector = m_EigenVectors.get_column(kthLargestPrincipalComp - 1);

    region = this->GetOutput(j)->GetRequestedRegion();
    OutputIterator outIterJ(this->GetOutput(j), region);

    unsigned int idx = 0;
    outIterJ.GoToBegin();
    while (!outIterJ.IsAtEnd())
    {
      outIterJ.Set(static_cast<typename OutputImageType::PixelType>(m_OneEigenVector[idx]));
      ++outIterJ;
      ++idx;
    }

    // Decrement to get the next principal component
    --kthLargestPrincipalComp;
  }

  // Fill extraneous outputs with zero
  for (; j < numberOfOutputs; ++j)
  {
    region = this->GetOutput(j)->GetRequestedRegion();
    OutputIterator outIterJ(this->GetOutput(j), region);

    outIterJ.GoToBegin();
    while (!outIterJ.IsAtEnd())
    {
      outIterJ.Set(0);
      ++outIterJ;
    }
  }

  // Delete eigenvectors at the end of generateData to free memory
  if (this->GetReleaseDataFlag())
  {
    m_EigenVectors.set_size(0, 0);
  }
}

template <typename TInputImage, typename TOutputImage>
void
ImagePCAShapeModelEstimator<TInputImage, TOutputImage>::SetNumberOfPrincipalComponentsRequired(unsigned int n)
{
  if (m_NumberOfPrincipalComponentsRequired != n)
  {
    m_NumberOfPrincipalComponentsRequired = n;

    this->Modified();

    // Modify the required number of outputs ( 1 extra for the mean image )
    this->SetNumberOfRequiredOutputs(m_NumberOfPrincipalComponentsRequired + 1);

    auto numberOfOutputs = static_cast<unsigned int>(this->GetNumberOfIndexedOutputs());
    if (numberOfOutputs < m_NumberOfPrincipalComponentsRequired + 1)
    {
      // Make and add extra outputs
      for (unsigned int idx = numberOfOutputs; idx <= m_NumberOfPrincipalComponentsRequired; ++idx)
      {
        const typename DataObject::Pointer output = this->MakeOutput(idx);
        this->SetNthOutput(idx, output.GetPointer());
      }
    }
    else if (numberOfOutputs > m_NumberOfPrincipalComponentsRequired + 1)
    {
      // Remove the extra outputs
      for (unsigned int idx = numberOfOutputs - 1; idx >= m_NumberOfPrincipalComponentsRequired + 1; idx--)
      {
        this->RemoveOutput(idx);
      }
    }
  }
}

template <typename TInputImage, typename TOutputImage>
void
ImagePCAShapeModelEstimator<TInputImage, TOutputImage>::SetNumberOfTrainingImages(unsigned int n)
{
  if (m_NumberOfTrainingImages != n)
  {
    m_NumberOfTrainingImages = n;

    this->Modified();

    // Modify the required number of inputs
    this->SetNumberOfRequiredInputs(m_NumberOfTrainingImages);
  }
}

template <typename TInputImage, typename TOutputImage>
void
ImagePCAShapeModelEstimator<TInputImage, TOutputImage>::EstimateShapeModels()
{
  this->CalculateInnerProduct();

  this->EstimatePCAShapeModelParameters();
}

template <typename TInputImage, typename TOutputImage>
void
ImagePCAShapeModelEstimator<TInputImage, TOutputImage>::CalculateInnerProduct()
{
  // Get the pointers to the input images and initialize the iterators
  // We use dynamic_cast since inputs are stored as DataObjects.  The
  // ImageToImageFilter::GetInput(int) always returns a pointer to a
  // TInputImage1 so it cannot be used for the second input.

  InputImagePointerArray inputImagePointerArray(m_NumberOfTrainingImages);

  m_InputImageIteratorArray.resize(m_NumberOfTrainingImages);

  for (unsigned int i = 0; i < m_NumberOfTrainingImages; ++i)
  {
    const InputImageConstPointer inputImagePtr = dynamic_cast<const TInputImage *>(ProcessObject::GetInput(i));

    inputImagePointerArray[i] = inputImagePtr;

    const InputImageConstIterator inputImageIt(inputImagePtr, inputImagePtr->GetBufferedRegion());

    m_InputImageIteratorArray[i] = inputImageIt;

    m_InputImageIteratorArray[i].GoToBegin();
  }

  // Set up the matrix to hold the inner product and the means from the
  // training data
  m_InputImageSize = (inputImagePointerArray[0])->GetBufferedRegion().GetSize();

  m_NumberOfPixels = 1;
  for (unsigned int i = 0; i < InputImageDimension; ++i)
  {
    m_NumberOfPixels *= m_InputImageSize[i];
  }

  // Calculate the means
  m_Means.set_size(m_NumberOfPixels);
  m_Means.fill(0);

  InputImageConstIterator tempImageItA;

  for (unsigned int img_number = 0; img_number < m_NumberOfTrainingImages; ++img_number)
  {
    tempImageItA = m_InputImageIteratorArray[img_number];

    for (unsigned int band_x = 0; band_x < m_NumberOfPixels; ++band_x)
    {
      m_Means[band_x] += tempImageItA.Get();
      ++tempImageItA;
    }
  }

  m_Means /= m_NumberOfTrainingImages;

  // Calculate the inner product
  m_InnerProduct.set_size(m_NumberOfTrainingImages, m_NumberOfTrainingImages);
  m_InnerProduct.fill(0);

  InputImageConstIterator tempImageItB;

  for (unsigned int band_x = 0; band_x < m_NumberOfTrainingImages; ++band_x)
  {
    for (unsigned int band_y = 0; band_y <= band_x; ++band_y)
    {
      // Pointer to the vector (in original matrix)
      tempImageItA = m_InputImageIteratorArray[band_x];

      // Pointer to the vector in the transposed matrix
      tempImageItB = m_InputImageIteratorArray[band_y];

      for (unsigned int pix_number = 0; pix_number < m_NumberOfPixels; ++pix_number)
      {
        m_InnerProduct[band_x][band_y] +=
          (tempImageItA.Get() - m_Means[pix_number]) * (tempImageItB.Get() - m_Means[pix_number]);

        ++tempImageItA;
        ++tempImageItB;
      }
    }
  }

  // Fill the rest of the inner product matrix and make it symmetric
  for (unsigned int band_x = 0; band_x < (m_NumberOfTrainingImages - 1); ++band_x)
  {
    for (unsigned int band_y = band_x + 1; band_y < m_NumberOfTrainingImages; ++band_y)
    {
      m_InnerProduct[band_x][band_y] = m_InnerProduct[band_y][band_x];
    }
  }

  if ((m_NumberOfTrainingImages - 1) != 0)
  {
    m_InnerProduct /= (m_NumberOfTrainingImages - 1);
  }
  else
  {
    m_InnerProduct.fill(0);
  }
}

template <typename TInputImage, typename TOutputImage>
void
ImagePCAShapeModelEstimator<TInputImage, TOutputImage>::EstimatePCAShapeModelParameters()
{
  MatrixOfDoubleType identityMatrix(m_NumberOfTrainingImages, m_NumberOfTrainingImages);

  identityMatrix.set_identity();

  const vnl_generalized_eigensystem eigenVectors_eigenValues(m_InnerProduct, identityMatrix);

  MatrixOfDoubleType eigenVectorsOfInnerProductMatrix = eigenVectors_eigenValues.V;

  // Calculate the principal shape variations
  //
  // m_EigenVectors capture the principal shape variations
  // m_EigenValues capture the relative weight of each variation
  // Multiply original image vectors with the eigenVectorsOfInnerProductMatrix
  // to derive the principal shapes.

  m_EigenVectors.set_size(m_NumberOfPixels, m_NumberOfTrainingImages);
  m_EigenVectors.fill(0);


  for (unsigned int img_number = 0; img_number < m_NumberOfTrainingImages; ++img_number)
  {
    InputImageConstIterator tempImageItA = m_InputImageIteratorArray[img_number];
    for (unsigned int pix_number = 0; pix_number < m_NumberOfPixels; ++pix_number)
    {
      const double pix_value = tempImageItA.Get();
      for (unsigned int vec_number = 0; vec_number < m_NumberOfTrainingImages; ++vec_number)
      {
        m_EigenVectors[pix_number][vec_number] +=
          (pix_value * eigenVectorsOfInnerProductMatrix[img_number][vec_number]);
      }
      ++tempImageItA;
    }
  }

  m_EigenVectors.normalize_columns();

  m_EigenValues.set_size(m_NumberOfTrainingImages);

  // Extract the diagonal elements into the Eigen value vector
  m_EigenValues = (eigenVectors_eigenValues.D).diagonal();

  // Flip the eigen values since the eigen vectors output
  // is ordered in descending order of their corresponding eigen values.
  m_EigenValues.flip();

  // Normalize the eigen values
  m_EigenVectorNormalizedEnergy = m_EigenValues;
  m_EigenVectorNormalizedEnergy.normalize();
}

} // namespace itk

#endif
