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
