#pragma once

#include <atomic>
#include <type_traits>

namespace gamescope
{
    // std::atomic<T> converts to T and, through its constructor, from T, so a
    // conditional expression with a T in one arm and a std::atomic<T> in the
    // other is ambiguous. The explicit constructor here leaves only the
    // conversion to T, so such expressions read the value as a plain T would.
    //
    // With another arithmetic type U in the other arm, the class operand is
    // converted to U rather than both arms taking the usual arithmetic
    // conversions, so `b ? int : AtomicValue<uint32_t>` would be an int where a
    // plain uint32_t gives an unsigned. The deleted constructor makes exactly
    // the expressions whose type would differ ambiguous, and so ill-formed.
    template <typename T>
    class AtomicValue
    {
    public:
        constexpr explicit AtomicValue( T value )
            : m_value{ value }
        {
        }

        template <typename U>
            requires ( std::is_arithmetic_v<U> && !std::is_same_v<std::common_type_t<U, T>, U> )
        AtomicValue( U ) = delete;

        AtomicValue( const AtomicValue & ) = delete;
        AtomicValue &operator = ( const AtomicValue & ) = delete;

        operator T() const
        {
            return m_value.load();
        }

        T operator = ( T value )
        {
            m_value.store( value );
            return value;
        }

    private:
        std::atomic<T> m_value;
    };
}
