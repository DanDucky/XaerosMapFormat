#include "ByteOutputStream.hpp"

#include <string>

#include "ztd/text/transcode.hpp"

xaero::ByteOutputStream::ByteOutputStream(std::ostream &stream) : stream(stream) {
}

std::ostream & xaero::ByteOutputStream::getStream() const {
    return stream;
}

xaero::ByteOutputStream::operator std::ostream&() const {
    return stream;
}

void xaero::ByteOutputStream::writeMUTF(const std::string &string) {
    const std::u8string output = ztd::text::transcode(string, ztd::text::ascii, ztd::text::mutf8, ztd::text::replacement_handler);

    write<std::uint16_t>(string.length());

    stream.write(reinterpret_cast<const char *>(&*output.begin()), string.length());

    if (stream.fail()) {
        throw std::out_of_range("somehow ran out of space or errored when writing a file");
    }
}
