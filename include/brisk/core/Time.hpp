/*
 * Brisk
 *
 * Cross-platform application framework
 * --------------------------------------------------------------
 *
 * Copyright (C) 2024 Brisk Developers
 *
 * This file is part of the Brisk library.
 *
 * Brisk is dual-licensed under the GNU General Public License, version 2 (GPL-2.0+),
 * and a commercial license. You may use, modify, and distribute this software under
 * the terms of the GPL-2.0+ license if you comply with its conditions.
 *
 * You should have received a copy of the GNU General Public License along with this program.
 * If not, see <http://www.gnu.org/licenses/>.
 *
 * If you do not wish to be bound by the GPL-2.0+ license, you must purchase a commercial
 * license. For commercial licensing options, please visit: https://brisklib.com
 */
#pragma once

#include "BasicTypes.hpp"
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <queue>
#include <thread>
#include "Math.hpp"
#include "Json.hpp"

namespace Brisk {

/**
 * @brief Converts a duration to seconds.
 *
 * This function converts a given duration to a double value representing the number of seconds.
 *
 * @param duration The duration to convert.
 * @return The duration in seconds.
 */
template <typename Rep, typename Period>
inline double toSeconds(std::chrono::duration<Rep, Period> duration) {
    return std::chrono::duration_cast<std::chrono::duration<double>>(duration).count();
}

/**
 * @brief Converts a duration to Hertz.
 *
 * This function converts a given duration to a double value representing the frequency in Hertz.
 *
 * @param duration The duration to convert.
 * @return The frequency in Hertz.
 */
template <typename Rep, typename Period>
inline double toHerzs(std::chrono::duration<Rep, Period> duration) {
    return 1.0 / toSeconds(duration);
}

using Clock = std::chrono::steady_clock;

/// @brief The start time of the application.
extern Clock::time_point appStartTime;

/**
 * @brief Returns the current time.
 *
 * This function retrieves the current time according to the steady clock.
 *
 * @return The current time point.
 */
inline Clock::time_point now() noexcept {
    return Clock::now();
}

/**
 * @brief Returns the duration since the application started.
 *
 * @return The duration since the application start.
 */
inline Clock::duration timeSinceStart() noexcept {
    return now() - appStartTime;
}

/**
 * @brief Returns the duration since the application started in seconds.
 *
 * @return The duration in seconds.
 */
inline double currentTime() noexcept {
    return toSeconds(timeSinceStart());
}

/**
 * @brief Represents a thread that handles a single timer.
 *
 * This class provides a mechanism to run a timer in a separate thread. The timer will call a `tick` method
 * periodically.
 */
class SingleTimerThread {
public:
    /**
     * @brief Constructs a SingleTimerThread instance and starts the thread.
     *
     * This constructor initializes the thread and starts it, allowing it to handle timer events.
     */
    explicit SingleTimerThread();

    /**
     * @brief Destructor for the SingleTimerThread.
     *
     * This destructor waits for the thread to finish execution before cleaning up resources.
     */
    ~SingleTimerThread();

    /**
     * @brief Terminates the timer thread.
     *
     * This method signals the thread to stop but doesn't wait for it to terminate.
     */
    void terminate();

    /**
     * @brief Virtual function called in a cycle.
     *
     * This method must be overridden to handle periodic events. It sets the time parameter to the next
     * desired call time.
     *
     * @param time The time point to be set for the next call.
     */
    virtual void tick(Clock::time_point& time) {}

private:
    std::thread m_thread;
    std::atomic<bool> m_terminated{ false };
    void run();
};

/**
 * @brief Represents a periodic timer.
 *
 * This structure provides functionality to manage a timer that triggers events at regular intervals.
 */
struct PeriodicTimer {
    /**
     * @brief Constructs a PeriodicTimer and optionally starts it immediately.
     *
     * @param startNow If true, the timer starts immediately.
     */
    PeriodicTimer(bool startNow = true);

    /**
     * @brief Checks if the specified period has elapsed.
     *
     * @param period The period to check.
     * @return True if the period has elapsed; otherwise, false.
     */
    bool elapsed(double period);

    /**
     * @brief Checks if the timer is currently active.
     *
     * @return True if the timer is active; otherwise, false.
     */
    bool active() const;

    /**
     * @brief Starts the periodic timer.
     */
    void start();

    /**
     * @brief Stops the periodic timer.
     */
    void stop();

    double time; ///< The time period for the timer.
};

using PerformanceDuration = std::chrono::nanoseconds;
using FractionalSeconds   = std::chrono::duration<double>;

PerformanceDuration perfNow();

struct PerformanceStatistics {
public:
    struct TimeSlice {
        PerformanceDuration start;
        PerformanceDuration stop;
    };

    PerformanceStatistics() = default;
    void addMeasurement(PerformanceDuration start, PerformanceDuration stop);

    static std::string ms(PerformanceDuration v);
    static std::string us(PerformanceDuration v);
    static std::string ns(PerformanceDuration v);
    std::string report(std::string (*cvt)(PerformanceDuration) = &PerformanceStatistics::ms) const;
    void reset();

    double load() const;

    const std::array<TimeSlice, 64 * 4>& slices() const;
    int slicesPos() const;

    const std::string& lastReport() const;

private:
    std::atomic<PerformanceDuration::rep> m_sum{ 0 };
    std::atomic<PerformanceDuration::rep> m_min{ std::numeric_limits<PerformanceDuration::rep>::max() };
    std::atomic<PerformanceDuration::rep> m_max{ std::numeric_limits<PerformanceDuration::rep>::min() };
    std::atomic<PerformanceDuration::rep> m_start{ 0 };
    std::atomic<int64_t> m_count{ 0 };
    std::array<TimeSlice, 64 * 4> m_slices;
    std::atomic<int> m_slicePos{ 0 };
    mutable std::string m_report;
    double m_load = 0.0;

    template <typename T>
    static void updateMaximum(std::atomic<T>& maximum_value, T const& value) noexcept {
        T prev_value = maximum_value;
        while (prev_value < value && !maximum_value.compare_exchange_weak(prev_value, value)) {}
    }

    template <typename T>
    static void updateMinimum(std::atomic<T>& maximum_value, T const& value) noexcept {
        T prev_value = maximum_value;
        while (prev_value > value && !maximum_value.compare_exchange_weak(prev_value, value)) {}
    }
};

struct Stopwatch {
public:
    explicit Stopwatch(PerformanceStatistics* stat);
    explicit Stopwatch(PerformanceStatistics& stat);
    explicit Stopwatch(const char* name);
    ~Stopwatch();
    PerformanceDuration time;
    PerformanceStatistics* stat;
    const char* name;
};

} // namespace Brisk

namespace std {
namespace chrono {

/**
 * @brief Converts a JSON value to a chrono duration.
 *
 * @param j The JSON value to convert.
 * @param dur The resulting chrono duration.
 * @return True if conversion is successful; otherwise, false.
 */
template <typename Rep, typename Period>
inline bool fromJson(const Brisk::Json& j, duration<Rep, Period>& dur) {
    if (auto v = j.to<Rep>()) {
        dur = duration<Rep, Period>(*v);
        return true;
    }
    return false;
}

/**
 * @brief Converts a chrono duration to a JSON value.
 *
 * @param j The resulting JSON value.
 * @param dur The chrono duration to convert.
 * @return True if conversion is successful; otherwise, false.
 */
template <typename Rep, typename Period>
inline bool toJson(Brisk::Json& j, const duration<Rep, Period>& dur) {
    j = dur.count();
    return true;
}

/**
 * @brief Converts a JSON value to a chrono time point.
 *
 * @param j The JSON value to convert.
 * @param time The resulting chrono time point.
 * @return True if conversion is successful; otherwise, false.
 */
template <typename Clock, typename Duration>
inline bool fromJson(const Brisk::Json& j, time_point<Clock, Duration>& time) {
    if (auto v = j.to<typename Duration::rep>()) {
        time = time_point<Clock, Duration>(Duration(*v));
        return true;
    }
    return false;
}

/**
 * @brief Converts a chrono time point to a JSON value.
 *
 * @param j The resulting JSON value.
 * @param time The chrono time point to convert.
 * @return True if conversion is successful; otherwise, false.
 */
template <typename Clock, typename Duration>
inline bool toJson(Brisk::Json& j, const time_point<Clock, Duration>& time) {
    j = time.time_since_epoch().count();
    return true;
}

} // namespace chrono
} // namespace std
