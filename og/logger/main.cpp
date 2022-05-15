#include "inc/logger.hpp"


std::optional<og::Logger> og::l;


int main(int argc, char** argv)
{
    using namespace og;

    l.emplace("loggers.hu");

    try
    {
        l->log(0, "Hello, world!");
        l->log(2, "Hello, warning!");
        l->log(1, "Hello, error!");
    }
    catch (std::exception const & e)
    {
        std::cout << "Exception thrown: " << e.what() << "\n";
    }

    return 0;
}
