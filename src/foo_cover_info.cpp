#include "stdafx.hpp"

namespace cinfo
{
	DECLARE_COMPONENT_VERSION(
		component_name,
		"1.0.2",
		"Copyright (C) 2022-2023 marc2003\n\n"
		"Build: " __TIME__ ", " __DATE__
	);

	VALIDATE_COMPONENT_FILENAME("foo_cover_info.dll");
}
