#pragma once

namespace cinfo
{
	class CoverInfo : public threaded_process_callback
	{
	public:
		CoverInfo(metadb_handle_list_cref handles) : m_handles(handles) {}

		void run(threaded_process_status& status, abort_callback& abort) override
		{
			auto image_api = fb2k::imageLoaderLite::get();
			auto transaction_ptr = metadb_index_manager_v2::get()->begin_transaction();

			const size_t count = m_handles.get_count();
			uint32_t files{}, found{};
			HashList to_refresh;
			HashSet hashes;

			for (const size_t i : std::views::iota(0U, count))
			{
				abort.check();

				const metadb_handle_ptr handle = m_handles[i];
				const pfc::string8 path = handle->get_path();

				status.set_progress(i + 1, count);
				status.set_item_path(path);

				metadb_index_hash hash;
				if (!hashHandle(handle, hash)) continue;
				if (!hashes.emplace(hash).second) continue;

				album_art_extractor::ptr ptr;
				if (!album_art_extractor::g_get_interface(ptr, path)) continue;

				Fields f;

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
			refresh(to_refresh);

			FB2K_console_formatter() << "Cover Info: " << count << " items scanned.";
			FB2K_console_formatter() << "Cover Info: " << files << " files are capable of containing covers.";
			FB2K_console_formatter() << "Cover Info: " << found << " files have covers.";
		}

	private:
		metadb_handle_list m_handles;
	};
}
