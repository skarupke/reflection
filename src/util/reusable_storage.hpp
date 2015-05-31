#pragma once

#include <vector>

template<typename T>
struct ReusableStorage
{
    struct Reusable
    {
        Reusable(T && object, ReusableStorage & owner)
            : object(std::move(object)), owner(owner)
        {
        }
        Reusable(Reusable &&) = default;
        ~Reusable()
        {
            object.clear();
            owner.Store(std::move(object));
        }

        T object;
    private:
        ReusableStorage & owner;
    };


    void Store(T && object)
    {
        if (object.capacity()) storage.push_back(std::move(object));
    }

    Reusable GetObject()
    {
        if (storage.empty()) return Reusable(T(), *this);
        else
        {
            Reusable result(std::move(storage.back()), *this);
            storage.pop_back();
            return result;
        }
    }

private:
    std::vector<T> storage;
};
