#pragma once
#include <functional>
#include <type_traits>
#include <utility>
#include <variant> // todo change to utility in c++26

namespace xaero {
    template<typename T>
    concept IndexableType = requires(T t) {
        t[0];
    };

    /**
     * Wrapper class for an indexable type. This is used for lookup tables as they might come in various types
     * @tparam T the type which subscripting it should return
     * @warning this works by using a function pointer to the subscript operator, so it is just a reference to the underlying data!
     * Do not destroy or move the data given to IndexableView if you plan on using it more!
     */
    template<typename T>
    struct IndexableView {
    private:
        using Entry = std::remove_reference_t<T>;

        std::conditional_t<!std::is_const_v<T>,
                           std::function<Entry&(std::size_t)>,
                           std::monostate> subscript;
        std::function<const Entry&(std::size_t)> const_subscript;
    public:
        template<IndexableType Container>
        IndexableView(Container& data) : subscript([&data](const std::size_t index) -> Entry& {
            return data[index];
        }),
        const_subscript([&data](const std::size_t index) -> const Entry& {
            return data[index];
        }) {
        }

        template<IndexableType Container>
        IndexableView(const Container& data) : const_subscript([&data](const std::size_t index) -> const Entry& {
            return data[index];
        }) {
            static_assert(!std::is_const_v<Container> || std::is_const_v<T>, "Element type is mutable but data type is const!");
        }

        IndexableView()=default;

        Entry& operator[](std::size_t index) {
            return subscript(index);
        }

        const Entry& operator[](std::size_t index) const {
            return const_subscript(index);
        }

    };
}