#pragma once

#include <array>

#include <node.h>

#include <qdb/client.h>

namespace quasardb
{

inline qdb_timespec_t ms_to_qdb_timespec(double ms)
{
    auto ns = ms * 1000000ull;

    qdb_timespec_t ts;
    ts.tv_sec = static_cast<qdb_time_t>(ns / 1000000000ull);
    ts.tv_nsec = static_cast<qdb_time_t>(ns - ts.tv_sec * 1000000000ull);

    return ts;
}

inline double qdb_timespec_to_ms(const qdb_timespec_t & ts)
{
    return static_cast<double>(ts.tv_sec) * 1000.0 + static_cast<double>(ts.tv_nsec / 1000000ull);
}
} // namespace quasardb
