// A queued GUI message can retain a Pd weak reference after its processor dies.
// This also covers reassigning a reference between independent Pd instances.
// Run inside the VST3: the standalone's Pd library supports one instance only.
class WeakReferenceLifetimeTest : public PlugDataUnitTest {
public:
    explicit WeakReferenceLifetimeTest(PluginEditor* editor)
        : PlugDataUnitTest(editor, "Weak Reference Lifetime Test")
    {
    }

private:
    void perform() override
    {
        beginTest("Assignment unregisters from the original instance");
        auto owner = std::make_unique<PluginProcessor>();
        int first = 0, second = 0;
        pd::WeakReference reference(&first, editor->pd);
        pd::WeakReference other(&second, owner.get());
        reference = other;
        editor->pd->clearWeakReferences(&first);
        expect(reference.isValid(), "the old instance must not invalidate the reassigned reference");
        owner->clearWeakReferences(&second);
        expect(!reference.isValid(), "the new instance must invalidate the reassigned reference");

        beginTest("Queued reference outlives its processor");
        pd::WeakReference retained(&second, owner.get());
        MessageManager::callAsync([this, retained] {
            expect(!retained.isValid(), "a reference must expire with its instance");
            expect(retained.getRaw<int>() == nullptr);
            expect(!retained.get<int>(), "an expired instance must not be locked");
            pd::WeakReference copy(retained);
            expect(!copy.isValid(), "copying an expired reference must be safe");
            signalDone(true);
        });
        owner.reset();
        editor->pd->setThis();
    }
};
