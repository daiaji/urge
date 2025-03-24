// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/media/audio_impl.h"

namespace content {

AudioImpl::AudioImpl(ContentProfile* profile,
                     filesystem::IOService* io_service,
                     I18NProfile* i18n_profile)
    : profile_(profile), io_service_(io_service), i18n_profile_(i18n_profile) {}

AudioImpl::~AudioImpl() {}

void AudioImpl::SetupMIDI(ExceptionState& exception_state) {}

void AudioImpl::BGMPlay(const std::string& filename,
                        int32_t volume,
                        int32_t pitch,
                        int32_t pos,
                        ExceptionState& exception_state) {}

void AudioImpl::BGMStop(ExceptionState& exception_state) {}

void AudioImpl::BGMFade(int32_t time, ExceptionState& exception_state) {}

int32_t AudioImpl::BGMPos(ExceptionState& exception_state) {
  return 0;
}

void AudioImpl::BGSPlay(const std::string& filename,
                        int32_t volume,
                        int32_t pitch,
                        int32_t pos,
                        ExceptionState& exception_state) {}

void AudioImpl::BGSStop(ExceptionState& exception_state) {}

void AudioImpl::BGSFade(int32_t time, ExceptionState& exception_state) {}

int32_t AudioImpl::BGSPos(ExceptionState& exception_state) {
  return 0;
}

void AudioImpl::MEPlay(const std::string& filename,
                       int32_t volume,
                       int32_t pitch,
                       ExceptionState& exception_state) {}

void AudioImpl::MEStop(ExceptionState& exception_state) {}

void AudioImpl::MEFade(int32_t time, ExceptionState& exception_state) {}

void AudioImpl::SEPlay(const std::string& filename,
                       int32_t volume,
                       int32_t pitch,
                       ExceptionState& exception_state) {}

void AudioImpl::SEStop(ExceptionState& exception_state) {}

void AudioImpl::Reset(ExceptionState& exception_state) {}

}  // namespace content
