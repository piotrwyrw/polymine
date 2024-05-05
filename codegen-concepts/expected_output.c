
#include <inttypes.h>

int32_t __lambda0_add(int32_t a, int32_t b)
{
        return a + b;
}

int32_t __lambda1_sub(int32_t a, int32_t b) {
        return a - b;
}

void __polymine_main()
{
        int32_t (*add)(int32_t, int32_t) = __lambda0_add;
        int32_t (*sub)(int32_t, int32_t) = __lambda1_sub;
        int32_t resultSum = add(3, 2);
        int32_t resultDiff = sub(5, 2);
}

int main(void)
{
        __polymine_main();
        return 0;
}