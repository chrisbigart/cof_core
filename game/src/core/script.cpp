#include "core/script.h"

#include "utils.h"


QDataStream& operator<<(QDataStream& stream, const script_t& script) {
	stream << script.script_id << script.enabled << script.finished;
	stream_write_string(stream, script.name, 64);
	stream_write_string(stream, script.text, 4096);
	return stream;
}


QDataStream& operator>>(QDataStream& stream, script_t& script) {
	stream >> script.script_id >> script.enabled >> script.finished;
	script.name = stream_read_string(stream, 64);
	script.text = stream_read_string(stream, 4096);

	return stream;
}