// GoIP.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <iostream>
#include <list>

#include "Poco/Net/HTTPClientSession.h"
#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPResponse.h"
#include "Poco/StreamCopier.h"
#include "Poco/InflatingStream.h"
#include "Poco/Path.h"
#include "Poco/URI.h"
#include "Poco/Thread.h"
#include "Poco/ThreadPool.h"
#include "Poco/Semaphore.h"
#include "Poco/Exception.h"
#include "Poco/UnicodeConverter.h"

using std::string;

using Poco::Net::HTTPClientSession;
using Poco::Net::HTTPRequest;
using Poco::Net::HTTPResponse;
using Poco::Net::HTTPMessage;
using Poco::Net::SocketAddress;
using Poco::Net::IPAddress;
using Poco::Net::StreamSocket;
using Poco::StreamCopier;
using Poco::Path;
using Poco::URI;
using Poco::Thread;
using Poco::ThreadPool;
using Poco::Semaphore;
using Poco::FastMutex;

Semaphore g_semaphore(5,5);

string getPageContent(IPAddress addr, string host, string path)
{
	std::string content;
	try
	{
		HTTPClientSession session(SocketAddress(addr, 80));

//#ifdef _DEBUG
//		session.setProxy("127.0.0.1",8888);
//#endif
		HTTPRequest req(HTTPRequest::HTTP_GET, path);
		req.setHost(host);

		session.sendRequest(req);

		HTTPResponse res;
		std::istream& rs = session.receiveResponse(res);

		StreamCopier::copyToString(rs, content);
	}
	catch (Poco::Exception& /*exc*/)
	{
		//ATLTRACE("%s\n",exc.displayText().c_str());
	}

	return content;
}

bool checkPageContent(string content)
{
	return (content.find("page?pn=0") != string::npos);
}

class Robot : public Poco::Runnable
{
public:
    Robot(const IPAddress& ipaddr);
	void run();
protected:
private:
	IPAddress _ipaddr;
	static FastMutex _mutex;
};

FastMutex Robot::_mutex;

Robot::Robot(const IPAddress& ipaddr)
    :_ipaddr(ipaddr)
{
}

void Robot::run()
{
	string cont = getPageContent(_ipaddr, "mitbbsfetch.appspot.com", "/");

	bool iswork = checkPageContent(cont);

	{
		Poco::FastMutex::ScopedLock lock(_mutex);

		if (iswork) {
			std::cout << _ipaddr.toString() << " works!" << std::endl;
		}
		else {
			std::cout << _ipaddr.toString() << " opps.." << std::endl;
		}
	}

	g_semaphore.set();
}

/// class Search Task

class TaskManager
{
public:
	struct Task {
		Robot *robot;
		//Poco::Thread* thread;
	};

	TaskManager():_pool() {}
    ~TaskManager();

	void addTask(IPAddress ip);
	void waitAll();
private:
	TaskManager(const TaskManager&);
    TaskManager& operator= (const TaskManager&);

	Poco::ThreadPool _pool;
	std::list<Task> _taskList;
private:
};


TaskManager::~TaskManager()
{
	waitAll();
}

void TaskManager::addTask(IPAddress ip)
{
	g_semaphore.wait();

	Robot* robot = new Robot(ip);

	_pool.start(*robot);

	Task task = {robot};
	_taskList.push_back(task);
}

void TaskManager::waitAll()
{
	_pool.joinAll();

	std::list<Task>::iterator iter;

	for (iter=_taskList.begin();iter!=_taskList.end();++iter)
	{
		delete iter->robot;
	}
	_taskList.clear();
}


int _tmain(int argc, _TCHAR* argv[])
{
	if (argc < 3) {
		std::cout << "Usage: GoIP ip range" << std::endl;
		return 1;
	}

	string ipstr;
	Poco::UnicodeConverter::toUTF8(argv[1], ipstr);

	int range = _ttoi(argv[2]);

	TaskManager taskmgr;

	u_long ip = htonl(inet_addr(ipstr.c_str()));

	for (int i=0; i<range; ++i) {
		in_addr inaddr;
		inaddr.S_un.S_addr = ntohl(ip);

		IPAddress ipaddr(&inaddr, sizeof(in_addr));

		taskmgr.addTask(ipaddr);

		++ip;
	}

	taskmgr.waitAll();

	return 0;
}
