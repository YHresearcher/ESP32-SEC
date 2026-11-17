#pragma once
#include <stdbool.h>
#include <stddef.h>

// Integration inventory only: entries are NOT proof that the implementation
// is installed, nor that missing radio hardware is present.
typedef enum {
    SEC_REQUIRE_CORE = 0,
    SEC_REQUIRE_SOFTWARE,
    SEC_REQUIRE_NRF24,
    SEC_REQUIRE_CC1101,
    SEC_REQUIRE_IR_RX,
    SEC_REQUIRE_IR_TX,
    SEC_REQUIRE_NFC,
    SEC_REQUIRE_GNSS,
    SEC_REQUIRE_SD
} sec_requirement_t;
typedef struct {
    const char *key;
    const char *label;
    sec_requirement_t requirement;
    bool driver_integrated;
} sec_feature_info_t;
const sec_feature_info_t *sec_feature_catalog(size_t *count);
size_t sec_feature_compiled_count(void);
