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
#include <bitcoin/bitcoin/utility/flush_lock.hpp>

#include <memory>
#include <bitcoin/bitcoin.hpp>
#include <boost/filesystem.hpp>
#include <bitcoin/bitcoin/unicode/file_lock.hpp>
#include <bitcoin/bitcoin/unicode/ifstream.hpp>
#include <bitcoin/bitcoin/unicode/ofstream.hpp>

namespace libbitcoin {

// static
bool flush_lock::create(const std::string& file)
{
    bc::ofstream stream(file);
    return stream.good();
}

// static
bool flush_lock::exists(const std::string& file)
{
    bc::ifstream stream(file);
    return stream.good();
    ////return boost::filesystem::exists(file);
}

// static
bool flush_lock::destroy(const std::string& file)
{
    return boost::filesystem::remove(file);
    ////std::remove(file.c_str());
}

flush_lock::flush_lock(const path& file)
  : file_(file.string()), locked_(false)
{
}

bool flush_lock::try_lock()
{
    return !exists(file_);
}

bool flush_lock::lock_shared()
{
    const auto this_id = boost::this_thread::get_id();

    LOG_VERBOSE(LOG_SYSTEM)
    << this_id
    << " flush_lock::lock_shared() was called.";

    if (locked_)
    {
        LOG_VERBOSE(LOG_SYSTEM)
        << this_id
        << " flush_lock::lock_shared() locked_ already true.";

        return true;
    }

    LOG_VERBOSE(LOG_SYSTEM)
    << this_id
    << " flush_lock::lock_shared() calling create() for file: "
    << file_;

    locked_ = create(file_);

    if (!locked_)
    {
        LOG_VERBOSE(LOG_SYSTEM)
        << this_id
        << " error flush_lock::lock_shared() failed to create() file: "
        << file_;
    }

    LOG_VERBOSE(LOG_SYSTEM)
    << this_id
    << " flush_lock::lock_shared() done. returning...";

    return locked_;
}

bool flush_lock::unlock_shared()
{
    if (!locked_)
        return true;

    bool retval = destroy(file_);

    // always set locked_ to false, even if boost::filesystem::remove() returns false
    locked_ = false;

    // a return value of false is not an error, it's useful for debugging purposes only
    // false indicates that the flush lock file was already deleted when remove() was called
    return retval;
}

} // namespace libbitcoin
