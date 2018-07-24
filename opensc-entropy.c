/*
 * opensc-entropy.c:
 * A simple wrapper around libopensc for retrieving random data from a smart card HWRNG
 * and feeding it into the kernel random pool using an ioctl. This makes it available
 * to any application using /dev/random.
 *
 * Copyright (C) 2012 Brian Wood
 * Portions Copyright (C) 2010 Steve Oliver <mrsteveman1@gmail.com> (originally named cardrand.c)
 * Portions copyright (C) 2001  Juha Yrjölä <juha.yrjola@iki.fi>
 *
 * This program is free software; you can redistribute it and/or
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

#include <syslog.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/random.h>

#define DEV_RANDOM	"/dev/random"

static const char *app_name = "opensc-entropy";
static sc_context_t *ctx = NULL;
static sc_card_t *card = NULL;
static const char *opt_driver = NULL;
static const char *opt_reader = NULL;
static sc_path_t current_path;
static sc_file_t *current_file = NULL;


int main(int argc, char * const argv[])
{
    // TODO: Must be root.
    // TODO: Handle card removal more gracefully.

    daemon(0,1);

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
    int count = maxLenght;
    int n = maxLenght;
    int n_bits = maxLenght*8;

    struct rand_pool_info *output;
    int fd = open(DEV_RANDOM, O_WRONLY);
    if (fd == -1)
        syslog(LOG_DAEMON,"Failed to open %s", DEV_RANDOM);

    while (1)
    {
        r = sc_get_challenge(card, buffer, count);
        if (r < 0)
        {
            syslog(LOG_DAEMON,"Failed to get random bytes: %s\n", sc_strerror(r));
        }
        //printf("Card returned: %s\n",buffer);

        output = (struct rand_pool_info *)malloc(sizeof(struct rand_pool_info) + n);
        if (!output)
        {
            syslog(LOG_DAEMON,"malloc failure in kernel_rng_add_entropy_no_bitcount_increase(%d)", n);
        }
        output -> entropy_count = n_bits;
        output -> buf_size      = n;
        memcpy(&(output -> buf[0]), buffer, n);

        if (ioctl(fd, RNDADDENTROPY, output) == -1)
        {
            syslog(LOG_DAEMON,"ioctl(RNDADDENTROPY) failed!");
        }
        free(output);
        sleep(1);
    }
    close(fd);


    return 0;
}

