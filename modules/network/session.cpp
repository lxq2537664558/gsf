#include "session.h"
#include "session_mgr.h"
#include "msg_binder.h"

#include <core/args.h>

#include <iostream>

gsf::network::Session::Session(int fd, int module_id, MsgBinder *binder, std::function<void (int)> disconnect_callback)
    : fd_(fd)
	, module_id_(module_id)
	, binder_(binder)
{
	disconnect_callback_ = disconnect_callback;

	in_buf_ = evbuffer_new();
	if (0 != gsf::SESSION_READ_BUFFER_SIZE) {
		evbuffer_expand(in_buf_, gsf::SESSION_READ_BUFFER_SIZE);
	}

	out_buf_ = evbuffer_new();
	if (0 != gsf::SESSION_WRITE_BUFFER_SIZE) {
		evbuffer_expand(out_buf_, gsf::SESSION_WRITE_BUFFER_SIZE);
	}
}

gsf::network::Session::~Session()
{
	if (in_buf_) {
		evbuffer_free(in_buf_);
	}
	
	if (out_buf_) {
		evbuffer_free(out_buf_); 
	}
}

void gsf::network::Session::read_cb(::bufferevent *bev, void *ctx)
{
	Session *_session_ptr = static_cast<Session*>(ctx);

	_session_ptr->read(bev);

	//evbuffer_add_buffer()
}

void gsf::network::Session::err_cb(::bufferevent *bev, short what, void *ctx)
{
	int32_t _result = 0;

	do {
		if (what & BEV_EVENT_EOF) {
			_result = eid::network::err_event_eof;
			break;
		}

		if (what & BEV_EVENT_ERROR) {
			_result = eid::network::err_event_error;
			break;
		}

		if (what & BEV_EVENT_TIMEOUT) {
			_result = eid::network::err_event_timeout;
			break;
		}

		if (what & BEV_EVENT_CONNECTED) {
			Session * _session_ptr = static_cast<Session *>(ctx);
			_session_ptr->new_connect();
			break;
		}

	} while (0);

	if (0 != _result) {
		Session * _session_ptr = static_cast<Session *>(ctx);
		_session_ptr->dis_connect(_result);

		bufferevent_free(bev);
	}
}

int gsf::network::Session::write(uint32_t msg_id, BlockPtr blockptr)
{

	int _ret = evbuffer_add(out_buf_, blockptr->buf_, blockptr->size_);
	evbuffer_write(out_buf_, fd_);

	return 0;
}

void gsf::network::Session::read(::bufferevent *bev)
{
	bufferevent_read_buffer(bev, in_buf_);

	uint32_t _buf_len = evbuffer_get_length(in_buf_);

	uint32_t _msgheadlen = sizeof(MsgHeadLen);

	char * _head = (char*)malloc(_msgheadlen);
	evbuffer_copyout(in_buf_, _head, _msgheadlen);

	uint32_t _msg_size = *reinterpret_cast<uint32_t*>(_head);

	if (_buf_len >= _msg_size) {

		while (_buf_len >= _msg_size)
		{
			auto _block = std::make_shared<Block>(_msg_size);
			evbuffer_remove(in_buf_, _block->buf_, _msg_size);	//! 将完整的包copy进msg_block.

			MsgID _msg_id = _block->get_msg_id();

			auto _func = binder_->get_func(_msg_id);
			if (_func) {
				std::string _str(_block->buf_ + _block->get_head_size(), _block->get_body_size());	//tmp
				_func(gsf::Args(fd_, _msg_id, _str));
			}

			_buf_len = evbuffer_get_length(in_buf_);
			if (_buf_len > _msgheadlen) {
				evbuffer_copyout(in_buf_, _head, _msgheadlen);
				_msg_size = *reinterpret_cast<uint32_t*>(_head);
			}
			else {
				break;
			}
		}

	}
}

void gsf::network::Session::dis_connect(int32_t err)
{
	disconnect_callback_(fd_);

	dispatch(module_id_, eid::network::dis_connect, gsf::Args(fd_, err));
}

void gsf::network::Session::new_connect()
{
	dispatch(module_id_, eid::network::new_connect, gsf::Args(fd_));
}