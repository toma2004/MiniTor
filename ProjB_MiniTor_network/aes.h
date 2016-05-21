/*
 * aes.h
 *
 *  Created on: Oct 21, 2014
 *      Author: csci551
 * Re-use code for packet encrypt/decrypt
 */

#ifndef AES_H_
#define AES_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <openssl/aes.h>
#include <limits.h>
#include <assert.h>

#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))

void class_AES_set_encrypt_key(unsigned char *key_text, AES_KEY *enc_key);
void class_AES_set_decrypt_key(unsigned char *key_text, AES_KEY *dec_key);
/*
 * class_AES_encrypt_with_padding:
 * encrypt IN of LEN bytes
 * into a newly malloc'ed buffer
 * that is returned in OUT of OUT_LEN bytes long
 * using ENC_KEY.
 *
 * It is the *caller*'s job to free(out).
 * In and out lengths will always be different because of manditory padding.
 */
void class_AES_encrypt_with_padding(unsigned char *in, int len, unsigned char **out, int *out_len, AES_KEY *enc_key);
/*
 * class_AES_decrypt:
 * decrypt IN of LEN bytes
 * into a newly malloc'ed buffer
 * that is returned in OUT of OUT_LEN bytes long
 * using DEC_KEY.
 *
 * It is the *caller*'s job to free(out).
 * In and out lengths will always be different because of manditory padding.
 */
void class_AES_decrypt_with_padding(unsigned char *in, int len, unsigned char **out, int *out_len, AES_KEY *dec_key);
#endif /* AES_H_ */
