/**
 * Copyright (c) 2011-2017 libbitcoin developers (see AUTHORS)
 *
 * This file is part of libbitcoin.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <bitcoin/bitcoin/utility/threadpool.hpp>

#include <thread>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/bitcoin/error.hpp>
#include <bitcoin/bitcoin/log/source.hpp>
#include <bitcoin/bitcoin/utility/asio.hpp>
#include <bitcoin/bitcoin/utility/assert.hpp>
#include <bitcoin/bitcoin/utility/thread.hpp>

namespace libbitcoin {

threadpool::threadpool(size_t number_threads, thread_priority priority)
  : size_(0)
{
    shutdown_ = false;
    const auto this_id = boost::this_thread::get_id();
    LOG_VERBOSE(LOG_SYSTEM)
    << this_id
    << " threadpool(" << number_threads << " threads)"
    << "with io_context service at: " << &service_;
    spawn(number_threads, priority);
}

threadpool::~threadpool()
{
    const auto this_id = boost::this_thread::get_id();
    LOG_VERBOSE(LOG_SYSTEM)
    << this_id
    << " ~threadpool()";
    shutdown();
    join();
}

// Should not be called during spawn.
bool threadpool::empty() const
{
    const auto this_id = boost::this_thread::get_id();
    LOG_VERBOSE(LOG_SYSTEM)
    << this_id
    << " threadpool::empty()";
    return size() != 0;
}

// Should not be called during spawn.
size_t threadpool::size() const
{
    const auto this_id = boost::this_thread::get_id();
    LOG_VERBOSE(LOG_SYSTEM)
    << this_id
    << " threadpool::size()";
    return size_.load();
}

// This is not thread safe.
void threadpool::spawn(size_t number_threads, thread_priority priority)
{
    const auto this_id = boost::this_thread::get_id();
    LOG_VERBOSE(LOG_SYSTEM)
    << this_id
    << " threadpool::spawn(" << number_threads << " threads)"
    << " with io_context service at: " << &service_;
    // This allows the pool to be restarted.
    // This function must not be called while there are any unfinished calls to the run(), run_one(), poll() or poll_one() functions.
    service_.restart();

    for (size_t i = 0; i < number_threads; ++i)
        spawn_once(priority);
}

void threadpool::spawn_once(thread_priority priority)
{
    ///////////////////////////////////////////////////////////////////////////
    // Critical Section
    unique_lock lockdown(shutdown_mutex_);
    if (!shutdown_)
    {
        const auto this_id = boost::this_thread::get_id();
        LOG_VERBOSE(LOG_SYSTEM)
        << this_id
        << " threadpool::spawn_once()";
        ///////////////////////////////////////////////////////////////////////////
        // Critical Section
        work_mutex_.lock_upgrade();

        // Work prevents the service from running out of work and terminating.
        if (!work_)
        {
            work_mutex_.unlock_upgrade_and_lock();
            //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            work_ = std::make_shared<asio::service::work>(service_);

            work_mutex_.unlock_and_lock_upgrade();
            //-----------------------------------------------------------------
        }

        work_mutex_.unlock_upgrade();
        ///////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////
        // Critical Section
        unique_lock lock(threads_mutex_);

        threads_.push_back(asio::thread([this, priority]()
        {
            set_priority(priority);
            service_.run();
        }));

        ++size_;
        ///////////////////////////////////////////////////////////////////////////
    }
    ///////////////////////////////////////////////////////////////////////////
}

void threadpool::abort()
{
    const auto this_id = boost::this_thread::get_id();
    LOG_VERBOSE(LOG_SYSTEM)
    << this_id
    << " threadpool::abort()";
    service_.stop();
}

void threadpool::shutdown()
{
    ///////////////////////////////////////////////////////////////////////////
    // Critical Section
    unique_lock lockdown(shutdown_mutex_);

    shutdown_ = true;
    const auto this_id = boost::this_thread::get_id();
    LOG_VERBOSE(LOG_SYSTEM)
    << this_id
    << " threadpool::shutdown()";
    ///////////////////////////////////////////////////////////////////////////
    // Critical Section
    unique_lock lock(work_mutex_);

    work_.reset();
    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////
}

void threadpool::join()
{
    ///////////////////////////////////////////////////////////////////////////
    // Critical Section
    unique_lock lockdown(shutdown_mutex_);
    const auto this_id = boost::this_thread::get_id();
    LOG_VERBOSE(LOG_SYSTEM)
    << this_id
    << " threadpool::join()";
    ///////////////////////////////////////////////////////////////////////////
    // Critical Section
    unique_lock lock(threads_mutex_);

    DEBUG_ONLY(const auto this_id = boost::this_thread::get_id();)

    for (auto& thread: threads_)
    {
        BITCOIN_ASSERT(this_id != thread.get_id());
        BITCOIN_ASSERT(thread.joinable());
        thread.join();
    }

    threads_.clear();
    size_.store(0);
    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////
}

asio::service& threadpool::service()
{
    const auto this_id = boost::this_thread::get_id();
    LOG_VERBOSE(LOG_SYSTEM)
    << this_id
    << " threadpool::service()";
    return service_;
}

const asio::service& threadpool::service() const
{
    const auto this_id = boost::this_thread::get_id();
    LOG_VERBOSE(LOG_SYSTEM)
    << this_id
    << " const threadpool::service() const";
    return service_;
}

} // namespace libbitcoin
