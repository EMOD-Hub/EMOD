
#pragma once

#include <cfloat>
#include <sstream>
#include <stdexcept>
#include <assert.h>
#include <vector>

#include "Exceptions.h"
#include "IArchive.h"

struct act_prob_t
{
    act_prob_t();
    static void serialize( Kernel::IArchive& ar, act_prob_t& ap );

    unsigned int num_acts;
    float prob_per_act;
};

typedef std::vector<act_prob_t> act_prob_vec_t;

class IDMAPI RangedFloat
{
    public:
        RangedFloat()
        : _min_value( -1.0f*FLT_MAX ) // need some defaults
        , _max_value( 1.0f*FLT_MAX )
        {
            _value = 0.0f;
        }

        RangedFloat( float min_in, float max_in )
        : _min_value( min_in )
        , _max_value( max_in )
        {
            //std::cout << "RangedFloat initialized with value " << 0 << ", min = " << _min_value << ", and max = " << _max_value << std::endl;
            _value = 0.0f;
        }

        RangedFloat( float initValue, float min_in, float max_in )
        : _min_value( min_in ) // need some defaults
        , _max_value( max_in )
        {
            //std::cout << "RangedFloat initialized with value " << initValue << ", min = " << _min_value << ", and max = " << _max_value << std::endl;
            if( initValue < _min_value )
            {
                throw Kernel::OutOfRangeException( __FILE__, __LINE__, __FUNCTION__, "value", initValue, _min_value );
            }
            else if( initValue > _max_value )
            {
                throw Kernel::OutOfRangeException( __FILE__, __LINE__, __FUNCTION__, "value", initValue, _max_value );
            }
            _value = initValue;
        }

        RangedFloat( const RangedFloat &initValue )
        : _min_value( -1.0f*FLT_MAX ) // need some defaults, maybe kill copy-constructor???
        , _max_value( 1.0f*FLT_MAX )
        {
            _value = initValue._value;
        }

        // to inline or not to inline, that is the question...
        RangedFloat& operator=( const RangedFloat &assignThis )
        {
            if( assignThis._value < _min_value )
            {
                throw Kernel::OutOfRangeException( __FILE__, __LINE__, __FUNCTION__, "value", assignThis._value, _min_value );
            }
            else if( assignThis._value > _max_value )
            {
                throw Kernel::OutOfRangeException( __FILE__, __LINE__, __FUNCTION__, "value", assignThis._value, _max_value );
            }
            _value = assignThis._value;
            return *this;
        }

        RangedFloat& operator=( float assignThis )
        {
            if( assignThis < _min_value )
            {
                throw Kernel::OutOfRangeException( __FILE__, __LINE__, __FUNCTION__, "RangedFloat::value", assignThis, _min_value );
            }
            else if( assignThis > _max_value )
            {
                throw Kernel::OutOfRangeException( __FILE__, __LINE__, __FUNCTION__, "RangedFloat::value", assignThis, _max_value );
            }
            _value = assignThis;
            return *this;
        }

        inline RangedFloat& operator+=( float assignThis )
        {
            _value += assignThis;
            if( _value < _min_value )
            {
                throw Kernel::OutOfRangeException( __FILE__, __LINE__, __FUNCTION__, "value", _value, 0 );
            }
            else if( _value > _max_value )
            {
                throw Kernel::OutOfRangeException( __FILE__, __LINE__, __FUNCTION__, "value", _value, 1.0 );
            }
            return *this;
        }

        inline RangedFloat& operator-=( float assignThis )
        {
            _value -= assignThis;
            if( _value < _min_value )
            {
                throw Kernel::OutOfRangeException( __FILE__, __LINE__, __FUNCTION__, "value", _value, 0 );
            }
            else if( _value > _max_value )
            {
                throw Kernel::OutOfRangeException( __FILE__, __LINE__, __FUNCTION__, "value", _value, 1.0 );
            }
            return *this;
        }

        inline RangedFloat& operator*=( float multThis )
        {
            _value *= multThis;
            if( _value < _min_value )
            {
                throw Kernel::OutOfRangeException( __FILE__, __LINE__, __FUNCTION__, "value", _value, 0 );
            }
            else if( _value > _max_value )
            {
                throw Kernel::OutOfRangeException( __FILE__, __LINE__, __FUNCTION__, "value", _value, 1.0 );
            }
            return *this;
        }

        inline RangedFloat& operator/=( float divThis )
        {
            // TBD: check for divide by 0!
            _value /= divThis;
            if( _value < _min_value )
            {
                throw Kernel::OutOfRangeException( __FILE__, __LINE__, __FUNCTION__, "value", _value, 0 );
            }
            else if( _value > _max_value )
            {
                throw Kernel::OutOfRangeException( __FILE__, __LINE__, __FUNCTION__, "value", _value, 1.0 );
            }
            return *this;
        }

        operator float() const { return _value; }
        virtual float getMin() const { return _min_value; }
        virtual float getMax() const { return _max_value; }

        void serialize( Kernel::IArchive& ar )
        {
            ar.startObject();
                ar.labelElement("_value"    ) & _value;
                ar.labelElement("_min_value") & _min_value;
                ar.labelElement("_max_value") & _max_value;
            ar.endObject();
        }

        // TODO - I dont understand why I need both
        static void serialize( Kernel::IArchive& ar, RangedFloat& rf )
        {
            ar.startObject();
                ar.labelElement("_value"    ) & rf._value;
                ar.labelElement("_min_value") & rf._min_value;
                ar.labelElement("_max_value") & rf._max_value;
            ar.endObject();
        }
    private:
    protected:
        float _value;
        /* const */ float _min_value;
        /* const */ float _max_value;
};

class IDMAPI NonNegativeFloat : public RangedFloat
{
    public:
        NonNegativeFloat()
        : RangedFloat( 0.0f, 0.0f, FLT_MAX )
        {
        }

        NonNegativeFloat( float value_in )
        : RangedFloat( value_in, 0.0f, FLT_MAX )
        {
        }

    protected:
        NonNegativeFloat( float value_in, float min_in, float max_in )
        : RangedFloat( value_in, min_in, max_in )
        {
        }
};

class IDMAPI ProbabilityNumber : public NonNegativeFloat
{
    public:
        ProbabilityNumber()
        : NonNegativeFloat( 0.0f, 0.0f, 1.0f )
        {
        }

        ProbabilityNumber( float value_in )
        : NonNegativeFloat( value_in, 0.0f, 1.0f )
        {
        }
};

typedef ProbabilityNumber Fraction;

class IDMAPI NaturalNumber
{
    public:
        NaturalNumber() { _value = 0; }
        NaturalNumber( int initValue ) { _value = initValue; }
        NaturalNumber( const NaturalNumber &initValue ) { _value = initValue._value; }

        NaturalNumber& operator=( const NaturalNumber &assignThis )
        {
            if( assignThis._value < 0 )
            {
                throw Kernel::OutOfRangeException( __FILE__, __LINE__, __FUNCTION__, "value", (float) _value, 0 );
            }
            _value = assignThis._value;
            return *this;
        }

        NaturalNumber& operator=( int assignThis )
        {
            _value = assignThis;
            if( _value < 0 )
            {
                throw Kernel::OutOfRangeException( __FILE__, __LINE__, __FUNCTION__, "value", (float)_value, 0 );
            }

            return *this;
        }

        inline NaturalNumber& operator+=( int assignThis )
        {
            _value += assignThis;
            if( _value < 0 )
            {
                throw Kernel::OutOfRangeException( __FILE__, __LINE__, __FUNCTION__, "value", (float)_value, 0 );
            }

            return *this;
        }

        inline NaturalNumber& operator++(int)
        {
            _value++;
            if( _value < 0 )
            {
                throw Kernel::OutOfRangeException( __FILE__, __LINE__, __FUNCTION__, "value", (float) _value, 0 );
            }

            return *this;
        }

        inline NaturalNumber& operator--(int)
        {
            _value--;
            if( _value < 0 )
            {
                throw Kernel::OutOfRangeException( __FILE__, __LINE__, __FUNCTION__, "value", (float) _value, 0 );
            }

            return *this;
        }

        inline NaturalNumber& operator*=( int multThis )
        {
            _value *= multThis;
            if( _value < 0 )
            {
                throw Kernel::OutOfRangeException( __FILE__, __LINE__, __FUNCTION__, "value", (float)_value, 0 );
            }

            return *this;
        }

        inline NaturalNumber& operator/=( int divThis )
        {
            // TBD: check for divide by 0!
            _value /= divThis;
            if( _value < 0 )
            {
                throw Kernel::OutOfRangeException( __FILE__, __LINE__, __FUNCTION__, "value", (float)_value, 0 );
            }

            return *this;
        }

        operator int() const { return _value; }

        void serialize( Kernel::IArchive& ar )
        {
            ar & _value;
        }

        static void serialize( Kernel::IArchive& ar, NaturalNumber& nn )
        {
            //ar.labelElement("_value") & nn._value;
            ar & nn._value;
        }

    private:
        int _value;
};
