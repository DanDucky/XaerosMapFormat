#include "ByteOutputStream.hpp"

#include "ztd/text/transcode.hpp"

ByteOutputStream::ByteOutputStream(std::ostream &stream) : stream(stream) {
}

std::ostream & ByteOutputStream::getStream() const {
    return stream;
}

ByteOutputStream::operator std::ostream&() const {
    return stream;
}

void ByteOutputStream::writeMUTF(const std::string &string) {
    const std::u8string output = ztd::text::transcode(string, ztd::text::ascii, ztd::text::mutf8, ztd::text::replacement_handler);

    write<std::uint16_t>(string.length());

    stream.write(reinterpret_cast<const char *>(output.begin().base()), string.length());
}
