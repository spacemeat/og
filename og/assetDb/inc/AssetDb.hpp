#pragma once

//#include "../gen-cpp/og_gen/inc/og_gen.hpp"
#include <map>
#include <functional>

namespace og
{
    struct CacheRef
    {
        void * assetDef;
    };


    template <class Fn> // TODO: concept for callable
    struct PartingShot
    {
        PartingShot(Fn whenDone)
        : endFn(whenDone)
        { }

        ~PartingShot()
        {
            endFn();
        }

        std::function<void()> endFn;
    };


    class AssetDbCache
    {
    public:
        AssetDbCache();

    public:
        auto Modify()
        {
            auto ps = PartingShot([this](){ this->endModifying(); });
            beginModifying();
            return ps;
        }

        void clear();
        size_t appendDataBlock(size_t size);

        size_t getNumWindows();
        auto Read()
        {
            auto ps = PartingShot([this](){ this->endReading(); });
            beginReading();
            return ps;
        }

        void * getAssetData(size_t handle);

    private:
        void beginModifying();
        void endModifying();

        void beginReading();
        void endReading();

        void setWindow(size_t window);

    private:
        std::vector<CacheRef> refs;
        std::map<std::string_view, size_t> refsDict;
    };


    template<class Data>
    class AssetDef
    {

    };


    template<class Data>
    class AssetDb
    {
    public:
        AssetDb();
        ~AssetDb();

        void syncWithSource();

        // mm will call for blocks of asset data it needs to populate the staging buffer or whatever.

    private:
        void syncDefs();
        void buildCache();

    private:
        std::string_view sourcePath;
        //gen::assetDb data;
        //gen::assetDb workingData;
        //gen::Diff<gen::assetDb> dataDiff;
        AssetDbCache cache;
    };
}

#include "AssetDb.inl.hpp"
