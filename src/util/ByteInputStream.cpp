#include "ByteInputStream.hpp"

#include <cmath>
#include <algorithm>
#include <ztd/text/transcode.hpp>
#include <ztd/text/utf8.hpp>


xaero::ByteInputStream::ByteInputStream(std::istream &inputStream) : stream(inputStream) {

}

std::istream & xaero::ByteInputStream::getStream() const {
    return stream;
}

xaero::ByteInputStream::operator std::istream&() const {
    return stream;
}

std::string xaero::ByteInputStream::getNextMUTF() {
    const auto length = getNext<std::uint16_t>();

    if (length == 0) return "";

    char8_t* const rawString = new char8_t[length];

    stream.read(reinterpret_cast<char*>(rawString), length);

    ztd::span<const char8_t> input(rawString, length);

    auto output = ztd::text::transcode(
        input,
        ztd::text::mutf8,
        ztd::text::ascii,
        ztd::text::replacement_handler
        );

    delete[] rawString;

    return output;
}

void xaero::ByteInputStream::skip(const int n) {
    stream.ignore(n);
}

bool xaero::ByteInputStream::eof() const {
    return stream.eof();
}
