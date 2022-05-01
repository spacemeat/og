#pragma once

#include "AssetDb.hpp"      // purely for editors
#include <iostream>

namespace og
{
    template<class Data>
    AssetDb<Data>::AssetDb()
    {

    }

    template<class Data>
    AssetDb<Data>::~AssetDb()
    {

    }

    template<class Data>
    void AssetDb<Data>::syncWithSource()
    {
        auto d = static_cast<Data *>(this);
        std::cout << d->get_cacheFile();
    }

    template<class Data>
    void AssetDb<Data>::syncDefs()
    {

    }

    template<class Data>
    void AssetDb<Data>::buildCache()
    {

    }
}
