#pragma once

template <class num>
void correct_partitions(int npart, num N, num sum, num* results);

template <class num>
void partition_fractions(int npart, float* pFraction, num N, num* results);

template <class num>
void partition_exp(int npart, num N, num* results);

#include "partition.tpp"