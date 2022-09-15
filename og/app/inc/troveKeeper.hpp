#include <string>
#include <humon/humon.hpp>
#include <unordered_map>
#include <optional>

namespace og
{
    class TroveKeeper
    {
    public:
        hu::Node loadAndKeep(std::string const & path);
        hu::Node keep(hu::Trove && trove, std::string const & path);
        hu::Trove & find(std::string const & path);

    private:
        std::unordered_map<std::string, hu::Trove> keptTroves;
    };

    extern std::optional<TroveKeeper> troves;
}
