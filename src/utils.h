/**
 * utils.h
 *
 * Collection of utility functions and macros for the pico2maple project.
 *
 * Copyright (c) 2024 Colin Luoma
 */

#ifndef PICO2MAPLE_UTILS_H
#define PICO2MAPLE_UTILS_H

#ifdef DEBUG
#define DEBUG_PRINT(fmt, args...) \
        printf("DEBUG: %s:%d:%s(): " fmt, __FILE__, __LINE__, __func__, ## args)
#else
#define DEBUG_PRINT(fmt, ...) ((void)0)
#endif

#endif //PICO2MAPLE_UTILS_H
