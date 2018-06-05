#ifndef BRICKS_DSP_ALIGNED_ARRAY_H
#define BRICKS_DSP_ALIGNED_ARRAY_H

#include <cassert>

namespace bricks {

/* Enough to align float buffers to 256 bit avx2 register boundaries */
constexpr size_t VECTOR_ALIGNMENT = 32;

/* Simple array wrapper class for data aligned for use with vector registers
 * as gcc doesn't seem to understand alignment in std::array  */
template<typename value_type, size_t length>
class AlignedArray
{
public:
    AlignedArray() = default;
    /* Constructor with default value */
    explicit AlignedArray(value_type default_val)
    {
        fill(default_val);
    }
    /* Constructor for copying from raw array, source must contain at least length elements */
    explicit AlignedArray(const value_type* source)
    {
        std::copy(source, source +  length, _data);
    }

    using iterator = value_type*;
    using const_iterator = const value_type*;

    AlignedArray::iterator begin() {return _data;}
    AlignedArray::iterator end() { return _data + length;}

    AlignedArray::const_iterator begin() const {return _data;}
    AlignedArray::const_iterator end() const {return _data + length;}

    value_type operator [](int i) const
    {
        assert(i < length);
        return _data[i];
    }
    value_type& operator [](int i)
    {
        assert(i < length);
        return _data[i];
    }

    size_t size() const {return length;}
    value_type* data() {return _data;}
    const value_type* data() const {return _data;}

    void fill(value_type value)
    {
        std::fill(_data, _data + length, value);
    }

private:
    alignas(VECTOR_ALIGNMENT) value_type _data[length];
};

static_assert(sizeof(AlignedArray<float, 64>) == 64 * sizeof(float));
} // namespace bricks

#endif //BRICKS_DSP_ALIGNED_ARRAY_H

