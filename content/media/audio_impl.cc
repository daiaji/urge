// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/media/audio_impl.h"

#include "imgui/imgui.h"
#include "magic_enum/magic_enum.hpp"

#include "content/context/execution_context.h"
#include "content/profile/command_ids.h"

namespace content {

namespace {

constexpr uint64_t kBgmFadeOutForMETime = 200;
constexpr uint64_t kBgmFadeInAfterMETime = 1000;

float FadeProgress(uint64_t start, uint64_t duration) {
  if (duration == 0)
    return 1.0f;
  const uint64_t elapsed = SDL_GetTicks() - start;
  if (elapsed >= duration)
    return 1.0f;
  return static_cast<float>(elapsed) / static_cast<float>(duration);
}

float LerpVolume(float from, float to, float progress) {
  return from + (to - from) * progress;
}

}  // namespace

AudioImpl::AudioImpl(ExecutionContext* execution_context)
    : EngineObject(execution_context),
      i18n_profile_(execution_context->i18n_profile) {
  if (!execution_context->audio_server)
    return;

  // Create slots
  bgm_ = execution_context->audio_server->CreateStream();
  bgs_ = execution_context->audio_server->CreateStream();
  me_ = execution_context->audio_server->CreateStream();
  se_ = execution_context->audio_server->CreateEmitter();

  // Set looping
  bgm_->SetLooping(true);
  bgs_->SetLooping(true);
  me_->SetLooping(false);

  // Initialize volume from config
  execution_context->audio_server->SetVolume(
      execution_context->engine_profile->audio_volume);

  // Configure MIDI SoundFont
  auto& sf = execution_context->engine_profile->midi_soundfont;
  if (!sf.empty())
    execution_context->audio_server->SetSoundFont(sf);

}

AudioImpl::~AudioImpl() = default;

void AudioImpl::CreateButtonGUISettings() {
  if (!context()->audio_server)
    return;

  if (ImGui::CollapsingHeader(
          i18n_profile_->GetI18NString(IDS_SETTINGS_AUDIO, "Audio").c_str())) {
    // Set global volume
    auto& profile = *context()->engine_profile;
    float volume = profile.audio_volume;
    ImGui::SliderFloat(
        i18n_profile_->GetI18NString(IDS_AUDIO_VOLUME, "Volume").c_str(),
        &volume, 0, 1);
    profile.audio_volume = volume;
    context()->audio_server->SetVolume(volume);
    if (ImGui::IsItemDeactivatedAfterEdit())
      profile.MarkDirty();

    ImGui::Separator();
    if (ImGui::Button("Reset##audio")) {
      profile.ResetAudioDefaults();
      context()->audio_server->SetVolume(profile.audio_volume);
      profile.SaveConfigure();
    }
  }
}

void AudioImpl::SetupMIDI(ExceptionState& exception_state) {
  LOG(INFO) << "[Audio] setup_midi called";
  if (!context()->audio_server)
    return;
  auto& sf = context()->engine_profile->midi_soundfont;
  if (!sf.empty())
    context()->audio_server->SetSoundFont(sf);
  else
    LOG(WARNING) << "[Audio] setup_midi: no SoundFont configured. "
                    "Set [Audio] SoundFont=<path> in Game.ini to enable MIDI.";
}

void AudioImpl::BGMPlay(const std::string& filename,
                        int32_t volume,
                        int32_t pitch,
                        uint64_t pos,
                        ExceptionState& exception_state) {
  if (!bgm_)
    return;

  auto result = bgm_->Play(filename, volume, pitch, pos);
  HandleAudioServiceError(result, filename, exception_state);
}

void AudioImpl::BGMStop(ExceptionState& exception_state) {
  if (!bgm_)
    return;

  bgm_->Stop();
}

void AudioImpl::BGMFade(int32_t time, ExceptionState& exception_state) {
  if (!bgm_)
    return;

  bgm_->Fade(time);
}

uint64_t AudioImpl::BGMPos(ExceptionState& exception_state) {
  if (!bgm_)
    return 0;

  return bgm_->Pos();
}

int32_t AudioImpl::BGMVolume(ExceptionState& exception_state) {
  if (!bgm_)
    return 0;

  return bgm_->GetVolume();
}

void AudioImpl::SetBGMVolume(int32_t volume, ExceptionState& exception_state) {
  if (!bgm_)
    return;

  bgm_->SetVolume(volume);
}

void AudioImpl::BGSPlay(const std::string& filename,
                        int32_t volume,
                        int32_t pitch,
                        uint64_t pos,
                        ExceptionState& exception_state) {
  if (!bgs_)
    return;

  auto result = bgs_->Play(filename, volume, pitch, pos);
  HandleAudioServiceError(result, filename, exception_state);
}

void AudioImpl::BGSStop(ExceptionState& exception_state) {
  if (!bgs_)
    return;

  bgs_->Stop();
}

void AudioImpl::BGSFade(int32_t time, ExceptionState& exception_state) {
  if (!bgs_)
    return;

  bgs_->Fade(time);
}

uint64_t AudioImpl::BGSPos(ExceptionState& exception_state) {
  if (!bgs_)
    return 0;

  return bgs_->Pos();
}

void AudioImpl::MEPlay(const std::string& filename,
                       int32_t volume,
                       int32_t pitch,
                       ExceptionState& exception_state) {
  if (!me_)
    return;

  auto result = me_->Play(filename, volume, pitch, 0);
  HandleAudioServiceError(result, filename, exception_state);
}

void AudioImpl::MEStop(ExceptionState& exception_state) {
  if (!me_)
    return;

  me_->Stop();
}

void AudioImpl::MEFade(int32_t time, ExceptionState& exception_state) {
  if (!me_)
    return;

  me_->Fade(time);
}

uint64_t AudioImpl::MEPos(ExceptionState& exception_state) {
  if (!me_)
    return 0;

  return me_->Pos();
}

void AudioImpl::SEPlay(const std::string& filename,
                       int32_t volume,
                       int32_t pitch,
                       ExceptionState& exception_state) {
  if (!se_)
    return;

  auto result = se_->Play(filename, volume, pitch);
  HandleAudioServiceError(result, filename, exception_state);
}

void AudioImpl::SEStop(ExceptionState& exception_state) {
  if (!se_)
    return;

  se_->Stop();
}

void AudioImpl::Reset(ExceptionState& exception_state) {
  if (bgm_) {
    bgm_->Stop();
    bgm_->SetExternalVolume(1.0f);
  }
  if (bgs_)
    bgs_->Stop();
  if (me_)
    me_->Stop();
  if (se_)
    se_->Stop();
  me_watch_state_ = MeWatchState::kMeNotPlaying;
  me_watch_state_start_ = 0;
  me_watch_fade_start_volume_ = 1.0f;
}

void AudioImpl::HandleAudioServiceError(ma_result result,
                                        const std::string& filename,
                                        ExceptionState& exception_state) {
  if (result == MA_INVALID_FILE) {
    LOG(WARNING) << "[Audio] Unsupport audio format - " << filename;
    return;
  } else if (result != MA_SUCCESS) {
    exception_state.ThrowError(
        ExceptionCode::CONTENT_ERROR, "error when playing audio: %s (%s)",
        filename.c_str(), magic_enum::enum_name(result).data());
    return;
  }
}

void AudioImpl::MeThreadMonitorInternal() {
  if (!me_ || !bgm_)
    return;
  switch (me_watch_state_) {
    case MeWatchState::kMeNotPlaying:
      if (me_->IsPlaying()) {
        LOG(INFO) << "[Audio] ME started; fading out BGM";
        bgm_->BeginMEPause();
        me_watch_state_ = MeWatchState::kBgmFadingOut;
        me_watch_state_start_ = SDL_GetTicks();
        me_watch_fade_start_volume_ = bgm_->GetExternalVolume();
      }
      break;

    case MeWatchState::kBgmFadingOut: {
      if (!me_->IsPlaying()) {
        me_watch_state_ = MeWatchState::kBgmFadingIn;
        me_watch_state_start_ = SDL_GetTicks();
        break;
      }

      const float progress = FadeProgress(me_watch_state_start_,
                                          kBgmFadeOutForMETime);
      bgm_->SetExternalVolume(
          LerpVolume(me_watch_fade_start_volume_, 0.0f, progress));
      if (progress >= 1.0f || !bgm_->IsPlaying()) {
        bgm_->SetExternalVolume(0.0f);
        bgm_->PauseForME();
        me_watch_state_ = MeWatchState::kMePlaying;
      }
      break;
    }

    case MeWatchState::kMePlaying:
      if (!me_->IsPlaying()) {
        LOG(INFO) << "[Audio] ME ended; fading in BGM";
        if (bgm_->ResumeFromME()) {
          me_watch_state_ = MeWatchState::kBgmFadingIn;
          me_watch_state_start_ = SDL_GetTicks();
          me_watch_fade_start_volume_ = bgm_->GetExternalVolume();
        } else {
          bgm_->SetExternalVolume(1.0f);
          me_watch_state_ = MeWatchState::kMeNotPlaying;
        }
      }
      break;

    case MeWatchState::kBgmFadingIn: {
      if (me_->IsPlaying()) {
        LOG(INFO) << "[Audio] ME restarted during BGM fade-in";
        bgm_->BeginMEPause();
        me_watch_state_ = MeWatchState::kBgmFadingOut;
        me_watch_state_start_ = SDL_GetTicks();
        me_watch_fade_start_volume_ = bgm_->GetExternalVolume();
        break;
      }

      if (!bgm_->IsPlaying()) {
        bgm_->SetExternalVolume(1.0f);
        me_watch_state_ = MeWatchState::kMeNotPlaying;
        break;
      }

      const float progress = FadeProgress(me_watch_state_start_,
                                          kBgmFadeInAfterMETime);
      bgm_->SetExternalVolume(
          LerpVolume(me_watch_fade_start_volume_, 1.0f, progress));
      if (progress >= 1.0f)
        me_watch_state_ = MeWatchState::kMeNotPlaying;
      break;
    }
  }

}

}  // namespace content
