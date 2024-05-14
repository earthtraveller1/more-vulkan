#pragma once

#define NO_COPY(c)                                                             \
    c(const c &) = delete;                                                     \
    c &operator=(const c &) = delete

#define VK_ERROR(c)                                                            \
    do {                                                                       \
        const auto r = c;                                                      \
        if (r != VK_SUCCESS) {                                                 \
            std::cerr << "[ERROR]: " << #c << " failed.\n";                    \
            throw mv::vulkan_exception{r};                                     \
        }                                                                      \
    } while (0)

#define YES_MOVE(c) c(c &&) = default;

#define YES_MOVE_ASSIGN(c) c &operator=(c &&) = default;

#ifdef USE_TRACE
#define TRACE(c)                                                               \
    std::cerr << "[TRACE]: " << #c << " called.\n";                            \
    c
#else
#define TRACE(c) c
#endif
