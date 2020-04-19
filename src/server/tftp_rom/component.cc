/*
 * \brief  TFTP client, ROM server
 * \author Emery Hemingway
 * \date   2016-02-24
 */

/*
 * Copyright (C) 2016-2020 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

/* Genode includes */
#include <world/session_requests.h>
#include <nic/packet_allocator.h>
#include <timer_session/connection.h>
#include <rom_session/rom_session.h>
#include <os/session_policy.h>
#include <os/path.h>
#include <base/attached_rom_dataspace.h>
#include <base/heap.h>
#include <root/component.h>
#include <base/component.h>
#include <util/list.h>
#include <util/string.h>
#include <util/endian.h>

/* LwIP includes */
#include <lwip/nic_netif.h>


namespace Tftp_rom {

	enum
		{ RRQ   = 1
		, WRQ   = 2
		, DATA  = 3
		, ACK   = 4
		, ERROR = 5
		, OACK  = 6
		};

	using namespace Genode;

	struct Transfer;
	struct Session_component;
	typedef Genode::Id_space<Session_component> Session_space;

	struct Main;

	typedef List<Session_component> Session_list;
}


using namespace Lwip;


extern "C" void initial_cb(void*, struct udp_pcb*, struct pbuf*,
                           const ip_addr_t*, u16_t);

extern "C" void data_cb(void*, struct udp_pcb*, struct pbuf*,
                        const ip_addr_t*, u16_t);


struct Tftp_rom::Transfer
{
	Transfer(Transfer const &);
	Transfer &operator = (Transfer const &);

	udp_pcb *_pcb; /* lwIP UDP context  */
	pbuf    *_chain_head { nullptr };

	Constructible<Attached_dataspace> _attached_dataspace { };

	typedef Genode::String<128> Filename;
	Filename const _filename;

	Timer::One_shot_timeout<Session_component> _timeout;

	Duration           _timeout_end;
	Microseconds const _timeout_period;

	Genode::off_t  _rom_offset { 0 };

	ip_addr_t const _addr;
	uint16_t  const _port;
	uint16_t        _block_num { 0 };  // TFTP block number

	enum State { FAILED, INIT, PENDING, COMPLETED };

	State _state { INIT };

	State state() const { return _state; }

	Transfer( Session_component &session
	        , char const        *namestr
	        , ip_addr           &ipaddr
	        , uint16_t           port
	        , Timer::Connection &timer
	        , Microseconds       timeout
	        );

	~Transfer()
	{
		if (_pcb != nullptr)
			udp_remove(_pcb);

		if (_chain_head != nullptr)
			pbuf_free(_chain_head);
	}

	void schedule_timeout()
	{
		Microseconds next { _timeout_period.value / 4 };
		_timeout.schedule(next);
	}

	bool retry(Duration current)
	{
		if (_timeout_end.less_than(current)) {
			error(_filename, " timed out");
			_state = FAILED;
			return false;
		} else {
			switch (_state) {
			case INIT:
				initial_request();
				break;
			case PENDING:
				send_ack(_block_num);
				break;
			default:
				return false;
			}
			schedule_timeout();
			return true;
		}
	}

	void initial_request()
	{
		Genode::size_t filename_len = Genode::strlen(_filename.string());

		pbuf *req = pbuf_alloc(PBUF_TRANSPORT, 2+filename_len+1+6+6+3, PBUF_RAM);

		uint8_t *buf = (uint8_t*)req->payload;

		buf[0] = 0x00;
		buf[1] = RRQ;

		off_t offset = 2;

		Genode::strncpy((char*)buf+offset, _filename.string(), filename_len+1);
		offset += filename_len+1;

		Genode::strncpy((char*)buf+offset, "octet", 6);
		offset += 5+1;

		Genode::strncpy((char*)buf+offset, "tsize", 6);

		udp_sendto(_pcb, req, &_addr, _port);
		schedule_timeout();
	}

	void send_ack(uint16_t number)
	{
		pbuf    *ack = pbuf_alloc(PBUF_TRANSPORT, 4, PBUF_RAM);
		uint8_t *buf = (uint8_t*)ack->payload;

		buf[0] = 0x00;
		buf[1] = ACK;

		buf[2] = number >> 8;
		buf[3] = number;

		udp_send(_pcb, ack);
		schedule_timeout();
	}

	void data_cb(pbuf *data)
	{
		using Genode::size_t;

		uint8_t *buf = (uint8_t*)data->payload;

		/* TFTP packets always start with zero */
		if (buf[0]) {
			send_ack(_block_num);
			return;
		}

		if (buf[1] == ERROR) {
			buf[data->len-1] = '\0';
			Genode::error(_filename.string(), ": ", (const char *)buf+4);
			// permanent error, inform the client
			_state = FAILED;
			return;
		}

		if ((buf[1] != DATA)
		 || (host_to_big_endian(*((uint16_t*)buf+1)) != (_block_num+1)))
		{
			send_ack(_block_num);
			return;
		}

		send_ack(++_block_num);
		_timeout_end.add(_timeout_period);

		pbuf_remove_header(data, 4);

		if (data->len < 512) {
			_state = COMPLETED;
		}

		/* hit the 32MB limit */
		if (_state == PENDING && _block_num == 0) {
			_block_num = 1;
		}

		if (_attached_dataspace->size()-_rom_offset < data->tot_len) {
			_state = FAILED;
		} else {
			uint8_t *rom_ptr = _attached_dataspace->local_addr<uint8_t>();
			u16_t n = pbuf_copy_partial(data, rom_ptr+_rom_offset, data->tot_len, 0);
			_rom_offset += n;
		}
	}

	void initial_cb( Genode::Env &env
	               , Ram_dataspace_capability &ds_cap
	               , void *arg, pbuf *data
	               , const ip_addr_t *addr, u16_t port )
	{
		if (!ip_addr_cmp(addr, &_addr)) {
			return;
		}

		_state = FAILED;

		char *buf = (char*)data->payload;
		size_t   len = data->len;
		if (len < 2+5+1+1+1) return;
		buf[len-1] = 0;

		if (buf[1] == ERROR) {
			Genode::error(_filename.string(), ": ", (const char *)buf+4);
			// permanent error, inform the client
			return;
		}

		if (buf[1] == OACK && Genode::strcmp("tsize", buf+2, 5) == 0) {
			size_t rom_len = 0;
			ascii_to(buf+2+5+1, rom_len);

			try {
				ds_cap = env.ram().alloc(rom_len);
				_attached_dataspace.construct(env.rm(), ds_cap);
				_state = PENDING;

				// swap out the callback
				udp_recv(_pcb, ::data_cb, arg);

				// ignore packets not from this address and port
				udp_connect(_pcb, addr, port);

				send_ack(0);
			} catch (...) {
				error("cannot allocate a ROM dataspace for ", _filename);
			}
		}
	}

};


struct Tftp_rom::Session_component :
	Genode::Rpc_object<Genode::Rom_session>
{
	Genode::Env              &_env;
	Genode::Allocator        &_alloc;
	Transfer                 *_transfer;

	Session_space::Element    _session_elem;
	Ram_dataspace_capability  _dataspace { };

	Session_component(Session_component const &);
	Session_component &operator = (Session_component const &);

	template <typename PROC>
	void with_transfer(PROC const &proc) {
		if (_transfer != nullptr) proc(*_transfer); }

	Session_component( Genode::Env       &env
	                 , Genode::Allocator &alloc
	                 , Session_space     &space
	                 , Session_space::Id  id
	                 , char const        *namestr
	                 , ip_addr           &ipaddr
	                 , uint16_t           port
	                 , Timer::Connection &timer
	                 , Microseconds       timeout
	                 )
	: _env(env)
	, _alloc(alloc)
	, _transfer(new (alloc)
		Transfer(*this, namestr, ipaddr, port,
		timer, timeout))
	, _session_elem(*this, space, id)
	{ }

	~Session_component()
	{
		if (_dataspace.valid())
			_env.ram().free(_dataspace);

		if (_transfer != nullptr)
			destroy(_alloc, _transfer);
	}

	void destruct()
	{
		Parent::Server::Id const id { _session_elem.id().value };
		_env.parent().session_response(id, Parent::SERVICE_DENIED);

		destroy(_alloc, this);
	}

	void handle_timeout(Duration dur)
	{
		with_transfer([&] (Transfer &transfer) {
			if (!transfer.retry(dur)) {
				destruct();
			}
		});
	}

	void data_cb(struct pbuf *data)
	{
		with_transfer([&] (Transfer &transfer) {
			transfer.data_cb(data);

			switch (transfer.state()) {

			case Transfer::INIT:
			case Transfer::PENDING:
				break;

			case Transfer::COMPLETED: {
				destroy(_alloc, &transfer);
				_transfer = nullptr;

				Parent::Server::Id const id { _session_elem.id().value };
				_env.parent().deliver_session_cap(id, _env.ep().manage(*this));
			} break;

			case Transfer::FAILED:
				destruct();
				break;

			}
		});
	}

	void initial_cb(void *arg, struct pbuf *data, const ip_addr_t *addr, u16_t port)
	{
		with_transfer([&] (Transfer &transfer) {
			transfer.initial_cb(_env, _dataspace, arg, data, addr, port);
			switch (transfer.state()) {
			case Transfer::INIT:
			case Transfer::PENDING:
				break;
			case Transfer::COMPLETED:
			case Transfer::FAILED:
				destruct();
				break;
			}
		});
	}

	/***************************
	 ** ROM session interface **
	 ***************************/

	Rom_dataspace_capability dataspace() override {
		return static_cap_cast<Rom_dataspace>(
			(Dataspace_capability)_dataspace); };

	void sigh(Signal_context_capability) override { }

};


Tftp_rom::Transfer::Transfer( Session_component &session
                            , char const        *namestr
                            , ip_addr           &ipaddr
                            , uint16_t           port
                            , Timer::Connection &timer
                            , Microseconds       timeout
                            )
: _pcb(udp_new())
, _filename(namestr)
, _timeout(timer, session, &Session_component::handle_timeout)
, _timeout_end(timer.curr_time())
, _timeout_period(timeout)
, _addr(ipaddr)
, _port(port)
{
	if (_pcb == nullptr) {
		Genode::error("failed to create UDP context");
		throw Genode::Service_denied();
	}

	udp_bind(_pcb, IP_ADDR_ANY, 0);

	/* set callback */
	udp_recv(_pcb, ::initial_cb, &session);

	initial_request();
	_timeout_end.add(_timeout_period);
}


/********************
 ** lwIP callbacks **
 ********************/


extern "C" void initial_cb(void *arg, struct udp_pcb*, struct pbuf *data,
                           const ip_addr_t *addr, u16_t port)
{
	auto session = static_cast<Tftp_rom::Session_component*>(arg);
	session->initial_cb(arg, data, addr, port);
	pbuf_free(data);
}


extern "C" void data_cb(void *arg, udp_pcb*, pbuf *data,
                        ip_addr_t const *, Genode::uint16_t)
{
	auto session = static_cast<Tftp_rom::Session_component*>(arg);
	session->data_cb(data);
	pbuf_free(data);
}


struct Tftp_rom::Main final :
	Genode::Session_request_handler
{
	Genode::Env       &_env;
	Allocator         &_alloc;
	Timer::Connection &_timer;

	Attached_rom_dataspace  _config_rom { _env, "config" };

	struct Dispatcher : Lwip::Nic_netif
	{
		Session_requests_rom _session_requests;

		Dispatcher( Genode::Env &env
		          , Genode::Allocator &alloc
		          , Genode::Xml_node config
		          , Session_request_handler &handler
		          )
		: Lwip::Nic_netif(env, alloc, config)
		, _session_requests(env, handler)
		{ }

		void status_callback() override
		{
			if (Lwip::Nic_netif::ready())
				_session_requests.schedule();
			// process requests when the Nic interface is ready
		}

	} _dispatcher { _env, _alloc, _config_rom.xml(), *this };

	Session_space  _sessions { };

	Main( Genode::Env &env
	    , Allocator &alloc
	    , Timer::Connection &timer
	    )
	: _env(env)
	, _alloc(alloc)
	, _timer(timer)
	{ }

	void handle_session_create(Session_state::Name const &name,
	                           Parent::Server::Id pid,
	                           Session_state::Args const &args) override
	{
		if (name != "ROM") throw Service_denied();

		Session_space::Id  const id { pid.value };
		bool session_created { false };

		try {
			_sessions.apply<Session_component&>(id,
				[&] (Session_component &) { session_created = true; });
		} catch (Id_space<Session_component>::Unknown_id) { }

		if (!session_created) {
			Session_label const label = label_from_args(args.string());
			Session_label const rom_name = label.last_element();

			_config_rom.update();
			Session_policy policy { label, _config_rom.xml() };

			ip_addr  ipaddr;
			unsigned port = 69;
			unsigned timeout = 10;

			try {
				char addr_str[53];
				policy.attribute("ip").value(addr_str, sizeof(addr_str));
				ipaddr_aton(addr_str, &ipaddr);
			} catch (Xml_attribute::Nonexistent_attribute) {
				Genode::error(label.string(), ": 'ip' not specified in policy");
				throw Service_denied();
			}

			policy.attribute_value("port", &port);
			policy.attribute_value("timeout", &timeout);
			Microseconds timeout_us { timeout*1000*1000 };

			enum { PATH_LEN = 1024 };

			String<PATH_LEN> dir { };
			policy.attribute_value("dir", &dir);
			Path<PATH_LEN> path { rom_name.string(), dir.string() };

			new (_alloc)
				Session_component(_env, _alloc, _sessions, id,
				                  path.base(), ipaddr, port,
					              _timer, timeout_us);
		}
	}

	void handle_session_close(Parent::Server::Id pid) override
	{
		Session_space::Id id { pid.value };
		_sessions.apply<Session_component&>(
			id, [&] (Session_component &session)
		{
			_env.ep().dissolve(session);
			destroy(_alloc, &session);
			_env.parent().session_response(pid, Parent::SESSION_CLOSED);
		});
	}

};


void Component::construct(Genode::Env &env)
{
	static Genode::Heap      heap  { env.pd(), env.rm() };
	static Timer::Connection timer { env };

	Lwip::genode_init(heap, timer);
	static Tftp_rom::Main main(env, heap, timer);
}
