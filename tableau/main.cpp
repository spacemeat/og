#include "../gen/og/tableau/inc/tableau.hpp"

int main(int argc, char** argv)
{
    auto tr = hu::Trove::fromFile("tableau.hu");
    if (auto && t = std::get_if<hu::Trove>(& tr))
    {
        auto tableau = og::tableau::tableau(t->root());
        std::cout << tableau;
    }
}
