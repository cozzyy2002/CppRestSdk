#pragma once

#ifdef MAQUETTE_EXPORTS
#define MAQUETTE_API __declspec(dllexport)
#else
#define MAQUETTE_API __declspec(dllexport)
#endif // MAQUETTE_EXPORTS

namespace MQTT {
	class CMaquette {
	public:
		virtual ~CMaquette();

		void connect();
		void disconnect();
		void subscribe();
		void publish();
	};

	MAQUETTE_API CMaquette* createMaquette();

} // namespace MQTT
