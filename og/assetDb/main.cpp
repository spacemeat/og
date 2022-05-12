#include "../logger/inc/logger.hpp"
#include "inc/AssetPack.hpp"
#include <iostream>


std::optional<og::Logger> og::l;


int main(int argc, char** argv)
{
    og::l.emplace("loggers.hu");

    // delete any *.adbcache

    // make default empty in-memory adbData

    auto adb = og::AssetPack("assets.hu", 0);

    // get diff

    // open cache file (creates new)

    //
}
