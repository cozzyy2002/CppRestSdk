#pragma once

class CEnumValue {
public:
	template<typename T>
	CEnumValue(T value, T max = T::_Count, T min = (T)0) : m_value(value), m_max(max), m_min(min) {
		_ASSERTE(min <= max);
		_ASSERTE(isValid());
	};

	virtual ~CEnumValue() {};

	template<typename T>
	bool operator==(T value) const { return m_value == value; };

	virtual operator LPCSTR() const { return toString(); };
	virtual LPCSTR toString() const {
		const LPCSTR* names = getValueNames();
		return (names && isValid()) ? names[m_value] : "UNKNOWN";
	};
	virtual operator int() const { return m_value; };

	virtual bool isValid() const { return isValid(m_value); };
	template<typename T>
	bool isValid(T value) const { return (m_min <= value) && (value <= m_max); };

protected:
	virtual const LPCSTR* getValueNames() const { return NULL; };

	int m_value;
	int m_max;
	int m_min;
};

// Macro used by m_valueNames[] array in derived classes
#define _TO_STRING(x) #x
