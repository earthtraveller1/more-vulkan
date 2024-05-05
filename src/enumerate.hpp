#include <cstddef>
#include <iterator>

template <typename T> struct enumerate {
    explicit enumerate(T p_begin, T p_end) : m_begin(p_begin), m_end(p_end) {}

    struct iterator_t {
        using It = T;
        It it;
        size_t index = 0;

        iterator_t(const It p_it, size_t p_index) : it(p_it), index(p_index) {}

        auto operator++() noexcept -> void {
            ++it;
            ++index;
        }

        auto operator++(int) noexcept -> iterator_t {
            return iterator_t{it++, index++};
        }

        auto operator==(const iterator_t &p_it) const noexcept -> bool {
            return (p_it.it == it) && (p_it.index == index);
        }

        auto operator!=(const iterator_t &p_it) const noexcept -> bool {
            return !(p_it == *this);
        }

        struct items_t {
            size_t index;
            decltype(*it) obj;
        };

        auto operator*() noexcept -> items_t {
            return items_t{index, *it};
        }
    };

    iterator_t begin() {
        return iterator_t{m_begin, 0};
    }

    iterator_t end() noexcept {
        return iterator_t{
            m_end, static_cast<size_t>(std::distance(m_begin, m_end))
        };
    }

  private:
    const T m_begin;
    const T m_end;
};
