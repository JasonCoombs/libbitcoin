#ifndef LIBBITCOIN_CONFIGURATION_HPP
#define LIBBITCOIN_CONFIGURATION_HPP

#include <bitcoin/bitcoin/define.hpp>
#include <bitcoin/bitcoin/config/settings.hpp>

namespace libbitcoin {
    namespace config {

class BC_API configuration
{
public:
    // Construct with defaults and no context
    configuration();
    ~configuration();
    // Initialize for the given context
    void init(libbitcoin::config::settings context);
};

    } // namespace config
} // namespace libbitcoin

#endif
