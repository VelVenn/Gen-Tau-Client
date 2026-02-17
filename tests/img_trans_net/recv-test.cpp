#include "img_trans/net/TReassembly.hpp"
#include "img_trans/net/TRecv.hpp"
#include "utils/TLog.hpp"

#include <exception>
#include <iostream>
#include <memory>
#include <thread>

#define T_LOG_TAG "[TRecv Test] "

using namespace gentau;
using namespace std;

int main()
{
	try {
		TReassembly::SharedPtr reassembler = std::make_shared<TReassembly>(nullptr);
		auto                   recv        = TRecv::createUni(reassembler);

        recv->start();

        cout << "TRecv is running. Press Ctrl+C to stop." << endl;

        while (true) {
            this_thread::sleep_for(100ms);
        }
	} catch (const exception& ex) {
		tLogError("Error happend: {}", ex.what());
		return -1;
	}
}