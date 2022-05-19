#include "inc/logger.hpp"


std::optional<og::Logger> og::l;


int main(int argc, char** argv)
{
    using namespace og;

    l.emplace("loggers.hu");

    try
    {
        log("Hello, world!");
        log(logger::logTags::warn, "Hello, warning!");
        log(logger::logTags::error, "Hello, error!");
    }
    catch (std::exception const & ex)
    {
        std::cout << "Exception thrown: " << ex.what() << "\n";
    }

    return 0;
}
