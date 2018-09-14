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
#ifndef LIBBITCOIN_RESUBSCRIBER_IPP
#define LIBBITCOIN_RESUBSCRIBER_IPP

#include <iostream>
#include <tuple>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <bitcoin/bitcoin/utility/assert.hpp>
#include <bitcoin/bitcoin/utility/dispatcher.hpp>
#include <bitcoin/bitcoin/utility/thread.hpp>
#include <bitcoin/bitcoin/utility/threadpool.hpp>
////#include <bitcoin/bitcoin/utility/track.hpp>

namespace libbitcoin {
/*
    template<class TupType, size_t... I>
    void print(const TupType& _tup, boost::fusion::detail::index_sequence<I...>)
    {
        std::cout << "(";
        (..., (std::cout << (I == 0? "" : ", ") << std::get<I>(_tup)));
        std::cout << ")\n";
    }
    
    template<class... T>
    void print (const std::tuple<T...>& _tup)
    {
        print(_tup, boost::fusion::detail::make_index_sequence<sizeof...(T)>());
    }
*/
template <typename... Args>
resubscriber<Args...>::resubscriber(threadpool& pool,
    const std::string& class_name)
  : stopped_(true), dispatch_(pool, class_name)
    /*, track<resubscriber<Args...>>(class_name)*/
{
}

template <typename... Args>
resubscriber<Args...>::~resubscriber()
{
    BITCOIN_ASSERT_MSG(subscriptions_.empty(), "resubscriber not cleared");
}

template <typename... Args>
void resubscriber<Args...>::start()
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    const auto this_id = boost::this_thread::get_id();
    LOG_DEBUG(LOG_SYSTEM)
    << this_id
    << " resubscriber::start() calling lock_upgrade() for subscribe_mutex_ of "
    << &subscribe_mutex_;

    subscribe_mutex_.lock_upgrade();

    LOG_DEBUG(LOG_SYSTEM)
    << this_id
    << " resubscriber::start() called lock_upgrade() successfully for subscribe_mutex_ of "
    << &subscribe_mutex_;

    if (stopped_)
    {
        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        subscribe_mutex_.unlock_upgrade_and_lock();
        stopped_ = false;
        subscribe_mutex_.unlock();
        //---------------------------------------------------------------------
        return;
    }

    LOG_DEBUG(LOG_SYSTEM)
    << this_id
    << " resubscriber::start() calling unlock_upgrade() for subscribe_mutex_ of "
    << &subscribe_mutex_;

    subscribe_mutex_.unlock_upgrade();

    LOG_DEBUG(LOG_SYSTEM)
    << this_id
    << " resubscriber::start() called unlock_upgrade() successfully for subscribe_mutex_ of "
    << &subscribe_mutex_;
    ///////////////////////////////////////////////////////////////////////////
}

template <typename... Args>
void resubscriber<Args...>::stop()
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    const auto this_id = boost::this_thread::get_id();
    LOG_DEBUG(LOG_SYSTEM)
    << this_id
    << " resubscriber::stop() calling lock_upgrade() for subscribe_mutex_ of "
    << &subscribe_mutex_;

    subscribe_mutex_.lock_upgrade();

    LOG_DEBUG(LOG_SYSTEM)
    << this_id
    << " resubscriber::stop() called lock_upgrade() successfully for subscribe_mutex_ of "
    << &subscribe_mutex_;

    if (!stopped_)
    {
        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        subscribe_mutex_.unlock_upgrade_and_lock();
        stopped_ = true;
        subscribe_mutex_.unlock();
        //---------------------------------------------------------------------
        return;
    }

    LOG_DEBUG(LOG_SYSTEM)
    << this_id
    << " resubscriber::stop() calling unlock_upgrade() for subscribe_mutex_ of "
    << &subscribe_mutex_;

    subscribe_mutex_.unlock_upgrade();

    LOG_DEBUG(LOG_SYSTEM)
    << this_id
    << " resubscriber::stop() called unlock_upgrade() successfully for subscribe_mutex_ of "
    << &subscribe_mutex_;
    ///////////////////////////////////////////////////////////////////////////
}

template <typename... Args>
void resubscriber<Args...>::subscribe(handler&& notify, Args... stopped_args)
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    const auto this_id = boost::this_thread::get_id();
    LOG_DEBUG(LOG_SYSTEM)
    << this_id
    << " resubscriber::subscribe() calling lock_upgrade() for subscribe_mutex_ of "
    << &subscribe_mutex_;

    subscribe_mutex_.lock_upgrade();

    LOG_DEBUG(LOG_SYSTEM)
    << this_id
    << " resubscriber::subscribe() called lock_upgrade() successfully for subscribe_mutex_ of "
    << &subscribe_mutex_;

    if (!stopped_)
    {
        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        subscribe_mutex_.unlock_upgrade_and_lock();
        subscriptions_.push_back(std::forward<handler>(notify));
        subscribe_mutex_.unlock();
        //---------------------------------------------------------------------
        return;
    }

    LOG_DEBUG(LOG_SYSTEM)
    << this_id
    << " resubscriber::subscribe() calling unlock_upgrade() for subscribe_mutex_ of "
    << &subscribe_mutex_;

    subscribe_mutex_.unlock_upgrade();

    LOG_DEBUG(LOG_SYSTEM)
    << this_id
    << " resubscriber::subscribe() called unlock_upgrade() successfully for subscribe_mutex_ of "
    << &subscribe_mutex_;
    ///////////////////////////////////////////////////////////////////////////

    notify(stopped_args...);
}

template <typename... Args>
void resubscriber<Args...>::invoke(Args... args)
{
    do_invoke(args...);
}

template <typename... Args>
void resubscriber<Args...>::relay(Args... args)
{
    // This enqueues work while maintaining order.
    dispatch_.ordered(&resubscriber<Args...>::do_invoke,
        this->shared_from_this(), args...);
}

// private
template <typename... Args>
void resubscriber<Args...>::do_invoke(Args... args)
{
    // Critical Section (prevent concurrent handler execution)
    ///////////////////////////////////////////////////////////////////////////
    const auto this_id = boost::this_thread::get_id();
    LOG_DEBUG(LOG_SYSTEM)
    << this_id
    << " resubscriber::do_invoke() instantiating unique_lock() for invoke_mutex_ of "
    << &invoke_mutex_;

    unique_lock lock(invoke_mutex_);

    LOG_DEBUG(LOG_SYSTEM)
    << this_id
    << " resubscriber::do_invoke() created unique_lock() successfully for invoke_mutex_ of "
    << &invoke_mutex_;

    // Critical Section (protect stop)
    ///////////////////////////////////////////////////////////////////////////
    LOG_DEBUG(LOG_SYSTEM)
    << this_id
    << " resubscriber::do_invoke() calling lock() for subscribe_mutex_ of "
    << &subscribe_mutex_;

    subscribe_mutex_.lock();

    LOG_DEBUG(LOG_SYSTEM)
    << this_id
    << " resubscriber::do_invoke() called lock() successfully for subscribe_mutex_ of "
    << &subscribe_mutex_;

    // Move subscribers from the member list to a temporary list.
    list subscriptions;
    std::swap(subscriptions, subscriptions_);

    subscribe_mutex_.unlock();
    ///////////////////////////////////////////////////////////////////////////

    // Subscriptions may be created while this loop is executing.
    // Invoke subscribers from temporary list and resubscribe as indicated.
    for ( auto& handler: subscriptions)
    {
        //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // DEADLOCK RISK, handler must not return to invoke.
        //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        std::tuple<Args...> tuple_{args...};

        LOG_DEBUG(LOG_SYSTEM)
        << this_id
        << " resubscriber::do_invoke() calling handler(args...) for handler @ "
        << &handler;

        if (handler(args...))
        {
            // Critical Section
            ///////////////////////////////////////////////////////////////////
            subscribe_mutex_.lock_upgrade();

            if (stopped_)
            {
                subscribe_mutex_.unlock_upgrade();
                //-------------------------------------------------------------
                continue;
            }

            subscribe_mutex_.unlock_upgrade_and_lock();
            //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            subscriptions_.push_back(handler);

            LOG_DEBUG(LOG_SYSTEM)
            << this_id
            << " resubscriber::do_invoke() called push_back() for handler @ "
            << &handler;

            subscribe_mutex_.unlock();
            ///////////////////////////////////////////////////////////////////
        }
    }

    ///////////////////////////////////////////////////////////////////////////
}

} // namespace libbitcoin

#endif
