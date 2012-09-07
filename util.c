#include "util.h"


int util_connect_card(sc_context_t *ctx, sc_card_t **cardp,
                      const char *reader_id, int do_wait, int verbose)
{
    sc_reader_t *reader, *found;
    sc_card_t *card;
    int r, tmp_reader_num;

    if (do_wait)
    {
        unsigned int event;

        if (sc_ctx_get_reader_count(ctx) == 0)
        {
            fprintf(stderr, "Waiting for a reader to be attached...\n");
            r = sc_wait_for_event(ctx, SC_EVENT_READER_ATTACHED, &found, &event, OPT_TIMEOUT_MS, NULL);
            if (r < 0)
            {
                fprintf(stderr, "Error while waiting for a reader: %s\n", sc_strerror(r));
                return 3;
            }
            r = sc_ctx_detect_readers(ctx);
            if (r < 0)
            {
                fprintf(stderr, "Error while refreshing readers: %s\n", sc_strerror(r));
                return 3;
            }
        }
#ifdef NOISY
        fprintf(stderr, "Waiting for a card to be inserted...\n");
#endif
        r = sc_wait_for_event(ctx, SC_EVENT_CARD_INSERTED, &found, &event, OPT_TIMEOUT_MS, NULL);
        if (r < 0)
        {
            fprintf(stderr, "Error while waiting for a card: %s\n", sc_strerror(r));
            return 3;
        }
        reader = found;
    }
    else
    {
        if (sc_ctx_get_reader_count(ctx) == 0)
        {
            fprintf(stderr,
                    "No smart card readers found.\n");
            return 1;
        }
        if (!reader_id)
        {
            unsigned int i;
            /* Automatically try to skip to a reader with a card if reader not specified */
            for (i = 0; i < sc_ctx_get_reader_count(ctx); i++)
            {
                reader = sc_ctx_get_reader(ctx, i);
                if (sc_detect_card_presence(reader) & SC_READER_CARD_PRESENT)
                {
#ifdef NOISY
                    fprintf(stderr, "Using reader with a card: %s\n", reader->name);
#endif
                    goto autofound;
                }
            }
            /* If no reader had a card, default to the first reader */
            reader = sc_ctx_get_reader(ctx, 0);
        }
        else
        {
            /* If the reader identifiers looks like an ATR, try to find the reader with that card */
            unsigned char atr_buf[SC_MAX_ATR_SIZE * 3];
            size_t atr_buf_len = sizeof(atr_buf);
            unsigned int i;
            if (sc_hex_to_bin(reader_id, atr_buf, &atr_buf_len) == SC_SUCCESS)
            {
                /* Loop readers, looking for a card with ATR */
                for (i = 0; i < sc_ctx_get_reader_count(ctx); i++)
                {
                    reader = sc_ctx_get_reader(ctx, i);
                    if (sc_detect_card_presence(reader) & SC_READER_CARD_PRESENT)
                    {
                        if (!memcmp(reader->atr.value, atr_buf, reader->atr.len))
                        {
                            fprintf(stderr, "Matched ATR in reader: %s\n", reader->name);
                            goto autofound;
                        }
                    }
                }
            }
            if (!sscanf(reader_id, "%d", &tmp_reader_num))
            {
                /* Try to get the reader by name if it does not parse as a number */
                reader = sc_ctx_get_reader_by_name(ctx, reader_id);
            }
            else
            {
                reader = sc_ctx_get_reader(ctx, tmp_reader_num);
            }
        }
autofound:
        if (!reader)
        {
            fprintf(stderr,
                    "Reader \"%s\" not found (%d reader(s) detected)\n", reader_id, sc_ctx_get_reader_count(ctx));
            return 1;
        }

        if (sc_detect_card_presence(reader) <= 0)
        {
            fprintf(stderr, "Card not present.\n");
            return 3;
        }
    }

    if (verbose)
        printf("Connecting to card in reader %s...\n", reader->name);
    if ((r = sc_connect_card(reader, &card)) < 0)
    {
        fprintf(stderr,
                "Failed to connect to card: %s\n",
                sc_strerror(r));
        return 1;
    }

    if (verbose)
        printf("Using card driver %s.\n", card->driver->name);

    if ((r = sc_lock(card)) < 0)
    {
        fprintf(stderr,
                "Failed to lock card: %s\n",
                sc_strerror(r));
        sc_disconnect_card(card);
        return 1;
    }

    *cardp = card;
    return 0;
}

