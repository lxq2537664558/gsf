//
// Created by pojol on 2017/2/13.
//

#ifndef _GSF_EVENT_HEADER_
#define _GSF_EVENT_HEADER_

#include "../module/module.h"
#include "event_handler.h"

#include <functional>
#include <tuple>
#include <list>
#include <vector>
#include <unordered_map>

#include <args/args.h>

#include "../../common/single.h"

namespace gsf
{
	typedef std::function<void(gsf::Args, EventHandlerPtr)> EventFunc;
	typedef std::pair<uint32_t, uint32_t> EventPair;

	class Door
	{
	public:
		Door();

		template <typename T>
		uint32_t get_door_id() const { return typeid(T).hash_code(); }

		virtual void listen(EventPair ep, EventFunc func);

		virtual void dispatch(EventPair ep, gsf::Args args, EventHandlerPtr callback = nullptr);

		template <typename T>
		std::pair<uint32_t, uint32_t> make_event(uint32_t event)
		{
			return std::make_pair(typeid(T).hash_code(), event);
		}

		std::pair<uint32_t, uint32_t> make_event(uint32_t type, uint32_t event)
		{
			return std::make_pair(type, event);
		}

    protected:
		uint32_t door_id_;
	};

	// 如果需要监听多个同步事件,辅助类
	/*
	struct AllSuccess
	{
		void listen(gsf::Door *door, std::vector<uint32_t> vec, std::function<void()> func)
		{
			count_ = vec.size();

			for (auto itr = vec.begin(); itr != vec.end(); ++itr)
			{
				door->listen_callback(*itr, [&](gsf::Args args) {
					count_--;

					if (count_ == 0) {
						func();
					}
				});
			}

		}

	private:
		uint32_t count_;
	};

	//AllSuccess as;
	//as.listen(this, {event_id::timer::make_timer_success}, [&](){
	//	std::cout << "success !" << std::endl;
	//});

	struct AnyoneFail
	{
		void listen(gsf::Door * door, std::vector<uint32_t> vec, std::function<void()> func)
		{
			for (auto itr = vec.begin(); itr != vec.end(); ++itr)
			{
				door->listen_callback(*itr, [&](gsf::Args args) {
					func();
					return;
				});
			}
		}
	};
	*/

	class EventModule
			: public gsf::utils::Singleton<EventModule>
			, public Module
	{
		friend class Door;

	public:
		EventModule();

	protected:
		void execute();

		void bind_event(uint32_t type_id, uint32_t event, EventFunc func);

		void add_cmd(uint32_t type_id, uint32_t event, gsf::Args args, EventHandlerPtr callback = nullptr);

		uint32_t make_door_id();

    private:
		typedef std::unordered_map<uint32_t, EventFunc> InnerMap;
		typedef std::unordered_map<uint32_t, InnerMap> TypeMap;

		typedef std::list<std::tuple<uint32_t, uint32_t, gsf::Args, EventHandlerPtr>> CmdList;

		TypeMap type_map_;
		CmdList cmd_list_;

		uint32_t door_id_;
	};
}

#endif