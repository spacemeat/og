#include "gen/inc/logger.hpp"

int main(int argc, char** argv)
{
    auto tr = hu::Trove::fromFile("loggers.hu");
    if (auto && t = std::get_if<hu::Trove>(& tr))
    {
        auto logger = og::logger::logger(t->root());
        logger.init();

        try
        {
            logger.log(0, "Hello, world!\n");
            logger.log(2, "Hello, warning!\n");
            logger.log(1, "Hello, error!\n");
        }
        catch (std::exception const & e)
        {
            std::cout << "Exception thrown: " << e.what() << "\n";
        }
    }

    return 0;
}
