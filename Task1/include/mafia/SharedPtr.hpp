#pragma once

#include <cstddef>
#include <utility>
#include <atomic>

template <class T>
class SharedPtr {
public:
    SharedPtr() = default;
    explicit SharedPtr(T* ptr);
    SharedPtr(const SharedPtr& other);
    SharedPtr(SharedPtr&& other) noexcept;

    ~SharedPtr();

    SharedPtr& operator=(const SharedPtr& other);
    SharedPtr& operator=(SharedPtr&& other) noexcept;

    T& operator*() const;
    T* operator->() const;


    T* get() const;

    void reset();
    void reset(T* ptr);

    void swap(SharedPtr& other) noexcept;

    bool operator==(const SharedPtr& other) const;
    bool operator!=(const SharedPtr& other) const;

private:
    void release();
    T* ptr_ = nullptr;
    std::atomic<std::size_t> * count_ = nullptr;
};

template <typename T>
SharedPtr<T>::SharedPtr(T* ptr)  {
    ptr_ = ptr;

    if (ptr != nullptr) {
        try {
            count_ = new std::atomic<std::size_t>(1);
        } catch (...) {
            delete ptr;
            throw;
        }
    }
}

template <typename T>
SharedPtr<T>::SharedPtr(const SharedPtr& other) {
    ptr_ = other.ptr_;
    count_ = other.count_;
    if (count_ != nullptr) {
        ++(*count_);
    }
}

template <typename T>
SharedPtr<T>::SharedPtr(SharedPtr&& other) noexcept{
    ptr_ = other.ptr_;
    count_ = other.count_;

    other.ptr_ = nullptr;
    other.count_ = nullptr;
}

template <typename T>
SharedPtr<T>::~SharedPtr() {
    release();
}

template <typename T>
SharedPtr<T>& SharedPtr<T>::operator=(const SharedPtr& other) {

    if (this == &other) {
        return *this;
    }

    release();

    count_ = other.count_;
    ptr_ = other.ptr_;
    if (count_ != nullptr) {
        ++(*count_);
    }

    return *this;
}

template <typename T>
SharedPtr<T>& SharedPtr<T>::operator=(SharedPtr&& other) noexcept{

    if (this == &other) {
        return *this;
    }

    release();

    count_ = other.count_;
    ptr_ = other.ptr_;

    other.ptr_ = nullptr;
    other.count_ = nullptr;

    return *this;
}

template <typename T>
T& SharedPtr<T>::operator*() const {
    return *ptr_;
}

template <typename T>
T* SharedPtr<T>::operator->() const {
    return ptr_;
}

template <typename T>
T* SharedPtr<T>::get() const {
    return ptr_;
}

template <typename T>
void SharedPtr<T>::reset(){
    release();
}

template <typename T>
void SharedPtr<T>::reset(T* ptr) {
    if (ptr == ptr_) {
        return;
    }
    SharedPtr temp(ptr);
    swap(temp);
}

template <typename T>
void SharedPtr<T>::swap(SharedPtr& other) noexcept{
    std::swap(count_, other.count_);
    std::swap(ptr_, other.ptr_);
}

template <typename T>
bool SharedPtr<T>::operator==(const SharedPtr& other) const {
    return ptr_ == other.ptr_;
}

template <typename T>
bool SharedPtr<T>::operator!=(const SharedPtr& other) const {
    return ptr_ != other.ptr_;
}

template <typename T>
void SharedPtr<T>::release(){
    if (count_ != nullptr) {
        if (count_->fetch_sub(1) == 1) {
            delete ptr_;
            delete count_;
        }
    }

    ptr_ = nullptr;
    count_ = nullptr;
}
