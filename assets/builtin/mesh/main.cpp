#include "gen-cpp/og/inc/og.hpp"
#include <fmt/format.h>

int main(int argc, char** argv)
{
    auto tr = hu::Trove::fromFile("sampleMesh.hu");
    if (auto && t = std::get_if<hu::Trove>(& tr))
    {
        auto mesh = og::mesh(t->root());
        auto meshData = mesh.get_meshData();
        if (meshData.has_value())
        {
            for (auto && md: *meshData)
            {
                auto vxs = md.get_vertices();
                if (vxs.has_value())
                {
                    for (auto && vx: *vxs)
                    {
                        for (int i = 0; i < 3; ++i)
                        {
                            fmt::print("{}; ", vx[i]);
                        }
                        fmt::print("\n");
                    }
                }
            }
        }
    }
}
