#pragma once

#define NO_COPY(c)                                                             \
    c(const c &) = delete;                                                     \
    c &operator=(const c &) = delete;
