#include "stdafx.hpp"

namespace db
{
	static constexpr std::array field_names =
	{
		"front_cover_width",
		"front_cover_height",
		"front_cover_size",
		"front_cover_format",
		"front_cover_bytes",
	};

	class MetadbIndexClient : public metadb_index_client
	{
	public:
		metadb_index_hash transform(const file_info&, const playable_location& location) final
		{
			return get_hash(location.get_path());
		}
	};

	static auto g_client = new service_impl_single_t<MetadbIndexClient>;
	static metadb_index_manager_v2::ptr g_cachedAPI;

	class InitStageCallback : public init_stage_callback
	{
	public:
		void on_init_stage(uint32_t stage) final
		{
			if (stage == init_stages::before_config_read)
			{
				g_cachedAPI = metadb_index_manager_v2::get();
				try
				{
					g_cachedAPI->add(g_client, guids::metadb_index, system_time_periods::week * 4);
					g_cachedAPI->dispatch_global_refresh();
				}
				catch (const std::exception& e)
				{
					g_cachedAPI->remove(guids::metadb_index);
					FB2K_console_print(Component::name, " stats: Critical initialisation failure: ", e);
				}
			}
		}
	};

	class InitQuit : public initquit
	{
	public:
		void on_quit() final
		{
			g_cachedAPI.release();
		}
	};

	class MetadbDisplayFieldProvider : public metadb_display_field_provider
	{
	public:
		bool process_field(uint32_t index, metadb_handle* handle, titleformat_text_out* out) final
		{
			const auto hash = get_hash(handle->get_path());
			const Fields f = get(hash);

			switch (index)
			{
			case 0:
				if (f.front_cover_width == 0) return false;
				out->write_int(titleformat_inputtypes::meta, f.front_cover_width);
				return true;
			case 1:
				if (f.front_cover_height == 0) return false;
				out->write_int(titleformat_inputtypes::meta, f.front_cover_height);
				return true;
			case 2:
				if (f.front_cover_bytes == 0) return false;
				out->write(titleformat_inputtypes::meta, pfc::format_file_size_short(f.front_cover_bytes));
				return true;
			case 3:
				if (f.front_cover_format.is_empty()) return false;
				out->write(titleformat_inputtypes::meta, f.front_cover_format);
				return true;
			case 4:
				if (f.front_cover_bytes == 0) return false;
				out->write_int(titleformat_inputtypes::meta, f.front_cover_bytes);
				return true;
			}
			return false;
		}

		uint32_t get_field_count() final
		{
			return static_cast<uint32_t>(field_names.size());
		}

		void get_field_name(uint32_t index, pfc::string_base& out) final
		{
			out = field_names[index];
		}
	};

	class FileOperationCallback : public file_operation_callback
	{
	public:
		void on_files_copied_sorted(t_pathlist from, t_pathlist to) final
		{
			update(from, to, false);
		}

		void on_files_deleted_sorted(t_pathlist) final {}

		void on_files_moved_sorted(t_pathlist from, t_pathlist to) final
		{
			update(from, to, true);
		}

	private:
		void update(t_pathlist from, t_pathlist to, bool clear_old)
		{
			HashList to_refresh;
			const size_t count = from.get_count();
			auto transaction_ptr = theAPI()->begin_transaction();

			for (size_t i = 0; i < count; ++i)
			{
				metadb_index_hash old_hash = get_hash(from[i]);
				metadb_index_hash new_hash = get_hash(to[i]);
				set(transaction_ptr, new_hash, get(old_hash));
				to_refresh += new_hash;

				if (clear_old)
				{
					set(transaction_ptr, old_hash, Fields());
					to_refresh += old_hash;
				}
			}

			transaction_ptr->commit();
			refresh(to_refresh);
		}
	};

	FB2K_SERVICE_FACTORY(FileOperationCallback);
	FB2K_SERVICE_FACTORY(InitStageCallback);
	FB2K_SERVICE_FACTORY(InitQuit);
	FB2K_SERVICE_FACTORY(MetadbDisplayFieldProvider);

	Fields get(metadb_index_hash hash)
	{
		mem_block_container_impl temp;
		theAPI()->get_user_data(guids::metadb_index, hash, temp);
		if (temp.get_size() > 0)
		{
			try
			{
				stream_reader_formatter_simple_ref reader(temp.get_ptr(), temp.get_size());
				Fields f;
				reader >> f.front_cover_width;
				reader >> f.front_cover_height;
				reader >> f.front_cover_bytes;
				reader >> f.front_cover_format;
				return f;
			}
			catch (exception_io_data) {}
		}
		return Fields();
	}

	metadb_index_hash get_hash(const char* path)
	{
		pfc::string8 tmp;
		filesystem::g_get_display_path(path, tmp);
		return hasher_md5::get()->process_single_string(tmp).xorHalve();
	}

	metadb_index_manager_v2::ptr theAPI()
	{
		if (g_cachedAPI.is_valid()) return g_cachedAPI;
		return metadb_index_manager_v2::get();
	}

	void refresh(const HashList& hashes)
	{
		fb2k::inMainThread([hashes]
			{
				theAPI()->dispatch_refresh(guids::metadb_index, hashes);
			});
	}

	void reset(metadb_handle_list_cref handles)
	{
		auto transaction_ptr = theAPI()->begin_transaction();

		HashList to_refresh;
		HashSet hashes;

		for (auto&& handle : handles)
		{
			const auto hash = get_hash(handle->get_path());
			if (hashes.emplace(hash).second)
			{
				set(transaction_ptr, hash, Fields());
				to_refresh += hash;
			}
		}
		transaction_ptr->commit();
		refresh(to_refresh);
	}

	void set(const metadb_index_transaction::ptr& ptr, metadb_index_hash hash, const Fields& f)
	{
		stream_writer_formatter_simple writer;
		writer << f.front_cover_width;
		writer << f.front_cover_height;
		writer << f.front_cover_bytes;
		writer << f.front_cover_format;
		ptr->set_user_data(guids::metadb_index, hash, writer.m_buffer.get_ptr(), writer.m_buffer.get_size());
	}
}
