#pragma once

// Configuração de defines para o MiniAudio antes do include
// Habilita suporte a formatos de áudio (MP3, Vorbis, FLAC) e define NOMINMAX
// para evitar conflitos com Windows.h

#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef MA_ENABLE_MP3
#define MA_ENABLE_MP3
#endif

#ifndef MA_ENABLE_VORBIS
#define MA_ENABLE_VORBIS
#endif

#ifndef MA_ENABLE_FLAC
#define MA_ENABLE_FLAC
#endif


