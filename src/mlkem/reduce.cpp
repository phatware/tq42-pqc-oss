#include "reduce.h"
#include "params.h"
#include <stdint.h>

/*************************************************
 * Name:        montgomery_reduce
 *
 * Description: Montgomery reduction; given a 32-bit integer a, computes
 *              16-bit integer congruent to a * R^-1 mod q,
 *              where R=2^16
 *
 * Arguments:   - int32_t a: input integer to be reduced;
 *                           has to be in {-q2^15,...,q2^15-1}
 *
 * Returns:     integer in {-q+1,...,q-1} congruent to a * R^-1 modulo q.
 **************************************************/
int16_t montgomery_reduce(int32_t a)
{
    int16_t u = static_cast<int16_t>(a * QINV);
    int32_t t = (int32_t)u * ML_Q;
    t = a - t;
    t >>= 16;
    return static_cast<int16_t>(t);
}

/*************************************************
 * Name:        barrett_reduce
 *
 * Description: Barrett reduction; given a 16-bit integer a, computes
 *              16-bit integer congruent to a mod q in {0,...,q}
 *
 * Arguments:   - int16_t a: input integer to be reduced
 *
 * Returns:     integer in {0,...,q} congruent to a modulo q.
 **************************************************/
int16_t barrett_reduce(int16_t a)
{
    int16_t t;
    const int16_t v = ((1U << 26) + ML_Q / 2) / ML_Q;

    t = (int32_t)v * a >> 26;
    t *= ML_Q;
    return a - t;
}

/*************************************************
 * Name:        csubq
 *
 * Description: Conditionallly subtract q
 *
 * Arguments:   - int16_t x: input integer
 *
 * Returns:     a - q if a >= q, else a
 **************************************************/
int16_t csubq(int16_t a)
{
    a -= ML_Q;
    a += (a >> 15) & ML_Q;
    return a;
}
