/**
 * encapsulate.c - Encapsule un paquet IP en trame (appelle protocol_encapsulate).
 */

#include "encapsulate.h"
#include "../include/protocol.h"

int encapsulate(const uint8_t *ip_payload, int ip_len, uint8_t *frame_out)
{
    return protocol_encapsulate(ip_payload, ip_len, frame_out);
}
