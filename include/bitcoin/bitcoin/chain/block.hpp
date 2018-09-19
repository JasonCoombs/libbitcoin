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
#ifndef LIBBITCOIN_CHAIN_BLOCK_HPP
#define LIBBITCOIN_CHAIN_BLOCK_HPP

#include <cstddef>
#include <cstdint>
#include <istream>
#include <memory>
#include <string>
#include <vector>
#include <boost/optional.hpp>
#include <bitcoin/bitcoin/chain/chain_state.hpp>
#include <bitcoin/bitcoin/chain/header.hpp>
#include <bitcoin/bitcoin/chain/transaction.hpp>
#include <bitcoin/bitcoin/define.hpp>
#include <bitcoin/bitcoin/error.hpp>
#include <bitcoin/bitcoin/math/hash.hpp>
#include <bitcoin/bitcoin/utility/asio.hpp>
#include <bitcoin/bitcoin/utility/data.hpp>
#include <bitcoin/bitcoin/utility/reader.hpp>
#include <bitcoin/bitcoin/utility/thread.hpp>
#include <bitcoin/bitcoin/utility/writer.hpp>

namespace libbitcoin {

class settings;

namespace chain {

class BC_API block
{
public:
    typedef std::vector<block> list;
    typedef std::vector<size_t> indexes;

    // THIS IS FOR LIBRARY USE ONLY, DO NOT CREATE A DEPENDENCY ON IT.
    struct validation
    {
        asio::time_point start_deserialize;
        asio::time_point end_deserialize;
        asio::time_point start_check;
        asio::time_point start_populate;
        asio::time_point start_accept;
        asio::time_point start_connect;
        asio::time_point start_notify;
        asio::time_point start_pop;
        asio::time_point start_push;
        asio::time_point end_push;
        float cache_efficiency;
    };

    // Constructors.
    //-------------------------------------------------------------------------

    block();

    block(block&& other);
    block(const block& other);

    block(chain::header&& header, transaction::list&& transactions);
    block(const chain::header& header, const transaction::list& transactions);

    // Operators.
    //-------------------------------------------------------------------------

    /// This class is move assignable but NOT copy assignable (performance).
    block& operator=(block&& other);
    block& operator=(const block& other) = delete;

    bool operator==(const block& other) const;
    bool operator!=(const block& other) const;

    // Deserialization.
    //-------------------------------------------------------------------------

    static block factory(const data_chunk& data, bool witness=false);
    static block factory(std::istream& stream, bool witness=false);
    static block factory(reader& source, bool witness=false);

    bool from_data(const data_chunk& data, bool witness=false);
    bool from_data(std::istream& stream, bool witness=false);
    bool from_data(reader& source, bool witness=false);

    bool is_valid() ;

    // Serialization.
    //-------------------------------------------------------------------------

    data_chunk to_data(bool witness=false) const;
    void to_data(std::ostream& stream, bool witness=false) const;
    void to_data(writer& sink, bool witness=false) const;
    hash_list to_hashes(bool witness=false) const;

    // Properties (size, accessors, cache).
    //-------------------------------------------------------------------------

    size_t serialized_size(bool witness=false) const;

    chain::header& header();
    void set_header(chain::header& value);
    void set_header(chain::header&& value);

     transaction::list& transactions() ;
    void set_transactions(transaction::list& value);
    void set_transactions(transaction::list&& value);

    hash_digest hash() ;

    // Utilities.
    //-------------------------------------------------------------------------

    static size_t locator_size(size_t top);
    static indexes locator_heights(size_t top);

    /// Clear witness from all inputs (does not change default hash).
    void strip_witness();

    // Validation.
    //-------------------------------------------------------------------------

    static uint64_t subsidy(size_t height, uint64_t subsidy_interval,
        uint64_t initial_block_subsidy_satoshi);

    uint64_t fees() ;
    uint64_t claim() ;
    uint64_t reward(size_t height, uint64_t subsidy_interval,
        uint64_t initial_block_subsidy_satoshi) ;
    hash_digest generate_merkle_root(bool witness=false) ;
    size_t signature_operations() ;
    size_t signature_operations(bool bip16, bool bip141) ;
    size_t total_non_coinbase_inputs() ;
    size_t total_inputs() ;
    size_t weight() ;

    bool is_extra_coinbases() ;
    bool is_final(size_t height, uint32_t block_time) ;
    bool is_distinct_transaction_set() ;
    bool is_valid_coinbase_claim(size_t height, uint64_t subsidy_interval,
                                 uint64_t initial_block_subsidy_satoshi) ;
    bool is_valid_coinbase_script(size_t height) ;
    bool is_valid_witness_commitment() ;
    bool is_forward_reference() ;
    bool is_internal_double_spend() ;
    bool is_valid_merkle_root() ;
    bool is_segregated() ;

    code check(uint64_t max_money, uint32_t timestamp_limit_seconds,
        uint32_t proof_of_work_limit, bool scrypt=false) ;
    code check_transactions(uint64_t max_money) ;
    code accept( settings& settings, bool transactions=true,
        bool header=true) ;
    code accept(const chain_state& state,  settings& settings,
        bool transactions=true, bool header=true) ;
    code accept_transactions(const chain_state& state) ;
    code connect() ;
    code connect(const chain_state& state) ;
    code connect_transactions(const chain_state& state) ;

    // THIS IS FOR LIBRARY USE ONLY, DO NOT CREATE A DEPENDENCY ON IT.
    mutable validation metadata;

protected:
    void reset();

private:
    typedef boost::optional<size_t> optional_size;

    optional_size total_inputs_cache() const;
    optional_size non_coinbase_inputs_cache() const;

    chain::header header_;
    transaction::list transactions_;

    // These share a mutext as they are not expected to contend.
    mutable boost::optional<bool> segregated_;
    mutable optional_size total_inputs_;
    mutable optional_size non_coinbase_inputs_;
    mutable boost::optional<size_t> base_size_;
    mutable boost::optional<size_t> total_size_;
    mutable upgrade_mutex mutex_;
};

} // namespace chain
} // namespace libbitcoin

#endif
