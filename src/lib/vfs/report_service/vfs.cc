/*
 * \brief  Report server plugin
 * \author Emery Hemingway
 * \date   2017-04-05
 */

/*
 * Copyright (C) 2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

/* Genode includes */
#include <vfs/directory_service.h>
#include <vfs/file_io_service.h>
#include <vfs/file_system_factory.h>
#include <vfs/vfs_handle.h>
#include <report_session/report_session.h>
#include <base/attached_ram_dataspace.h>
#include <base/attached_rom_dataspace.h>
#include <base/session_label.h>
#include <base/heap.h>
#include <base/service.h>

namespace Report_service {
	using namespace Vfs;
	struct Handle;
	struct Directory;
	struct Session_file;
	struct Service;
	class File_system;

	using Genode::size_t;

	typedef Genode::List<Handle>           Handles;
	typedef Genode::List<Session_file>     File_list;
	typedef Genode::List<Directory>        Dir_list;
	typedef Genode::Id_space<Session_file> File_space;

	typedef Genode::Path<160> Path;

	Path first_element(Path const &in)
	{
		Path out;
		for (size_t i = 1; in.base()[i]; ++i) {
			if (in.base()[i] == '/') {
				Genode::strncpy(out.base(), in.base(), i+1);
				break;
			}
		}
		return out;
	}

	void strip_first_element(Path &path)
	{
		for (int i = 1; path.base()[i]; ++i) {
			if (path.base()[i] == '/') {
				Genode::strncpy(path.base(), path.base()+i,
				                path.capacity()-i);
				break;
			}
		}
	}
}


struct Report_service::Handle : Vfs::Vfs_handle, Handles::Element
{
	Session_file &file;

	Handle(Session_file      &sf,
	       Directory_service &ds,
		   File_io_service   &fs,
		   Genode::Allocator &alloc,
		   int                flags)
	: Vfs_handle(ds, fs, alloc, flags), file(sf) { }
};


struct Report_service::Session_file :
	Genode::Rpc_object<Report::Session, Session_file>,
	File_space::Element,
	File_list::Element
{
	Vfs::Io_response_handler &io_handler;

	Directory &parent;
	Path name;

	Handles handles;

	Genode::Attached_ram_dataspace ram_ds;

	/* logical report length */
	size_t length = 0;

	/* dataspace reference count */
	int cap_ref = 0;

	Session_file(Genode::Env &env, Vfs::Io_response_handler &ioh,
	             File_space &id_space,
		         File_space::Id id,
	             Directory &parent,
	             char const *name, size_t buffer_size)
	:
		File_space::Element(*this, id_space, id),
		io_handler(ioh), parent(parent), name(name),
		ram_ds(env.ram(), env.rm(), buffer_size)
	{ }

	bool closed() const {
		return cap_ref < 1 && handles.first() == nullptr; }

	/******************************
	 ** Report session interface **
	 ******************************/

	Dataspace_capability dataspace() override {
		return ram_ds.cap(); }

	void submit(size_t len) override
	{
		length = len;
		io_handler.handle_io_response(handles.first() ?
			handles.first()->context : nullptr);
		/* XXX: Call once, or for each handle? */
	}

	void response_sigh(Signal_context_capability) override { }

	/**
	 * Request a response from the recipient of reports
	 *
	 * By calling this method, the client expects that the server will
	 * replace the content of the dataspace with new information.
	 *
	 * \return length of response in bytes
	 */
	size_t obtain_response() override {
		return 0; }
};


struct Report_service::Directory : Dir_list::Element
{
	Dir_list  dirs;
	File_list files;

	Path const name;

	Directory(Path const &name) : name(name) { }

	~Directory()
	{
		if (dirs.first())
			Genode::error("destructing ",name," with child dirs");
		if (files.first())
			Genode::error("destructing ",name," with child files");
	}

	Directory *lookup_dir(Path const &dirname)
	{
		for (Directory *subdir = dirs.first();
		     subdir; subdir = subdir->next())
		{
			if (subdir->name == dirname.base())
				return subdir;
		}
		return nullptr;
	}

	Session_file *lookup_file(Path const &filename)
	{
		for (Session_file *file = files.first();
		     file; file = file->next())
		{
			if (file->name == filename.base())
				return file;
		}
		return nullptr;
	}
};


class Report_service::File_system : public Vfs::File_system
{
	private:

		Genode::Env &env;
		Genode::Allocator &alloc;
		Vfs::Io_response_handler &io_handler;

		Directory root { "/" };

		/**
		 * Clean a directory tree
		 *
		 * Called every time a file is removed,
		 * so at most one directory removal per
		 * level.
		 */
		void clean(Directory &dir)
		{
			for (Directory *subdir = dir.dirs.first();
			     subdir; subdir = subdir->next())
			{
				clean(*subdir); /* recursive function */

				if (!subdir->dirs.first() &&
				    !subdir->files.first())
				{
					dir.dirs.remove(subdir);
					destroy(alloc, subdir);
					break;
				}
			}
		}

		/**
		 * Populate the directory tree and insert a file
		 */
		Directory &mkdir_of(Path const &filepath)
		{
			Path path(filepath);

			Directory *parent = &root;
			while (!path.has_single_element()) {
				Path const dirname = first_element(path);
				Directory *dir = parent->lookup_dir(dirname);
				if (!dir) {
					dir = new (alloc) Directory(dirname);
					parent->dirs.insert(dir);
				}
				parent = dir;
				strip_first_element(path);
			}

			return *parent;
		}

		/**
		 * Lookup a file or return nil
		 */
		Session_file *lookup_file(char const *filepath)
		{
			Path path(filepath);

			Directory *parent = &root;
			while (!path.has_single_element()) {
				Path const dirname = first_element(path);
				parent = parent->lookup_dir(dirname);
				if (!parent)
					return nullptr;
				strip_first_element(path);
			}

			return parent->lookup_file(path);
		}

		/**
		 * Lookup a directory or return nil
		 */
		Directory *lookup_dir(char const *dirpath)
		{
			Path path(dirpath);
			if (path == "/")
				return &root;

			Directory *parent = &root;
			while (!path.has_single_element()) {
				Path const dirname = first_element(path);
				parent = parent->lookup_dir(dirname);
				if (!parent)
					return nullptr;
				strip_first_element(path);
			}
			return parent->lookup_dir(path);
		}

		/**
		 * Apply a file or directory functor
		 */
		template <typename DIR_FUNC, typename FILE_FUNC>
		void apply(char const *pathp,
		           DIR_FUNC  const &dir_func,
		           FILE_FUNC const &file_func)
		{
			Path path(pathp);
			if (path == "/")
				return dir_func(root);

			Directory *parent = &root;
			while (!path.has_single_element()) {
				Path const dirname = first_element(path);
				parent = parent->lookup_dir(dirname);
				if (!parent)
					return;
				strip_first_element(path);
			}
			if (Directory const *d = parent->lookup_dir(path))
				dir_func(*d);
			else
			if (Session_file const *f = parent->lookup_file(path))
				file_func(*f);
		}

		Genode::Sliced_heap sliced_heap { env.ram(), env.rm() };

		File_space file_space;

		void handle_request(Xml_node const &request)
		{
			using namespace Genode;

			if (!request.has_attribute("id"))
				return;

			File_space::Id const id { request.attribute_value("id", 0UL) };

			if (request.has_type("create")) {
				if (request.attribute_value("service", String<8>()) != "Report")
					return;
				if (!request.has_sub_node("args"))
					return;

				typedef Session_state::Args Args;
				Args const args = request.sub_node("args").decoded_content<Args>();

				Session_label const label = label_from_args(args.string());
				Path path = path_from_label<Path>(label.string());
				path.append(".report");
				size_t buffer_size =
				Arg_string::find_arg(args.string(), "buffer_size").aligned_size();

				/* XXX: new reports with the same name shadow older reports*/
				Directory &parent = mkdir_of(path);
				Session_file *file = new (sliced_heap)
					Session_file(env, io_handler, file_space, id,
					             parent, path.last_element(), buffer_size);
				parent.files.insert(file);

				Session_capability cap = env.ep().manage(*file);
				env.parent().deliver_session_cap(Parent::Server::Id{id.value}, cap);
			} else

			if (request.has_type("close")) {
				/*
				 * Sessions may not close until all handles
				 * on the file are closed.
				 */
				file_space.apply<Session_file>(id, [&] (Session_file &file) {
					if (file.closed()) {
						/* if there are no open handles, destroy the session */
						Directory &parent = file.parent;
						parent.files.remove(&file);
						destroy(sliced_heap, &file);
						env.parent().session_response(
							Parent::Server::Id{id.value}, Parent::SESSION_CLOSED);
						clean(root);
					} else {
						Genode::warning("refusing to close client session "
						                "while report file is open");
					}
				});
			}
		}

		Genode::Attached_rom_dataspace requests_rom { env, "vfs_report_service -> session_requests" };

		void handle_requests()
		{
			requests_rom.update();
			requests_rom.xml().for_each_sub_node([&] (Xml_node const &req) {
				handle_request(req); });
		}		

		Genode::Signal_handler<File_system> request_handler {
			env.ep(), *this, &File_system::handle_requests };

	public:

		File_system(Genode::Env       &env,
		            Genode::Allocator &alloc,
		            Genode::Xml_node   config,
		            Vfs::Io_response_handler &ioh)
		:
			env(env), alloc(alloc), io_handler(ioh)
		{
			env.parent().announce("Report");
			requests_rom.sigh(request_handler);
			handle_requests();
		}

		~File_system() { }

		void apply_config(Genode::Xml_node const &node) override { }


		/***********************
		 ** Directory_service **
		 ***********************/

		char const *leaf_path(char const *path) override
		{
			char const *leaf = nullptr;

			auto  dir_func = [&] (Directory    const&) { leaf = path; };
			auto file_func = [&] (Session_file const&) { leaf = path; };

			apply(path, dir_func, file_func);
			if (!leaf)
				Genode::error("no leaf path at ", leaf);
			return leaf;
		}

		Stat_result stat(char const *path, Stat &st) override
		{
			Stat_result err = STAT_ERR_NO_ENTRY;
			st = Stat{};

			auto  dir_func = [&] (Directory const &dir) {
				for (Session_file const *f = dir.files.first();
				     f; f = f->next()) { ++st.size; }
					
				st.mode = STAT_MODE_DIRECTORY;
				st.inode = (Genode::addr_t)&dir;
				st.device = (Genode::addr_t)this;
				err = STAT_OK;
			};

			auto file_func = [&] (Session_file const &file) {
				st.size = file.length;
				st.mode = STAT_MODE_FILE;
				st.inode = file.id().value; /* use the session id */
				st.device = (Genode::addr_t)this;
				err = STAT_OK;
			};

			apply(path, dir_func, file_func);
			return err;
		}

		bool directory(char const *path) override {
			return lookup_dir(path) != nullptr; }

		Open_result open(char const  *path,
		                 unsigned     mode,
		                 Vfs_handle **out_handle,
		                 Allocator   &alloc) override
		{
			if (Session_file *file = lookup_file(path)) {
				Handle *handle = new (alloc)
					Handle(*file, *this, *this, alloc, mode);
				file->handles.insert(handle);
				*out_handle = handle;
				return OPEN_OK;
			}
			return OPEN_ERR_UNACCESSIBLE;
		}

		void close(Vfs_handle *vfs_handle) override
		{
			Handle *handle = static_cast<Handle*>(vfs_handle);
			Session_file &file = handle->file;
			file.handles.remove(handle);
			destroy(vfs_handle->alloc(), vfs_handle);

			/*
			 * process any deferred session close requests when the
			 * application yields to signal handling
			 */
			if (file.closed())
				Genode::Signal_transmitter(request_handler).submit();
		}

		Unlink_result unlink(char const *path) override
		{
			Unlink_result err = UNLINK_ERR_NO_ENTRY;

			auto  dir_func = [&] (Directory const&) {
				err = UNLINK_ERR_NO_PERM; };
			auto file_func = [&] (Session_file const&) {
				err = UNLINK_ERR_NO_PERM; };

			apply(path, dir_func, file_func);
			return err;
		}


		Dataspace_capability dataspace(char const *path) override
		{
			if (Session_file *file = lookup_file(path)) {
				++file->cap_ref;
				return file->dataspace();
			}
			return Dataspace_capability();
		}

		void release(char const *path, Dataspace_capability) override
		{
			if (Session_file *file = lookup_file(path)) {
				--file->cap_ref;
				if (file->closed())
					Genode::Signal_transmitter(request_handler).submit();
			}
		};

		file_size num_dirent(char const *path) override
		{
			file_size dirents = 0;
			if (Directory *dir = lookup_dir(path)) {
				for (Directory const *x = dir->dirs.first();
				     x; x = x->next()) { ++dirents; }
				for (Session_file const *x = dir->files.first();
				     x; x = x->next()) { ++dirents; }
			}
			return dirents;
		}

		Dirent_result dirent(char const *path, file_offset index, Dirent &ent) override
		{
			if (Directory *dir = lookup_dir(path)) {
				file_offset i = 0;
				for (Directory const *subdir = dir->dirs.first();
				     subdir; subdir = subdir->next())
				{
					if (i == index) {
						ent.fileno = (Genode::addr_t)subdir;
						ent.type = DIRENT_TYPE_DIRECTORY;
						Genode::strncpy(ent.name, subdir->name.base()+1,
						                sizeof(ent.name));
						return DIRENT_OK;
					}
					++i;
				}

				for (Session_file const *file = dir->files.first();
				     file; file = file->next())
				{
					if (i == index) {
						/*
						 * Zero inode files may be misinterpreted
						 * as files pending deletion and may be
						 * hidden or ignored by applications.
						 */
						ent.fileno = file->id().value+1;
						ent.type = DIRENT_TYPE_FILE;
						Genode::strncpy(ent.name, file->name.base()+1,
						                sizeof(ent.name));
						return DIRENT_OK;
					}
					++i;
				}

				ent = Dirent{};
				return DIRENT_OK;
			}

			return DIRENT_ERR_INVALID_PATH;
		}

		/********************************
		 ** File I/O service interface **
		 ********************************/

		Write_result write(Vfs_handle *vfs_handle,
		                   char const *src, file_size count,
		                   file_size &out_count) override {
			return WRITE_ERR_INVALID; }

		Read_result read(Vfs_handle *vfs_handle,
		                 char *dst, file_size count,
		                 file_size &out_count) override
		{
			Handle *handle = static_cast<Handle*>(vfs_handle);
			Session_file const &file = handle->file;
			if (handle->seek() < file.length) {
				size_t n = Genode::min(
					file.length - handle->seek(),
					count);
				char const *src =
					file.ram_ds.local_addr<char const>()+handle->seek();
				Genode::memcpy(dst, src, n);
				out_count = n;
			} else {
				out_count = 0;
			}
			return READ_OK;
		}

		bool read_ready(Vfs_handle *vfs_handle) override
		{
			Handle *handle = static_cast<Handle*>(vfs_handle);
			return handle->file.length != 0;
			return true;
		}

		bool notify_read_ready(Vfs_handle *vfs_handle)
		{
			Genode::error("VFS Report server: ",__func__," not implemented");
			return false;
		}

		void register_read_ready_sigh(Vfs_handle *vfs_handle,
		                              Signal_context_capability sigh) override
		{
			Genode::error("VFS Report server: ",__func__," not implemented");
		}

		bool check_unblock(Vfs_handle *vfs_handle,
		                   bool rd, bool wr, bool ex) override
		{
			Genode::error("VFS Report server: ",__func__," not implemented");
			return true;
		}


		/***********************
		 ** File system stubs **
		 ***********************/

		Readlink_result readlink(const char*, char*, Vfs::file_size, Vfs::file_size&) override {
			Genode::error("VFS Report server: ",__func__," not implemented");
			return READLINK_ERR_NO_PERM; }

		Rename_result rename(char const *from, char const *to) override {
			Genode::error("VFS Report server: ",__func__," not implemented");
			return RENAME_ERR_NO_PERM; }

		Mkdir_result mkdir(char const *path, unsigned mode) override {
			Genode::error("VFS Report server: ",__func__," not implemented");
			return MKDIR_ERR_NO_PERM; }

		Symlink_result symlink(const char*, const char*) override {
			Genode::error("VFS Report server: ",__func__," not implemented");
			return SYMLINK_ERR_NO_PERM; }

		Ftruncate_result ftruncate(Vfs_handle *vfs_handle, file_size) override
		{
			/* report ok because libc always executes ftruncate() when opening rw */
			Genode::error("VFS Report server: ",__func__," not implemented");
			return FTRUNCATE_OK;
		}

		char const *type() override { return "report_server"; }
};

extern "C" Vfs::File_system_factory *vfs_file_system_factory(void)
{
	struct Factory : Vfs::File_system_factory
	{
		Report_service::File_system *create(Genode::Env &env,
		                                   Genode::Allocator &alloc,
		                                   Genode::Xml_node config,
		                                   Vfs::Io_response_handler &io_handler) override
		{
			return new (alloc)
				Report_service::File_system(env, alloc, config, io_handler);
		}
	};

	static Factory f;
	return &f;
}
