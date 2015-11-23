#pragma once

#include <typeinfo>
#include <type_traits>
#include <utility>
#include <cstddef>
#include <new>
#include <memory>
#include <typeinfo>
#include "util/forwarding_constructor.hpp"

template<size_t Size, size_t Alignment, typename Allocator>
struct TypeErasureStorage
{
    static_assert(Size >= sizeof(typename Allocator::pointer), "need to at least be able to store a pointer to do heap allocations");
private:
    template<typename T>
    using TemplatedAllocator = typename std::allocator_traits<Allocator>::template rebind_alloc<T>;
    template<typename T>
    using AllocatorTraits = std::allocator_traits<TemplatedAllocator<T> >;
    template<typename T>
    using PointerType = typename AllocatorTraits<T>::pointer;
public:

    template<typename T, typename... Args>
    void construct(Args &&... args)
    {
        TemplatedAllocator<T> allocator;
        if (is_in_place<T>()) AllocatorTraits<T>::construct(allocator, &reinterpret_cast<T &>(storage), std::forward<Args>(args)...);
        else AllocatorTraits<T>::construct(allocator, *new (&storage) PointerType<T>(AllocatorTraits<T>::allocate(allocator, 1)), std::forward<Args>(args)...);
    }

    template<typename T>
    void destroy()
    {
        TemplatedAllocator<T> allocator;
        if (is_in_place<T>()) AllocatorTraits<T>::destroy(allocator, &reinterpret_cast<T &>(storage));
        else
        {
            PointerType<T> pointer = reinterpret_cast<PointerType<T> &>(storage);
            if (pointer)
            {
                AllocatorTraits<T>::destroy(allocator, pointer);
                AllocatorTraits<T>::deallocate(allocator, pointer, 1);
            }
        }
    }

    template<typename T>
    T & get_reference()
    {
        if (is_in_place<T>()) return reinterpret_cast<T &>(storage);
        else return *reinterpret_cast<PointerType<T> &>(storage);
    }
    template<typename T>
    const T & get_reference() const
    {
        if (is_in_place<T>()) return reinterpret_cast<const T &>(storage);
        else return *reinterpret_cast<const PointerType<T> &>(storage);
    }

    template<typename T>
    static constexpr bool is_in_place()
    {
        return sizeof(T) <= Size
                && Alignment % alignof(T) == 0
                /*&& std::is_nothrow_move_assignable<T>::value*/;
    }

private:
    typename std::aligned_storage<Size, Alignment>::type storage;
};

template<typename T, typename Storage>
struct TypeErasureTarget
{
    static const std::type_info & target_type()
    {
        return typeid(T);
    }
    static void * target(const std::type_info & info, Storage & storage)
    {
        if (typeid(T) == info) return &storage.template get_reference<T>();
        else return nullptr;
    }
};

template<size_t Size, size_t Alignment, typename Allocator>
struct BaseVTable
{
    typedef TypeErasureStorage<Size, Alignment, Allocator> Storage;
    void (*destroy)(Storage & object);
    const std::type_info & target_type;
    void * (*target)(const std::type_info & type, Storage & object);

    template<typename T>
    BaseVTable(T *)
        : destroy([](Storage & storage)
        {
            storage.template destroy<T>();
        })
        , target_type(TypeErasureTarget<T, Storage>::target_type())
        , target(&TypeErasureTarget<T, Storage>::target)
    {
    }
};

template<size_t Size, size_t Alignment, typename Allocator>
struct MoveVTable : BaseVTable<Size, Alignment, Allocator>
{
private:
    template<typename T>
    using PointerType = typename std::allocator_traits<Allocator>::template rebind_alloc<T>::pointer;
public:
    typedef TypeErasureStorage<Size, Alignment, Allocator> Storage;

    template<typename T>
    MoveVTable(T *)
        : BaseVTable<Size, Alignment, Allocator>(static_cast<T *>(nullptr))
        , move_construct(&templated_move_construct<T>)
        , swap_ptr(&templated_swap<T>)
        , move_and_destroy(&templated_move_and_destroy<T>)
    {
    }

    void (*move_construct)(Storage & lhs, Storage && rhs);

    void swap(Storage & lhs, const MoveVTable * rhs_vtable, Storage & rhs) const
    {
        if (rhs_vtable == this)
        {
            swap_ptr(lhs, rhs);
        }
        else if (rhs_vtable)
        {
            Storage temp;
            move_and_destroy(temp, std::move(lhs));
            rhs_vtable->move_and_destroy(lhs, std::move(rhs));
            move_and_destroy(rhs, std::move(temp));
        }
        else
        {
            move_and_destroy(rhs, std::move(lhs));
        }
    }

private:
    void (*swap_ptr)(Storage &, Storage &);
    void (*move_and_destroy)(Storage &, Storage &&);

protected:
    template<typename T>
    static void templated_move_construct(Storage & lhs, Storage && rhs)
    {
        if (Storage::template is_in_place<T>()) lhs.template construct<T>(std::move(rhs.template get_reference<T>()));
        else
        {
            PointerType<T> & rhs_ptr = reinterpret_cast<PointerType<T> &>(rhs);
            new (&lhs) PointerType<T>(rhs_ptr);
            rhs_ptr = nullptr;
        }
    }
    template<typename T>
    static void templated_swap(Storage & lhs, Storage & rhs)
    {
        Storage temp;
        templated_move_and_destroy<T>(temp, std::move(rhs));
        templated_move_and_destroy<T>(rhs, std::move(lhs));
        templated_move_and_destroy<T>(lhs, std::move(temp));
    }
    template<typename T>
    static void templated_move_and_destroy(Storage & lhs, Storage && rhs)
    {
        templated_move_construct<T>(lhs, std::move(rhs));
        rhs.template destroy<T>();
    }
};

template<size_t Size, size_t Alignment, typename Allocator>
struct CopyVTable : MoveVTable<Size, Alignment, Allocator>
{
    typedef MoveVTable<Size, Alignment, Allocator> Base;

    template<typename T>
    CopyVTable(T *)
        : Base(static_cast<T *>(nullptr))
        , copy_construct(&templated_copy_construct<T>)
        , copy_assign(&templated_copy_assign<T>)
    {
    }

    typedef TypeErasureStorage<Size, Alignment, Allocator> Storage;
    void (*copy_construct)(Storage & lhs, const Storage & rhs);
    void (*copy_assign)(Storage & lhs, const Storage & rhs);

private:

    template<typename T>
    static void templated_copy_construct(Storage & lhs, const Storage & rhs)
    {
        lhs.template construct<T>(rhs.template get_reference<T>());
    }
    template<typename T>
    static
    typename std::enable_if<std::is_copy_assignable<T>::value>::type
    templated_copy_assign(Storage & lhs, const Storage & rhs)
    {
        lhs.template get_reference<T>() = rhs.template get_reference<T>();
    }
    template<typename T>
    static
    typename std::enable_if<!std::is_copy_assignable<T>::value>::type
    templated_copy_assign(Storage & lhs, const Storage & rhs)
    {
        Storage temp;
        templated_copy_construct<T>(temp, rhs);
        lhs.template destroy<T>();
        Base::template templated_move_and_destroy<T>(lhs, std::move(temp));
    }
};

template<size_t Size, size_t Alignment, typename Allocator>
struct EqualityVTable
{
    typedef TypeErasureStorage<Size, Alignment, Allocator> Storage;
    bool (*equals)(const Storage &, const Storage &);
    template<typename T>
    EqualityVTable(T *)
        : equals([](const Storage & lhs, const Storage & rhs) -> bool
        {
            return lhs.template get_reference<T>() == rhs.template get_reference<T>();
        })
    {
    }
};

template<size_t Size, size_t Alignment, typename Allocator>
struct RegularVTable : EqualityVTable<Size, Alignment, Allocator>
{
    typedef TypeErasureStorage<Size, Alignment, Allocator> Storage;
    bool (*less)(const Storage &, const Storage &);

    template<typename T>
    RegularVTable(T *)
        : EqualityVTable<Size, Alignment, Allocator>(static_cast<T *>(nullptr))
        , less([](const Storage & lhs, const Storage & rhs) -> bool
        {
            return lhs.template get_reference<T>() < rhs.template get_reference<T>();
        })
    {
    }
};

namespace detail
{
template<size_t Size>
struct DefaultTypeErasureAlignment
    : std::integral_constant<size_t, Size <= 1 ? 1 : Size == 2 ? 2 : Size <= 4 ? 4 : Size <= 8 ? 8 : 16>
{
};
typedef std::allocator<int> DefaultTypeErasureAllocator;
}

template<size_t Size, template<size_t, size_t, typename> class VTable, size_t Alignment = detail::DefaultTypeErasureAlignment<Size>::value, typename Allocator = detail::DefaultTypeErasureAllocator>
struct BaseTypeErasure
{
    BaseTypeErasure()
        : vtable()
    {
    }
    template<typename T>
    BaseTypeErasure(forwarding_constructor, T && object)
        : vtable(&GetVTable<typename std::decay<T>::type>())
    {
        storage.template construct<typename std::decay<T>::type>(std::forward<T>(object));
    }
    template<typename T>
    BaseTypeErasure(T object)
        : BaseTypeErasure(forwarding_constructor{}, std::move(object))
    {
    }
    BaseTypeErasure(const BaseTypeErasure & other)
        : vtable(other.vtable)
    {
        if (vtable) vtable->copy_construct(storage, other.storage);
    }
    BaseTypeErasure(BaseTypeErasure && other)
        : vtable(other.vtable)
    {
        if (vtable) vtable->move_construct(storage, std::move(other.storage));
    }
    BaseTypeErasure & operator=(const BaseTypeErasure & other)
    {
        if (vtable && vtable == other.vtable) vtable->copy_assign(storage, other.storage);
        else BaseTypeErasure(other).swap(*this);
        return *this;
    }
    BaseTypeErasure & operator=(BaseTypeErasure & other)
    {
        return *this = const_cast<const BaseTypeErasure &>(other);
    }
    BaseTypeErasure & operator=(BaseTypeErasure && other)
    {
        swap(other);
        return *this;
    }
    template<typename T>
    BaseTypeErasure & operator=(T && other)
    {
        BaseTypeErasure(forwarding_constructor{}, std::forward<T>(other)).swap(*this);
        return *this;
    }
    ~BaseTypeErasure()
    {
        if (vtable) vtable->destroy(storage);
    }

    void swap(BaseTypeErasure & other)
    {
        if (vtable) vtable->swap(storage, other.vtable, other.storage);
        else if (other.vtable) other.vtable->swap(other.storage, vtable, storage);
        std::swap(vtable, other.vtable);
    }

    template<typename T>
    T * target()
    {
        if (vtable) return static_cast<T *>(vtable->target(typeid(T), storage));
        else return nullptr;
    }
    template<typename T>
    const T * target() const
    {
        return const_cast<BaseTypeErasure &>(*this).target<T>();
    }
    const std::type_info & target_type() const
    {
        if (vtable) return vtable->target_type;
        else return typeid(void);
    }

protected:
    TypeErasureStorage<Size, Alignment, Allocator> storage;
    typedef VTable<Size, Alignment, Allocator> MyVTable;
    const VTable<Size, Alignment, Allocator> * vtable;

    template<typename T>
    static const MyVTable & GetVTable()
    {
        static const MyVTable vtable = MyVTable(static_cast<T *>(nullptr));
        return vtable;
    }
};

template<size_t Size, template<size_t, size_t, typename> class Vtable, size_t Alignment = detail::DefaultTypeErasureAlignment<Size>::value, typename Allocator = detail::DefaultTypeErasureAllocator>
struct EqualityTypeErasure : BaseTypeErasure<Size, Vtable, Alignment, Allocator>
{
    typedef BaseTypeErasure<Size, Vtable, Alignment, Allocator> Base;
    using Base::Base;

    bool operator==(const EqualityTypeErasure & other) const
    {
        if (!this->vtable) return !other.vtable;
        else return this->vtable == other.vtable && this->vtable->equals(this->storage, other.storage);
    }
    bool operator!=(const EqualityTypeErasure & other) const
    {
        return !(*this == other);
    }
};

template<size_t Size, template<size_t, size_t, typename> class Vtable, size_t Alignment = detail::DefaultTypeErasureAlignment<Size>::value, typename Allocator = detail::DefaultTypeErasureAllocator>
struct RegularTypeErasure : EqualityTypeErasure<Size, Vtable, Alignment, Allocator>
{
    typedef EqualityTypeErasure<Size, Vtable, Alignment, Allocator> Base;
    using Base::Base;

    bool operator<(const RegularTypeErasure & other) const
    {
        return this->vtable < other.vtable
                || (!(other.vtable < this->vtable) && this->vtable->less(this->storage, other.storage));
    }
    bool operator>(const RegularTypeErasure & other) const
    {
        return other < *this;
    }
    bool operator<=(const RegularTypeErasure & other) const
    {
        return !(other < *this);
    }
    bool operator>=(const RegularTypeErasure & other) const
    {
        return !(*this < other);
    }
};

template<typename Func, size_t Size, template<size_t, template<size_t, size_t, typename> class, size_t, typename> class Base, template<size_t, size_t, typename> class Vtable, size_t Alignment = detail::DefaultTypeErasureAlignment<Size>::value, typename Allocator = detail::DefaultTypeErasureAllocator>
struct CallableTypeErasure;
template<typename Result, typename... Args, size_t Size, template<size_t, template<size_t, size_t, typename> class, size_t, typename> class Base, template<size_t, size_t, typename> class Vtable, size_t Alignment, typename Allocator>
struct CallableTypeErasure<Result (Args...), Size, Base, Vtable, Alignment, Allocator> : Base<Size, Vtable, Alignment, Allocator>
{
    typedef Base<Size, Vtable, Alignment, Allocator> MyBase;

    CallableTypeErasure()
        : MyBase(), call_ptr()
    {
    }
    template<typename T>
    CallableTypeErasure(T object)
        : MyBase(forwarding_constructor{}, std::move(object))
        , call_ptr(&caller<Result, T>::call)
    {
    }

    Result operator()(Args... args) const
    {
        return call_ptr(const_cast<CallableTypeErasure *>(this)->storage, std::forward<Args>(args)...);
    }

private:
    template<typename FuncResult, typename T>
    struct caller
    {
        static FuncResult call(TypeErasureStorage<Size, Alignment, Allocator> & storage, Args... arguments)
        {
            return storage.template get_reference<T>()(std::forward<Args>(arguments)...);
        }
    };
    template<typename T>
    struct caller<void, T>
    {
        static void call(TypeErasureStorage<Size, Alignment, Allocator> & storage, Args... arguments)
        {
            storage.template get_reference<T>()(std::forward<Args>(arguments)...);
        }
    };
    Result (*call_ptr)(TypeErasureStorage<Size, Alignment, Allocator> &, Args...);
};
