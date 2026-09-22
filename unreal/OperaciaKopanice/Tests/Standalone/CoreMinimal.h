#pragma once
// Minimal value-type adapter for compiling the actual demo rules without UE.
// This directory must NEVER be added to the Unreal module's include paths.
#include <cstdint>
#include <algorithm>
#include <vector>
#include <unordered_set>
using int32 = std::int32_t;
using uint8 = std::uint8_t;
using uint32 = std::uint32_t;
using TCHAR = char;
#define TEXT(x) x
#define INDEX_NONE -1
struct FMath { template<class T> static T Clamp(T X,T A,T B) { return std::clamp(X,A,B); } };
template<class T> struct TArray : std::vector<T>
{
    using std::vector<T>::vector;
    void Add(const T& V) { this->push_back(V); }
    int32 Num() const { return static_cast<int32>(this->size()); }
    bool IsEmpty() const { return this->empty(); }
    void Insert(const T& V,int32 I) { this->insert(this->begin()+I,V); }
};
template<class T> struct TSet : std::unordered_set<T>
{
    void Add(const T& V) { this->insert(V); }
    bool Contains(const T& V) const { return this->count(V)!=0; }
};
#define OPERACIAKOPANICE_API
struct FIntPoint
{
    int32 X, Y;
    constexpr FIntPoint(int32 x = 0, int32 y = 0) : X(x), Y(y) {}
    constexpr bool operator==(const FIntPoint& other) const { return X == other.X && Y == other.Y; }
    constexpr FIntPoint operator-(const FIntPoint& other) const { return {X-other.X, Y-other.Y}; }
    FIntPoint& operator+=(const FIntPoint& other) { X += other.X; Y += other.Y; return *this; }
};
