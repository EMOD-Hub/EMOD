
#include "stdafx.h"
#include "RANDOM.h"
#include "Debug.h"
#include <assert.h>
#include <math.h>
#include <cfloat>
#include <numeric> //std::accumulate()
#include "Log.h"
#include "IArchive.h"

#include <memory.h>    // memset
#include <climits>     // UINT_MAX
#include <wmmintrin.h> // AES
#include <smmintrin.h> // _mm_insert_epi64
#include <tmmintrin.h> // _mm_shuffle_epi8

// Needed the following to make Linux happy
#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

#define PRNG_COUNT  (1<<20) // Let's start with ~1 million

#if NUMPY
There is code below that is from Numpy.  The following license information is
for that code that is marked as numpy.

Copyright( c ) 2005 - 2020, NumPy Developers.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met :

*Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above
copyright notice, this list of conditions and the following
disclaimer in the documentation and / or other materials provided
with the distribution.

* Neither the name of the NumPy Developers nor the names of any
contributors may be used to endorse or promote products derived
from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES( INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION ) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
( INCLUDING NEGLIGENCE OR OTHERWISE ) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#endif

SETUP_LOGGING("RANDOM")

namespace Kernel
{
// ----------------------------------------------------------------------------
// --- RANDOMBASE
// ----------------------------------------------------------------------------

double RANDOMBASE::cdf_random_num_precision = 0.01; // precision of random number obtained from a cumulative probability function

RANDOMBASE::RANDOMBASE( size_t nCache )
    : cache_count( nCache )
    , index( UINT_MAX )   // Make sure fill_bits() is called...
    , random_bits( nullptr )
    , random_floats( nullptr )
    , bGauss( false )
    , eGauss_( 0.0f )
{
    if( cache_count == 0 )
    {
        cache_count = PRNG_COUNT;
    }
    random_bits = reinterpret_cast<uint32_t*>(malloc( cache_count * sizeof( uint32_t ) ));
    random_floats = reinterpret_cast<float*>(malloc( cache_count * sizeof( float ) ));
}

RANDOMBASE::~RANDOMBASE()
{
#ifdef _DEBUG
    free(random_bits);
    free(random_floats);
#endif
}

bool RANDOMBASE::SmartDraw( float prob )
{
    if( prob == 0.0 )
    {
        return false;
    }
    else if( prob == 1.0 )
    {
        return true;
    }
    else
    {
        return prob > e();
    }
}


uint32_t RANDOMBASE::ul()
{
    if (index >= cache_count)
    {
        fill_bits();
        bits_to_float();
        index = 0;
    }

    return random_bits[index++];
}

float RANDOMBASE::e()
{
    if (index >= cache_count)
    {
        fill_bits();
        bits_to_float();
        index = 0;
    }

    return random_floats[index++];
}

// Finds an uniformally distributed number between 0 (inclusive) and N (exclusive)
uint16_t RANDOMBASE::uniformZeroToN16( uint16_t N )
{
    uint32_t ulA = ul();
    uint32_t ll = (ulA & 0xFFFFL) * N;
    ll >>= 16;
    ll += (ulA >> 16) * N;
    return (uint16_t)(ll >> 16);
}

// Finds an uniformally distributed number between 0 (inclusive) and N (exclusive)
uint32_t RANDOMBASE::uniformZeroToN32( uint32_t N )
{
    uint64_t ulA = uint64_t( ul() );
    uint64_t ulB = uint64_t( ul() );
    ulB <<= 32;
    ulA += ulB;
    uint64_t ll = (ulA & 0xFFFFFFFFL) * N;
    ll >>= 32;
    return uint32_t( ll );
}

// randomRound() - rounds the 'val' to the nearest integer but randomly rounds it up or down.
// If the input value is 5.3, then 70% of the time it should return 5 and 30% of the time 6.
uint32_t RANDOMBASE::randomRound( float val )
{
    uint32_t i_val = uint32_t( val );
    float remainder = val - float( i_val );
    if( remainder > 0.0 )
    {
        if( e() < (1.0 - remainder) )
        {
            i_val = i_val; // round low;
        }
        else
        {
            i_val += 1;
        }
    }
    return i_val;
}

std::set<uint32_t> RANDOMBASE::chooseMofN( uint32_t M, uint32_t N )
{
    // ----------------------------------------------------------------------------------
    // --- Robert Floyd's Algorithm for Sampling without Replacement
    // --- http://www.nowherenearithaca.com/2013/05/robert-floyds-tiny-and-beautiful.html
    // ----------------------------------------------------------------------------------
    release_assert( M <= N );

    std::set<uint32_t> selected_indexes;
    for( uint32_t j = (N - M); j < N; j++ )
    {
        uint32_t index = uniformZeroToN32( j + 1 ); // +1 so that the method includes j
        if( selected_indexes.find( index ) == selected_indexes.end() )
        {
            selected_indexes.insert( index );
        }
        else
        {
            selected_indexes.insert( j );
        }
    }
    return selected_indexes;
}

void RANDOMBASE::fill_bits()
{
    assert(false);
}

#define FLOAT_EXP   8
#define DOUBLE_EXP 11

void RANDOMBASE::bits_to_float()
{
    __m128i m = _mm_set1_epi32(0x007FFFFF);
    __m128i o = _mm_set1_epi32(0x00000001);
    __m128 f = _mm_set1_ps(1.0f);
    __m128i fi = _mm_castps_si128(f);
    for (size_t i = 0; i < cache_count; i += 4)
    {
        __m128i x = _mm_load_si128(reinterpret_cast<__m128i const*>(random_bits+i));    // x = bits
//        x = _mm_and_si128(x, m);                                    // x &= 0x007FFFFF
        x = _mm_srli_epi32(x, (FLOAT_EXP+1));                       // x = x >> 9 (we just want the 23 mantissa bits)
        x = _mm_or_si128(x, o);                                     // x |= 0x00000001
        __m128i y = _mm_or_si128(fi, x);                            // y = fi | x
        __m128 z = _mm_castsi128_ps(y);                             // z = y interpreted as floating point
        z = _mm_sub_ps(z, f);                                       // z -= 1.0f
        _mm_store_ps(random_floats + i, z);
    }
}

/******************************Public*Routine******************************\
* eGauss()                                                                 *
*                                                                          *
* Returns a normal deviate for each call.  It generates two deviates at a  *
* time and caches one for the next call.                                   *
*                                                                          *
*  Sat 26-Mar-1994 14:10:12 -by- Charles Whitmer [chuckwh]                 *
* I actually wrote this back in 1982 for some physics stuff!               *
\**************************************************************************/

double RANDOMBASE::eGauss()
{
    if (bGauss)
    {
        bGauss = false;
        return eGauss_;
    }

    double rad, norm;
    double s, r1, r2;

    rad = -2.0 * log(ee());
    do
    {
        r1 = ee() - 0.5;
        r2 = ee() - 0.5;
        s = r1 * r1 + r2 * r2;
    }
    while (s > 0.25);
    norm = sqrt(rad / s);
    eGauss_ = r1 * norm;
    bGauss = true;
    return r2 * norm;
}

// !!!!!!!!!!!!!!!
// !!! FROM NUMPY
// !!!!!!!!!!!!!!!
// Gamma-distributed random number
float RANDOMBASE::rand_gamma( float k, float theta )
{
    float retv = 0.0f;
    float b, c, U, V, X, Y;

    if( k <= 0.0f || theta <= 0.0f )
    {
        // Invalid input; return error (-1.0f)
        retv = -1.0f;
    }
    else if( k == HUGE_VALF || theta == HUGE_VALF )
    {
        // Invalid input; return error (-1.0f)
        retv = -1.0f;
    }

    // Switch approximations based on shape factor
    if( k == 1.0f )
    {
        retv = -logf( e() );
    }
    else if( k < 1.0f )
    {
        for( ;;)
        {
            U = e();
            V = -logf( e() );
            if( U <= (1.0f - k) )
            {
                X = powf( U, 1.0f / k );
                if( X <= V )
                {
                    retv = X;
                    break;
                }
            }
            else
            {
                Y = -logf( (1.0f - U) / k );
                X = powf( 1.0f - k + k*Y, 1.0f / k );
                if( X <= (V + Y) )
                {
                    retv = X;
                    break;
                }
            }
        }
    }
    else
    {
        b = k - 1.0f / 3.0f;
        c = 1.0f / sqrtf( 9.0f*b );
        for( ;;)
        {
            do
            {
                X = static_cast<float>(eGauss());
                V = 1.0f + c*X;
            } while( V <= 0.0f );

            V = V*V*V;
            U = e();
            if( U < (1.0f - 0.0331f*X*X*X*X) )
            {
                retv = b*V;
                break;
            }
            if( logf( U ) < 0.5f*X*X + b*(1.0f - V + logf( V )) )
            {
                retv = b*V;
                break;
            }
        }
    }

    // Account for scaling
    retv *= theta;

    // Ensure > 0.0f
    retv += FLT_MIN;

    return retv;
}

uint64_t RANDOMBASE::negative_binomial( float n, float p )
{
    if( p <= 0.0 )
    {
        return 0.0;
    }

    float y = rand_gamma( n, (1 - p) / p );
    LOG_VALID_F("Gamma with k = %f and theta = %f generated %f \n", n, (1 - p) / p, y);
    return Poisson( y );
}

double RANDOMBASE::ee()
{
    union
    {
        double ee;
        struct
        {
            uint32_t Low;
            uint32_t High;
        };
    } ee_ul;

    uint32_t ll = ul();    // Choose a random 32 bits.

    ee_ul.ee = 1.0;
    ee_ul.High += (ll >> (DOUBLE_EXP + 1));
    ee_ul.Low = (ll << (31 - DOUBLE_EXP)) + (1 << (30 - DOUBLE_EXP));

    return ee_ul.ee - 1.0;
}

// Poisson() added by Philip Eckhoff, uses Gaussian approximation for ratetime>10
uint64_t RANDOMBASE::Poisson(double ratetime)
{
    if (ratetime <= 0)
    {
        return 0;
    }
    uint64_t events = 0;
    double Time = 0;
    double tempval;
    if (ratetime < 10)
    {
        while (Time < 1)
        {
            Time += -log(e()) / ratetime;
            if (Time < 1)
            {
                events++;
            }
        }
    }
    else
    {
        tempval = (eGauss() * sqrt(ratetime) + ratetime + .5);
        if (tempval < 0)
        {
            events = 0;
        }
        else
        {
            events = uint64_t(tempval);
        }
    }
    return events;
}

// Poisson_true added by Philip Eckhoff, actual Poisson, without approximation
uint32_t RANDOMBASE::Poisson_true(double ratetime)
{
    if (ratetime <= 0)
    {
        return 0;
    }
    uint32_t events = 0;
    double Time = 0;
    while (Time < 1)
    {
        Time += -log(e()) / ratetime;
        if (Time < 1)
        {
            events++;
        }
    }
    return events;
}

//  expdist() added by Philip Eckhoff
double RANDOMBASE::expdist(double rate) // rate = 1/mean
{
    if (rate == 0)
    {
        return 0;
    }
    else
    {
        return -log(e()) / rate;
    }
}

// Weibull(lambda, kappa) added by Daniel Klein
double RANDOMBASE::Weibull(double lambda, double kappa)
{
    if (lambda <= 0 || kappa <= 0)
    {
        return 0;
    }
    return lambda * pow( -log(e()), 1/kappa );
}

// NOTE: The only reason for the arguments to be floats is that
//       it kept the regression tests from changing.
//       We get a slightly different result for 1 / inv_kappa
//       if the numbers are doubles instead of floats.  This slightly
//       different number causes thing to change slightly.
double RANDOMBASE::Weibull2(float lambda, float inv_kappa)
{
    if( inv_kappa == 0.0 )
    {
        return lambda ;
    }
    else
    {
        return Weibull( lambda, 1.0f/inv_kappa );
    }
}

// LogLogistic(alpha, beta) added by Anna Bershteyn
// alpha is a scale parameter > 0, beta is a shape parameter > 0
double RANDOMBASE::LogLogistic(double alpha, double beta)
{
    if (alpha <= 0 || beta <= 0)
    {
        return 0;
    }
    double uniform_rand = e();
    return alpha * pow(uniform_rand/(1-uniform_rand), 1/beta);
}

double RANDOMBASE::time_varying_rate_dist( const std::vector <float>& v_rate, float timestep, float rate)
{ 
    double e_log = -log(e());
    double tempsum = 0.0f;
    int temp_break_step = 0;
    double ret= 0.0f;
    release_assert( v_rate.size()>0 );

    for ( auto rit = v_rate.begin(); rit!= v_rate.end()-1; ++rit )
    {
        tempsum = tempsum + double(*rit) * double(timestep) +  double(0.5*(*(rit+1)-*rit) * double(timestep))+ double(rate) * double(timestep);

        if (tempsum  > e_log)
        {
            int break_step = temp_break_step + 1;
            ret = double(break_step) * double(timestep)  - (tempsum - e_log)/( double(v_rate[break_step-1]) + 0.5*( v_rate[break_step]- v_rate[break_step - 1]) + double(rate)) ;

            return ret;
        }

        temp_break_step ++ ;
    }

    ret =  double(temp_break_step) *  double(timestep) + (e_log-tempsum)/(double(rate) + double(v_rate.back()));
    return ret;
}

uint64_t RANDOMBASE::binomial_true(uint64_t n, double p)
{
    int64_t tempval = 0;

    if (n <= 0 || p <= 0)
    {
        tempval = 0;
    }
    else if (p >= 1)
    {
        tempval = n;
    }
    else
    {
        for (int i = 0; i < n; i++)
        {
            if (e() < p)
            {
                tempval++;
            }
        }
    }

    if (tempval < 0)
    {
        tempval = 0;
    }

    if (uint64_t(tempval) > n)
    {
        tempval = n;
    }

    return uint64_t(tempval);
}

//---------------------------------------------------------------------------------
// From the distributions.c:
//
// Copyright 2005 Robert Kern( robert.kern@gmail.com )
// 
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files( the "Software" ),
// to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and / or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions :
// 
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// NOTE: This was the notice when the binomial_approx() code was extracted.
//---------------------------------------------------------------------------------
uint64_t RANDOMBASE::binomial_approx( uint64_t n, double p )
{
    if( p > 1.0 )
        p = 1.0;
    else if( p < 0.0 )
        p = 0.0;

    double p_tmp = (p < 0.5) ? p : (1 - p);

    uint64_t tempval = 0;
    if( p_tmp*n <= 30.0 )
    {
        tempval = numpy_binomial_inversion( n, p_tmp );
    }
    else
    {
        tempval = numpy_binomial_btpe( n, p_tmp );
    }
    tempval = (p < 0.5) ? tempval : n - tempval;

    return tempval;
}

// See numpy copyright notice above
uint64_t RANDOMBASE::numpy_binomial_inversion( uint64_t n, double p )
{
    double np = double(n) * p;
    double q = 1.0 - p;

    // In scenarios where the Vector Cohort models is used and there is vector migration,
    // the following line is used a lot.  The profiler has show this line to represent
    // a significant amount of the total processing.  I broke it apart so that one can see
    // the constributions of each function in the profiler.
    //double qn = exp( n * log( q ) );
    double log_q = log( q );
    double n_log_q = double(n) * log_q;
    double qn = exp( n_log_q );

    double px = qn;
    uint64_t bound = min( n, uint64_t(np + 10.0*sqrt( np*q + 1.0 )) );

    uint64_t X = 0;
    double U = e();
    while( U > px )
    {
        X++;
        if( X > bound )
        {
            X = 0;
            px = qn;
            U = e();
        }
        else
        {
            U -= px;
            px = (double(n - X + 1) * p * px) / (double(X) * q);
        }
    }
    return X;
}

// See numpy copyright notice above
uint64_t RANDOMBASE::numpy_binomial_btpe( uint64_t n, double p )
{
    /* initialize */
    double r = min( p, 1.0 - p );
    double q = 1.0 - r;
    double fm = double( n )*r + r;
    int64_t m = uint64_t( floor( fm ) );
    double p1 = floor( 2.195*sqrt( double( n )*r*q ) - 4.6*q ) + 0.5;
    double xm = double( m ) + 0.5;
    double xl = xm - p1;
    double xr = xm + p1;
    double c = 0.134 + 20.5 / (15.3 + double( m ));
    double a = (fm - xl) / (fm - xl*r);
    double laml = a*(1.0 + a / 2.0);
    a = (xr - fm) / (xr*q);
    double lamr = a*(1.0 + a / 2.0);
    double p2 = p1*(1.0 + 2.0*c);
    double p3 = p2 + c / laml;
    double p4 = p3 + c / lamr;

    double nrq = 0.0;
    double u = 0.0;
    double v = 0.0;
    int64_t y = 0;
    int64_t k = 0;

    /* sigh ... */
Step10:
    {
        nrq = double(n)*r*q;
        u = e()*p4;
        v = e();
        if( u > p1 ) goto Step20;
        y = int64_t(floor( xm - p1*v + u ));
        goto Step60;
    }

Step20:
    {
        if( u > p2 ) goto Step30;
        double x = xl + (u - p1) / c;
        v = v*c + 1.0 - fabs( double(m) - x + 0.5 ) / p1;
        if( v > 1.0 ) goto Step10;
        y = int64_t(floor( x ));
        goto Step50;
    }

Step30:
    {
        if( u > p3 ) goto Step40;
        y = int64_t(floor( xl + log( v ) / laml ));
        if( y < 0 ) goto Step10;
        v = v*(u - p2)*laml;
        goto Step50;
    }

Step40:
    {
        y = int64_t(floor( xr - log( v ) / lamr ));
        if( y > int64_t(n) ) goto Step10;
        v = v*(u - p3)*lamr;
    }

Step50:
    {
        k = llabs( y - m );
        if( (k > 20) && (k < (int64_t(nrq / 2.0) - 1)) ) goto Step52;

        double s = r / q;
        a = s*(n + 1);
        double F = 1.0;
        if( m < y )
        {
            for( int64_t i = m + 1; i <= y; i++ )
            {
                F *= (a / i - s);
            }
        }
        else if( m > y )
        {
            for( int64_t i = y + 1; i <= m; i++ )
            {
                F /= (a / i - s);
            }
        }
        if( v > F ) goto Step10;
        goto Step60;
    }

Step52:
    {
        double rho = (double(k) / (nrq))*((double(k)*(double(k) / 3.0 + 0.625) + 0.16666666666666666) / nrq + 0.5);
        double t = -double(k)*double(k) / (2.0 * nrq);
        double A = log( v );
        if( A < (t - rho) ) goto Step60;
        if( A >( t + rho ) ) goto Step10;

        double x1 = double(y + 1);
        double f1 = double(m + 1);
        double z  = double(n + 1 - m);
        double w  = double(n - y + 1);
        double x2 = x1*x1;
        double f2 = f1*f1;
        double z2 = z*z;
        double w2 = w*w;
        if( A > (xm*log( f1 / x1 )
                  + (double(n - m) + 0.5)*log( z / w )
                  + double(y - m)*log( w*r / (x1*q) )
                  + (13680.0 - (462.0 - (132.0 - (99.0 - 140.0 / f2) / f2) / f2) / f2) / f1 / 166320.0
                  + (13680.0 - (462.0 - (132.0 - (99.0 - 140.0 / z2) / z2) / z2) / z2) / z  / 166320.0
                  + (13680.0 - (462.0 - (132.0 - (99.0 - 140.0 / x2) / x2) / x2) / x2) / x1 / 166320.0
                  + (13680.0 - (462.0 - (132.0 - (99.0 - 140.0 / w2) / w2) / w2) / w2) / w  / 166320.0) )
        {
            goto Step10;
        }
    }

Step60:
    {
        if( p > 0.5 )
        {
            y = n - y;
        }
    }

    return y;
}

std::vector<uint64_t> RANDOMBASE::multinomial_approx( uint64_t N, const std::vector<float>& rProbabilities )
{
    std::vector<uint64_t> subsets(rProbabilities.size(), 0);

    uint64_t total_subsets = 0;
    double total_fraction = 0.0;

    for( int i = 0; i < rProbabilities.size(); i++ )
    {
        double expected_fraction_of_total = rProbabilities[ i ];

        uint64_t subset = 0;
        if( total_fraction < 1.0 )
        {
            uint64_t total_remaining = N - total_subsets;
            double expected_fraction_of_remaining = expected_fraction_of_total / (1.0 - total_fraction);
            subset = binomial_approx( total_remaining, expected_fraction_of_remaining );
        }
        subsets[i] = subset;

        total_fraction += expected_fraction_of_total;
        total_subsets += subset;
    }

    return subsets;
}

// !!!!!!!!!!!
// !!! NUMPY
// !!!!!!!!!!!
// assuming our samples are usually small (< 10) so we need less code from numpy
int64_t RANDOMBASE::numpy_hypergeometric_sample( int64_t good, int64_t bad, int64_t sample )
{
    int64_t total = good + bad;
    int64_t computed_sample = 0;
    if( sample > total / 2 )
    {
        computed_sample = total - sample;
    }
    else
    {
        computed_sample = sample;
    }

    int64_t remaining_total = total;
    int64_t remaining_good = good;

    while( (computed_sample > 0) && (remaining_good > 0) &&
           (remaining_total > remaining_good) )
    {
        --remaining_total;
        int64_t ran = uniformZeroToN32( remaining_total+1 );
        if( ran < remaining_good )
        {
            // Selected a "good" one, so decrement remaining_good.
            --remaining_good;
        }
        --computed_sample;
    }

    if( remaining_total == remaining_good )
    {
        // Only "good" choices are left.
        remaining_good -= computed_sample;
    }

    int64_t result = 0;
    if( sample > total / 2 )
    {
        result = remaining_good;
    }
    else
    {
        result = good - remaining_good;
    }

    return result;
}

// !!!!!!!!!!!
// !!! NUMPY
// !!!!!!!!!!!
// this is from random_multivariate_hypergeometric_marginals() in numpy
std::vector<uint32_t> RANDOMBASE::multivariate_hypergeometric( const std::vector<uint32_t>& rColors, uint32_t nsample )
{
    int64_t nsample64 = nsample;
    int64_t total = std::accumulate( rColors.begin(), rColors.end(), 0 );
    size_t num_colors = rColors.size();

    std::vector<uint32_t> variates(rColors.size(),0);
    if( (total == 0) || (nsample == 0) )
    {
        // Nothing to do.
        return variates;
    }

    bool more_than_half = nsample64 > (total / 2);
    if( more_than_half )
    {
        // NOTE: if nsample > total, then this will be a negative number
        nsample64 = total - nsample64;
    }

    for( size_t i = 0; i < num_colors; i += num_colors )
    {
        int64_t num_to_sample = nsample64;
        int64_t remaining = total;
        for( size_t j = 0; (num_to_sample > 0) && (j + 1 < num_colors); ++j )
        {
            remaining -= rColors[ j ];
            uint32_t r = numpy_hypergeometric_sample( rColors[ j ], remaining, num_to_sample );
            variates[ i + j ] = r;
            num_to_sample -= r;
        }

        if( num_to_sample > 0 )
        {
            variates[ i + num_colors - 1 ] = num_to_sample;
        }

        if( more_than_half )
        {
            for( size_t k = 0; k < num_colors; ++k )
            {
                variates[ i + k ] = rColors[ k ] - variates[ i + k ];
            }
        }
    }

    return variates;
}

double RANDOMBASE::get_cdf_random_num_precision()
{
    return cdf_random_num_precision;
}

void RANDOMBASE::serialize( IArchive& ar, RANDOMBASE* obj )
{
    RANDOMBASE& rb = *obj;
    ar.labelElement( "cache_count"   ) & rb.cache_count;

    if( ar.IsReader() )
    {
        free( rb.random_bits );
        free( rb.random_floats );

        rb.random_bits   = reinterpret_cast<uint32_t*>(malloc( rb.cache_count * sizeof( uint32_t ) ));
        rb.random_floats = reinterpret_cast<float*   >(malloc( rb.cache_count * sizeof( float    ) ));
    }

    ar.labelElement( "index"         ) & rb.index;
    ar.labelElement( "random_bits"   ); ar.serialize( rb.random_bits, rb.cache_count );
    ar.labelElement( "random_floats" ); ar.serialize( rb.random_floats, rb.cache_count );
    ar.labelElement( "bGauss"        ) & rb.bGauss;
    ar.labelElement( "eGauss_"       ) & rb.eGauss_;
}

// ----------------------------------------------------------------------------
// --- LINEAR_CONGRUENTIAL
// ----------------------------------------------------------------------------
BEGIN_QUERY_INTERFACE_BODY( LINEAR_CONGRUENTIAL )
END_QUERY_INTERFACE_BODY( LINEAR_CONGRUENTIAL )

LINEAR_CONGRUENTIAL::LINEAR_CONGRUENTIAL( uint32_t iSequence, size_t nCache )
    : RANDOMBASE( nCache )
    , iSeq( iSequence )
{
}

LINEAR_CONGRUENTIAL::~LINEAR_CONGRUENTIAL()
{
}

void LINEAR_CONGRUENTIAL::fill_bits()
{
    for( size_t i = 0; i < cache_count; ++i )
    {
        random_bits[ i ] = iSeq = 69069 * iSeq + 1;
    }
}

REGISTER_SERIALIZABLE( LINEAR_CONGRUENTIAL );

void LINEAR_CONGRUENTIAL::serialize( Kernel::IArchive& ar, LINEAR_CONGRUENTIAL* obj )
{
    RANDOMBASE::serialize( ar, obj );
    LINEAR_CONGRUENTIAL& lc = *obj;
    ar.labelElement( "iSeq" ) & lc.iSeq;
}

// ----------------------------------------------------------------------------
// --- PSEUDO_DES
// ----------------------------------------------------------------------------
BEGIN_QUERY_INTERFACE_BODY( PSEUDO_DES )
END_QUERY_INTERFACE_BODY( PSEUDO_DES )

PSEUDO_DES::PSEUDO_DES( uint64_t iSequence, size_t nCache )
    : RANDOMBASE( nCache )
    , iSeq( uint32_t( iSequence & 0xFFFFFFFF ) ) // lower 32-bits
    , iNum( uint32_t( iSequence >> 32        ) ) // upper 32-bits
{
}

PSEUDO_DES::~PSEUDO_DES()
{
}

const uint32_t c1[4] = {0xBAA96887L, 0x1E17D32CL, 0x03BCDC3CL, 0x0F33D1B2L};
const uint32_t c2[4] = {0x4B0F3B58L, 0xE874F0C3L, 0x6955C5A6L, 0x55A7CA46L};

#define HI(x) ((uint32_t) ((uint16_t*) &x)[1])
#define LO(x) ((uint32_t) ((uint16_t*) &x)[0])
#define XCHG(x) ((LO(x) << 16) | HI(x))

void PSEUDO_DES::fill_bits()
{
    uint32_t kk[3];
    uint32_t iA;
    uint32_t iB;
#ifdef _DEBUG
    uint32_t ul;
#endif

    for (size_t i = 0; i < cache_count; ++i)
    {
        iA = iNum ^ c1[0];
        iB = LO(iA) * LO(iA) + ~(HI(iA) * HI(iA));
        kk[0] = iSeq ^ ((XCHG(iB) ^ c2[0]) + LO(iA) * HI(iA));

        iA = kk[0] ^ c1[1];
        iB = LO(iA) * LO(iA) + ~(HI(iA) * HI(iA));
        kk[1] = iNum ^ ((XCHG(iB) ^ c2[1]) + LO(iA) * HI(iA));

        iNum++;
        if (iNum == 0)
            iSeq++;

        iA = kk[1] ^ c1[2];
        iB = LO(iA) * LO(iA) + ~(HI(iA) * HI(iA));
        kk[2] = kk[0] ^ ((XCHG(iB) ^ c2[2]) + LO(iA) * HI(iA));

        iA = kk[2] ^ c1[3];
        iB = LO(iA) * LO(iA) + ~(HI(iA) * HI(iA));

        random_bits[i] =
#ifdef _DEBUG
            ul =
#endif
            kk[1] ^ ((XCHG(iB) ^ c2[3]) + LO(iA) * HI(iA));
    }
}

REGISTER_SERIALIZABLE( PSEUDO_DES );

void PSEUDO_DES::serialize( Kernel::IArchive& ar, PSEUDO_DES* obj )
{
    RANDOMBASE::serialize( ar, obj );
    PSEUDO_DES& des = *obj;
    ar.labelElement( "iSeq" ) & des.iSeq;
    ar.labelElement( "iNum" ) & des.iNum;
}

// ----------------------------------------------------------------------------
// --- AES_COUNTER
// ----------------------------------------------------------------------------
BEGIN_QUERY_INTERFACE_BODY( AES_COUNTER )
END_QUERY_INTERFACE_BODY( AES_COUNTER )

inline __m128i AES_128_ASSIST(__m128i temp1, __m128i temp2)
{
    __m128i   temp3;

    temp2 = _mm_shuffle_epi32(temp2, 0xFF);
    temp3 = _mm_slli_si128(temp1, 0x4);
    temp1 = _mm_xor_si128(temp1, temp3);
    temp3 = _mm_slli_si128(temp3, 0x4);
    temp1 = _mm_xor_si128(temp1, temp3);
    temp3 = _mm_slli_si128(temp3, 0x4);
    temp1 = _mm_xor_si128(temp1, temp3);
    temp1 = _mm_xor_si128(temp1, temp2);

    return temp1;
}

void AES_128_Key_Expansion(const unsigned char *userkey, AES_KEY *key)
{
    __m128i temp1, temp2;
    __m128i *Key_Schedule = (__m128i*)key;

    temp1            = _mm_loadu_si128((__m128i*)userkey);
    Key_Schedule[0]  = temp1;
    temp2            = _mm_aeskeygenassist_si128 (temp1 ,0x1);
    temp1            = AES_128_ASSIST(temp1, temp2);
    Key_Schedule[1]  = temp1;
    temp2            = _mm_aeskeygenassist_si128 (temp1,0x2);
    temp1            = AES_128_ASSIST(temp1, temp2);
    Key_Schedule[2]  = temp1;
    temp2            = _mm_aeskeygenassist_si128 (temp1,0x4);
    temp1            = AES_128_ASSIST(temp1, temp2);
    Key_Schedule[3]  = temp1;
    temp2            = _mm_aeskeygenassist_si128 (temp1,0x8);
    temp1            = AES_128_ASSIST(temp1, temp2);
    Key_Schedule[4]  = temp1;
    temp2            = _mm_aeskeygenassist_si128 (temp1,0x10);
    temp1            = AES_128_ASSIST(temp1, temp2);
    Key_Schedule[5]  = temp1;
    temp2            = _mm_aeskeygenassist_si128 (temp1,0x20);
    temp1            = AES_128_ASSIST(temp1, temp2);
    Key_Schedule[6]  = temp1;
    temp2            = _mm_aeskeygenassist_si128 (temp1,0x40);
    temp1            = AES_128_ASSIST(temp1, temp2);
    Key_Schedule[7]  = temp1;
    temp2            = _mm_aeskeygenassist_si128 (temp1,0x80);
    temp1            = AES_128_ASSIST(temp1, temp2);
    Key_Schedule[8]  = temp1;
    temp2            = _mm_aeskeygenassist_si128 (temp1,0x1b);
    temp1            = AES_128_ASSIST(temp1, temp2);
    Key_Schedule[9]  = temp1;
    temp2            = _mm_aeskeygenassist_si128 (temp1,0x36);
    temp1            = AES_128_ASSIST(temp1, temp2);
    Key_Schedule[10] = temp1;
}

int AES_set_encrypt_key(const unsigned char *userKey, const int bits, AES_KEY *key)
{
    if (!userKey || !key) return -1;

    AES_128_Key_Expansion(userKey, key);
    key->nr = 10;
    return 0;
}

void AES_Init_Ex(AES_KEY *pExpanded)
{
    __m128i key;
    memset(&key, 0, sizeof(key));
    AES_set_encrypt_key((const unsigned char *)&key, 128, pExpanded);
}

AES_COUNTER::AES_COUNTER( uint64_t iSequence, size_t nCache )
    : RANDOMBASE( nCache )
    , m_keySchedule()
    , m_nonce(iSequence)
    , m_iteration(0)
{
    AES_Init_Ex(&m_keySchedule);
}

AES_COUNTER::~AES_COUNTER()
{
}

void AES_Get_Bits_Ex(void *buffer, size_t bytes, uint64_t nonce, uint32_t count, AES_KEY *pKey)
{
    __m128i *in = (__m128i *)buffer;
    __m128i *out = in;
    __m128i *key = pKey->KEY;
    size_t length;
    unsigned int nr = pKey->nr;

    __m128i ctr_block, tmp, ONE, BSWAP_EPI64;
    unsigned int i,j;

    if (bytes%16) {
        length = bytes / 16 + 1;
    }
    else {
        length = bytes / 16;
    }

    memset(&ctr_block, 0xCC, sizeof(ctr_block));

    ONE         = _mm_set_epi32(0,1,0,0);
    BSWAP_EPI64 = _mm_setr_epi8(7,6,5,4,3,2,1,0,15,14,13,12,11,10,9,8);
    ctr_block   = _mm_insert_epi64(ctr_block, nonce, 1);
    ctr_block   = _mm_insert_epi32(ctr_block, count, 1);
    ctr_block   = _mm_srli_si128(ctr_block, 4);
    ctr_block   = _mm_shuffle_epi8(ctr_block, BSWAP_EPI64);
    ctr_block   = _mm_add_epi64(ctr_block, ONE);

    {
        memset(in, 0, sizeof(__m128i));
        tmp       = _mm_shuffle_epi8(ctr_block, BSWAP_EPI64);
        ctr_block = _mm_add_epi64(ctr_block, ONE);
        tmp       = _mm_xor_si128(tmp, key[0]);

        for (j = 1; j < nr; j++) {
            tmp = _mm_aesenc_si128 (tmp, key[j]);
        }

        tmp = _mm_aesenclast_si128(tmp, key[j]);
        tmp = _mm_xor_si128(tmp,_mm_loadu_si128(in));
        _mm_storeu_si128(out,tmp);
    }

    for (i = 1; i < length; i++) {
        tmp       = _mm_shuffle_epi8(ctr_block, BSWAP_EPI64);
        ctr_block = _mm_add_epi64(ctr_block, ONE);
        tmp       = _mm_xor_si128(tmp, key[0]);

        for (j = 1; j < nr; j++) {
            tmp = _mm_aesenc_si128(tmp, key[j]);
        }

        tmp = _mm_aesenclast_si128(tmp, key[j]);
        tmp = _mm_xor_si128(tmp,_mm_loadu_si128(in+i-1));
        _mm_storeu_si128(out+i,tmp);
    }
}

void AES_COUNTER::fill_bits()
{
    memset(random_bits, 0, sizeof(__m128i));
    AES_Get_Bits_Ex(random_bits, cache_count * sizeof(uint32_t), m_nonce, m_iteration++, &m_keySchedule);
}

REGISTER_SERIALIZABLE( AES_COUNTER );

void serialize_AES_KEY( Kernel::IArchive& ar, AES_KEY& key )
{
    size_t num_keys = 2 * NUM_KEYS_IN_SCHEDULE;

    ar.startObject();
    ar.labelElement("KEY");
    ar.startArray( num_keys );
    uint64_t* vals = (uint64_t*)(key.KEY);
    for( size_t i = 0; i < num_keys; ++i )
    {
        ar & vals[i];
    }
    ar.endArray();
    ar.labelElement("nr") & key.nr;
    ar.endObject();
}

void AES_COUNTER::serialize( Kernel::IArchive& ar, AES_COUNTER* obj )
{
    RANDOMBASE::serialize( ar, obj );
    AES_COUNTER& aes = *obj;
    ar.labelElement( "m_keySchedule" ); serialize_AES_KEY( ar, aes.m_keySchedule );
    ar.labelElement( "m_nonce"     ) & aes.m_nonce;
    ar.labelElement( "m_iteration" ) & aes.m_iteration;
}

}
