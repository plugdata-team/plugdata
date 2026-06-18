#include "Dialogs/Dialogs.h"

// Walks the first-run onboarding flow.
//
// Pass 1 clicks "Next" through every page (welcome, use case, theme, panels,
// keymap, display, docs), force-painting each, until the dialog closes itself
// on the last page. Page selections are left at their defaults, so each
// page's apply() re-applies the current configuration - and every write goes
// to the isolated test settings copy anyway.
//
// Pass 2 reopens the dialog (the "reopen from settings" path), navigates
// forward and back, clicks the selection cards on the current page, and then
// leaves through "Skip & use defaults" - the cancellation path.
//
// Both passes must end with "onboarding_completed" set in the (isolated)
// settings, since both close() paths persist it.

class OnboardingTest : public PlugDataUnitTest
{
public:
    OnboardingTest(PluginEditor* editor) : PlugDataUnitTest(editor, "Onboarding Test")
    {
    }

private:
    void perform() override
    {
        beginTest("Navigate forward through every onboarding page");
        Dialogs::showOnboardingDialog(&editor->openedDialog, editor);
        clickNextUntilClosed(20);
    }

    void clickNextUntilClosed(int const stepsLeft)
    {
        auto* dialog = editor->openedDialog.get();
        if (!dialog) {
            // The last page's Next closed the dialog
            expect(SettingsFile::getInstance()->getProperty<bool>("onboarding_completed"), "completing onboarding must persist the completion flag");
            reopenAndCancel();
            return;
        }
        if (stepsLeft == 0) {
            expect(false, "onboarding must close after clicking through all pages");
            editor->openedDialog.reset(nullptr);
            signalDone(false);
            return;
        }

        dialog->createComponentSnapshot(dialog->getLocalBounds());

        auto* nextButton = TestHelpers::findButtonWithText(dialog, "Next");
        if (!nextButton)
            nextButton = TestHelpers::findButtonWithText(dialog, "Get started");
        if (nextButton) {
            nextButton->onClick();
        } else {
            expect(false, "onboarding must have an advance button");
            editor->openedDialog.reset(nullptr);
            signalDone(false);
            return;
        }

        Timer::callAfterDelay(50, [this, stepsLeft] {
            clickNextUntilClosed(stepsLeft - 1);
        });
    }

    void reopenAndCancel()
    {
        beginTest("Reopen, navigate back and forth, click cards, then skip");
        Dialogs::showOnboardingDialog(&editor->openedDialog, editor);

        Timer::callAfterDelay(50, [this] {
            auto* dialog = editor->openedDialog.get();
            if (!dialog) {
                expect(false, "onboarding must reopen");
                signalDone(false);
                return;
            }

            auto findAdvance = [dialog] {
                auto* b = TestHelpers::findButtonWithText(dialog, "Next");
                return b ? b : TestHelpers::findButtonWithText(dialog, "Get started");
            };
            auto* backButton = TestHelpers::findButtonWithText(dialog, "Back");
            if (auto* n = findAdvance())
                n->onClick(); // to the use-case page
            if (backButton)
                backButton->onClick(); // back to welcome
            if (auto* n = findAdvance())
                n->onClick(); // forward again

            // Click the selection cards on the current page (use case cards);
            // selecting one covers the card hover/selection paths. Buttons are
            // skipped so this can't navigate or close the dialog
            HeapArray<Component::SafePointer<Component>> targets;
            TestHelpers::collectComponents(dialog, targets);
            for (auto& target : targets) {
                if (auto* c = target.getComponent()) {
                    if (c->isShowing() && c->isEnabled() && !dynamic_cast<Button*>(c))
                        TestHelpers::simulateClick(c);
                }
            }

            Timer::callAfterDelay(50, [this] {
                if (auto* dialog = editor->openedDialog.get()) {
                    if (auto* skipButton = TestHelpers::findButtonWithText(dialog, "Skip & use defaults"))
                        skipButton->onClick();
                }

                Timer::callAfterDelay(50, [this] {
                    expect(editor->openedDialog == nullptr, "skip must close the onboarding dialog");
                    editor->openedDialog.reset(nullptr);
                    expect(SettingsFile::getInstance()->getProperty<bool>("onboarding_completed"), "skipping must also persist the completion flag");
                    signalDone(true);
                });
            });
        });
    }
};
