#include "event.h"

void gsf::EventModule::execute()
{
	while (!cmd_list_.empty())
	{
		auto itr = cmd_list_.begin();

		auto tItr = type_map_.find(std::get<0>(*itr));
		if (tItr != type_map_.end()) {
			auto iItr = tItr->second.find(std::get<1>(*itr));
			if (iItr != tItr->second.end()) {
				iItr->second(std::get<2>(*itr), std::get<3>(*itr));
			}
		}

		cmd_list_.pop_front();
	}

	while (!remote_callback_list_.empty())
	{
		auto itr = remote_callback_list_.begin();

		auto fItr = remote_map_.find(std::get<0>(*itr));
		if (fItr != remote_map_.end()) {
			fItr->second(std::get<1>(*itr), std::get<2>(*itr));
		}

		remote_callback_list_.pop_front();
	}

	while (!remote_event_list_.empty())
	{
		auto itr = remote_event_list_.begin();

		uint32_t _eid = std::get<0>(*itr);
		auto fitr = remote_event_map_.find(_eid);
		if (fitr != remote_event_map_.end()){
			fitr->second(std::get<1>(*itr), std::get<2>(*itr), std::get<3>(*itr));
		}

		remote_event_list_.pop_front();
	}
}

gsf::EventModule::EventModule()
{

}


void gsf::EventModule::bind_event(uint32_t type_id, uint32_t event, EventFunc func)
{
	auto regf = [&](InnerMap &itr) {
		itr.insert(std::make_pair(event, func));
	};

	auto typeItr = type_map_.find(type_id);
	if (typeItr != type_map_.end()) {

		auto eventItr = typeItr->second.find(event);
		if (eventItr != typeItr->second.end()) {
			printf("repeated event!\n");
			return;
		}
		
		regf(typeItr->second);
	}
	else {
		InnerMap _map;
		regf(_map);

		type_map_.insert(std::make_pair(type_id, _map));
	}
}

void gsf::EventModule::bind_remote_event(uint32_t type_id, RemoteEventFunc func)
{
	auto itr = remote_event_map_.find(type_id);
	if (itr != remote_event_map_.end()){
		return;
	}

	remote_event_map_.insert(std::make_pair(type_id, func));
}

void gsf::EventModule::add_cmd(uint32_t type_id, uint32_t event, gsf::Args args, EventHandlerPtr callback /*= nullptr*/)
{
	if (event == eid::network::bind_remote_callback) {

		uint32_t _module_id = args.pop_uint32(0);	//预留在这里，如果是分布式则需要将这次注册同步到协调Server
		uint32_t _msg_id = args.pop_uint32(1);
		auto _func = args.pop_remote_callback(2);

		remote_map_.insert(std::make_pair(_msg_id, _func));

		return;
	}

	cmd_list_.push_back(std::make_tuple(type_id, event, args, callback));
}

void gsf::EventModule::add_remote_callback(uint32_t msg_id, uint32_t fd, BlockPtr blockptr)
{
	remote_callback_list_.push_back(std::make_tuple(msg_id, fd, blockptr));
}

void gsf::EventModule::add_remote_cmd(uint32_t type_id, std::vector<uint32_t> fd_list, uint32_t msg_id, BlockPtr blockptr)
{
	remote_event_list_.push_back(std::make_tuple(type_id, fd_list, msg_id, blockptr));
}

gsf::IEvent::IEvent()
{
}

void gsf::IEvent::listen(Module *target, uint32_t event, EventFunc func)
{
	EventModule::get_ref().bind_event(target->get_module_id(), event, func);
}

void gsf::IEvent::dispatch(uint32_t target, uint32_t event, gsf::Args args, EventHandlerPtr callback /* = nullptr */)
{
	EventModule::get_ref().add_cmd(target, event, args, callback);
}

void gsf::IEvent::remote_callback(uint32_t fd, uint32_t msg_id, BlockPtr blockptr)
{
	EventModule::get_ref().add_remote_callback(msg_id, fd, blockptr);
}

void gsf::IEvent::listen_remote(Module *target, RemoteEventFunc func)
{
	EventModule::get_ref().bind_remote_event(target->get_module_id(), func);
}

void gsf::IEvent::dispatch_remote(uint32_t target, uint32_t fd, uint32_t msg_id, BlockPtr blockptr)
{
	std::vector<uint32_t> fd_list;
	fd_list.push_back(fd);
	EventModule::get_ref().add_remote_cmd(target, fd_list, msg_id, blockptr);
}
