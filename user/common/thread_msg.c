//
// Created by jk on 2015/7/1.
//

#include "thread_msg.h"

MSG_THREAD* allocMsg(u8 len)
{
    return eat_mem_alloc(len);
}

void freeMsg(MSG_THREAD* msg)
{
    eat_mem_free(msg);
}

eat_bool sendMsg(EatUser_enum peer, void* msg, u8 len)
{
    return eat_send_msg_to_user(eat_get_task_id(), peer, EAT_TRUE, len, EAT_NULL, &msg);
}

