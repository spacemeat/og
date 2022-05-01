#include "../og/assetDb/gen/inc/assetDb.hpp"
#include "../og/featureDb/gen/inc/featureDb.hpp"
#include "../og/featureReq/gen/inc/featureReq.hpp"
#include "../og/tableau/gen/inc/tableau.hpp"


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


