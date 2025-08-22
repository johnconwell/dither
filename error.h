#ifndef __ERROR_H
#define __ERROR_H

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

struct PairHash
{
    template <class T1, class T2>
    std::size_t operator () (const std::pair<T1, T2>& p) const {
        auto h1 = std::hash<T1>{}(p.first);
        auto h2 = std::hash<T2>{}(p.second);
        return h1 ^ (h2 << 1);
    }
};

enum ErrorDiffusionAlgorithm
{
    LINEAR,
    FLOYD_STEINBERG,
    JARVICE_JUDICE_NINKE,
    STUCKI,
    ATKINSON,
    BURKES,
    SIERRA,
    SIERRA_TWO_ROW,
    SIERRA_LITE
};

struct Error
{
    static inline constexpr std::pair<int, int> R    {1,  0};
    static inline constexpr std::pair<int, int> RR   {2,  0};
    static inline constexpr std::pair<int, int> LLD  {-2, 1};
    static inline constexpr std::pair<int, int> LD   {-1, 1};
    static inline constexpr std::pair<int, int> D    {0,  1};
    static inline constexpr std::pair<int, int> RD   {1,  1};
    static inline constexpr std::pair<int, int> RRD  {2,  1};
    static inline constexpr std::pair<int, int> LLDD {-2, 2};
    static inline constexpr std::pair<int, int> LDD  {-1, 2};
    static inline constexpr std::pair<int, int> DD   {0,  2};
    static inline constexpr std::pair<int, int> RDD  {1,  2};
    static inline constexpr std::pair<int, int> RRDD {2,  2};

    static inline const std::vector<std::pair<int, int>> COORDINATES_LINEAR = {R};
    static inline const std::vector<std::pair<int, int>> COORDINATES_FLOYD_STEINBERG = {R, LD, D, RD};
    static inline const std::vector<std::pair<int, int>> COORDINATES_JARVICE_JUDICE_NINKE = {R, RR, LLD, LD, D, RD, RRD, LLDD, LDD, DD, RDD, RRDD};
    static inline const std::vector<std::pair<int, int>> COORDINATES_STUCKI = {R, RR, LLD, LD, D, RD, RRD, LLDD, LDD, DD, RDD, RRDD};
    static inline const std::vector<std::pair<int, int>> COORDINATES_ATKINSON = {R, RR, LD, D, RD, DD};
    static inline const std::vector<std::pair<int, int>> COORDINATES_BURKES = {R, RR, LLD, LD, D, RD, RRD};
    static inline const std::vector<std::pair<int, int>> COORDINATES_SIERRA = {R, RR, LLD, LD, D, RD, RRD, LDD, DD, RDD};
    static inline const std::vector<std::pair<int, int>> COORDINATES_SIERRA_TWO_ROW = {R, RR, LLD, LD, D, RD, RRD};
    static inline const std::vector<std::pair<int, int>> COORDINATES_SIERRA_LITE = {R, LD, D};

    static const std::unordered_map<std::pair<int, int>, const double, PairHash> SCALARS_LINEAR;
    static const std::unordered_map<std::pair<int, int>, const double, PairHash> SCALARS_FLOYD_STEINBERG;
    static const std::unordered_map<std::pair<int, int>, const double, PairHash> SCALARS_JARVICE_JUDICE_NINKE;
    static const std::unordered_map<std::pair<int, int>, const double, PairHash> SCALARS_STUCKI;
    static const std::unordered_map<std::pair<int, int>, const double, PairHash> SCALARS_ATKINSON;
    static const std::unordered_map<std::pair<int, int>, const double, PairHash> SCALARS_BURKES;
    static const std::unordered_map<std::pair<int, int>, const double, PairHash> SCALARS_SIERRA;
    static const std::unordered_map<std::pair<int, int>, const double, PairHash> SCALARS_SIERRA_TWO_ROW;
    static const std::unordered_map<std::pair<int, int>, const double, PairHash> SCALARS_SIERRA_LITE;

    std::vector<std::pair<int, int>> coordinates;
    std::unordered_map<std::pair<int, int>, const double, PairHash> scalars;

    Error();
    Error(ErrorDiffusionAlgorithm algorithm);
    std::string to_string();
};

#endif
