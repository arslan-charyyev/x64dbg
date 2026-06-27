#pragma once

#include <vector>
#include <cstring>

// Mirrors src/bridge/bridgelist.h. Must be included after BridgeAlloc/BridgeFree
// are declared (see Bridge.h). __debugbreak() is provided by Types.h on Linux.

typedef struct
{
    int count; //Number of element in the list.
    size_t size; //Size of list in bytes (used for type checking).
    void* data; //Pointer to the list contents. Must be deleted by the caller using BridgeFree (or BridgeList::Free).
} ListInfo;

#define ListOf(Type) ListInfo*

template<typename Type>
class BridgeList
{
public:
    explicit BridgeList()
    {
        memset(&_listInfo, 0, sizeof(_listInfo));
    }

    ~BridgeList()
    {
        Cleanup();
    }

    Type* Data() const
    {
        return reinterpret_cast<Type*>(_listInfo.data);
    }

    int Count() const
    {
        if(_listInfo.size != _listInfo.count * sizeof(Type)) //make sure the user is using the correct type.
            __debugbreak();
        return _listInfo.count;
    }

    void Cleanup()
    {
        if(_listInfo.data)
        {
            BridgeFree(_listInfo.data);
            _listInfo.data = nullptr;
        }
    }

    ListInfo* operator&()
    {
        Cleanup();
        return &_listInfo;
    }

    Type & operator[](size_t index) const
    {
        if(index >= size_t(Count())) //make sure the out-of-bounds access is caught as soon as possible.
            __debugbreak();
        return Data()[index];
    }

    static bool CopyData(ListInfo* listInfo, const std::vector<Type> & listData)
    {
        if(!listInfo)
            return false;
        listInfo->count = int(listData.size());
        listInfo->size = listInfo->count * sizeof(Type);
        if(listInfo->count)
        {
            listInfo->data = BridgeAlloc(listInfo->size);
            Type* curItem = reinterpret_cast<Type*>(listInfo->data);
            for(const auto & item : listData)
            {
                *curItem = item;
                ++curItem;
            }
        }
        else
            listInfo->data = nullptr;
        return true;
    }

    static bool Free(const ListInfo* listInfo)
    {
        if(!listInfo || listInfo->size != listInfo->count * sizeof(Type) || (listInfo->count && !listInfo->data))
            return false;
        BridgeFree(listInfo->data);
        return true;
    }

    static bool ToVector(const ListInfo* listInfo, std::vector<Type> & listData, bool freedata = true)
    {
        if(!listInfo || listInfo->size != listInfo->count * sizeof(Type) || (listInfo->count && !listInfo->data))
            return false;
        listData.resize(listInfo->count);
        for(int i = 0; i < listInfo->count; i++)
            listData[i] = ((Type*)listInfo->data)[i];
        if(freedata && listInfo->data)
            BridgeFree(listInfo->data);
        return true;
    }

private:
    ListInfo _listInfo;
};
