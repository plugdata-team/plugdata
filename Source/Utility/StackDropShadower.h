#include "Utility/StackShadow.h"
#include "Utility/Config.h"

#if JUCE_WINDOWS
#    include <juce_gui_basics/native/juce_ScopedThreadDPIAwarenessSetter_windows.h>
#endif

#include <juce_gui_basics/detail/juce_WindowingHelpers.h>
#include <juce_audio_processors/juce_audio_processors.h>

class StackDropShadower : private ComponentListener {
public:
    /** Creates a DropShadower. */
    explicit StackDropShadower(DropShadow const& shadowType, int cornerRadius = 0)
        : shadow(shadowType)
        , shadowCornerRadius(cornerRadius)
    {
    }

    /** Destructor. */
    ~StackDropShadower() override
    {
        if (virtualDesktopWatcher != nullptr)
            virtualDesktopWatcher->removeListener(this);

        if (owner != nullptr) {
            owner->removeComponentListener(this);
            owner = nullptr;
        }

        updateParent();

        ScopedValueSetter<bool> const setter(reentrant, true);
        shadowWindows.clear();
    }

    /** Attaches the DropShadower to the component you want to shadow. */
    void setOwner(Component* componentToFollow)
    {
        if (componentToFollow != owner) {
            if (owner != nullptr)
                owner->removeComponentListener(this);

            owner = componentToFollow;

            if (!owner) {
                for (int i = 4; --i >= 0;) {
                    // there seem to be rare situations where the dropshadower may be deleted by
                    // callbacks during this loop, so use a weak ref to watch out for this..
                    WeakReference<Component> sw(shadowWindows[i]);

                    if (sw != nullptr) {
                        sw->setVisible(false);
                    }
                }
                return;
            }

            updateParent();
            owner->addComponentListener(this);

            // The visibility of `owner` is transitively affected by the visibility of its parents. Thus we need to trigger the
            // componentVisibilityChanged() event in case it changes for any of the parents.
            visibilityChangedListener = std::make_unique<ParentVisibilityChangedListener>(*owner,
                static_cast<ComponentListener&>(*this));

            virtualDesktopWatcher = std::make_unique<VirtualDesktopWatcher>(*owner);
            virtualDesktopWatcher->addListener(this, [this]() { updateShadows(); });

            updateShadows();
        }
    }

    void repaint()
    {
        for (int i = 4; --i >= 0;) {
            // there seem to be rare situations where the dropshadower may be deleted by
            // callbacks during this loop, so use a weak ref to watch out for this..
            WeakReference<Component> sw(shadowWindows[i]);
            if (sw != nullptr) {
                sw->repaint();
            }
        }
    }

private:
    void componentMovedOrResized(Component& c, bool, bool) override
    {
        if (owner == &c)
            updateShadows();
    }

    void componentBroughtToFront(Component& c) override
    {
        if (owner == &c)
            updateShadows();
    }

    void componentChildrenChanged(Component&) override
    {
        updateShadows();
    }

    void componentParentHierarchyChanged(Component& c) override
    {
        if (owner == &c) {
            updateParent();
            updateShadows();
        }
    }

    void componentVisibilityChanged(Component& c) override
    {
        if (owner == &c)
            updateShadows();
    }

    void updateShadows()
    {
        if (reentrant)
            return;

        ScopedValueSetter<bool> const setter(reentrant, true);

        if (owner != nullptr
            && owner->isShowing()
            && owner->getWidth() > 0 && owner->getHeight() > 0
            && (ProjectInfo::canUseSemiTransparentWindows() || owner->getParentComponent() != nullptr)
            && (virtualDesktopWatcher == nullptr || !virtualDesktopWatcher->shouldHideDropShadow())) {
            while (shadowWindows.size() < 4)
                shadowWindows.add(new ShadowWindow(owner, shadow, shadowCornerRadius));

            int const shadowEdge = jmax(shadow.offset.x, shadow.offset.y) + shadow.radius * 2.0f;

            // Reduce by shadow radius. We'll compensate later by drawing the shadow twice as large
            auto b = owner->getBounds().reduced(shadow.radius);
            int const x = b.getX();
            int const y = b.getY() - shadowEdge;
            int const w = b.getWidth();
            int const h = b.getHeight() + shadowEdge + shadowEdge;

            for (int i = 4; --i >= 0;) {
                // there seem to be rare situations where the dropshadower may be deleted by
                // callbacks during this loop, so use a weak ref to watch out for this..
                WeakReference<Component> sw(shadowWindows[i]);

                if (sw != nullptr) {
                    sw->setVisible(true);
                    sw->setAlwaysOnTop(owner->isAlwaysOnTop());

                    if (sw == nullptr)
                        return;

                    switch (i) {
                    case 0:
                        sw->setBounds(x - shadowEdge, y, shadowEdge, h);
                        break;
                    case 1:
                        sw->setBounds(x + w, y, shadowEdge, h);
                        break;
                    case 2:
                        sw->setBounds(x, y, w, shadowEdge);
                        break;
                    case 3:
                        sw->setBounds(x, b.getBottom(), w, shadowEdge);
                        break;
                    default:
                        break;
                    }

                    if (sw == nullptr)
                        return;

                    sw->toBehind(owner.get());
                }
            }
        } else {
            shadowWindows.clear();
        }
    }

    void updateParent()
    {
        if (Component* p = lastParentComp)
            p->removeComponentListener(this);

        lastParentComp = owner != nullptr ? owner->getParentComponent() : nullptr;

        if (Component* p = lastParentComp)
            p->addComponentListener(this);
    }

    class ShadowWindow : public Component {
    public:
        ShadowWindow(Component* comp, DropShadow const& ds, int cornerRadius)
            : target(comp)
            , shadow(ds)
            , shadowCornerRadius(cornerRadius)
        {
            setVisible(true);
            setAccessible(false);
            setInterceptsMouseClicks(false, false);

            if (comp->isOnDesktop()) {
#if JUCE_WINDOWS
                auto const scope = [&]() -> std::unique_ptr<ScopedThreadDPIAwarenessSetter> {
                    if (comp != nullptr)
                        if (auto* handle = comp->getWindowHandle())
                            return std::make_unique<ScopedThreadDPIAwarenessSetter>(handle);

                    return nullptr;
                }();
#endif

                setSize(1, 1); // to keep the OS happy by not having zero-size windows
                addToDesktop(ComponentPeer::windowIgnoresMouseClicks
                    | ComponentPeer::windowIgnoresKeyPresses);
            } else if (Component* const parent = comp->getParentComponent()) {
                parent->addChildComponent(this);
            }
        }

        void paint(Graphics& g) override
        {
            if (auto* c = dynamic_cast<TopLevelWindow*>(target.get())) {
                auto shadowPath = Path();
                shadowPath.addRoundedRectangle(getLocalArea(c, c->getLocalBounds().reduced(shadow.radius * 0.9f)).toFloat(), windowCornerRadius);

                auto radius = c->isActiveWindow() ? shadow.radius * 2.0f : shadow.radius * 1.5f;
                StackShadow::renderDropShadow(g, shadowPath, shadow.colour, radius, shadow.offset);
            } else {
                auto shadowPath = Path();
                shadowPath.addRoundedRectangle(getLocalArea(target, target->getLocalBounds()).toFloat(), shadowCornerRadius);
                StackShadow::renderDropShadow(g, shadowPath, shadow.colour, shadow.radius, shadow.offset);
            }
        }

        void resized() override
        {
            repaint(); // (needed for correct repainting)
        }

        float getDesktopScaleFactor() const override
        {
            if (target != nullptr)
                return target->getDesktopScaleFactor();

            return Component::getDesktopScaleFactor();
        }

    private:
        WeakReference<Component> target;
        DropShadow shadow;

        inline static float const windowCornerRadius = 7.5f;

        int shadowCornerRadius;

        JUCE_DECLARE_NON_COPYABLE(ShadowWindow)
    };

    class VirtualDesktopWatcher final : public ComponentListener
        , private Timer {
    public:
        explicit VirtualDesktopWatcher(Component& c)
            : component(&c)
        {
            component->addComponentListener(this);
            update();
        }

        ~VirtualDesktopWatcher() override
        {
            stopTimer();

            if (auto* c = component.get())
                c->removeComponentListener(this);
        }

        bool shouldHideDropShadow() const
        {
            return hasReasonToHide;
        }

        void addListener(void* listener, std::function<void()> cb)
        {
            listeners[listener] = std::move(cb);
        }

        void removeListener(void* listener)
        {
            listeners.erase(listener);
        }

        void componentParentHierarchyChanged(Component& c) override
        {
            if (component.get() == &c)
                update();
        }

    private:
        void update()
        {
            auto const newHasReasonToHide = [this]() {
                if (!component.wasObjectDeleted() && isWindows && component->isOnDesktop()) {
                    startTimerHz(5);
#if JUCE_BSD
                    return false;
#else
                    return !detail::WindowingHelpers::isWindowOnCurrentVirtualDesktop(component->getWindowHandle());
#endif
                }

                stopTimer();
                return false;
            }();

            if (std::exchange(hasReasonToHide, newHasReasonToHide) != newHasReasonToHide)
                for (auto& l : listeners)
                    l.second();
        }

        void timerCallback() override
        {
            update();
        }

        WeakReference<Component> component;
        bool const isWindows = (SystemStats::getOperatingSystemType() & SystemStats::Windows) != 0;
        bool hasReasonToHide = false;
        std::map<void*, std::function<void()>> listeners;
    };

    class ParentVisibilityChangedListener : public ComponentListener {
    public:
        ParentVisibilityChangedListener(Component& r, ComponentListener& l)
            : root(&r)
            , listener(&l)
        {
            updateParentHierarchy();
        }

        ~ParentVisibilityChangedListener() override
        {
            for (auto& compEntry : observedComponents)
                if (auto* comp = compEntry.get())
                    comp->removeComponentListener(this);
        }

        void componentVisibilityChanged(Component& component) override
        {
            if (root != &component)
                listener->componentVisibilityChanged(*root);
        }

        void componentParentHierarchyChanged(Component& component) override
        {
            if (root == &component)
                updateParentHierarchy();
        }

    private:
        class ComponentWithWeakReference {
        public:
            explicit ComponentWithWeakReference(Component& c)
                : ptr(&c)
                , ref(&c)
            {
            }

            Component* get() const { return ref.get(); }

            bool operator<(ComponentWithWeakReference const& other) const { return ptr < other.ptr; }

        private:
            Component* ptr;
            WeakReference<Component> ref;
        };

        void updateParentHierarchy()
        {
            auto const lastSeenComponents = std::exchange(observedComponents, [&] {
                std::set<ComponentWithWeakReference> result;

                for (auto node = root; node != nullptr; node = node->getParentComponent())
                    result.emplace(*node);

                return result;
            }());

            auto const withDifference = [](auto const& rangeA, auto const& rangeB, auto&& callback) {
                std::vector<ComponentWithWeakReference> result;
                std::set_difference(rangeA.begin(), rangeA.end(), rangeB.begin(), rangeB.end(), std::back_inserter(result));

                for (const auto& item : result)
                    if (auto* c = item.get())
                        callback(*c);
            };

            withDifference(lastSeenComponents, observedComponents, [this](auto& comp) { comp.removeComponentListener(this); });
            withDifference(observedComponents, lastSeenComponents, [this](auto& comp) { comp.addComponentListener(this); });
        }

        Component* root = nullptr;
        ComponentListener* listener = nullptr;
        std::set<ComponentWithWeakReference> observedComponents;

        JUCE_DECLARE_NON_COPYABLE(ParentVisibilityChangedListener)
        JUCE_DECLARE_NON_MOVEABLE(ParentVisibilityChangedListener)
    };

    WeakReference<Component> owner;
    OwnedArray<Component> shadowWindows;
    DropShadow shadow;
    bool reentrant = false;
    WeakReference<Component> lastParentComp;

    int shadowCornerRadius;

    std::unique_ptr<ParentVisibilityChangedListener> visibilityChangedListener;
    std::unique_ptr<VirtualDesktopWatcher> virtualDesktopWatcher;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StackDropShadower)
    JUCE_DECLARE_WEAK_REFERENCEABLE(StackDropShadower)
};
