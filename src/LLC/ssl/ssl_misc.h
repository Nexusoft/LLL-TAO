/**
 * @file ssl_misc.h
 * @brief SSL 3.0 helper functions
 *
 * @section License
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (C) 2010-2019 Oryx Embedded SARL. All rights reserved.
 *
 * This file is part of CycloneSSL Open.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * @author Oryx Embedded SARL (www.oryx-embedded.com)
 * @version 1.9.6
 **/

#ifndef _SSL_MISC_H
#define _SSL_MISC_H

//Dependencies
#include "core/crypto.h"
#include "tls.h"

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//SSL 3.0 related constants
extern const uint8_t sslPad1[48];
extern const uint8_t sslPad2[48];

//SSL 3.0 related functions
error_t sslExpandKey(const uint8_t *secret, size_t secretLen,
   const uint8_t *random, size_t randomLen, uint8_t *output, size_t outputLen);

error_t sslComputeMac(TlsEncryptionEngine *encryptionEngine,
   const TlsRecord *record, const uint8_t *data, size_t dataLen, uint8_t *mac);

uint32_t sslVerifyPadding(TlsEncryptionEngine *decryptionEngine,
   const uint8_t *data, size_t dataLen, size_t *paddingLen);

uint32_t sslVerifyMac(TlsEncryptionEngine *decryptionEngine,
   const TlsRecord *record, const uint8_t *data, size_t dataLen,
   size_t maxDataLen, const uint8_t *mac);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
