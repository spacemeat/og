#include "gen-cpp/inc/og.hpp"
#include <fmt/format.h>

int main(int argc, char** argv)
{
    auto tr = hu::Trove::fromFile("featureDb.hu");
    if (auto && t = std::get_if<hu::Trove>(& tr))
    {
        auto featureDb = og::featureDb(t->root());
        auto features = featureDb.get_features();
        auto f = features[3];

        std::cout << "fragment_shader_bit?: " << og::HumonFormat(std::get<2>(f.get_queues()[1])) << "\n";
    }
}
