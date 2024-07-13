#pragma once

template <class T, unsigned int N, unsigned int S=0>
struct moving_average {
    volatile void add(T value) {
        data[index] = value;
        total += data[index++];
        if (index > (N-1)) {
            index = 0;
        }
        total -= data[index];
    }

    T get() {
        return total / (N >> S);
    }

    const int size = N;
    volatile unsigned int index = 0;
    volatile T total;
    volatile T data[N];
};
