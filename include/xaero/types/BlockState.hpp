#pragma once
#include <string>
#include <tag_compound.h>

struct BlockState {
private:
    [[nodiscard]] std::string_view strippedName() const;
public:
    std::string name;
    nbt::tag_compound properties;

    explicit BlockState(nbt::tag_compound nbt);
    explicit BlockState(std::string name, nbt::tag_compound properties);

    [[nodiscard]] nbt::tag_compound getNBT() const;

    [[nodiscard]] bool operator==(const BlockState & other) const;
};
