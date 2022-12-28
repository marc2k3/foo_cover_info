#pragma once

namespace db
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
	metadb_index_hash get_hash(const char* path);
	metadb_index_manager_v2::ptr theAPI();
	void refresh(const HashList& hashes);
	void reset(metadb_handle_list_cref handles);
	void set(const metadb_index_transaction::ptr& ptr, metadb_index_hash hash, const Fields& f);
}
