#ifndef RANDOMH
#define RANDOMH

typedef unsigned Rand_t[4];

#ifdef __cplusplus
extern "C" {
#endif
// Random sampling ............................................................

extern int RanInit(         // Output +ve seed used, either from input or time
    Rand_t      Rand,    // Output generator state
    int         seed);   // Input seed: +ve = value, -ve = time seed

extern int Ranint(          // Output random sample, in [2^31, 2^31)
    Rand_t      Rand);   // Update generator state

extern unsigned Rangrid(    // Output random sample, from [0, bound-1]
    Rand_t      Rand,    // Update generator state
    unsigned    bound);  // Input supremum (0 means 2^32)

extern float Ranfloat(      // Output single-precision sample inside (0.0, 1.0)
    Rand_t      Rand);   // Update generator state

extern double Randouble(    // Output random sample, inside (0.0, 1.0)
    Rand_t      Rand);   // Update generator state

extern double Rangauss(     // Output random sample from normal distribution
    Rand_t      Rand);   // Update generator state

extern double Rancauchy(    // Output random sample from Cauchy distribution
    Rand_t      Rand);   // Update generator state

extern double Rangamma(     // Output random sample from gamma distribution
    Rand_t      Rand,    // Update generator state
    double      c);      // Exponent of gamma distribution

extern unsigned Ranpoiss(   // Output random sample from Poisson distribution
    Rand_t      Rand,    // Update generator state
    double      c);      // Mean of Poisson distribution

extern unsigned Ranbinom(   // Output random sample from binomial distribution
    Rand_t      Rand,    // Update generator state
    unsigned    n,       // Range of binomial
    double      p);      // Mean/Range

extern double Ranbeta(      // Output random sample from beta distribution
    Rand_t      Rand,    // Update generator state
    int         r,       // Number of successes in beta distribution
    int         n);      // Total number in beta distribution

extern unsigned Rangeom(    // Output random sample from truncated geometric
    Rand_t      Rand,    // Update generator state
    double      alpha,   // Defines ratio alpha/(alpha+1) < 1
    unsigned    N);      // Supremum (0 means infinity)

extern void Ranperm(        // Random permutation of {0,1,2,...,n-1}
    Rand_t      Rand,    // Update generator state
    int         n,       // Dimension
    int    *    perm);   // Output permutation

extern void Ran2gauss(
    Rand_t      Rand,    // Update generator state
    double      g1,      // Input x gradient at origin
    double      g2,      // Input y gradient at origin
    double      A11,     // Input xx curvature >= 0
    double      A12,     // Input xy curvature, A12*A12 <= A11*A22
    double      A22,     // Input yy curvature >= 0
    double   *  x,       // Output x sample position in (0,inf)
    double   *  y);      // Output y sample position in (0,inf)

extern double Ran1pos(      // Output sample in (0,inf) from Gaussian
    Rand_t      Rand,    // Update generator state
    double      g,       // Input Gaussian gradient at origin
    double      A);      // Input Gaussian curvature

extern void Ran2pos(        // Generate sample in (0,inf)^2 from Gaussian
    Rand_t      Rand,    // Update generator state
    double      g1,      // Input x gradient at origin
    double      g2,      // Input y gradient at origin
    double      A11,     // Input xx curvature >= 0
    double      A12,     // Input xy curvature, A12*A12 <= A11*A22
    double      A22,     // Input yy curvature >= 0
    double   *  x,       // Output x sample position in (0,inf)
    double   *  y);      // Output y sample position in (0,inf)

extern double Ran1posneg(   // Output sample in (-inf,inf) from bi-Gaussian
    Rand_t      Rand,    // Update generator state
    double      f,       // Input coefficient of x
    double      u,       // Input coefficient of |x|, >= 0
    double      A);      // Input Gaussian curvature

extern void Ran2posneg(     // Output sample in (-inf,inf)^2 from bi-Gaussian^2
    Rand_t      Rand,    // Update generator state
    double      f,       // Input coefficient of x
    double      g,       // Input coefficient of y
    double      u,       // Input coefficient of |x|, >= 0
    double      v,       // Input coefficient of |y|, >= 0
    double      A11,     // Input xx curvature >= 0
    double      A12,     // Input xy curvature, A12*A12 <= A11*A22
    double      A22,     // Input yy curvature >= 0
    double   *  x,       // Output x sample position in (0,inf)
    double   *  y);      // Output x sample position in (0,inf)

extern double RanPos1(      // Output sample in [0,inf) from spike+Gaussian
    Rand_t      Rand,    // Update generator state
    double      s,       // Input spike amplitude
    double      g,       // Input Gaussian gradient at origin
    double      A);      // Input Gaussian curvature

extern double RanPos01(     // Output sample in (0,1) from Gaussian
    Rand_t      Rand,    // Update generator state
    double      g,       // Input Gaussian gradient at origin
    double      A);      // Input Gaussian curvature

extern double Ran1posX(     // Output sample in (0,inf) from x*Gaussian(x)
    Rand_t      Rand,    // Update generator state
    double      g,       // Input Gaussian gradient at origin
    double      A);      // Input Gaussian curvature

// Random-related scalars .....................................................

extern double logerf(       // Output log INT[0,inf] exp(-g*t-A*t*t/2)dt
    double      g,       // Input gradient at origin
    double      A);      // Input curvature at origin

extern double logerf2(      // Output log INT[0,inf] exp(-g.x-x.A.x/2)dxdy
    double      g1,      // Input x gradient at origin
    double      g2,      // Input y gradient at origin
    double      A11,     // Input xx curvature >= 0
    double      A12,     // Input xy curvature, A12*A12 <= A11*A22
    double      A22);    // Input yy curvature >= 0

extern double logfactorial( // Output log( k! )
    unsigned    k);      // Input k

extern double logGamma(     // Output log( Gamma(x) )
    double      x);      // Input x

extern double InvNorm(      // Output inverse normal = # of standard deviations
    double      x);      // Input x = cumulative probability

//.............................................................................
#ifdef __cplusplus
};
#endif

#undef  E_RAN_ARITH
#define E_RAN_ARITH  -299   // Require 32 bits integer precision

#endif
