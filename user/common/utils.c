/*
 * utils.c
 *
 *  Created on: 2016年2月25日
 *      Author: jk
 */

#include "utils.h"

/*
 * 去掉字符串开头的空格
 *
 */
const unsigned char* string_trimLeft(const unsigned char* string)
{
    const unsigned char* p = string;

    while(*p == ' ') p++;

    return p;
}



//equivalent to eat_acsii_to_ucs2
void ascii_2_unicode(unsigned short* out, const unsigned char* in)
{
    int i = 0;
    unsigned char* outp = (unsigned char*)out;
    const unsigned char* inp = in;

    while( inp[i] )
    {
        outp[i*2] = inp[i];
        outp[i*2+1] = 0x00;
        i++;
    }

}
