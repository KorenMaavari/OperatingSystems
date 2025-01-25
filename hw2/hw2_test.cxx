#include "hw2_test.h"

int set_weight(int weight) {
    long r = syscall(334, weight);
    return r;
}

int get_weight() {
    long r = syscall(335);
    return r;
}

int get_siblings_sum() {
	long r = syscall(336);
    return r;
}

pid_t get_lightest_divisor_ancestor() {
	long r = syscall(337);
    return r;
}