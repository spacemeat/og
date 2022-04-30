#include "../gen/og/assetDb/inc/assetDb.hpp"
#include "../gen/og/featureDb/inc/featureDb.hpp"
#include "../gen/og/featureReq/inc/featureReq.hpp"
#include "../gen/og/tableau/inc/tableau.hpp"


std::vector<og::featureReq::featureReq> getReqs(og::tableau::tableau const & tableau)
{
    auto vec = std::vector<og::featureReq::featureReq> { };
    return vec;
}



int main(int argc, char** argv)
{
    auto tr = hu::Trove::fromFile("tableau.hu");
    if (auto && t = std::get_if<hu::Trove>(& tr))
    {
        auto tableau = og::tableau::tableau(t->root());
        std::cout << tableau.get_name();
    }

    return 0;
}


