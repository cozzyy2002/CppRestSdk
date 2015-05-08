// RestApplication.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

using namespace web;
using namespace web::http;
using namespace web::http::client;

int _tmain(int argc, _TCHAR* argv[])
{
	if(argc < 2) {
		std::wcerr << L"Usage: " << argv[0] << L" server_url" << std::endl;
		return 1;
	}

	try {
		http_client client(argv[1]);
		auto result = client.request(methods::GET)
			.then([](http_response& resp) {
				return resp.content_ready();
			});
		http_response& resp = result.get();
		std::wcout
			<< L"status code=" << (int)resp.status_code()
			<< L", content length=" << resp.headers().content_length()
			<< L"\n" << resp.extract_string().get() << std::endl;
	} catch(std::exception& ex) {
		std::cerr << ex.what() << std::endl;
	}

	return 0;
}
