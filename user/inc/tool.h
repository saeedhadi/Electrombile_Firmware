/*
 * tool.h
 *
 *  Created on: 2015��10��16��
 *      Author: jk
 */

#ifndef ELECTROMBILE_FIRMWARE_TOOL_H
#define ELECTROMBILE_FIRMWARE_TOOL_H

//TODO: this file will replaced by modem.h and utils.h

eat_bool tool_modem_write(const unsigned char *data);
unsigned char *tool_StrstrAndReturnEndPoint(char *str1, const char *str2);


#endif /* ELECTROMBILE_FIRMWARE_TOOL_H */





