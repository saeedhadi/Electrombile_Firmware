/*
 * utils.h
 *
 *  Created on: 2016年2月25日
 *      Author: jk
 */

#ifndef USER_INC_UTILS_H_
#define USER_INC_UTILS_H_

const unsigned char* string_trimLeft(const unsigned char* string);
void string_trimRight(unsigned char* string);
char *string_bypass(const char *str1, const char *str2);

void ascii2unicode(unsigned short* out, const unsigned char* in);
void unicode2ascii(unsigned char* out, const unsigned short* in);

int miniLZO_compress(const char* src, int src_len, char* dst, int dst_len);
int miniLZO_decompress(const char* src, int src_len, char* dst, int dst_len);

#endif /* USER_INC_UTILS_H_ */
