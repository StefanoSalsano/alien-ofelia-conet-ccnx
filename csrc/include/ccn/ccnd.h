/**
 * @file ccn/ccnd.h
 * 
 * Definitions pertaining to the CCNx daemon.
 *
 * Part of the CCNx C Library.
 *
 * Copyright (C) 2008, 2009 Palo Alto Research Center, Inc.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2.1
 * as published by the Free Software Foundation.
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details. You should have received
 * a copy of the GNU Lesser General Public License along with this library;
 * if not, write to the Free Software Foundation, Inc., 51 Franklin Street,
 * Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef CCN_CCND_DEFINED
#define CCN_CCND_DEFINED

#include "../../ccnd/ccnd_private.h"


#define CCN_DEFAULT_LOCAL_SOCKNAME "/tmp/.ccnd.sock"
#define CCN_LOCAL_PORT_ENVNAME "CCN_LOCAL_PORT"

/**
 * ccnx registered port number
 * see http://www.iana.org/assignments/port-numbers
 */
#define CCN_DEFAULT_UNICAST_PORT_NUMBER 9695U
#define CCN_DEFAULT_UNICAST_PORT       "9695"

/**
 * Link adapters sign on by sending this greeting to ccnd.
 * Not for use over the wire.
 */
#define CCN_EMPTY_PDU "CCN\202\000"
#define CCN_EMPTY_PDU_LENGTH 5

//call the static function in ccnd.c process_incoming_content() making it available as if it were not static
void call_process_incoming_content(struct ccnd_handle *h, struct face *face, unsigned char *wire_msg, size_t wire_size);

struct face * record_connection(struct ccnd_handle *h, int fd, 		struct sockaddr *who, socklen_t wholen, 	int setflags);

#endif
