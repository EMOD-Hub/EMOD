
#include "stdafx.h"

#include <iostream>
#include "Instrumentation.h"
#include "Log.h"
#include "Debug.h"

SETUP_LOGGING( "Instrumentation" )

#ifndef WIN32
// dummy implementations for non-windows platforms
//

Stopwatch::Stopwatch( bool ) { }
void Stopwatch::Start() { }
void Stopwatch::Stop() { }
double Stopwatch::ResultNanoseconds() { return 0; }
void Stopwatch::PrintResultSeconds(const char * label) { }

#else

//////////////////////////////////////////////////////////////////////////
// adapted from http://stackoverflow.com/questions/922829/c-boost-posix-time-elapsed-seconds-elapsed-fractional-seconds

//! \brief Stopwatch for timing performance values
//!
//! This stopwatch class is designed for timing performance of various
//! software operations.  If the values you get back a greater than a 
//! few seconds, you should be using a different tool.
//! On a Core 2 Duo E6850 @ 3.00GHz, the start/stop sequence takes 
//! approximately 230 nano seconds in the debug configuration and 180 
//! nano seconds in the release configuration.  If you are timing on the
//! sub-microsecond scale, please take this into account and test it on
//! your machine.


//! \param start if set to true will initialize and then start the 
//! timer.
Stopwatch::Stopwatch(bool start)
{
    _start = 0;
    _stop = 0;
    if(start)
        Start();
}

//! Starts the stopwatch running
void Stopwatch::Start()
{
    QueryPerformanceCounter((LARGE_INTEGER*)&_start);
}

//! Run this when the event being timed is complete
void Stopwatch::Stop()
{
    QueryPerformanceCounter((LARGE_INTEGER*)&_stop);
}

//! Stops the timer and returns the result
//double Stopwatch::StopResult()
//{
//    Stop();
//    return ResultNanoseconds();
//}

//! You can get the result of the stopwatch start-stop sequence at
//! your leisure.
double Stopwatch::ResultNanoseconds()
{
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    double cyclesPerNanosecond = static_cast<double>(frequency.QuadPart) / 1000000000.0;

    std::uint64_t elapsed = _stop - _start;
    return elapsed / cyclesPerNanosecond;
}

void Stopwatch::PrintResultSeconds(const char * label)
{
    LOG_INFO_F("%s - %f secs\n", label, ResultNanoseconds() / 1000000000);
}

#endif
