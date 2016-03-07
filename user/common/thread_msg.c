//
// Created by jk on 2015/7/1.
//

#include "thread_msg.h"

eat_bool sendMsg(EatUser_enum peer, void* msg, u8 len)
{
    return eat_send_msg_to_user(eat_get_task_id(), peer, EAT_TRUE, len, EAT_NULL, &msg);
}

