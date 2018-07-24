// gcc -I. -lopensc opensc-random.c -o opensc-random

/*
 * opensc-random.c: Output random data from a smartcard to stdout
 *
 * Usage:
 *
 *      Two modes: 1) Write a specific number of random bytes to stdout:
 *
 *        $ opensc-tools 48 | base64
 *        mhM4rNRLci0QEtAXfRree+htPjHemieUKH9L5qzvmee+JeNtU2KelK6Hi91H1A4s
 *
 *      2) Continuously write to stdout (warning, may be very slow!):
 *
 *         $ opensc-tools | dd of=random.dat bs=1 count=4096
 *         4096+0 records in
 *         4096+0 records out
 *         4096 bytes (4.1 kB) copied, 132.724 s, 0.0 kB/s
 *
 *
 * Copyright (C) 2012 Brian Wood
 * Copyright (C) 2001 Juha Yrjola <juha.yrjola@iki.fi>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "util.h"
#include "libopensc/opensc.h"
#include "libopensc/cardctl.h"

static const char *app_name = "opensc-random";
static sc_context_t *ctx = NULL;
static sc_card_t *card = NULL;
static const char *opt_driver = NULL;
static const char *opt_reader = NULL;
static sc_path_t current_path;
static sc_file_t *current_file = NULL;


int main(int argc, char * const argv[])
{
    int r, err = 0;
    sc_context_param_t ctx_param;
    int lcycle = SC_CARDCTRL_LIFECYCLE_ADMIN;

#ifdef NOISY
    fprintf(stderr, "Using libopensc version %s\n", sc_get_version());
#endif

    memset(&ctx_param, 0, sizeof(ctx_param));
    ctx_param.ver      = 0;
    ctx_param.app_name = app_name;

    r = sc_context_create(&ctx, &ctx_param);
    if (r)
    {
        fprintf(stderr, "Failed to establish context: %s\n", sc_strerror(r));
        return 1;
    }

    if (opt_driver != NULL)
    {
        err = sc_set_card_driver(ctx, opt_driver);
        if (err)
        {
            fprintf(stderr, "Driver '%s' not found!\n", opt_driver);
            return 1;
        }
    }

    err = util_connect_card(ctx, &card, opt_reader, OPT_WAIT, 0);
    if (err) return 1;

    sc_format_path("3F00", &current_path);
    r = sc_select_file(card, &current_path, &current_file);
    if (r)
    {
        fprintf(stderr, "unable to select MF: %s\n", sc_strerror(r));
        return 1;
    }

    r = sc_card_ctl(card, SC_CARDCTL_LIFECYCLE_SET, &lcycle);
    if (r && r != SC_ERROR_NOT_SUPPORTED)
    {
        fprintf(stderr, "unable to change lifecycle: %s\n", sc_strerror(r));
        return 1;
    }

    unsigned int maxLenght = util_find_max_supported_lenght(card, MAX_BLOCK);
    unsigned char buffer[maxLenght];

    if (argc > 1)
    {
        // Write requested number of bytes, in blocks of MAX_BLOCK bytes.
        int bytes = atoi(argv[1]);

#ifdef NOISY
        fprintf(stderr, "Requesting %i bytes\n", bytes);
#endif

        int block = 0, i;
        while (bytes > 0)
        {
            if (bytes > maxLenght)
                block = maxLenght;
            else
                block = bytes;

            r = sc_get_challenge(card, buffer, block);
            if (r < 0)
            {
                fprintf(stderr, "Failed to get random bytes: %s\n", sc_strerror(r));
                return -1;
            }

            for (i=0; i<block; i++)
            {
                if (ferror(stdout)) return 0;
                putchar(buffer[i]);
            }

            bytes -= block;
        }
    }
    else
    {
#ifdef NOISY
        fprintf(stderr, "Continuous Mode");
#endif
        int wfd = fileno(stdout);

        // Write until the pipe breaks
        int nw;
        while (1)
        {
            r = sc_get_challenge(card, buffer, maxLenght);
            if (r < 0)
            {
                fprintf(stderr, "Failed to get random bytes: %s\n", sc_strerror(r));
                return -1;
            }

            if ((nw = write(wfd, buffer, maxLenght)) == 0 || (nw == -1))
                return 0;
        }
    }

    return 0;
}
