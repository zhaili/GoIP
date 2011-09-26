// GoIP.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <iostream>

#include "Poco/Net/HTTPClientSession.h"
#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPResponse.h"
#include "Poco/StreamCopier.h"
#include "Poco/InflatingStream.h"
#include "Poco/Path.h"
#include "Poco/URI.h"
#include "Poco/Thread.h"
#include "Poco/Exception.h"
#include "Poco/UnicodeConverter.h"

using std::string;

using Poco::Net::HTTPClientSession;
using Poco::Net::HTTPRequest;
using Poco::Net::HTTPResponse;
using Poco::Net::HTTPMessage;
using Poco::Net::SocketAddress;
using Poco::Net::StreamSocket;
using Poco::StreamCopier;
using Poco::Path;
using Poco::URI;
using Poco::FastMutex;

string getPageContent(string addr, string host, string path)
{
	std::string content;
	try
	{
		//URI uri(addr);

		//std::string path(uri.getPathAndQuery());
		//if (path.empty()) path = "/";

		HTTPClientSession session(addr, 80);

#ifdef _DEBUG
		session.setProxy("127.0.0.1",8888);
#endif
		HTTPRequest req(HTTPRequest::HTTP_GET, path);
		req.setHost(host);
		//req.add("User-Agent", "Mozilla/5.0 (Windows NT 5.1; rv:2.0) Gecko/20100101 Firefox/4.0");

		session.sendRequest(req);

		HTTPResponse res;
		std::istream& rs = session.receiveResponse(res);

		StreamCopier::copyToString(rs, content);
	}
	catch (Poco::Exception& exc)
	{
		//ATLTRACE("%s\n",exc.displayText().c_str());
	}

	return content;
}


int _tmain(int argc, _TCHAR* argv[])
{
	if (argc == 1) {
		std::cout << "Usage: GoIP ip" << std::endl;
		return 1;
	}

	string ip;
	Poco::UnicodeConverter::toUTF8(argv[1], ip);

	//string cont = getPageContent("78.140.176.182", "mitbbsfetch.appspot.com", "/");
	//string cont = getPageContent("203.208.46.29", "mitbbsfetch.appspot.com", "/");
	string cont = getPageContent(ip, "mitbbsfetch.appspot.com", "/");

	std::cout << cont << std::endl;

	return 0;
}

