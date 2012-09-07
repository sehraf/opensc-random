#ifndef UTIL_H_INCLUDED
#define UTIL_H_INCLUDED

#include "libopensc/opensc.h"
#include "libopensc/cardctl.h"

#include <unistd.h>
#include <string.h>
#include <stdlib.h>


//#define NOISY				    // Define if you want all the normal output you would see from opensc-explorer

#define MAX_BLOCK 128           // Maximum block size to request from the smartcard
#define OPT_WAIT 1              // Change to zero if you would rather NOT wait for card insertion
#define OPT_TIMEOUT_MS 30000    // Wait up to 30 seconds for the card/reader


int util_connect_card(sc_context_t *ctx, sc_card_t **cardp,
                      const char *reader_id, int do_wait, int verbose);


#endif // UTIL_H_INCLUDED
