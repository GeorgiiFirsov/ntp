#include "logger/logger.hpp"
#include "logger/logger_internal.hpp"


namespace ntp::logger {

logger_t SetLogger(logger_t new_logger)
{
    //
    // Just trust, if Logger makes everything fine :)
    //

    return details::Logger::InstanceMut().Exchange(new_logger);
}

}  // namespace ntp::logger
