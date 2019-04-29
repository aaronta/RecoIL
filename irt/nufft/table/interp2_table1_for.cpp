// interp2_table1_for.c
// 2D periodic interpolation using table lookup.
//
// forward direction: (for m = 1,...,M)
// f(t_m) = \double_sum_{k1,2=0}^{K1,2-1} c[k1,k2]
//	h_1( (t_m1 - k1) mod K1 ) h_2( (t_m2 - k2) mod K2 )
//
// adjoint direction: (for k1,2=0,...,K1,2-1) (note complex conjugate!)
// c[k1,k2] = \sum_{m=1}^M f(t_m)
//	h_1^*( (t_m1 - k1) mod K1 ) h_2^*( (t_m2 - k2) mod K2 )
//
// Interpolators h_1,2 are nonzero (and tabulated) for -J/2 <= t <= J/2.
//
// Copyright 03-30-2004 Yingying Zhang and Jeff Fessler, University of Michigan

#include <math.h>
#include <stdlib.h>
#include <omp.h>
#include "def,table2.h"


/*
* interp2_table0_complex_per()
* 2D, 0th order, complex, periodic
*/
void interp2_table0_complex_per(
const double *r_ck,	/* [K1,K2] in */
const double *i_ck,
const int K1,
const int K2,
const double *r_h1,	/* [J1*L1+1,1] in */
const double *i_h1,
const double *r_h2,	/* [J2*L2+1,1] in */
const double *i_h2,
const int J1,
const int J2,
const int L1,
const int L2,
const double *p_tm,	/* [M,2] in */
const int M,
double *r_fm,		/* [M,1] out */
double *i_fm)
{
	// Let's declare variables once and for all -- AMC
	int mm, jj1, jj2, n2, k2mod, k12mod, k1, n1, k1mod, kk, koff1, k2;
	double p2, coef2r, coef2i, sum1r, sum1i, sum2i, sum2r, p1, coef1r, coef1i, t1, t2;
	double *r_fm_private, *i_fm_private;
	/* trick: shift table pointer to center */
	{
	const int ncenter1 = floor(J1 * L1/2.);
	r_h1 += ncenter1;
	i_h1 += ncenter1;
	}
	{
	const int ncenter2 = floor(J2 * L2/2.);
	r_h2 += ncenter2;
	i_h2 += ncenter2;
	}

	(void) memset((void *) r_fm, 0, M*sizeof(*r_fm));
	(void) memset((void *) i_fm, 0, M*sizeof(*i_fm));

#pragma omp parallel default(none) shared(p_tm, r_fm, i_fm, r_h1, i_h1,\
	r_ck, i_ck, r_h2, i_h2) private(r_fm_private, i_fm_private, \
		t1, t2, mm, koff1, jj1, jj2, k1, k2, kk, p1, p2, n1, n2, coef2r, coef2i, \
		 coef1r, coef1i, k1mod, k2mod, k12mod, sum1r, sum2r, sum1i, sum2i)
	{
		omp_set_num_threads(omp_get_max_threads());
		r_fm_private = (double *)malloc(M*sizeof(*r_fm_private));
		i_fm_private = (double *)malloc(M*sizeof(*i_fm_private));

		/* interp */
		#pragma omp for schedule(dynamic)
  	for (mm=0; mm < M; mm++) {
			/* Cleaning up crazy pointer arithmatic (Do. Not. Do. This.) -- AMC
			t2 = p_tm[M];
			t1 = *p_tm++;
			*/

			t2 = p_tm[M+mm];
			t1 = p_tm[mm];

			sum2r = 0.0;
			sum2i = 0.0;
			koff1 = 1 + floor(t1 - J1 / 2.);
			k2 = 1 + floor(t2 - J2 / 2.);

			for (jj2=0; jj2 < J2; jj2++, k2++) {
				p2 = (t2 - k2) * L2;
				n2 = /* ncenter2 + */ iround(p2);
				coef2r = r_h2[n2];
				coef2i = i_h2[n2];
				k2mod = mymod(k2, K2);
				k12mod = k2mod * K1;

				sum1r = 0.0;
				sum1i = 0.0;
				k1 = koff1;

				for (jj1=0; jj1 < J1; jj1++, k1++) {
					p1 = (t1 - k1) * L1;
					n1 = /* ncenter1 + */ iround(p1);
					coef1r = r_h1[n1];
					coef1i = i_h1[n1];
					k1mod = mymod(k1, K1);
					kk = k12mod + k1mod; /* 2D array index */

					/* sum1 += coef1 * ck */
					sum1r += coef1r * r_ck[kk] - coef1i * i_ck[kk];
					sum1i += coef1r * i_ck[kk] + coef1i * r_ck[kk];
				} /* j1 */

				/* sum2 += coef2 * sum1 */
				sum2r += coef2r * sum1r - coef2i * sum1i;
				sum2i += coef2r * sum1i + coef2i * sum1r;
			} /* j2 */
			/* Cleaning up crazy pointer arithmatic. (Do. Not. Do. This.) -- AMC
			*r_fm++ = sum2r;
			*i_fm++ = sum2i;
			*/
			r_fm_private[mm] = sum2r;
			i_fm_private[mm] = sum2i;
  	}
		#pragma omp critical
		{
			for (mm = 0; mm < M; mm++) {
				r_fm[mm] += r_fm_private[mm];
				i_fm[mm] += i_fm_private[mm];
			}
		}
		free(r_fm_private);
		free(i_fm_private);
	}
}


/*
* interp2_table0_real_per()
* 2D, 0th-order, real, periodic
*/
void interp2_table0_real_per(
const double *r_ck,	/* [K1,K2] in */
const double *i_ck,
const int K1,
const int K2,
const double *r_h1,	/* [J1*L1+1,1] in */
const double *r_h2,	/* [J2*L2+1,1] in */
#ifdef Provide_flip
const int flip1,	/* sign flips every K? */
const int flip2,
#endif
const int J1,
const int J2,
const int L1,
const int L2,
const double *p_tm,	/* [M,2] in */
const int M,
double *r_fm,		/* [M,1] out */
double *i_fm)
{
	int mm;

	/* trick: shift table pointer to center */
	{
	const int ncenter1 = floor(J1 * L1/2.);
	r_h1 += ncenter1;
	}
	{
	const int ncenter2 = floor(J2 * L2/2.);
	r_h2 += ncenter2;
	}

	/* interp */
    for (mm=0; mm < M; mm++) {
	int jj1, jj2;
	const double t2 = p_tm[M];
	const double t1 = *p_tm++;
	double sum2r = 0;
	double sum2i = 0;
	const int koff1 = 1 + floor(t1 - J1 / 2.);
	int k2 = 1 + floor(t2 - J2 / 2.);

	for (jj2=0; jj2 < J2; jj2++, k2++) {
		const double p2 = (t2 - k2) * L2;
		const int n2 = /* ncenter2 + */ iround(p2);
		double coef2r = r_h2[n2];
		const int wrap2 = floor(k2 / (double) K2);
		const int k2mod = k2 - K2 * wrap2;
		const int k12mod = k2mod * K1;

		register double sum1r = 0;
		register double sum1i = 0;
		int k1 = koff1;

#ifdef Provide_flip
		if (flip2 && (wrap2 % 2))
			coef2r = -coef2r; /* trick: sign flip */
#endif

	for (jj1=0; jj1 < J1; jj1++, k1++) {
		const double p1 = (t1 - k1) * L1;
		const int n1 = /* ncenter1 + */ iround(p1);
		register double coef1r = r_h1[n1];
		const int wrap1 = floor(k1 / (double) K1);
		const int k1mod = k1 - K1 * wrap1;
		const int kk = k12mod + k1mod; /* 2D array index */

#ifdef Provide_flip
		if (flip1 && (wrap1 % 2))
			coef1r = -coef1r; /* trick: sign flip */
#endif

		/* sum1 += coef1 * ck */
		sum1r += coef1r * r_ck[kk];
		sum1i += coef1r * i_ck[kk];
	} /* j1 */

		/* sum2 += coef2 * sum1 */
		sum2r += coef2r * sum1r;
		sum2i += coef2r * sum1i;
	} /* j2 */

	*r_fm++ = sum2r;
	*i_fm++ = sum2i;
    }
}


/*
* interp2_table1_real_per()
* 2D, 1st-order, real, periodic
*/
void interp2_table1_real_per(
const double *r_ck,	/* [K1,K2] in */
const double *i_ck,
const int K1,
const int K2,
const double *r_h1,	/* [J1*L1+1,1] in */
const double *r_h2,	/* [J2*L2+1,1] in */
#ifdef Provide_flip
const int flip1,	/* sign flips every K? */
const int flip2,
#endif
const int J1,
const int J2,
const int L1,
const int L2,
const double *p_tm,	/* [M,2] in */
const int M,
double *r_fm,		/* [M,1] out */
double *i_fm)
{
	int mm;

	/* trick: shift table pointer to center */
	{
	const int ncenter1 = floor(J1 * L1/2.);
	r_h1 += ncenter1;
	}
	{
	const int ncenter2 = floor(J2 * L2/2.);
	r_h2 += ncenter2;
	}

	/* interp */
    for (mm=0; mm < M; mm++) {
	int jj1, jj2;
	const double t2 = p_tm[M];
	const double t1 = *p_tm++;
	double sum2r = 0;
	double sum2i = 0;
	const int koff1 = 1 + floor(t1 - J1 / 2.);
	int k2 = 1 + floor(t2 - J2 / 2.);

	for (jj2=0; jj2 < J2; jj2++, k2++) {
		const double p2 = (t2 - k2) * L2;
		const int n2 = floor(p2);
		const double alf2 = p2 - n2;
		register const double *ph2 = r_h2 + n2;
		double coef2r = (1 - alf2) * *ph2 + alf2 * *(ph2+1);
		const int wrap2 = floor(k2 / (double) K2);
		const int k2mod = k2 - K2 * wrap2;
		const int k12mod = k2mod * K1;

		register double sum1r = 0;
		register double sum1i = 0;
		int k1 = koff1;

#ifdef Provide_flip
		if (flip2 && (wrap2 % 2))
			coef2r = -coef2r; /* trick: sign flip */
#endif

	for (jj1=0; jj1 < J1; jj1++, k1++) {
		const double p1 = (t1 - k1) * L1;
		const int n1 = floor(p1);
		const double alf1 = p1 - n1;
		register const double *ph1 = r_h1 + n1;
		register double coef1r = (1 - alf1) * *ph1 + alf1 * *(ph1+1);
		const int wrap1 = floor(k1 / (double) K1);
		const int k1mod = k1 - K1 * wrap1;
		const int kk = k12mod + k1mod; /* 2D array index */

#ifdef Provide_flip
		if (flip1 && (wrap1 % 2))
			coef1r = -coef1r; /* trick: sign flip */
#endif

		/* sum1 += coef1 * ck */
		sum1r += coef1r * r_ck[kk];
		sum1i += coef1r * i_ck[kk];
	} /* j1 */

		/* sum2 += coef2 * sum1 */
		sum2r += coef2r * sum1r;
		sum2i += coef2r * sum1i;
	} /* j2 */

	*r_fm++ = sum2r;
	*i_fm++ = sum2i;
    }
}
