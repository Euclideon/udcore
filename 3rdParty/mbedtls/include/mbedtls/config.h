/**
 * This is a Euclideon file to wrap the two config files we use.
 * Beware when updating mbedTLS!
 */

#if defined(FULL_MBEDTLS)
# include "config-full.h"
#else
# include "config-minimal.h"
#endif