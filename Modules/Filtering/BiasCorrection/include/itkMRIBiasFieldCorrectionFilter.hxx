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
#ifndef itkMRIBiasFieldCorrectionFilter_hxx
#define itkMRIBiasFieldCorrectionFilter_hxx


namespace itk
{

template <typename TImage, typename TImageMask, typename TBiasField>
MRIBiasEnergyFunction<TImage, TImageMask, TBiasField>::MRIBiasEnergyFunction()
  : m_BiasField(nullptr)
  , m_Image(nullptr)
  , m_Mask(nullptr)
{
  for (unsigned int & factor : m_SamplingFactor)
  {
    factor = 1;
  }
}

template <typename TImage, typename TImageMask, typename TBiasField>
void
MRIBiasEnergyFunction<TImage, TImageMask, TBiasField>::InitializeDistributions(Array<double> classMeans,
                                                                               Array<double> classSigmas)
{
  m_InternalEnergyFunction = std::make_unique<InternalEnergyFunction>(classMeans, classSigmas);
}

template <typename TImage, typename TImageMask, typename TBiasField>
unsigned int
MRIBiasEnergyFunction<TImage, TImageMask, TBiasField>::GetNumberOfParameters() const
{
  if (m_BiasField == nullptr)
  {
    return 0;
  }
  return m_BiasField->GetNumberOfCoefficients();
}

template <typename TImage, typename TImageMask, typename TBiasField>
auto
MRIBiasEnergyFunction<TImage, TImageMask, TBiasField>::GetValue(const ParametersType & parameters) const -> MeasureType
{
  if (m_Image.IsNull())
  {
    itkExceptionMacro("Image is null");
  }

  if (!m_InternalEnergyFunction)
  {
    itkExceptionMacro("EnergyFunction is null");
  }

  if (m_BiasField == nullptr)
  {
    itkExceptionMacro("BiasField is null");
  }

  MeasureType total = 0.0;

  m_BiasField->SetCoefficients(parameters);

  // Mask, bias and original image have the same dimensions
  // and corresponding indexes

  if (m_SamplingFactor[0] == 1 && m_SamplingFactor[1] == 1 && m_SamplingFactor[2] == 1)
  {
    ImageRegionIterator<ImageType>             iIter(m_Image, m_Region);
    typename TBiasField::SimpleForwardIterator bIter(m_BiasField);

    bIter.Begin();
    iIter.GoToBegin();
    total = 0.0;

    // Fastest for full sampling
    if (!m_Mask)
    {
      while (!bIter.IsAtEnd())
      {
        const double diff = iIter.Get() - bIter.Get();
        total = total + (*m_InternalEnergyFunction)(diff);
        ++bIter;
        ++iIter;
      }
    }
    else
    {
      ImageRegionIterator<MaskType> mIter(m_Mask, m_Region);
      mIter.GoToBegin();
      while (!bIter.IsAtEnd())
      {
        if (mIter.Get() > 0.0)
        {
          const double diff = iIter.Get() - bIter.Get();
          total = total + (*m_InternalEnergyFunction)(diff);
        }
        ++bIter;
        ++iIter;
        ++mIter;
      }
    }
  }
  else
  {
    typename ImageType::IndexType  origIndex = m_Region.GetIndex();
    typename TBiasField::IndexType indexBias(SpaceDimension);
    typename ImageType::SizeType   size = m_Region.GetSize();
    // Use indexing for incomplete sampling

    if (!m_Mask)
    {
      indexBias[2] = 0;
      typename ImageType::IndexType curIndex;
      for (curIndex[2] = origIndex[2]; curIndex[2] < origIndex[2] + (IndexValueType)size[2];
           curIndex[2] = curIndex[2] + (IndexValueType)m_SamplingFactor[2])
      {
        indexBias[1] = static_cast<int>((curIndex[2] % 2) * 0.5 * m_SamplingFactor[1]);
        for (curIndex[1] = origIndex[1]; curIndex[1] < origIndex[1] + (IndexValueType)size[1];
             curIndex[1] = curIndex[1] + (IndexValueType)m_SamplingFactor[1])
        {
          indexBias[0] = static_cast<int>((curIndex[2] % 2) * 0.5 * m_SamplingFactor[0]);
          for (curIndex[0] = origIndex[0]; curIndex[0] < origIndex[0] + (IndexValueType)size[0];
               curIndex[0] = curIndex[0] + (IndexValueType)m_SamplingFactor[0])
          {
            const double biasVal = m_BiasField->Evaluate(indexBias);
            const double imageVal = m_Image->GetPixel(curIndex);
            total += (*m_InternalEnergyFunction)(imageVal - biasVal);
            indexBias[0] = indexBias[0] + m_SamplingFactor[0];
          }
          indexBias[1] = indexBias[1] + m_SamplingFactor[1];
        }
        indexBias[2] = indexBias[2] + m_SamplingFactor[2];
      }
    }
    else
    {
      indexBias[2] = 0;
      typename ImageType::IndexType curIndex;
      for (curIndex[2] = origIndex[2]; curIndex[2] < origIndex[2] + (IndexValueType)size[2];
           curIndex[2] = curIndex[2] + (IndexValueType)m_SamplingFactor[2])
      {
        indexBias[1] = static_cast<int>((curIndex[2] % 2) * 0.5 * m_SamplingFactor[1]);
        for (curIndex[1] = origIndex[1]; curIndex[1] < origIndex[1] + (IndexValueType)size[1];
             curIndex[1] = curIndex[1] + (IndexValueType)m_SamplingFactor[1])
        {
          indexBias[0] = static_cast<int>((curIndex[2] % 2) * 0.5 * m_SamplingFactor[0]);
          for (curIndex[0] = origIndex[0]; curIndex[0] < origIndex[0] + (IndexValueType)size[0];
               curIndex[0] = curIndex[0] + (IndexValueType)m_SamplingFactor[0])
          {
            if (m_Mask->GetPixel(curIndex) > 0.0)
            {
              const double biasVal = m_BiasField->Evaluate(indexBias);
              const double imageVal = m_Image->GetPixel(curIndex);
              total += (*m_InternalEnergyFunction)(imageVal - biasVal);
            }
            indexBias[0] = indexBias[0] + m_SamplingFactor[0];
          }
          indexBias[1] = indexBias[1] + m_SamplingFactor[1];
        }
        indexBias[2] = indexBias[2] + m_SamplingFactor[2];
      }
    }
  }

  return total;
}

template <typename TInputImage, typename TOutputImage, typename TMaskImage>
MRIBiasFieldCorrectionFilter<TInputImage, TOutputImage, TMaskImage>::MRIBiasFieldCorrectionFilter()
  : m_EnergyFunction(nullptr)
  , m_NormalVariateGenerator(NormalVariateGeneratorType::New())
  , m_InputMask(nullptr)
  , m_OutputMask(nullptr)
  , m_InternalInput(InternalImageType::New())
  , m_SlabBackgroundMinimumThreshold(NumericTraits<InputImagePixelType>::min())
  , m_OptimizerShrinkFactor(std::pow(m_OptimizerGrowthFactor, -0.25))
{
  m_NormalVariateGenerator->Initialize(time(nullptr));

  if constexpr (ImageDimension == 3)
  {
    m_UsingInterSliceIntensityCorrection = true;
    m_SlicingDirection = 2;
  }
  else
  {
    m_UsingInterSliceIntensityCorrection = false;
  }

  this->SetNumberOfLevels(2);
}

template <typename TInputImage, typename TOutputImage, typename TMaskImage>
void
MRIBiasFieldCorrectionFilter<TInputImage, TOutputImage, TMaskImage>::SetNumberOfLevels(unsigned int num)
{
  if (m_NumberOfLevels == num)
  {
    return;
  }

  this->Modified();

  // Clamp value to be at least one
  m_NumberOfLevels = num;
  if (m_NumberOfLevels < 1)
  {
    m_NumberOfLevels = 1;
  }

  // Resize the schedules
  m_Schedule = ScheduleType(m_NumberOfLevels, ImageDimension, 0);

  // Determine initial shrink factor
  unsigned int startfactor = 1;
  startfactor = startfactor << (m_NumberOfLevels - 1);

  // Set the starting shrink factors
  this->SetStartingShrinkFactors(startfactor);
}

template <typename TInputImage, typename TOutputImage, typename TMaskImage>
void
MRIBiasFieldCorrectionFilter<TInputImage, TOutputImage, TMaskImage>::SetStartingShrinkFactors(unsigned int factor)
{
  const auto fixedArray = FixedArray<unsigned int, ImageDimension>::Filled(factor);
  this->SetStartingShrinkFactors(fixedArray.GetDataPointer());
}

template <typename TInputImage, typename TOutputImage, typename TMaskImage>
void
MRIBiasFieldCorrectionFilter<TInputImage, TOutputImage, TMaskImage>::SetStartingShrinkFactors(
  const unsigned int * factors)
{
  for (unsigned int dim = 0; dim < ImageDimension; ++dim)
  {
    m_Schedule[0][dim] = factors[dim];
    if (m_Schedule[0][dim] == 0)
    {
      m_Schedule[0][dim] = 1;
    }
  }

  for (unsigned int level = 1; level < m_NumberOfLevels; ++level)
  {
    for (unsigned int dim = 0; dim < ImageDimension; ++dim)
    {
      m_Schedule[level][dim] = m_Schedule[level - 1][dim] / 2;
      if (m_Schedule[level][dim] == 0)
      {
        m_Schedule[level][dim] = 1;
      }
    }
  }

  this->Modified();
}

template <typename TInputImage, typename TOutputImage, typename TMaskImage>
const unsigned int *
MRIBiasFieldCorrectionFilter<TInputImage, TOutputImage, TMaskImage>::GetStartingShrinkFactors() const
{
  return (m_Schedule.data_block());
}

template <typename TInputImage, typename TOutputImage, typename TMaskImage>
void
MRIBiasFieldCorrectionFilter<TInputImage, TOutputImage, TMaskImage>::SetSchedule(const ScheduleType & schedule)
{
  if (schedule.rows() != m_NumberOfLevels || schedule.columns() != ImageDimension)
  {
    itkDebugMacro("Schedule has wrong dimensions");
    return;
  }

  if (schedule == m_Schedule)
  {
    return;
  }

  this->Modified();
  for (unsigned int level = 0; level < m_NumberOfLevels; ++level)
  {
    for (unsigned int dim = 0; dim < ImageDimension; ++dim)
    {
      m_Schedule[level][dim] = schedule[level][dim];

      // set schedule to max( 1, min(schedule[level],
      //  schedule[level-1] );
      if (level > 0)
      {
        m_Schedule[level][dim] = std::min(m_Schedule[level][dim], m_Schedule[level - 1][dim]);
      }

      if (m_Schedule[level][dim] < 1)
      {
        m_Schedule[level][dim] = 1;
      }
    }
  }
}

template <typename TInputImage, typename TOutputImage, typename TMaskImage>
bool
MRIBiasFieldCorrectionFilter<TInputImage, TOutputImage, TMaskImage>::IsScheduleDownwardDivisible(
  const ScheduleType & schedule)
{
  for (unsigned int ilevel = 0; ilevel < schedule.rows() - 1; ++ilevel)
  {
    for (unsigned int idim = 0; idim < schedule.columns(); ++idim)
    {
      if (schedule[ilevel][idim] == 0)
      {
        return false;
      }
      if ((schedule[ilevel][idim] % schedule[ilevel + 1][idim]) > 0)
      {
        return false;
      }
    }
  }

  return true;
}

template <typename TInputImage, typename TOutputImage, typename TMaskImage>
void
MRIBiasFieldCorrectionFilter<TInputImage, TOutputImage, TMaskImage>::SetInputMask(ImageMaskType * inputMask)
{
  if (m_InputMask != inputMask)
  {
    if (this->CheckMaskImage(inputMask))
    {
      m_InputMask = inputMask;
      this->Modified();
    }
    else
    {
      itkExceptionMacro("The size of the mask differs from the input image");
    }
  }
}

template <typename TInputImage, typename TOutputImage, typename TMaskImage>
void
MRIBiasFieldCorrectionFilter<TInputImage, TOutputImage, TMaskImage>::SetOutputMask(ImageMaskType * outputMask)
{
  if (m_OutputMask != outputMask)
  {
    if (this->CheckMaskImage(outputMask))
    {
      m_OutputMask = outputMask;
      this->Modified();
    }
    else
    {
      itkExceptionMacro("The size of the mask differs from the input image");
    }
  }
}

template <typename TInputImage, typename TOutputImage, typename TMaskImage>
void
MRIBiasFieldCorrectionFilter<TInputImage, TOutputImage, TMaskImage>::Initialize()
{
  m_InternalInput->SetRegions(this->GetInput()->GetLargestPossibleRegion());
  m_InternalInput->Allocate();

  // Copy the input image to the internal image and cast the type to
  // float (InternalImageType::PixelType)
  CopyAndConvertImage(this->GetInput(), m_InternalInput.GetPointer(), this->GetInput()->GetLargestPossibleRegion());

  // If the bias is multiplicative, we will use logarithm
  // the following section applies logarithm to key parameters and
  // image data
  if (this->GetBiasFieldMultiplicative())
  {
    const unsigned int size = m_TissueClassMeans.Size();
    for (unsigned int i = 0; i < size; ++i)
    {
      m_TissueClassSigmas[i] = std::log(1.0 + m_TissueClassSigmas[i] / (m_TissueClassMeans[i] + 1.0));
      m_TissueClassMeans[i] = std::log(m_TissueClassMeans[i] + 1.0);
    }

    m_OptimizerInitialRadius = std::log(1.0 + m_OptimizerInitialRadius);

    this->Log1PImage(m_InternalInput, m_InternalInput.GetPointer());
  }

  // Initialize the energy function
  if (m_TissueClassMeans.Size() < 1)
  {
    itkExceptionMacro("Tissue Class Means is empty");
  }

  if (!m_EnergyFunction)
  {
    m_EnergyFunction = EnergyFunctionType::New();
    m_EnergyFunction->InitializeDistributions(m_TissueClassMeans, m_TissueClassSigmas);
  }

  m_EnergyFunction->SetImage(m_InternalInput);

  if (m_InputMask.IsNotNull())
  {
    m_EnergyFunction->SetMask(m_InputMask);
  }
}

template <typename TInputImage, typename TOutputImage, typename TMaskImage>
auto
MRIBiasFieldCorrectionFilter<TInputImage, TOutputImage, TMaskImage>::EstimateBiasField(InputImageRegionType region,
                                                                                       unsigned int         degree,
                                                                                       int maximumIteration)
  -> BiasFieldType
{
  itkDebugMacro("Estimating bias field ");

  bool                          cleanCoeffs = false;
  BiasFieldType::DomainSizeType biasSize;
  this->GetBiasFieldSize(region, biasSize);
  BiasFieldType bias(static_cast<unsigned int>(biasSize.size()), degree, biasSize);

  if (bias.GetNumberOfCoefficients() == m_BiasFieldCoefficients.size())
  {
    bias.SetCoefficients(m_BiasFieldCoefficients);
  }
  else
  { // Init all bias field coefficients to zero
    cleanCoeffs = true;
    m_BiasFieldCoefficients.clear();
    for (unsigned int i = 0; i < bias.GetNumberOfCoefficients(); ++i)
    {
      m_BiasFieldCoefficients.push_back(0.0);
    }
    bias.SetCoefficients(m_BiasFieldCoefficients);
  }

  // Update the energy function;
  m_EnergyFunction->SetBiasField(&bias);
  m_EnergyFunction->SetRegion(region);

  // Initialize the 1+1 optimizer
  auto optimizer = OptimizerType::New();
  optimizer->SetDebug(this->GetDebug());
  optimizer->SetNormalVariateGenerator(m_NormalVariateGenerator);
  optimizer->SetCostFunction(m_EnergyFunction);
  optimizer->SetMaximumIteration(maximumIteration);
  if (m_OptimizerGrowthFactor > 0)
  {
    if (m_OptimizerShrinkFactor > 0)
    {
      optimizer->Initialize(m_OptimizerInitialRadius, m_OptimizerGrowthFactor, m_OptimizerShrinkFactor);
    }
    else
    {
      optimizer->Initialize(m_OptimizerInitialRadius, m_OptimizerGrowthFactor);
    }
  }
  else
  {
    if (m_OptimizerShrinkFactor > 0)
    {
      optimizer->Initialize(m_OptimizerInitialRadius, m_OptimizerShrinkFactor);
    }
    else
    {
      optimizer->Initialize(m_OptimizerInitialRadius);
    }
  }

  Array<double> scales(bias.GetNumberOfCoefficients());
  scales.Fill(100);
  optimizer->SetScales(scales);

  const int                                   noOfBiasFieldCoefficients = bias.GetNumberOfCoefficients();
  typename EnergyFunctionType::ParametersType initialPosition(noOfBiasFieldCoefficients);
  for (int i = 0; i < noOfBiasFieldCoefficients; ++i)
  {
    initialPosition[i] = bias.GetCoefficients()[i];
  }
  optimizer->SetInitialPosition(initialPosition);

  try
  {
    for (unsigned int level = 0; level < m_NumberOfLevels; ++level)
    {
      typename EnergyFunctionType::SamplingFactorType energySampling;
      for (unsigned int dim = 0; dim < ImageDimension; ++dim)
      {
        energySampling[dim] = m_Schedule[level][dim];
      }
      m_EnergyFunction->SetSamplingFactors(energySampling);
      optimizer->MaximizeOff();
      optimizer->StartOptimization();
    }
  }
  catch (const ExceptionObject & ie)
  {
    std::cerr << ie << std::endl;
  }
  catch (const std::exception & e)
  {
    std::cerr << e.what() << std::endl;
  }

  bias.SetCoefficients(optimizer->GetCurrentPosition());
  if (cleanCoeffs)
  {
    m_BiasFieldCoefficients.clear();
  }

  return bias;
}

template <typename TInputImage, typename TOutputImage, typename TMaskImage>
void
MRIBiasFieldCorrectionFilter<TInputImage, TOutputImage, TMaskImage>::CorrectImage(BiasFieldType &      bias,
                                                                                  InputImageRegionType region)
{
  itkDebugMacro("Correcting the image ");
  using Pixel = InternalImagePixelType;
  ImageRegionIterator<InternalImageType> iIter(m_InternalInput, region);
  BiasFieldType::SimpleForwardIterator   bIter(&bias);

  BiasFieldType::DomainSizeType biasSize;
  this->GetBiasFieldSize(region, biasSize);

  bIter.Begin();
  if (m_OutputMask.IsNotNull())
  {
    itkDebugMacro("Output mask is being used");
    ImageRegionIterator<ImageMaskType> mIter(m_OutputMask, region);
    mIter.GoToBegin();
    while (!bIter.IsAtEnd())
    {
      const double inputPixel = iIter.Get();
      const double diff = inputPixel - bIter.Get();
      if (mIter.Get() > 0.0)
      {
        iIter.Set((Pixel)diff);
      }
      else
      {
        iIter.Set((Pixel)inputPixel);
      }
      ++mIter;
      ++bIter;
      ++iIter;
    }
  }
  else
  {
    itkDebugMacro("Output mask is not being used");
    while (!bIter.IsAtEnd())
    {
      const double diff = iIter.Get() - bIter.Get();
      iIter.Set((Pixel)diff);
      ++bIter;
      ++iIter;
    }
  }
}

template <typename TInputImage, typename TOutputImage, typename TMaskImage>
void
MRIBiasFieldCorrectionFilter<TInputImage, TOutputImage, TMaskImage>::CorrectInterSliceIntensityInhomogeneity(
  InputImageRegionType region)
{
  const IndexValueType lastSlice =
    region.GetIndex()[m_SlicingDirection] + static_cast<IndexValueType>(region.GetSize()[m_SlicingDirection]);
  InputImageRegionType sliceRegion;
  InputImageIndexType  index = region.GetIndex();
  InputImageSizeType   size = region.GetSize();

  sliceRegion.SetSize(size);
  BiasFieldType bias = this->EstimateBiasField(sliceRegion, 0, m_InterSliceCorrectionMaximumIteration);
  const double  globalBiasCoef = bias.GetCoefficients()[0];

  size[m_SlicingDirection] = 1;
  sliceRegion.SetSize(size);

  while (index[m_SlicingDirection] < lastSlice)
  {
    sliceRegion.SetIndex(index);

    m_BiasFieldCoefficients.clear();
    m_BiasFieldCoefficients.push_back(globalBiasCoef);
    bias = this->EstimateBiasField(sliceRegion, 0, m_InterSliceCorrectionMaximumIteration);

    this->CorrectImage(bias, sliceRegion);
    index[m_SlicingDirection] += 1;
  }
}

template <typename TInputImage, typename TOutputImage, typename TMaskImage>
void
MRIBiasFieldCorrectionFilter<TInputImage, TOutputImage, TMaskImage>::GenerateData()
{
  itkDebugMacro("Initializing filter...");
  this->Initialize();
  itkDebugMacro("Filter initialized.");

  if (m_UsingSlabIdentification)
  {
    itkDebugMacro("Searching slabs...");

    auto identifier = MRASlabIdentifier<InputImageType>::New();
    // Find slabs
    identifier->SetImage(this->GetInput());
    identifier->SetNumberOfSamples(m_SlabNumberOfSamples);
    identifier->SetBackgroundMinimumThreshold(m_SlabBackgroundMinimumThreshold);
    identifier->SetTolerance(m_SlabTolerance);
    identifier->GenerateSlabRegions();
    m_Slabs = identifier->GetSlabRegionVector();

    itkDebugMacro(<< m_Slabs.size() << " slabs found.");
  }
  else
  {
    // Creates a single region which is the largest possible region of
    // the input image.
    m_Slabs.clear();
    m_Slabs.push_back(this->GetInput()->GetLargestPossibleRegion());
  }

  this->AdjustSlabRegions(m_Slabs, this->GetOutput()->GetRequestedRegion());
  itkDebugMacro("After adjustment, there are " << static_cast<SizeValueType>(m_Slabs.size()) << " slabs.");

  auto iter = m_Slabs.begin();

  BiasFieldType::DomainSizeType biasSize;
  this->GetBiasFieldSize(*iter, biasSize);
  BiasFieldType bias(static_cast<unsigned int>(biasSize.size()), m_BiasFieldDegree, biasSize);

  const int           nCoef = bias.GetNumberOfCoefficients();
  std::vector<double> lastBiasCoef;
  lastBiasCoef.resize(nCoef);
  for (int i = 0; i < nCoef; ++i)
  {
    lastBiasCoef[i] = 0;
  }

  while (iter != m_Slabs.end())
  {
    // Correct inter-slice intensity inhomogeneity
    // using 0th degree Legendre polynomial

    if (m_UsingInterSliceIntensityCorrection)
    {
      itkDebugMacro("  Correcting inter-slice intensity...");
      this->CorrectInterSliceIntensityInhomogeneity(*iter);
      itkDebugMacro("  Inter-slice intensity corrected.");
    }

    // Correct 3D bias
    if (m_UsingBiasFieldCorrection)
    {
      itkDebugMacro("  Correcting bias...");

      m_BiasFieldCoefficients.clear();
      for (int i = 0; i < nCoef; ++i)
      {
        m_BiasFieldCoefficients.push_back(lastBiasCoef[i]);
      }

      bias = this->EstimateBiasField(*iter, m_BiasFieldDegree, m_VolumeCorrectionMaximumIteration);

      m_EstimatedBiasFieldCoefficients.resize(bias.GetCoefficients().size());
      for (unsigned int k = 0; k < bias.GetCoefficients().size(); ++k)
      {
        m_EstimatedBiasFieldCoefficients[k] = bias.GetCoefficients()[k];
      }

      m_BiasFieldCoefficients.clear();
      for (int i = 0; i < nCoef; ++i)
      {
        lastBiasCoef[i] = (bias.GetCoefficients()[i] + lastBiasCoef[i]) / 2;
        m_BiasFieldCoefficients.push_back(lastBiasCoef[i]);
      }

      this->CorrectImage(bias, *iter);
      itkDebugMacro("  Bias corrected.");
    }
    ++iter;
  }

  if (this->GetBiasFieldMultiplicative())
  {
    // For multiple calls it is necessary to restore the tissue classes
    // and the Initial Radius (i.e. everything that has been
    // log-transformed in the initialization)
    const unsigned int size = m_TissueClassMeans.Size();
    for (unsigned int i = 0; i < size; ++i)
    {
      m_TissueClassMeans[i] = std::exp(m_TissueClassMeans[i]) - 1.0;
      m_TissueClassSigmas[i] = std::exp(m_TissueClassSigmas[i]) * (1.0 + m_TissueClassMeans[i]) - m_TissueClassMeans[i];
    }
    m_OptimizerInitialRadius = std::exp(m_OptimizerInitialRadius) - 1.0;
  }

  if (m_GeneratingOutput)
  {
    itkDebugMacro("Generating the output image...");

    if (this->GetBiasFieldMultiplicative())
    {
      ExpImage(m_InternalInput, m_InternalInput.GetPointer());
    }

    OutputImageType * output = this->GetOutput();
    output->SetRegions(this->GetInput()->GetLargestPossibleRegion());
    output->Allocate();
    CopyAndConvertImage(m_InternalInput.GetPointer(), output, output->GetRequestedRegion());

    itkDebugMacro("The output image generated.");
  }
}

template <typename TInputImage, typename TOutputImage, typename TMaskImage>
void
MRIBiasFieldCorrectionFilter<TInputImage, TOutputImage, TMaskImage>::SetTissueClassStatistics(
  const Array<double> & means,
  const Array<double> & sigmas)
{
  const SizeValueType meanSize = means.Size();
  const SizeValueType sigmaSize = sigmas.Size();

  if (meanSize == 0)
  {
    itkExceptionMacro("arrays of Means is empty");
  }

  if (sigmaSize == 0)
  {
    itkExceptionMacro("arrays of Sigmas is empty");
  }

  if (meanSize != sigmaSize)
  {
    itkExceptionMacro("arrays of Means and Sigmas have different lengths");
  }

  m_TissueClassMeans = means;
  m_TissueClassSigmas = sigmas;
}

template <typename TInputImage, typename TOutputImage, typename TMaskImage>
bool
MRIBiasFieldCorrectionFilter<TInputImage, TOutputImage, TMaskImage>::CheckMaskImage(ImageMaskType * mask)
{
  const InputImageRegionType region = this->GetInput()->GetBufferedRegion();

  const ImageMaskRegionType maskRegion = mask->GetBufferedRegion();

  if (region.GetSize() != maskRegion.GetSize())
  {
    return false;
  }
  return true;
}

template <typename TInputImage, typename TOutputImage, typename TMaskImage>
void
MRIBiasFieldCorrectionFilter<TInputImage, TOutputImage, TMaskImage>::Log1PImage(InternalImageType * source,
                                                                                InternalImageType * target)
{
  const InternalImageRegionType region = source->GetRequestedRegion();

  ImageRegionIterator<InternalImageType> s_iter(source, region);
  ImageRegionIterator<InternalImageType> t_iter(target, region);

  InternalImagePixelType pixel;
  while (!s_iter.IsAtEnd())
  {
    pixel = s_iter.Get();

    if (pixel < 0)
    {
      t_iter.Set(0.0);
    }
    else
    {
      t_iter.Set(std::log(pixel + 1));
    }

    ++s_iter;
    ++t_iter;
  }
}

template <typename TInputImage, typename TOutputImage, typename TMaskImage>
void
MRIBiasFieldCorrectionFilter<TInputImage, TOutputImage, TMaskImage>::ExpImage(InternalImageType * source,
                                                                              InternalImageType * target)
{
  const InternalImageRegionType region = source->GetLargestPossibleRegion();

  ImageRegionIterator<InternalImageType> s_iter(source, region);
  ImageRegionIterator<InternalImageType> t_iter(target, region);

  while (!s_iter.IsAtEnd())
  {
    double temp = s_iter.Get();
    // t_iter.Set( m_EnergyFunction->GetEnergy0(temp));
    temp = std::exp(temp) - 1;
    t_iter.Set((InternalImagePixelType)temp);

    ++s_iter;
    ++t_iter;
  }
}

template <typename TInputImage, typename TOutputImage, typename TMaskImage>
void
MRIBiasFieldCorrectionFilter<TInputImage, TOutputImage, TMaskImage>::GetBiasFieldSize(
  InputImageRegionType            region,
  BiasFieldType::DomainSizeType & biasSize)
{
  InputImageSizeType size = region.GetSize();
  int                biasDim = 0;
  for (unsigned int dim = 0; dim < ImageDimension; ++dim)
  {
    if (size[dim] > 1)
    {
      ++biasDim;
    }
  }

  biasSize.resize(biasDim);

  biasDim = 0;
  for (unsigned int dim = 0; dim < ImageDimension; ++dim)
  {
    if (size[dim] > 1)
    {
      biasSize[biasDim] = size[dim];
      ++biasDim;
    }
  }
}

template <typename TInputImage, typename TOutputImage, typename TMaskImage>
void
MRIBiasFieldCorrectionFilter<TInputImage, TOutputImage, TMaskImage>::AdjustSlabRegions(
  SlabRegionVectorType & slabs,
  OutputImageRegionType  requestedRegion)
{
  OutputImageIndexType indexFirst = requestedRegion.GetIndex();
  OutputImageSizeType  size = requestedRegion.GetSize();
  OutputImageIndexType indexLast = indexFirst;

  for (SizeValueType i = 0; i < ImageDimension; ++i)
  {
    indexLast[i] = indexFirst[i] + static_cast<IndexValueType>(size[i]) - 1;
  }

  const IndexValueType coordFirst = indexFirst[m_SlicingDirection];
  const IndexValueType coordLast = indexLast[m_SlicingDirection];


  OutputImageSizeType  tempSize = size;
  OutputImageIndexType tempIndex = indexFirst;

  auto iter = slabs.begin();
  while (iter != slabs.end())
  {
    const IndexValueType coordFirst2 = iter->GetIndex()[m_SlicingDirection];
    const IndexValueType coordLast2 =
      coordFirst2 + static_cast<IndexValueType>(iter->GetSize()[m_SlicingDirection]) - 1;

    IndexValueType tempCoordFirst = coordFirst2;
    if (coordFirst > coordFirst2)
    {
      tempCoordFirst = coordFirst;
    }

    IndexValueType tempCoordLast = coordLast2;
    if (coordLast < coordLast2)
    {
      tempCoordLast = coordLast;
    }

    if (tempCoordFirst <= tempCoordLast)
    {
      tempIndex[m_SlicingDirection] = tempCoordFirst;
      tempSize[m_SlicingDirection] = tempCoordLast - tempCoordFirst + 1;
      OutputImageRegionType tempRegion;
      tempRegion.SetIndex(tempIndex);
      tempRegion.SetSize(tempSize);
      *iter = tempRegion;
    }
    else
    {
      // No overlapping, so remove the slab from the vector
      slabs.erase(iter);
    }
    ++iter;
  }
}

template <typename TInputImage, typename TOutputImage, typename TMaskImage>
void
MRIBiasFieldCorrectionFilter<TInputImage, TOutputImage, TMaskImage>::PrintSelf(std::ostream & os, Indent indent) const
{
  Superclass::PrintSelf(os, indent);

  itkPrintSelfObjectMacro(EnergyFunction);
  itkPrintSelfObjectMacro(NormalVariateGenerator);

  itkPrintSelfObjectMacro(InputMask);
  itkPrintSelfObjectMacro(OutputMask);
  itkPrintSelfObjectMacro(InternalInput);

  os << indent << "SlicingDirection: " << m_SlicingDirection << std::endl;

  itkPrintSelfBooleanMacro(BiasFieldMultiplicative);
  itkPrintSelfBooleanMacro(UsingInterSliceIntensityCorrection);
  itkPrintSelfBooleanMacro(UsingSlabIdentification);
  itkPrintSelfBooleanMacro(UsingBiasFieldCorrection);
  itkPrintSelfBooleanMacro(GeneratingOutput);

  os << indent << "SlabNumberOfSamples: " << m_SlabNumberOfSamples << std::endl;
  os << indent << "SlabBackgroundMinimumThreshold: "
     << static_cast<typename NumericTraits<InputImagePixelType>::PrintType>(m_SlabBackgroundMinimumThreshold)
     << std::endl;
  os << indent << "SlabTolerance" << m_SlabTolerance << std::endl;

  os << indent << "BiasFieldDegree: " << m_BiasFieldDegree << std::endl;
  os << indent << "NumberOfLevels: " << m_NumberOfLevels << std::endl;
  os << indent << "Schedule: " << static_cast<typename NumericTraits<ScheduleType>::PrintType>(m_Schedule) << std::endl;

  os << indent << "VolumeCorrectionMaximumIteration: " << m_VolumeCorrectionMaximumIteration << std::endl;
  os << indent << "InterSliceCorrectionMaximumIteration: " << m_InterSliceCorrectionMaximumIteration << std::endl;

  os << indent << "OptimizerInitialRadius: " << m_OptimizerInitialRadius << std::endl;
  os << indent << "OptimizerGrowthFactor: " << m_OptimizerGrowthFactor << std::endl;
  os << indent << "OptimizerShrinkFactor: " << m_OptimizerShrinkFactor << std::endl;

  os << indent << "TissueClassMeans: " << m_TissueClassMeans << std::endl;
  os << indent << "TissueClassSigmas: " << m_TissueClassSigmas << std::endl;
}
} // end namespace itk

#endif
