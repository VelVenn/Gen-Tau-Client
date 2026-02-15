#include "utils/TLog.hpp"
#include "utils/TSignal.hpp"

#include <string>

#define T_LOG_TAG_IMG "[SignalTest] "

using namespace gentau;
using namespace std;

class RealSignalOwner
{
  public:
	TSignal<RealSignalOwner, std::string> onTestSignal;

    RealSignalOwner() {
        onTestSignal("Hello from RealSignalOwner!");
    }
};

class Hacker : public RealSignalOwner
{
  public:
	RealSignalOwner& victim;

	Hacker(RealSignalOwner& signalOwner) : victim(signalOwner) {}
};

int main()
{
	RealSignalOwner signalOwner;
	Hacker          hacker{ signalOwner };

	hacker.onTestSignal.connect([](const std::string& msg) {
		tImgTransLogInfo("Received signal with message: {}", msg);
	});

	return 0;
}