#pragma once

class CoverInfo : public threaded_process_callback
{
public:
	CoverInfo(metadb_handle_list_cref handles) : m_handles(handles) {}

	void run(threaded_process_status& status, abort_callback& abort) final
	{
		auto image_api = fb2k::imageLoaderLite::get();
		auto transaction_ptr = db::theAPI()->begin_transaction();

		db::HashList to_refresh;
		db::HashSet hashes;
		const size_t count = m_handles.get_count();
		size_t index{}, files{}, found{};

		for (auto&& handle : m_handles)
		{
			abort.check();

			const pfc::string8 path = handle->get_path();

			status.set_progress(++index, count);
			status.set_item_path(path);

			const auto hash = db::get_hash(path);
			if (!hashes.emplace(hash).second) continue;

			album_art_extractor::ptr ptr;
			if (!album_art_extractor::g_get_interface(ptr, path)) continue;

			db::Fields f;

			try
			{
				album_art_data_ptr data = ptr->open(nullptr, path, abort)->query(album_art_ids::cover_front, abort);

				if (data.is_valid())
				{
					fb2k::imageInfo_t info = image_api->getInfo(data);
					f.front_cover_width = info.width;
					f.front_cover_height = info.height;
					f.front_cover_bytes = static_cast<uint32_t>(data->get_size());
					if (info.formatName) f.front_cover_format = info.formatName;
					found++;
				}
			}
			catch (...) {}

			set(transaction_ptr, hash, f);
			to_refresh += hash;
			files++;
		}

		transaction_ptr->commit();
		db::refresh(to_refresh);

		FB2K_console_print(Component::name, ": ", count, " items scanned.");
		FB2K_console_print(Component::name, ": ", files, " files are capable of containing covers.");
		FB2K_console_print(Component::name, ": ", found, " files have covers.");
	}

private:
	metadb_handle_list m_handles;
};
