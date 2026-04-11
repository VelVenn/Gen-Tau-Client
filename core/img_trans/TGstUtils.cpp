#include "img_trans/vid_render/TGstUtils.hpp"

using namespace std;

namespace gentau {
namespace gst {
string_view getIssueTypeLiteral(IssueType type) noexcept
{
	switch (type) {
		case IssueType::UNKNOWN:
			return "UNKNOWN";
		case IssueType::PIPELINE_INTERNAL:
			return "PIPELINE INTERNAL";
		case IssueType::PIPELINE_RESOURCE:
			return "PIPELINE RESOURCE";
		case IssueType::PIPELINE_STREAM:
			return "PIPELINE STREAM";
		case IssueType::PIPELINE_OTHER:
			return "PIPELINE OTHER";
		case IssueType::PUSH_FATAL:
			return "PUSH FATAL";
		case IssueType::PUSH_BUSY:
			return "PUSH BUSY";
		case IssueType::GENERIC:
			return "GENERIC";
		default:
			return "UNDEFINED";
	}
}

string_view getStateLiteral(StateType state) noexcept
{
	switch (state) {
		case StateType::NULL_STATE:
			return "NULL STATE";
		case StateType::READY:
			return "READY";
		case StateType::PAUSED:
			return "PAUSED";
		case StateType::RUNNING:
			return "RUNNING";
		default:
			return "UNDEFINED";
	}
}
}  // namespace gst_utils
}  // namespace gentau