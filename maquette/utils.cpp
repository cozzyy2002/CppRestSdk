#include "stdafx.h"
#include "maquette/Utils.h"

using namespace MQTT;

CUtils::CUtils()
{}


CUtils::~CUtils()
{}

std::string CUtils::dump(const data_t& data)
{
	if(data.empty()) return "<empty>";

	static const size_t width = 16;
	typedef struct {
		char pos[9];			// byte position(includes terminating null)
		char hex[width][3];		// hex representation(includes terminating null)
		char space;
		char c[width];			// charactor representation or '.' if unprintable charactor
		char eol;
	} format_t;

	size_t size = data.size();
	size_t height = size / width + ((size % width) ? 1 : 0);
	std::string ret(height * sizeof(format_t), ' ');

	format_t* p = (format_t*)&ret[0];
	const byte* d = data.data();
	for(size_t line = 0; line < height; line++, p++) {
		sprintf_s(p->pos, "%0*x", ARRAYSIZE(p->pos) - 1, d);
		p->pos[ARRAYSIZE(p->pos) - 1] = ' ';
		size_t column;
		for(column = 0; column < width; column++, d++) {
			if(0 == size--) break;
			sprintf_s(p->hex[column], "%0*x", ARRAYSIZE(p->hex[0]) - 1, *d);
			p->hex[column][ARRAYSIZE(p->hex[0]) - 1] = ' ';
			p->c[column] = (isprint(*d) && !iscntrl(*d)) ? (char)*d : '.';
		}
		p->c[column] = (line < height - 1) ? '\n' : '\0';	// Add LF except last line
	}
	return ret;
}
