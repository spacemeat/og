#include "../assetDb/gen/inc/og_assetDb.hpp"
#include "../featureDb/gen/inc/og_featureDb.hpp"


std::vector<og::featureDb::featureReq> getReqs(og::featureDb::tableau const & tableau)
{
    auto vec = std::vector<og::featureDb::featureReq> { };
    return vec;
}



int main(int argc, char** argv)
{
    auto tr = hu::Trove::fromFile("tableau.hu");
    if (auto && t = std::get_if<hu::Trove>(& tr))
    {
        auto tableau = og::featureDb::tableau(t->root());
        std::cout << tableau.get_name();
    }

    return 0;
}


