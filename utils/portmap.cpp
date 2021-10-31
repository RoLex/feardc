// Tool to add or remove port mapping rules using the interfaces provided by DC++.

#include "base.h"

#include <iostream>
#include <memory>

#include <dcpp/Mapper_MiniUPnPc.h>
#include <dcpp/Mapper_NATPMP.h>
#include <dcpp/ScopedFunctor.h>
#include <dcpp/Util.h>

#ifdef _WIN32
#include <dcpp/w.h>

#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif

using namespace std;
using namespace dcpp;

void help() {
	cout << "Arguments to run portmap with:" << endl << "\t portmap <port> <type> <description> <method> [remove]" << endl
		<< "<port> is a port number to forward." << endl
		<< "<type> must be either 0 (for TCP) or 1 (for UDP)." << endl
		<< "<description> is the description of the port forwarding rule." << endl
		<< "<method> must be either 0, 1, 2 (for NAT-PMP, MiniUPnP, Win UPnP)." << endl
		<< "[remove] (optional) may be set to 1 to remove the rule." << endl;
}

enum { Port = 1, Type, Description, Method, LastCompulsory = Method, Remove };

string getLocalIp() {
	// imitate Util::getLocalIp but avoid calls to managers that haven't been initialized.

	string tmp;

	char buf[256];
	gethostname(buf, 255);
	hostent* he = gethostbyname(buf);
	if(he == NULL || he->h_addr_list[0] == 0)
		return Util::emptyString;
	sockaddr_in dest;
	int i = 0;

	// We take the first ip as default, but if we can find a better one, use it instead...
	memcpy(&(dest.sin_addr), he->h_addr_list[i++], he->h_length);
	tmp = inet_ntoa(dest.sin_addr);
	if(Util::isPrivateIp(tmp, false) || strncmp(tmp.c_str(), "169", 3) == 0) {
		while(he->h_addr_list[i]) {
			memcpy(&(dest.sin_addr), he->h_addr_list[i], he->h_length);
			string tmp2 = inet_ntoa(dest.sin_addr);
			if(!Util::isPrivateIp(tmp2, false) && strncmp(tmp2.c_str(), "169", 3) != 0) {
				tmp = tmp2;
			}
			i++;
		}
	}
	return tmp;
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
	ScopedFunctor([] { WSACleanup(); });
#endif

	if(argc <= LastCompulsory) {
		help();
		return 1;
	}

	auto type = argv[Type][0];
	if(type != '0' && type != '1') {
		cout << "Error: invalid type." << endl;
		help();
		return 1;
	}
	auto tcp = type == '0';

	unique_ptr<Mapper> pMapper;
	switch(argv[Method][0]) {

	case '0': pMapper.reset(new Mapper_NATPMP(getLocalIp(), false)); break;
	case '1': pMapper.reset(new Mapper_MiniUPnPc(getLocalIp(), false)); break;

	default: cout << "Error: invalid method." << endl; help(); return 1;
	}
	auto& mapper = *pMapper;

	ScopedFunctor([&mapper] { mapper.uninit(); });
	if(!mapper.init()) {
		cout << "Failed to initialize the " << mapper.getName() << " interface." << endl;
		return 2;
	}
	cout << "Successfully initialized the " << mapper.getName() << " interface." << endl;

	if(!mapper.open(argv[Port], tcp ? Mapper::PROTOCOL_TCP : Mapper::PROTOCOL_UDP, argv[Description])) {
		cout << "Failed to map the " << argv[Port] << " " << (tcp ? "TCP" : "UDP") << " port with the " << mapper.getName() << " interface." << endl;
		return 3;
	}
	cout << "Successfully mapped the " << argv[Port] << " " << (tcp ? "TCP" : "UDP") << " port with the " << mapper.getName() << " interface." << endl;

	if(argc > Remove && argv[Remove][0] == '1') {
		if(mapper.close()) {
			cout << "Successfully removed the rule." << endl;
		} else {
			cout << "Failed to remove the rule." << endl;
		}
	}

	return 0;
}
