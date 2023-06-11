# CHANGELOG.md

## 2.0.20 (2023-06-11)

Fixes:

 - Change TIMEOUT_DATA_FRAME_MS to 100ms to reduce retries on Windows

Features:

 - None

## 2.0.19 (2023-06-11)

Fixes:

 - Handle retries when errors occur receiving frames
 - Generate retries when sent data is not acknowledged
 - Resolve misc. timing issues
 - Better use of flag fill to avoid getting data from server before we're actually ready for it

Features:

 - None

## 2.0.15 (2023-05-31)

Fixes:

 - Improve handling of rx abort in data frame
 - Fix build to allow use of latest Pico SDK (reference pico_sdk_import.cmake in sdk)
 
Features:

 - None

## 2.0.14 (2023-05-30)

Fixes:

 - None
 
Features:

 - First public release
 