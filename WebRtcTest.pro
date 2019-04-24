QT -= gui

CONFIG += c++11 console
CONFIG -= app_bundle

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += main.cpp \
    webrtc/analog_agc.c    \
    webrtc/digital_agc.c      \
    webrtc/fft4g.c      \
    webrtc/ns_core.c      \
    webrtc/resample_48khz.c   \
    webrtc/resample_fractional.c \
    webrtc/spl_sqrt_floor.c \
    webrtc/complex_bit_reverse.c \
    webrtc/division_operations.c  \
    webrtc/get_scaling_square.c  \
    webrtc/nsx_core.c      \
    webrtc/resample_by_2.c   \
    webrtc/ring_buffer.c    \
    webrtc/vector_scaling_operations.c \
    webrtc/complex_fft.c     \
    webrtc/dot_product_with_scale.c \
    webrtc/min_max_operations.c  \
    webrtc/nsx_core_c.c     \
    webrtc/resample_by_2_internal.c \
    webrtc/spl_init.c \
    webrtc/copy_set_operations.c \
    webrtc/downsample_fast.c  \
    webrtc/noise_suppression.c  \
    webrtc/nsx_core_neon_offsets.c  \
    webrtc/resample_by_2_mips.c  \
    webrtc/splitting_filter.c \
    webrtc/cross_correlation.c  \
    webrtc/energy.c     \
    webrtc/noise_suppression_x.c \
    webrtc/real_fft.c      \
    webrtc/resample.c         \
    webrtc/spl_sqrt.c \
    zaudioplaythread.cpp \
    zaudiocapthread.cpp

HEADERS += \
    webrtc/analog_agc.h   \
    webrtc/defines.h      \
    webrtc/gain_control.h  \
    webrtc/ns_core.h     \
    webrtc/real_fft.h   \
    webrtc/signal_processing_library.h \
    webrtc/windows_private.h \
    webrtc/complex_fft_tables.h  \
    webrtc/digital_agc.h  \
    webrtc/noise_suppression.h   \
    webrtc/nsx_core.h    \
    webrtc/resample_by_2_internal.h \
    webrtc/spl_inl.h    \
    webrtc/cpu_features_wrapper.h  \
    webrtc/fft4g.h      \
    webrtc/noise_suppression_x.h  \
    webrtc/nsx_defines.h \
    webrtc/ring_buffer.h     \
    webrtc/typedefs.h \
    zaudioplaythread.h \
    zaudiocapthread.h \
    zgbl.h

LIBS += -lasound
