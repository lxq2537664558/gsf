﻿#ifndef _GSF_ACCEPTOR_HEADER_
#define _GSF_ACCEPTOR_HEADER_

#include <vector>
#include <memory>
#include <functional>

#include <core/module.h>
#include <core/event.h>

#include <event2/util.h>
#include <event2/listener.h>

namespace gsf
{
	namespace network
	{
		class SessionMgr;
		class MsgBinder;

		class AcceptorModule
			: public gsf::Module
			, public gsf::IEvent
		{
		public:
			AcceptorModule(const std::string &name);
			AcceptorModule();
			virtual ~AcceptorModule();

			void before_init() override;
			void init() override;

			void execute() override;

			void shut() override;
			void after_shut() override;

		private:

			gsf::ArgsPtr make_acceptor(const gsf::ArgsPtr &args);

			void accept_bind(const std::string &ip, int port);
			static void accept_listen_cb(::evconnlistener *listener
				, evutil_socket_t fd
				, sockaddr *sa
				, int socklen
				, void *arg);

			gsf::ArgsPtr send_msg(const gsf::ArgsPtr &args);

		private:

			uint32_t module_id_;

			SessionMgr *session_mgr_;
			MsgBinder *binder_;

			event_base *event_base_ptr_;

			::evconnlistener *accept_listener_;
		};

	}
}


#endif
