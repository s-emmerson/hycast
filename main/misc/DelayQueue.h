/**
 * This file declares a thread-safe delay-queue. Each element has a time-point
 * when it becomes available.
 *
 * Copyright 2017 University Corporation for Atmospheric Research. All rights
 * reserved. See the the file COPYING in the top-level source-directory for
 * licensing conditions.
 *
 *        File: DelayQueue.h
 *  Created on: May 23, 2017
 *      Author: Steven R. Emmerson
 */

#ifndef MISC_DELAYQUEUE_H
#define MISC_DELAYQUEUE_H

#include <chrono>
#include <memory>

namespace hycast {

/**
 * @tparam Value     Type of value being stored in the queue. Must support
 *                   copy assignment and move assignment.
 * @tparam Dur       Duration type
 */
template<typename Value, typename Dur = std::chrono::seconds>
class DelayQueue final
{
    class                 Impl;
    std::shared_ptr<Impl> pImpl;

public:
    typedef Dur Duration;

    /**
     * Default constructs.
     * @throws std::bad_alloc     If necessary memory can't be allocated.
     * @throws std::system_error  If a system error occurs.
     */
    explicit DelayQueue();

    /**
     * Adds a value to the queue.
     * @param[in] value  The value to be added
     * @param[in] delay  The delay for the element before it becomes available
     *                   in units of the template parameter
     * @exceptionsafety  Strong guarantee
     * @threadsafety     Safe
     */
    void push(
            const Value&    value,
            const Duration& delay = Duration(0)) const;

    /**
     * Returns the value whose reveal-time is the earliest and not later than
     * the current time and removes it from the queue. Blocks until such a value
     * is available.
     * @return  The value with the earliest reveal-time that's not later than
     *          the current time.
     * @exceptionsafety Basic guarantee
     * @threadsafety    Safe
     */
    Value pop() const;

    /**
     * Indicates if the queue is empty.
     * @return `true`   The queue is empty
     * @return `false`  The queue is not empty
     * @exceptionsafety Nothrow
     * @threadsafety    Safe
     */
    bool empty() const noexcept;

    /**
     * Clears the queue of all elements.
     * @exceptionsafety Nothrow
     * @threadsafety    Safe
     */
    void clear() noexcept;
};

} // namespace

#include "DelayQueue.cpp" // For automatic template instatiation

#endif /* MISC_DELAYQUEUE_H */
