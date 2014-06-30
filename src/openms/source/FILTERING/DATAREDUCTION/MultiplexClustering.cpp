// --------------------------------------------------------------------------
//                   OpenMS -- Open-Source Mass Spectrometry
// --------------------------------------------------------------------------
// Copyright The OpenMS Team -- Eberhard Karls University Tuebingen,
// ETH Zurich, and Freie Universitaet Berlin 2002-2013.
//
// This software is released under a three-clause BSD license:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of any author or any participating institution
//    may be used to endorse or promote products derived from this software
//    without specific prior written permission.
// For a full list of authors, refer to the file AUTHORS.
// --------------------------------------------------------------------------
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL ANY OF THE AUTHORS OR THE CONTRIBUTING
// INSTITUTIONS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// --------------------------------------------------------------------------
// $Maintainer: Lars Nilse $
// $Authors: Lars Nilse $
// --------------------------------------------------------------------------

#include <OpenMS/KERNEL/StandardTypes.h>
#include <OpenMS/CONCEPT/Constants.h>
#include <OpenMS/TRANSFORMATIONS/RAW2PEAK/PeakPickerHiRes.h>
#include <OpenMS/TRANSFORMATIONS/FEATUREFINDER/FeatureFinderAlgorithmPickedHelperStructs.h>
#include <OpenMS/FILTERING/DATAREDUCTION/PeakPattern.h>
#include <OpenMS/FILTERING/DATAREDUCTION/FilterResult.h>
#include <OpenMS/FILTERING/DATAREDUCTION/FilterResultRaw.h>
#include <OpenMS/FILTERING/DATAREDUCTION/FilterResultPeak.h>
#include <OpenMS/FILTERING/DATAREDUCTION/SplinePackage.h>
#include <OpenMS/FILTERING/DATAREDUCTION/SplineSpectrum.h>
#include <OpenMS/FILTERING/DATAREDUCTION/MultiplexFiltering.h>
#include <OpenMS/FILTERING/DATAREDUCTION/MultiplexClustering.h>
#include <OpenMS/MATH/STATISTICS/StatisticFunctions.h>
#include <OpenMS/MATH/MISC/CubicSpline2d.h>

#include <vector>
#include <algorithm>
#include <iostream>

using namespace std;

namespace OpenMS
{

	MultiplexClustering::MultiplexClustering(MSExperiment<Peak1D> exp_picked, std::vector<std::vector<PeakPickerHiRes::PeakBoundary> > boundaries, double x)
    : x_(x)
	{
        
        if (exp_picked.size() != boundaries.size())
        {
            throw Exception::IllegalArgument(__FILE__, __LINE__, __PRETTY_FUNCTION__,"Centroided data and the corresponding list of peak boundaries do not contain same number of spectra.");
        }
        
	}
    
    void MultiplexClustering::cluster()
    {
        x_ = 5;
    }
    
    MultiplexClustering::PeakWidthEstimator::PeakWidthEstimator(MSExperiment<Peak1D> exp_picked, std::vector<std::vector<PeakPickerHiRes::PeakBoundary> > boundaries, int quantiles)
    {
        if (exp_picked.size() != boundaries.size())
        {
            throw Exception::IllegalArgument(__FILE__, __LINE__, __PRETTY_FUNCTION__,"Centroided data and the corresponding list of peak boundaries do not contain same number of spectra.");
        }

        std::vector<double> mz;
        std::vector<double> peak_width;

        MSExperiment<Peak1D>::Iterator it_rt;
        vector<vector<PeakPickerHiRes::PeakBoundary> >::const_iterator it_rt_boundaries;
        for (it_rt = exp_picked.begin(), it_rt_boundaries = boundaries.begin();
            it_rt < exp_picked.end() && it_rt_boundaries < boundaries.end();
            ++it_rt, ++it_rt_boundaries)
        {
            MSSpectrum<Peak1D>::Iterator it_mz;
            vector<PeakPickerHiRes::PeakBoundary>::const_iterator it_mz_boundary;
            for (it_mz = it_rt->begin(), it_mz_boundary = it_rt_boundaries->begin();
                 it_mz < it_rt->end(), it_mz_boundary < it_rt_boundaries->end();
                 ++it_mz, ++it_mz_boundary)
            {
                mz.push_back(it_mz->getMZ());
                peak_width.push_back((*it_mz_boundary).mz_max - (*it_mz_boundary).mz_min);
            }
        }
        std::sort(mz.begin(), mz.end());
        std::sort(peak_width.begin(), peak_width.end());
        
        std::vector<double> mz_quantiles;
        std::vector<double> peak_width_quantiles;
        for (int i = 1; i < quantiles; ++i)
        {
            mz_quantiles.push_back(mz[(int) mz.size() * i / quantiles]);
            peak_width_quantiles.push_back(peak_width[(int) peak_width.size() * i / quantiles]);
        }
        
        mz_min_ = mz_quantiles.front();
        mz_max_ = mz_quantiles.back();
        
        spline_ = new CubicSpline2d(mz_quantiles, peak_width_quantiles);
    }
    
    double MultiplexClustering::PeakWidthEstimator::getPeakWidth(double mz) const
    {
        if (mz < mz_min_)
        {
            return (*spline_).eval(mz_min_);
        }
        else if (mz > mz_max_)
        {
            return (*spline_).eval(mz_max_);
        }
        else
        {
            return (*spline_).eval(mz);
        }
    }
    
}
