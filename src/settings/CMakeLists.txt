ADD_LIBRARY(settings
    settings.h
    settings.cpp
)

TARGET_LINK_LIBRARIES(settings
    ${CONAN_LIBS}
)
