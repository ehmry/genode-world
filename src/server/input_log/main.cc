/*
 * \brief  Server that generates Input events from LOG messages
 * \author Emery Hemingway
 * \date   2017-04-26
 */

/*
 * Copyright (C) 2016 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

/* Genode includes */
#include <input/event.h>
#include <input/component.h>
#include <libc/component.h>
#include <os/session_policy.h>
#include <os/static_root.h>
#include <log_session/connection.h>
#include <base/attached_rom_dataspace.h>
#include <root/component.h>
#include <base/session_label.h>
#include <base/heap.h>
#include <base/log.h>

/* C++ regular expressions */
#include <regex>

namespace Input_log {

	using namespace Genode;
	class Session_component;
	class Root_component;
	struct Main;

}


class Input_log::Session_component : public Rpc_object<Log_session>
{
	private:

		Input::Event_queue &_queue;

		struct Regex
		{
			std::basic_regex<char> pattern;
			int code;

			Regex(std::string s, int c) : pattern(s), code(c) { }
		};

		std::vector<Regex> _regexen;

	public:

		Session_component(Session_policy const &policy,
		                  Input::Event_queue &queue)
		: _queue(queue)
		{
			policy.for_each_sub_node([&] (Xml_node const &node) {
				if (node.has_type("search")) {
					unsigned long code;
					Xml_attribute const ra = node.attribute("regex");
					node.attribute("code").value(&code);

					_regexen.emplace_back(
						std::string(ra.value_base(), ra.value_size()),
						code);
				} else {
					warning("ignoring policy node ",node);
				}
			});
		}

		/***************************
		 ** Log session interface **
		 ***************************/

		size_t write(Log_session::String const &msg) override
		{
			using Input::Event;

			size_t n = Genode::strlen(msg.string());

			for(Regex const r : _regexen) {
				if (std::regex_search(msg.string(), r.pattern)) {
					_queue.add(Event(Event::PRESS, r.code, 0, 0, 0, 0), false);
					_queue.add(Event(Event::RELEASE, r.code, 0, 0, 0, 0), true);
				}
   			 }

			return n;
		}
};


class Input_log::Root_component :
	public Genode::Root_component<Input_log::Session_component>
{
	private:

		Env &_env;

		Attached_rom_dataspace _config_rom { _env, "config" };

		Input::Event_queue &_queue;

	protected:

		Input_log::Session_component *_create_session(char const *args) override
		{
			Session_label const label = label_from_args(args);
			try {
				Session_policy const policy(label, _config_rom.xml());
				return new (md_alloc())
					Session_component(policy, _queue);
			} catch (Session_policy::No_policy_defined) {
				throw Root::Unavailable();
			}
		}

	public:

		Root_component(Env &env, Allocator &alloc, Input::Event_queue &queue)
		:
			Genode::Root_component<Input_log::Session_component>(env.ep(), alloc),
			_env(env), _queue(queue)
		{
			queue.enabled(true);
		}
};


struct Input_log::Main
{
	Genode::Env &env;

	Input::Session_component input { env, env.ram() };

	Static_root<Input::Session> input_root { env.ep().manage(input) };

	Genode::Sliced_heap sliced_heap { env.ram(), env.rm() };

	Input_log::Root_component log_root {
		env, sliced_heap, input.event_queue() };

	Main(Genode::Env &env) : env(env)
	{
		env.parent().announce(env.ep().manage(input_root));
		env.parent().announce(env.ep().manage(log_root));
	}
};


void Libc::Component::construct(Libc::Env &env)
{
	static Input_log::Main inst(env);
}
