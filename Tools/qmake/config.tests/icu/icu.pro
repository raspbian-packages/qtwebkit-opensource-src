SOURCES = icu.cpp
CONFIG += console
CONFIG -= qt dylib

win32 {
    CONFIG(static, static|shared) {
        LIBS += $$QMAKE_LIBS_CORE
        CONFIG(debug, debug|release) {
            LIBS += -lsicuind -lsicuucd -lsicudtd
        } else {
            LIBS += -lsicuin -lsicuuc -lsicudt
        }
    } else {
        LIBS += -licuin -licuuc -licudt
    }
} else:qtConfig(pkg-config):packagesExist("icu-i18n") {
    PKGCONFIG += icu-i18n
} else {
    LIBS += -licui18n -licuuc -licudata
}
