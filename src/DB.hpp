#pragma once

namespace cinfo
{
	struct Fields
	{
		uint32_t front_cover_width{};
		uint32_t front_cover_height{};
		uint32_t front_cover_bytes{};
		pfc::string8 front_cover_format;
	};

	using HashList = pfc::list_t<metadb_index_hash>;
	using HashSet = std::set<metadb_index_hash>;

	Fields get(metadb_index_hash hash);
	bool hashHandle(const metadb_handle_ptr& handle, metadb_index_hash& hash);
	metadb_index_hash generate_hash(const char* path);
	metadb_index_manager::ptr theAPI();
	void refresh(const HashList& hashes);
	void reset(metadb_handle_list_cref handles);
	void set(const metadb_index_transaction::ptr& ptr, metadb_index_hash hash, Fields f);
}
