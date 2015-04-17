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
		client.request(methods::GET)
			.then([](http_response& resp) {
				std::cout << "Status code=" << resp.status_code() << std::endl;
				std::cout << "Content length=" << resp.headers().content_length() << std::endl;
			})
			.wait();
	} catch(std::exception& ex) {
		std::cerr << ex.what() << std::endl;
	}

	return 0;
}

