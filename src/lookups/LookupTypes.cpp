#include <algorithm>
#include <xaero/lookups/LookupTypes.hpp>
#include <nbt_tags.h>
#include <ranges>

bool xaero::ValueCompare::operator()(const nbt::value &lhs, const nbt::value &rhs) const noexcept {
    if (lhs.get_ptr() != nullptr && rhs.get_ptr() != nullptr) { // this is about to be some serious cancer
        if (lhs.get_type() != rhs.get_type()) return lhs.get_type() < rhs.get_type();

        switch (lhs.get_type()) {
            case nbt::tag_type::End:
                return false;
            case nbt::tag_type::Byte:
                return lhs.as<nbt::tag_byte>().get() < rhs.as<nbt::tag_byte>().get();
            case nbt::tag_type::Short:
                return lhs.as<nbt::tag_short>().get() < rhs.as<nbt::tag_short>().get();
            case nbt::tag_type::Int:
                return lhs.as<nbt::tag_int>().get() < rhs.as<nbt::tag_int>().get();
            case nbt::tag_type::Long:
                return lhs.as<nbt::tag_long>().get() < rhs.as<nbt::tag_long>().get();
            case nbt::tag_type::Float:
                return lhs.as<nbt::tag_float>().get() < rhs.as<nbt::tag_float>().get();
            case nbt::tag_type::Double:
                return lhs.as<nbt::tag_double>().get() < rhs.as<nbt::tag_double>().get();
            case nbt::tag_type::Byte_Array: {
                const auto& aArray = lhs.as<nbt::tag_byte_array>().get();
                const auto& bArray = rhs.as<nbt::tag_byte_array>().get();
                return std::ranges::lexicographical_compare(aArray.begin(), aArray.end(), bArray.begin(), bArray.end(),
                    [](const auto& a, const auto& b) {
                        return a < b;
                    });
            }
            case nbt::tag_type::String:
                return lhs.as<nbt::tag_string>().get() < rhs.as<nbt::tag_string>().get();
            case nbt::tag_type::List: {
                const auto& aArray = lhs.as<nbt::tag_list>();
                const auto& bArray = rhs.as<nbt::tag_list>();

                return std::ranges::lexicographical_compare(aArray.begin(), aArray.end(), bArray.begin(), bArray.end(), ValueCompare());
            }
            case nbt::tag_type::Compound:
                return CompoundCompare()(lhs.as<nbt::tag_compound>(), rhs.as<nbt::tag_compound>());
            case nbt::tag_type::Int_Array: {
                const auto& aArray = lhs.as<nbt::tag_int_array>().get();
                const auto& bArray = rhs.as<nbt::tag_int_array>().get();

                return std::ranges::lexicographical_compare(aArray, bArray);
            }
            case nbt::tag_type::Long_Array: {
                const auto& aArray = lhs.as<nbt::tag_long_array>().get();
                const auto& bArray = rhs.as<nbt::tag_long_array>().get();

                return std::ranges::lexicographical_compare(aArray, bArray);
            }

            case nbt::tag_type::Null:
            default:
                return false; // should never happen!!!
        }
    } else {
        return lhs.get_ptr() == nullptr && rhs.get_ptr() == nullptr;
    }
}

bool xaero::CompoundCompare::operator()(const nbt::tag_compound &lhs, const nbt::tag_compound &rhs) const noexcept {
    return std::ranges::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), [](const std::pair<const std::string, nbt::value>& a, const std::pair<const std::string, nbt::value>& b) -> bool {
        if (a.first != b.first) {
            return a.first < b.first;
        }

        return ValueCompare()(a.second, b.second);
    });
}

std::size_t xaero::NameHash::operator()(const std::string_view &name) const noexcept {
    const auto split = name.find_first_of(':');
    return std::hash<std::string_view>{}(split != std::string_view::npos ? name.substr(split + 1) : name);
}

bool xaero::NameEquals::operator()(const std::string_view &a, const std::string_view &b) const noexcept {
    const auto aSplit = a.find_first_of(':');
    const auto bSplit = b.find_first_of(':');

    return (aSplit != std::string_view::npos ? a.substr(aSplit + 1) : a) == (bSplit != std::string_view::npos ? b.substr(bSplit + 1) : b);
}
