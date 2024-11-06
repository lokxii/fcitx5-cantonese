#include "cantonese.hpp"
#include <fcitx/inputpanel.h>

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
        for (size_t i = 0; i < input_.size(); i++) {
            candidates_.emplace_back(std::make_unique<CantoneseCandidateWord>(
                engine, std::string(1, input_[i])));
        }

        for (size_t i = 0; i < 10 && i < input_.size(); i++) {
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
    FCITX_INFO() << "Buffer: " << buffer_.userInput();
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
    : instance_(instance), factory_([this](fcitx::InputContext& ic) {
          return new CantoneseState(this, &ic);
      }) {
    // TODO: Read csv build in memory db
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