#include "msg_loop.h"
#include "SysTime.h"
#include "coronet.h"

#define MQ_FLUSH_TICK 10 //冲刷消息队列的时间间隔
#define PEEK_WAIT_TIME 5 //如果消息队列为空，最长的等待时间

msg_loop_t create_msg_loop(on_packet _on_packet,on_new_connection _on_new_connection,
	on_connection_disconnect _on_connection_disconnect,on_send_block _on_send_block)
{
	msg_loop_t m = (msg_loop_t)calloc(1,sizeof(*m));
	m->_on_packet = _on_packet;
	m->_on_new_connection = _on_new_connection;
	m->_on_connection_disconnect = _on_connection_disconnect;
	m->_on_send_block = _on_send_block;
	m->last_sync_tick = GetCurrentMs();
	return m;
}

int32_t _coronet_add_timer(coronet_t coron,struct coronet_timer *_timer);
static inline void dispatch_msg(msg_loop_t m,msg_t _msg)
{
	switch(_msg->type)
	{
		case MSG_RPACKET:
			{
				rpacket_t r = (rpacket_t)_msg;
				m->_on_packet((datasocket_t)r->ptr,r);
				rpacket_destroy(&r);
			}
			break;
		case MSG_NEW_CONNECTION:
			{
				m->_on_new_connection((datasocket_t)_msg->ptr);
				destroy_msg(&_msg);
			}
			break;
		case MSG_DISCONNECTED:
			{
				datasocket_t s = (datasocket_t)_msg->ptr;
				m->_on_connection_disconnect(s,s->close_reason);
				destroy_msg(&_msg);
			}
			break;
		case MSG_SEND_BLOCK:
			{
				datasocket_t s = (datasocket_t)_msg->ptr;
				m->_on_send_block(s);
				destroy_msg(&_msg);			
			}
			break;
		case MSG_USER_TIMER_TIMEOUT:
			{
				struct coronet_timer *_timer = (struct coronet_timer *)_msg->ptr;
				if( _timer->_callback(_timer->ud,GetCurrentMs()) == 1)
					_coronet_add_timer(_timer->coron,_timer);
				else
				{
					//不需要添加了,直接删除
					DestroyWheelItem(&_timer->wheel_item);
					free(_timer);
				}
				destroy_msg(&_msg);
			};
	}
}

void msg_loop_once(msg_loop_t m,netservice_t s,uint32_t ms)
{
	msg_t _msg = NULL;
	uint32_t timeout = GetCurrentMs() + ms;
	uint32_t now_tick;
	do
	{
		_msg = net_peek_msg(s,PEEK_WAIT_TIME);
		if(_msg)
			dispatch_msg(m,_msg);
		now_tick = GetCurrentMs();
		if(now_tick - m->last_sync_tick >= MQ_FLUSH_TICK)
		{
			m->last_sync_tick = now_tick;
			mq_flush();
		}		
	}while(now_tick < timeout);
}

void destroy_msg_loop(msg_loop_t *m)
{
	free(*m);
	*m = NULL;
}
