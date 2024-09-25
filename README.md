# stm32-libs

Some STM32 libraries for HAL.

- [BMP180](bmp180/README.md)

## How to Add a Library

- Add library executable path to CmakeLists.txt:

```cmake
# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
    Drivers/BMP180/Src/bmp180.c
)
```

- Add library include path to CmakeLists.txt:

```cmake
# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
    Drivers/BMP180/Inc
)
```

- Then you can import it from your source code:

```c

#include "bmp180.h"

int main(void)
{
    BMP180_Init();
    ...
}
```