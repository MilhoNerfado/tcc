#include <stdint.h>

/* take 64 bits of data in v[0] and v[1] and 128 bits of key[0] - key[3] */
void encipher(unsigned int num_rounds, uint32_t v[2], uint32_t const key[4]);

/* take 64 bits of data in v[0] and v[1] and 128 bits of key[0] - key[3] */
void decipher(unsigned int num_rounds, uint32_t v[2], uint32_t const key[4]);
