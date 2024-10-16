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
#include <brisk/core/Time.hpp>
#include <brisk/core/Threading.hpp>

namespace Brisk {

Clock::time_point appStartTime = Clock::now();

SingleTimerThread::SingleTimerThread() : m_thread(&SingleTimerThread::run, this) {}

SingleTimerThread::~SingleTimerThread() {
    terminate();
    m_thread.join();
}

void SingleTimerThread::terminate() {
    m_terminated = true;
}

void SingleTimerThread::run() {
    setThreadPriority(ThreadPriority::Highest);
    setThreadName("SingleTimerThread");
    auto time = now();
    while (!m_terminated) {
        tick(time);
        std::this_thread::sleep_until(time);
    }
}

PeriodicTimer::PeriodicTimer(bool startNow) : time(NAN) {
    if (startNow)
        start();
}

void PeriodicTimer::stop() {
    time = NAN;
}

void PeriodicTimer::start() {
    time = currentTime();
}

bool PeriodicTimer::active() const {
    return !std::isnan(time);
}

bool PeriodicTimer::elapsed(double period) {
    if (!active())
        return false;

    double now = currentTime();
    if (now - time >= period) {
        time = now;
        return true;
    }
    return false;
}

PerformanceDuration perfNow() {
    return std::chrono::duration_cast<PerformanceDuration>(timeSinceStart());
}

void PerformanceStatistics::addMeasurement(PerformanceDuration start, PerformanceDuration stop) {
    int p                         = m_slicePos++;
    m_slices[p % m_slices.size()] = TimeSlice{ start, stop };
    PerformanceDuration time      = stop - start;
    m_sum += time.count();

    updateMaximum(m_max, time.count());
    updateMinimum(m_min, time.count());

    if (m_count++ == 0)
        m_start = perfNow().count();
}

std::string PerformanceStatistics::ms(PerformanceDuration v) {
    return fmt::format("{:7.3f}ms", v.count() / 1000'000.0);
}

std::string PerformanceStatistics::us(PerformanceDuration v) {
    return fmt::format("{:7.3f}us", v.count() / 1000.0);
}

std::string PerformanceStatistics::ns(PerformanceDuration v) {
    return fmt::format("{:7.3f}ns", static_cast<double>(v.count()));
}

std::string PerformanceStatistics::report(std::string (*cvt)(PerformanceDuration)) const {
    int64_t count = m_count.load();
    if (count)
        m_report = fmt::format("num={:7}   sum={}   avg={}   min={}   max={}   cpu={:.1f}%", count,
                               cvt(PerformanceDuration(m_sum)), cvt(PerformanceDuration(m_sum) / count),
                               cvt(PerformanceDuration(m_min)), cvt(PerformanceDuration(m_max)), load());
    else
        m_report = fmt::format("num={:7}", count);
    return m_report;
}

void PerformanceStatistics::reset() {
    m_load  = load();
    m_sum   = 0;
    m_min   = std::numeric_limits<PerformanceDuration::rep>::max();
    m_max   = std::numeric_limits<PerformanceDuration::rep>::min();
    m_start = 0;
    m_count = 0;
}

double PerformanceStatistics::load() const {
    return 100.0 * toSeconds(PerformanceDuration(m_sum)) /
           (currentTime() - toSeconds(PerformanceDuration(m_start)));
}

const std::array<PerformanceStatistics::TimeSlice, 64 * 4>& PerformanceStatistics::slices() const {
    return m_slices;
}

int PerformanceStatistics::slicesPos() const {
    return m_slicePos;
}

const std::string& PerformanceStatistics::lastReport() const {
    return m_report;
}

Stopwatch::Stopwatch(PerformanceStatistics* stat) : stat(stat), name(nullptr) {
    time = perfNow();
}

Stopwatch::Stopwatch(PerformanceStatistics& stat) : stat(&stat), name(nullptr) {
    time = perfNow();
}

Stopwatch::Stopwatch(const char* name) : stat(nullptr), name(name) {
    time = perfNow();
}

Stopwatch::~Stopwatch() {
    const PerformanceDuration cur = perfNow();
    if (stat) {
        stat->addMeasurement(time, cur);
    } else if (name) {
        LOG_TRACE(core, "Stopwatch \"{}\" = {:.3f}ms", name,
                  std::chrono::duration_cast<FractionalSeconds>(cur - time).count() * 1000.0);
    }
}
} // namespace Brisk
