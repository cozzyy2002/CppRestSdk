#pragma once

#ifdef MAQUETTE_EXPORTS
#define MAQUETTE_API __declspec(dllexport)
#else
#define MAQUETTE_API __declspec(dllimport)
#endif // MAQUETTE_EXPORTS

namespace MQTT {
	typedef enum _QOS {
		QOS_0 = 0,
		QOS_1,
		QOS_2,
	} QOS;

	typedef std::vector<byte> data_t;

	class IMaquetteCallback {
	public:
		virtual BOOL postMessage(WPARAM wParam, LPARAM lParam) = 0;

		virtual void onConnAck(bool accepted) {};
		virtual void onConnectionClosed() {};
		virtual void onSubAck(bool accepted) {};
		virtual void onPublished(LPCTSTR topic, const data_t& payload) {};
	};

	class CMaquette {
	public:
		virtual ~CMaquette() {};

		virtual void connect(LPCTSTR serverUrl, LPCTSTR clientId, DWORD keepAlive = 120) = 0;
		virtual void disconnect() = 0;
		virtual void subscribe(LPCTSTR topic, QOS qos = QOS_0) = 0;
		virtual void publish(LPCTSTR topic, const data_t& payload, QOS qos = QOS_0, bool retain = false) = 0;
		virtual LRESULT onUserEvent(WPARAM wParam, LPARAM lParam) = 0;
	};

} // namespace MQTT

MAQUETTE_API MQTT::CMaquette* createMaquette(MQTT::IMaquetteCallback* callback);
