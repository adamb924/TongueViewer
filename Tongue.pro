HEADERS       = glwidget.h \
                window.h \
                ReadSettings.h
SOURCES       = glwidget.cpp \
                main.cpp \
                window.cpp \
                ReadSettings.cpp
QT           += opengl \ 
		xml

# install
target.path = $$[QT_INSTALL_EXAMPLES]/opengl/hellogl
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS hellogl.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/opengl/hellogl
INSTALLS += target sources
