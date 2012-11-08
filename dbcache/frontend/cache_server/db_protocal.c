#include "global_table.h"#include "common_define.h"#include "db_protocal.h"#include "allocator.h"static allocator_t wpacket_allocator = NULL;void init_protocal_processer(){	if(wpacket_allocator == NULL)		wpacket_allocator = (allocator_t)create_block_obj_allocator(SINGLE_THREAD,sizeof(struct wpacket));}void basetype_write2_wpk(basetype_t b,wpacket_t wpk){	wpacket_write_uint8(wpk,b->type);	switch(b->type)	{		case DB_INT8:			wpacket_write_uint8(wpk,basetype_get_int8(b));			break;		case DB_INT16:			wpacket_write_uint16(wpk,basetype_get_int16(b));			break;		case DB_INT32:			wpacket_write_uint32(wpk,basetype_get_int32(b));			break;		case DB_INT64:			wpacket_write_uint64(wpk,basetype_get_int64(b));			break;				};}basetype_t create_basetype_by_rpacket(rpacket_t r){	basetype_t ret;	uint8_t type = rpacket_read_uint8(r);	switch(type)	{		case DB_INT8:			ret = basetype_create_int8(rpacket_read_uint8(r));			break;		case DB_INT16:			ret = basetype_create_int16(rpacket_read_uint16(r));			break;		case DB_INT32:			ret = basetype_create_int32(rpacket_read_uint32(r));			break;		case DB_INT64:			ret = basetype_create_int64(rpacket_read_uint64(r));			break;				}	return ret;}void update_value_by_rpacket(basetype_t b,rpacket_t r){	switch(b->type)	{		case DB_INT8:			basetype_set_int8(b,rpacket_read_uint8(r));			break;		case DB_INT16:			basetype_set_int16(b,rpacket_read_uint16(r));			break;		case DB_INT32:			basetype_set_int32(b,rpacket_read_uint32(r));			break;		case DB_INT64:			basetype_set_int64(b,rpacket_read_uint64(r));			break;				}}wpacket_t execute_get(global_table_t gtb,rpacket_t r,uint32_t coro_id){	wpacket_t wpk = wpacket_create(MUTIL_THREAD,wpacket_allocator,64,0);	wpacket_write_uint32(wpk,coro_id);	const char *key = rpacket_read_string(r);	if(NULL == key)	{		printf("execute_get empty key\n");		wpacket_write_uint8(wpk,(uint8_t)-1);		return wpk;	}	basetype_t a = global_table_find(gtb,key,global_hash(key));	if(NULL == a)	{		printf("execute_get find error\n");		wpacket_write_uint8(wpk,(uint8_t)-1);		return wpk;	}	wpacket_write_uint8(wpk,(uint8_t)0);	basetype_write2_wpk(a,wpk);	basetype_release(&a);		return wpk;}wpacket_t execute_set(global_table_t gtb,rpacket_t r,uint32_t coro_id){	wpacket_t wpk = wpacket_create(MUTIL_THREAD,wpacket_allocator,64,0);	wpacket_write_uint32(wpk,coro_id);	const char *key = rpacket_read_string(r);	if(NULL == key)	{		wpacket_write_uint8(wpk,(uint8_t)-1);		return wpk;	}			basetype_t a = global_table_find(gtb,key,global_hash(key));	if(NULL != a)	{			update_value_by_rpacket(a,r);	}	else	{		a = create_basetype_by_rpacket(r);		if(NULL == a)		{			wpacket_write_uint8(wpk,(uint8_t)-1);			return wpk;		}		if(NULL == global_table_insert(gtb,key,a,global_hash(key)))		{			basetype_release(&a);			wpacket_write_uint8(wpk,(uint8_t)-1);			return wpk;		}	}	wpacket_write_uint8(wpk,(uint8_t)0);	basetype_release(&a);	return wpk;}wpacket_t execute_del(global_table_t gtb,rpacket_t r,uint32_t coro_id){	wpacket_t wpk = wpacket_create(MUTIL_THREAD,wpacket_allocator,64,0);	wpacket_write_uint32(wpk,coro_id);	const char *key = rpacket_read_string(r);	if(NULL == key)	{		printf("execute_del empty key\n");		wpacket_write_uint8(wpk,(uint8_t)-1);		return wpk;	}	basetype_t a = global_table_remove(gtb,key,global_hash(key));	if(NULL == a)	{		wpacket_write_uint8(wpk,(uint8_t)-1);		return wpk;	}	wpacket_write_uint8(wpk,(uint8_t)0);	basetype_release(&a);	printf("del:%s\n",key);	return wpk;}