#pragma once

#include "AssetDb.hpp"      // purely for editors
#include <fmt/core.h>

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
        fmt::print(d->get_cache());
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
