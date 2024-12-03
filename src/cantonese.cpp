#include <fcitx-utils/keysymgen.h>
#include <fcitx/inputpanel.h>

#include "cantonese.hpp"

static const std::array<fcitx::Key, 10> selectionKeys = {
    fcitx::Key{FcitxKey_1},
    fcitx::Key{FcitxKey_2},
    fcitx::Key{FcitxKey_3},
    fcitx::Key{FcitxKey_4},
    fcitx::Key{FcitxKey_5},
    fcitx::Key{FcitxKey_6},
    fcitx::Key{FcitxKey_7},
    fcitx::Key{FcitxKey_8},
    fcitx::Key{FcitxKey_9},
    fcitx::Key{FcitxKey_0},
};

static const std::array<fcitx::Key, 20> punctuation_keys = {
    fcitx::Key{FcitxKey_comma},       fcitx::Key{FcitxKey_period},
    fcitx::Key{FcitxKey_slash},       fcitx::Key{FcitxKey_colon},
    fcitx::Key{FcitxKey_apostrophe},  fcitx::Key{FcitxKey_quotedbl},
    fcitx::Key{FcitxKey_bracketleft}, fcitx::Key{FcitxKey_bracketright},
    fcitx::Key{FcitxKey_braceleft},   fcitx::Key{FcitxKey_braceright},
    fcitx::Key{FcitxKey_less},        fcitx::Key{FcitxKey_greater},
    fcitx::Key{FcitxKey_backslash},   fcitx::Key{FcitxKey_question},
    fcitx::Key{FcitxKey_semicolon},   fcitx::Key{FcitxKey_asciitilde},
    fcitx::Key{FcitxKey_parenleft},   fcitx::Key(FcitxKey_parenright),
    fcitx::Key{FcitxKey_exclam},      fcitx::Key{FcitxKey_grave},
};

static const std::array<std::string, 20> punctuations = {
    "，", "。", "…",  "：", "【", "】", "「", "」", "『", "』",
    "《", "》", "、", "？", "；", "〜", "（", "）", "！", "°",
};

class CantoneseCandidateWord : public fcitx::CandidateWord {
   public:
    CantoneseCandidateWord(CantoneseEngine* engine, std::string text)
        : engine_(engine) {
        setText(fcitx::Text(std::move(text)));
    }

    void select(fcitx::InputContext* inputContext) const override {
        inputContext->commitString(text().toString());
        auto state = inputContext->propertyFor(engine_->factory());
        state->reset();
    }

   private:
    CantoneseEngine* engine_;
};

class CantoneseCandidateList : public fcitx::CandidateList,
                               public fcitx::PageableCandidateList,
                               public fcitx::CursorMovableCandidateList {
   public:
    CantoneseCandidateList(CantoneseEngine* engine, const std::string& input)
        : input_(input) {
        setPageable(this);
        setCursorMovable(this);
        auto imec = engine->ime().candidates(input);
        for (const auto& c : imec) {
            candidates_.emplace_back(
                std::make_unique<CantoneseCandidateWord>(engine, c));
        }
        imec.clear();

        for (size_t i = 0; i < 10 && i < candidates_.size(); i++) {
            page_.emplace_back(
                std::make_tuple(std::to_string(i + 1) + ". ", i));
        }
    }

    const fcitx::Text& label(int idx) const override {
        return std::get<0>(page_[idx]);
    }

    const fcitx::CandidateWord& candidate(int idx) const override {
        return *candidates_[std::get<1>(page_[idx])];
    }

    int size() const override { return page_.size(); }

    fcitx::CandidateLayoutHint layoutHint() const override {
        return fcitx::CandidateLayoutHint::Vertical;
    }
    bool usedNextBefore() const override { return false; }

    void prev() override {
        if (!hasPrev()) {
            return;
        }
        cursor_ = 0;
        page_no_ -= 1;
        page_ = {};
        page_.reserve(10);
        for (size_t i = 0; i < 10 && i + 10 * page_no_ < candidates_.size();
             i++) {
            page_.emplace_back(std::make_tuple(
                std::to_string(i + 1) + ". ", i + 10 * page_no_));
        }
    }
    void next() override {
        if (!hasNext()) {
            return;
        }
        cursor_ = 0;
        page_no_ += 1;
        page_ = {};
        page_.reserve(10);
        for (size_t i = 0; i < 10 && i + 10 * page_no_ < candidates_.size();
             i++) {
            page_.emplace_back(std::make_tuple(
                std::to_string(i + 1) + ". ", i + 10 * page_no_));
        }
    }

    bool hasPrev() const override { return page_no_ > 0; }
    bool hasNext() const override { return page_no_ < candidates_.size() / 10; }

    void prevCandidate() override {
        if (hasNext()) {
            cursor_ = (cursor_ + 9) % 10;
        } else {
            cursor_ = (cursor_ + candidates_.size() % 10 - 1) % 10;
        }
    }
    void nextCandidate() override {
        if (hasNext()) {
            cursor_ = (cursor_ + 1) % 10;
        } else {
            cursor_ = (cursor_ + 1) % (candidates_.size() % 10);
        }
    }

    int cursorIndex() const override { return cursor_; }

   private:
    std::vector<std::tuple<fcitx::Text, size_t>> page_;
    std::vector<std::unique_ptr<CantoneseCandidateWord>> candidates_;
    size_t page_no_ = 0;
    int cursor_ = 0;
    std::string input_;
};

void CantoneseState::keyEvent(fcitx::KeyEvent& event) {
    size_t idx = event.key().keyListIndex(punctuation_keys);
    if (idx < punctuation_keys.size()) {
        event.accept();
        ic_->commitString(punctuations[idx]);
        return;
    }
    if (buffer_.empty()) {
        if (event.key().check(FcitxKey_space)) {
            return;
        }
    }
    if (not buffer_.empty()) {
        if (event.key().check(FcitxKey_BackSpace)) {
            buffer_.backspace();
            updateUI();
            return event.filterAndAccept();
        }
        if (event.key().check(FcitxKey_Return)) {
            ic_->commitString(buffer_.userInput());
            reset();
            return event.filterAndAccept();
        }
        if (event.key().check(FcitxKey_space)) {
            ic_->commitString(buffer_.userInput());
            reset();
            return;
        }
    }
    if (auto candidateList = ic_->inputPanel().candidateList()) {
        int idx = event.key().keyListIndex(selectionKeys);
        if (idx >= 0 && idx < candidateList->size()) {
            event.accept();
            candidateList->candidate(idx).select(ic_);
            return;
        }
        if (not candidateList->empty()) {
            if (event.key().check(FcitxKey_ISO_Left_Tab) or
                event.key().check(FcitxKey_Up)) {
                candidateList->toCursorMovable()->prevCandidate();
                ic_->updateUserInterface(
                    fcitx::UserInterfaceComponent::InputPanel);
                return event.filterAndAccept();
            }
            if (event.key().check(FcitxKey_Tab) or
                event.key().check(FcitxKey_Down)) {
                candidateList->toCursorMovable()->nextCandidate();
                ic_->updateUserInterface(
                    fcitx::UserInterfaceComponent::InputPanel);
                return event.filterAndAccept();
            }
            if (candidateList->toPageable()->hasNext() and
                event.key().check(FcitxKey_Right)) {
                candidateList->toPageable()->next();
                ic_->updateUserInterface(
                    fcitx::UserInterfaceComponent::InputPanel);
                return event.filterAndAccept();
            }
            if (candidateList->toPageable()->hasPrev() and
                event.key().check(FcitxKey_Left)) {
                candidateList->toPageable()->prev();
                ic_->updateUserInterface(
                    fcitx::UserInterfaceComponent::InputPanel);
                return event.filterAndAccept();
            }
            if (event.key().check(FcitxKey_space)) {
                event.accept();
                candidateList->candidate(candidateList->cursorIndex())
                    .select(ic_);
                return;
            }
        }
    }
    if (event.key().isSimple()) {
        buffer_.type(event.key().sym());
        updateUI();
        return event.filterAndAccept();
    }
}

void CantoneseState::updateUI() {
    auto& inputPanel = ic_->inputPanel();
    inputPanel.reset();

    if (buffer_.size()) {
        inputPanel.setCandidateList(std::make_unique<CantoneseCandidateList>(
            engine_, buffer_.userInput()));
    }

    if (ic_->capabilityFlags().test(fcitx::CapabilityFlag::Preedit)) {
        fcitx::Text preedit(
            buffer_.userInput(), fcitx::TextFormatFlag::HighLight);
        inputPanel.setClientPreedit(preedit);
    } else {
        fcitx::Text preedit(buffer_.userInput());
        inputPanel.setPreedit(preedit);
    }
    ic_->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
    ic_->updatePreedit();
}

CantoneseEngine::CantoneseEngine(fcitx::Instance* instance)
    : instance_(instance),
      factory_([this](fcitx::InputContext& ic) {
          return new CantoneseState(this, &ic);
      }),
      ime_(
          IME(std::filesystem::path(std::getenv("HOME")) /
              ".local/share/fcitx5/cantonese/data")) {
    instance->inputContextManager().registerProperty(
        "cantoneseState", &factory_);
}

void CantoneseEngine::keyEvent(
    const fcitx::InputMethodEntry& entry,
    fcitx::KeyEvent& keyEvent) {
    FCITX_UNUSED(entry);
    if (keyEvent.isRelease() || keyEvent.key().states()) {
        return;
    }
    auto ic = keyEvent.inputContext();
    auto* state = ic->propertyFor(&factory_);
    state->keyEvent(keyEvent);
}

void CantoneseEngine::reset(
    const fcitx::InputMethodEntry&,
    fcitx::InputContextEvent& event) {
    auto* state = event.inputContext()->propertyFor(&factory_);
    state->reset();
}

FCITX_ADDON_FACTORY(CantoneseEngineFactory);
