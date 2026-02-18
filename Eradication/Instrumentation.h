
#pragma once

#include <cstdint>
class Stopwatch
{
public:
    Stopwatch(bool start = false);

    void Start();
    void Stop();
    double ResultNanoseconds();
    void PrintResultSeconds(const char * label);

private:
    std::uint64_t _start;
    std::uint64_t _stop;
};

#define REPORT_TIME(enable, label, block) \
{\
    Stopwatch s;\
    if (enable) { s.Start(); }\
    block;\
    if (enable) \
    {\
        s.Stop();\
        s.PrintResultSeconds(label);\
    }\
}

#define BEGIN_REPORT_TIME(enable) \
{\
    Stopwatch s;\
    if (enable) { s.Start(); }

#define END_REPORT_TIME(enable, label) \
    if (enable) \
    {\
        s.Stop();\
        s.PrintResultSeconds(label);\
    }\
}
