/*
 * \brief  Testing 'log()' with negative integer
 * \author Emery Hemingway
 * \date   2017-04-26
 *
 */

/*
 * Copyright (C) 2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#include <libc/component.h>
#include <trace/timestamp.h>
#include <timer_session/connection.h>
#include <base/attached_rom_dataspace.h>
#include <base/log.h>

/* Std C++ includes */
#include <vector>
#include <string>
#include <cstdlib>

using namespace Genode;

struct Main : Signal_handler<Main>
{
	Timer::Connection timer;

	std::vector<std::string> msgs;

	void handle_timeout()
	{
		Genode::log(msgs[std::rand() % (msgs.size())].c_str());

		/* Schedule random timeout */
		timer.trigger_once(std::rand() % (1 << 22));
	}

	Main(Genode::Env &env)
	:
		Signal_handler<Main>(env.ep(), *this, &Main::handle_timeout),
		timer(env, "message_timeout")
	{
		Attached_rom_dataspace config_rom(env, "config");
		config_rom.xml().for_each_sub_node("msg", [&] (Xml_node const &node) {
			try {
				Xml_attribute attr = node.attribute("string");
				msgs.emplace_back(attr.value_base(), attr.value_size());
			} catch (...) { }
		});

		timer.sigh(*this);
		handle_timeout();
	}
};


void Libc::Component::construct(Libc::Env &env)
{
	std::srand(Trace::timestamp());

	static Main test(env);
}
