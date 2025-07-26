#pragma once

template<typename T>
struct OptionalOwnerPtr {
private:
    const bool owns;
public:
    T* const pointer;

    explicit OptionalOwnerPtr(T* const pointer, const bool owns) : owns(owns), pointer(pointer) {
    }
    OptionalOwnerPtr() : owns(false), pointer(nullptr) {
    }

    [[nodiscard]] T& operator*() {
        return *pointer;
    }

    OptionalOwnerPtr(OptionalOwnerPtr&)=delete;
    OptionalOwnerPtr(OptionalOwnerPtr&& other) noexcept : owns(other.owns), pointer(other.pointer){
        const_cast<bool&>(other.owns) = false;
    }
    OptionalOwnerPtr & operator=(OptionalOwnerPtr &&other) noexcept {
        const_cast<T*&>(pointer) = other.pointer;
        const_cast<bool&>(owns) = other.owns;
        const_cast<bool&>(other.owns) = false;
        return *this;
    }


    ~OptionalOwnerPtr() {
        if (owns) delete pointer;
    }

    [[nodiscard]] T* get() {
        return pointer;
    }

    [[nodiscard]] const T* get() const {
        return pointer;
    }
};
