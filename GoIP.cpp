// GoIP.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <iostream>
#include <sstream>
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
#include "Poco/Timespan.h"

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

#define MAX_THREADS 5

Semaphore g_semaphore(MAX_THREADS,MAX_THREADS);

string getPageContent(IPAddress addr, string host, string path)
{
	std::string content;
	try
	{
		Poco::Net::StreamSocket sock;
		sock.connect(SocketAddress(addr, 80), Poco::Timespan(3,0));
		sock.setSendTimeout(Poco::Timespan(3,0));
		sock.setReceiveTimeout(Poco::Timespan(3,0));

		HTTPRequest req(HTTPRequest::HTTP_GET, path);
		req.setHost(host);

		std::ostringstream oss;
		req.write(oss);

		string s = oss.str();
		sock.sendBytes(s.c_str(), s.length());

		char response[1024];
		int n;
		do {
            n = sock.receiveBytes(response, 1024);
            content.append(response, n);
		} while (n!=0 && n<5000);
	}
	catch (Poco::Exception& /*exc*/)
	{
		//std::cout << exc.displayText() << std::endl;
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
