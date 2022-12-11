#pragma once

template <typename T>
T* MarkPointer(T* ptr) noexcept
{
    return reinterpret_cast<T*>(
               reinterpret_cast<unsigned long long>(ptr) | 1ull
           );
}

template <typename T>
T* UnmarkPointer(T* ptr) noexcept
{
    return reinterpret_cast<T*>(
               reinterpret_cast<unsigned long long>(ptr) & ~1ull
           );
}

template <typename T>
bool IsMarkedPointer(T* ptr) noexcept
{
    return !!reinterpret_cast<T*>(
               reinterpret_cast<unsigned long long>(ptr) & 1ull
           );
}
