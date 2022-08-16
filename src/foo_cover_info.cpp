#include "stdafx.hpp"

namespace cinfo
{
	DECLARE_COMPONENT_VERSION(
		component_name,
		"1.0.0",
		"Copyright (C) 2022 marc2003\n\n"
		"Build: " __TIME__ ", " __DATE__
	);

	class InstallationValidator : public component_installation_validator
	{
	public:
		bool is_installed_correctly() override
		{
			return test_my_name("foo_cover_info.dll") && core_version_info_v2::get()->test_version(2, 0, 0, 0);
		}
	};

	FB2K_SERVICE_FACTORY(InstallationValidator);
}
