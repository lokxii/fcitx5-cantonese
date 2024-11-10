#ifndef _FCITX5_QUWEI_QUWEI_H_
#define _FCITX5_QUWEI_QUWEI_H_

#include <fcitx-utils/inputbuffer.h>
#include <fcitx/addonfactory.h>
#include <fcitx/addonmanager.h>
#include <fcitx/inputcontextproperty.h>
#include <fcitx/inputmethodengine.h>
#include <fcitx/instance.h>

#include "Cantonese-IME/ime.hpp"

class CantoneseEngine;

class CantoneseState : public fcitx::InputContextProperty {
   public:
    CantoneseState(CantoneseEngine* engine, fcitx::InputContext* ic)
        : engine_(engine), ic_(ic) {}

    void keyEvent(fcitx::KeyEvent& keyEvent);
    void updateUI();
    void reset() {
        buffer_.clear();
        updateUI();
    }

   private:
    CantoneseEngine* engine_;
    fcitx::InputContext* ic_;
    fcitx::InputBuffer buffer_{
        {fcitx::InputBufferOption::AsciiOnly,
         fcitx::InputBufferOption::FixedCursor}};
};

class CantoneseEngine : public fcitx::InputMethodEngineV2 {
   public:
    CantoneseEngine(fcitx::Instance* instance);
    void keyEvent(
        const fcitx::InputMethodEntry& entry,
        fcitx::KeyEvent& keyEvent) override;
    void reset(const fcitx::InputMethodEntry&, fcitx::InputContextEvent& event)
        override;
    auto factory() const { return &factory_; }
    auto instance() const { return instance_; }
    auto& ime() { return ime_; }

   private:
    fcitx::Instance* instance_;
    fcitx::FactoryFor<CantoneseState> factory_;
    IME ime_;
};

class CantoneseEngineFactory : public fcitx::AddonFactory {
    fcitx::AddonInstance* create(fcitx::AddonManager* manager) override {
        return new CantoneseEngine(manager->instance());
    }
};
#endif  // _FCITX5_QUWEI_QUWEI_H_
