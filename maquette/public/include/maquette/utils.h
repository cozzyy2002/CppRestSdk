#pragma once

#include "maquette/maquette.h"

namespace MQTT {

	class MAQUETTE_API CUtils {
	public:
		CUtils();
		virtual ~CUtils();

		static std::string dump(const MQTT::data_t& data);
	};

} // namespace MQTT

