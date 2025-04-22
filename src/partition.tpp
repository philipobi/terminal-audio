#include <cmath>
#include "utils.h"
#include "partition.h"
#include "stdio.h"

template <class num>
void correct_partitions(int npart, num N, num sum, num* results){
    int i;
    num * pResult;
    while (sum > N) {
        for (i = 0, pResult = results; i < npart && sum > N; i++, pResult++) {
            if (*pResult > 1) {
                --(*pResult);
                --sum;
            }
        }
    }
    while (sum < N) {
        for (i = 0, pResult = results; i < npart && sum < N; i++, pResult++) {
            ++(*pResult);
            ++sum;
        }
    }
}

template <class num>
void partition_fractions(int npart, float* pFraction, num N, num* results) {
    int i;
    num sum = 0, * pResult;
    for (i = 0, pResult = results; i < npart; i++, pResult++, pFraction++) {
        sum += (*pResult = max(1, std::round(*pFraction * N)));
    }
    correct_partitions(npart, N, sum, results);
}

template <class num>
void partition_exp(int npart, num N, num* results) {
    num N_2 = N / 2;
    double fact_2 = 1 / (1 - ldexp((double)1, -npart));
    int k;
    num sum = 0, * pResult;
    for (k = 1, pResult = results; k <= npart; k++) {
        sum += (*pResult++ = max(
            1,
            std::round(N_2 * fact_2 * std::exp((k - npart) * (double)M_LN2))
        ));
    }
    correct_partitions(npart, N, sum, results);
}

