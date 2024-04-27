// вставьте сюда ваш код для класса SimpleVector
// внесите необходимые изменения для поддержки move-семантики

#include "array_ptr.h"
#include <algorithm>
#include <cassert>
#include <initializer_list>
#include <cstddef>
#include <stdexcept>
#include <utility>

class ReserveProxyObj {
public:
    size_t capacity_;

    ReserveProxyObj(size_t capacity) {
        capacity_ = capacity;
    }
};

template <typename Type>
class SimpleVector {
public:
    using Iterator = Type*;
    using ConstIterator = const Type*;

    SimpleVector() noexcept = default;


    // Создаёт вектор из size элементов, инициализированных значением по умолчанию
    explicit SimpleVector(size_t size)
        : ptr_(size)
        , size_(size)
        , capacity_(size) {
        // в цикле заполняем вектор значениями по умолчанию, используя std::move
        for (auto it = begin(); it != end(); ++it) {
            //std::exchange(*it, std::move(Type()));
            *it = std::move(Type());
        }
    }

    // Создаёт вектор из size элементов, инициализированных значением value
    SimpleVector(size_t size, const Type& value)
        : ptr_(size)
        , size_(size)
        , capacity_(size) {
        // в цикле заполняем вектор значениями value, используя std::move
        for (auto it = begin(); it != end(); ++it) {
            // std::exchange(*it, std::move(value));
            *it = std::move(value);
        }
    }

    // Создаёт вектор из std::initializer_list
    SimpleVector(std::initializer_list<Type> init)
        : ptr_(init.size())
        , size_(init.size())
        , capacity_(init.size()) {
        std::move(init.begin(), init.end(), ptr_.Get());
    }

    SimpleVector(ReserveProxyObj reserve)
        : SimpleVector(reserve.capacity_) {
        size_ = 0;
    }

    // Возвращает количество элементов в массиве
    size_t GetSize() const noexcept {
        return size_;
    }

    // Возвращает вместимость массива
    size_t GetCapacity() const noexcept {
        return capacity_;
    }

    // Сообщает, пустой ли массив
    bool IsEmpty() const noexcept {
        return size_ == 0;
    }

    // Возвращает ссылку на элемент с индексом index
    Type& operator[](size_t index) noexcept {
        return ptr_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    const Type& operator[](size_t index) const noexcept {
        return ptr_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    Type& At(size_t index) {
        if (index >= size_) {
            throw std::out_of_range("Invalid index");
        }
        return ptr_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    const Type& At(size_t index) const {
        if (index >= size_) {
            throw std::out_of_range("Invalid index");
        }
        return ptr_[index];
    }

    // Обнуляет размер массива, не изменяя его вместимость
    void Clear() noexcept {
        size_ = 0;
    }

    // Изменяет размер массива.
    // При увеличении размера новые элементы получают значение по умолчанию для типа Type
    void Resize(size_t new_size) {
        // если размера хватает, просто обновляем его
        if (new_size <= size_) {
            size_ = new_size;
        }
        // если размера не хватает, но хватает вместимости, заполняем новые элементы значениями по умолчанию
        else if (new_size > size_ && new_size < capacity_) {
            for (auto it = begin() + size_; it != begin() + new_size; ++it) {
                *it = std::move(Type());
            }
        }
        // иначе увеличиваем вместимость и заполняем новые элементы значениями по умолчанию
        else {
            size_t new_capacity = std::max(capacity_ * 2, new_size);
            SimpleVector new_vector(new_capacity);
            std::move(begin(), end(), new_vector.begin());
            swap(new_vector);
            for (auto it = begin() + size_; it != begin() + new_size; ++it) {
                *it = std::move(Type());
            }
        }
        // обновляем размер
        size_ = new_size;
    }

    // Добавляет элемент в конец вектора
    // При нехватке места увеличивает вдвое вместимость вектора в методе CapacityChangeRequired()
    void PushBack(const Type& item) {
        CapacityChangeRequired();
        ptr_[size_] = item;
        ++size_;
    }

    void PushBack(Type&& item) {
        CapacityChangeRequired();
        std::exchange(ptr_[size_], std::move(item));
        ++size_;
    }

    // Конструктор копирования
    SimpleVector(const SimpleVector& other) {
        SimpleVector vector(other.size_);
        std::copy(other.begin(), other.end(), vector.begin());
        swap(vector);
    }

    // Оператор копирования
    SimpleVector& operator=(const SimpleVector& rhs) {
        // проверяем, что вектор не копируется сам в себя
        if (this != &rhs) {
            SimpleVector copy(rhs);
            swap(copy);
        }
        return *this;
    }

    // Конструктор перемещения
    SimpleVector(SimpleVector&& other) noexcept
        : ptr_(std::move(other.ptr_))
        , size_(std::move(other.size_))
        , capacity_(std::move(other.capacity_)) {
        other.size_ = 0;
        other.capacity_ = 0;
    }

    // Оператор перемещения
    SimpleVector& operator=(SimpleVector&& rhs) noexcept {
        // проверяем, что не будем перемещать вектор сам в себя
        if (this != &rhs) {
            ptr_ = std::move(rhs.ptr_);
            size_ = std::move(rhs.size_);
            capacity_ = std::move(rhs.capacity_);
            rhs.size_ = 0;
            rhs.capacity_ = 0;
        }
        return *this;
    }

    // Вставляет значение value в позицию pos.
    // Возвращает итератор на вставленное значение
    // Если перед вставкой значения вектор был заполнен полностью,
    // вместимость вектора должна увеличиться вдвое, а для вектора вместимостью 0 стать равной 1
    Iterator Insert(ConstIterator pos, const Type& value) {
        size_t num_steps = std::distance(cbegin(), pos); // определяем количество шагов до позиции вставляемого значения
        CapacityChangeRequired(); // увеличиваем вместимость при необходимости 
        std::copy_backward(begin() + num_steps, begin() + size_, end()); // копируем текущие значения, начиная с конца, до позиции вставки
        ptr_[num_steps] = value; // вставляем значение в заданную позицию
        ++size_;

        return begin() + num_steps;
    }

    Iterator Insert(ConstIterator pos, Type&& value) {
        size_t num_steps = std::distance(cbegin(), pos); // определяем количество шагов до позиции вставляемого значения
        CapacityChangeRequired(); // увеличиваем вместимость при необходимости 
        std::move_backward(begin() + num_steps, begin() + size_, end()); // сдвигаем существующие элементы вправо, начиная с конца, до позиции вставки
        ptr_[num_steps] = std::move(value); // перемещаем значение в заданную позицию
        ++size_;

        return begin() + num_steps;
    }

    // "Удаляет" последний элемент вектора. Вектор не должен быть пустым
    void PopBack() noexcept {
        --size_;
    }

    void Reserve(size_t new_capacity) {
        if (new_capacity <= capacity_) {
            return;
        }
        size_t temp = size_; // "запоминаем" размер вектора
        SimpleVector new_vector(new_capacity); // перевыделяем память, создав новый вектор с новой вместимостью
        std::move(begin(), end(), new_vector.begin()); // перемещаем элементы в новый вектор
        swap(new_vector);
        size_ = temp; // возвращаем размер
    }

    // Удаляет элемент вектора в указанной позиции
    Iterator Erase(ConstIterator pos) {
        size_t num_steps = std::distance(cbegin(), pos); // определяем количество шагов до позиции удаляемого значения
        std::move(begin() + num_steps + 1, end(), begin() + num_steps); // перемещаем существующие элементы начиная с позиции удаляемого элемента
        --size_;

        return begin() + num_steps;
    }

    // Обменивает значение с другим вектором
    void swap(SimpleVector& other) noexcept {
        ptr_.swap(other.ptr_);
        std::swap(other.capacity_, capacity_);
        std::swap(other.size_, size_);
    }

    // Возвращает итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator begin() noexcept {
        return &ptr_[0];
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator end() noexcept {
        // Напишите тело самостоятельно
        return &ptr_[size_];
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator begin() const noexcept {
        // Напишите тело самостоятельно
        return &ptr_[0];
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator end() const noexcept {
        // Напишите тело самостоятельно
        return &ptr_[size_];
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cbegin() const noexcept {
        // Напишите тело самостоятельно
        return &ptr_[0];
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cend() const noexcept {
        // Напишите тело самостоятельно
        return &ptr_[size_];
    }

private:
    ArrayPtr<Type> ptr_;
    size_t size_ = 0;
    size_t capacity_ = 0;

    // Метод, поверяющий необходимо ли изменение емкости
    void CapacityChangeRequired() {
        // если вместимости хватает, ничего не требуется
        if (capacity_ > size_) {
            return;
        }

        // Если вместимость равна нулю, резервируем память для одного элемента, а размер обнуляем
        if (capacity_ == 0) {
            Reserve(1);
            size_ = 0;
        }
        else {
            // Иначе увеличиваем вместимость вектора в два раза
            size_t temp = size_;
            SimpleVector new_vector(capacity_ * 2);
            std::move(begin(), end(), new_vector.begin());
            swap(new_vector);
            size_ = temp;
        }
    }
};

ReserveProxyObj Reserve(size_t capacity_to_reserve) {
    return ReserveProxyObj(capacity_to_reserve);
};

template <typename Type>
inline bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    if (lhs.GetSize() != rhs.GetSize()) {
        return false;
    }
    return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
inline bool operator!=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs == rhs);
}

template <typename Type>
inline bool operator<(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
inline bool operator<=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(rhs < lhs);
}

template <typename Type>
inline bool operator>(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return (rhs < lhs);
}

template <typename Type>
inline bool operator>=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs < rhs);
}