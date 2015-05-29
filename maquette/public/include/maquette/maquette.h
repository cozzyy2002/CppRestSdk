#pragma once

#ifdef MAQUETTE_EXPORTS
#define MAQUETTE_API __declspec(dllexport)
#else
#define MAQUETTE_API __declspec(dllimport)
#endif // MAQUETTE_EXPORTS

namespace MQTT {
	typedef std::vector<byte> data_t;

	class IMaquetteCallback {
	public:
		virtual BOOL postMessage(WPARAM wParam, LPARAM lParam) = 0;
	};

	class CMaquette {
	public:
		virtual ~CMaquette() {};

		virtual void connect(LPCTSTR serverUrl) = 0;
		virtual void disconnect() = 0;
		virtual void subscribe(LPCTSTR topic) = 0;
		virtual void publish(LPCTSTR topic, const data_t& payload) = 0;
	};

} // namespace MQTT

MAQUETTE_API MQTT::CMaquette* createMaquette();
MAQUETTE_API LRESULT maquetteWndProc(HWND hWnd, WPARAM wParam, LPARAM lParam);
