#include "Dialogs/Deken.h"
#include "Dialogs/PatchStore.h"

// Drives the patch store and the Deken externals browser entirely offline.
//
// Both normally fetch a catalog over HTTPS and install into the user's app
// data folders. The ENABLE_TESTING seams added to DownloadPool and
// PackageManager let the catalog come from injected mock data (or a forced
// failure), and redirect Deken installs to a temp folder. The actual
// download/extract path is exercised with a local file:// zip.
//
// PatchStore: a mock catalog (free + paid + malformed entries) populates the
// browser; search filtering, opening a patch, going back, the
// malformed-metadata path (empty catalog) and the network-failure path
// (databaseDownloadFailed) are all covered.
//
// Deken: a mock package tree populates the list; name/description/object
// searching, the installed/explore toggles, a real install from a local zip
// followed by uninstall, the failure path and the malformed-data path are
// covered.

class StoreDekenTest : public PlugDataUnitTest
{
public:
    StoreDekenTest(PluginEditor* editor) : PlugDataUnitTest(editor, "Store & Deken Test")
    {
    }

private:
    static String mockStoreCatalog()
    {
        // Mirrors the structure of plugdata.org/store.json
        return R"JSON({
            "Patches": [
                { "Title": "Test Synth", "Author": "tester", "Release date": "2024-01-01",
                  "Download": "https://example.invalid/testsynth.zip", "Description": "A test synthesizer patch",
                  "Price": "Free", "StoreThumb": "thumb1", "Version": "1.0" },
                { "Title": "Paid Reverb", "Author": "tester", "Release date": "2024-02-01",
                  "Download": "https://example.invalid/reverb.zip", "Description": "A lush reverb",
                  "Price": "5 USD", "StoreThumb": "thumb2", "Version": "2.0" },
                { "Title": "Drum Machine", "Author": "someone", "Release date": "2023-12-01",
                  "Download": "https://example.invalid/drums.zip", "Description": "Beats and grooves",
                  "Price": "Free", "StoreThumb": "thumb3", "Version": "1.1" }
            ]
        })JSON";
    }

    void perform() override
    {
        testPatchStore();
    }

    // ---------------- Patch store ----------------

    void testPatchStore()
    {
        beginTest("Patch store catalog, search and navigation");

        DownloadPool::mockStoreShouldFail = false;
        DownloadPool::mockStoreJson = mockStoreCatalog();

        store = std::make_unique<PatchStore>();
        editor->addAndMakeVisible(store.get());
        store->setBounds(0, 0, 850, 550);

        // downloadDatabase() runs on a pool thread then calls back async
        Timer::callAfterDelay(300, [this] {
            store->createComponentSnapshot(store->getLocalBounds());

            // Search: type into the store's search field
            if (auto* input = TestHelpers::findChildOfType<TextEditor>(store.get())) {
                input->setText("Reverb", sendNotification);
                store->createComponentSnapshot(store->getLocalBounds());
                input->setText("no_such_patch_xyz", sendNotification); // empty result
                input->setText("", sendNotification);
            }

            // Click the first patch tile to open its detail view, then exercise
            // the store's buttons (back, search, refresh)
            TestHelpers::clickThrough(store.get());
            store->createComponentSnapshot(store->getLocalBounds());

            testPatchStoreMalformed();
        });
    }

    void testPatchStoreMalformed()
    {
        beginTest("Patch store malformed metadata and failure");

        // Malformed JSON -> parsed catalog is empty, must not crash
        DownloadPool::mockStoreJson = "{ this is not valid json ][";
        DownloadPool::getInstance()->downloadDatabase();

        Timer::callAfterDelay(200, [this] {
            // Forced network failure -> databaseDownloadFailed path
            DownloadPool::mockStoreShouldFail = true;
            DownloadPool::getInstance()->downloadDatabase();

            Timer::callAfterDelay(200, [this] {
                DownloadPool::mockStoreShouldFail = false;
                DownloadPool::mockStoreJson.clear();

                editor->removeChildComponent(store.get());
                store.reset();

                testDeken();
            });
        });
    }

    // ---------------- Deken ----------------

    static MemoryBlock mockDekenTree()
    {
        // Mirrors the pre-parsed deken .bin: a tree of packages, each with
        // version children carrying metadata and an Objects child
        ValueTree root("Packages");
        auto makePackage = [](String const& name, String const& desc, String const& url, StringArray const& objects) {
            ValueTree pkg(name);
            pkg.setProperty("Name", name, nullptr);
            ValueTree version("Version");
            version.setProperty("Author", "deken-tester", nullptr);
            version.setProperty("Timestamp", "2024-01-01 00:00:00", nullptr);
            version.setProperty("URL", url, nullptr);
            version.setProperty("Description", desc, nullptr);
            version.setProperty("Version", "1.0", nullptr);
            ValueTree objectsTree("Objects");
            for (auto const& obj : objects) {
                ValueTree o("Object");
                o.setProperty("Name", obj, nullptr);
                objectsTree.addChild(o, -1, nullptr);
            }
            version.addChild(objectsTree, -1, nullptr);
            pkg.addChild(version, -1, nullptr);
            return pkg;
        };

        root.addChild(makePackage("else", "A large library of objects", "https://example.invalid/else.zip", { "osc", "saw", "clip" }), -1, nullptr);
        root.addChild(makePackage("cyclone", "Max/MSP compatibility", "https://example.invalid/cyclone.zip", { "coll", "zl" }), -1, nullptr);

        MemoryBlock block;
        MemoryOutputStream stream(block, false);
        root.writeToStream(stream);
        return block;
    }

    void testDeken()
    {
        beginTest("Deken catalog, search and install");

        // Redirect installs to a temp folder so the user's Externals stays clean
        dekenTempDir = File::getSpecialLocation(File::tempDirectory).getChildFile("plugdata_deken_test");
        dekenTempDir.deleteRecursively();
        dekenTempDir.createDirectory();
        PackageManager::filesystem = dekenTempDir;

        PackageManager::mockShouldFail = false;
        PackageManager::mockPackageData = mockDekenTree();

        deken = std::make_unique<Deken>();
        editor->addAndMakeVisible(deken.get());
        deken->setBounds(0, 0, 850, 550);

        // The package manager refreshes on a thread; wait for it
        waitForDekenIdle(20, [this] {
            deken->createComponentSnapshot(deken->getLocalBounds());

            // Switch to explore + search and filter by name, description, object
            if (auto* searchButton = TestHelpers::findButtonWithText(deken.get(), "Search")) {
                searchButton->setToggleState(true, sendNotification);
            }
            if (auto* input = TestHelpers::findChildOfType<TextEditor>(deken.get())) {
                input->setText("else", sendNotification);       // name match
                input->setText("compatibility", sendNotification); // description match
                input->setText("coll", sendNotification);        // object match
                input->setText("no_such_package_xyz", sendNotification); // empty
                input->setText("", sendNotification);
            }
            TestHelpers::clickThrough(deken.get());

            testDekenInstall();
        });
    }

    void testDekenInstall()
    {
        beginTest("Deken install from a local archive, then uninstall");

        // Build a small valid zip that looks like an installed external
        auto const zipFile = dekenTempDir.getChildFile("mockpkg.zip");
        {
            ZipFile::Builder builder;
            auto const innerFile = dekenTempDir.getChildFile("mock_object.pd");
            innerFile.replaceWithText("#N canvas 0 0 200 200 12;\n#X obj 20 20 mock_object;\n");
            builder.addFile(innerFile, 5, "mockpkg/mock_object.pd");
            FileOutputStream zipStream(zipFile);
            builder.writeToStream(zipStream, nullptr);
        }

        PackageInfo mockPackage("mockpkg", "tester", "2024-01-01", URL(zipFile).toString(false),
                                "A mock package", "1.0", { "mock_object" });

        auto* manager = PackageManager::getInstance();
        bool const existedBefore = manager->packageExists(mockPackage);
        expect(!existedBefore, "the mock package must not be installed before the test");

        auto* task = manager->install(mockPackage);
        expect(task != nullptr, "install must start a download task");

        // Poll for the install to finish, then verify and uninstall
        waitForCondition(30, [manager, mockPackage] { return manager->packageExists(mockPackage); }, [this, manager, mockPackage] {
            bool const installed = manager->packageExists(mockPackage);
            expect(installed, "installing from a local archive must register the package");
            expect(dekenTempDir.getChildFile("mockpkg").exists(), "the package files must be extracted into the install folder");

            // Uninstalling must remove it from the register
            manager->uninstall(mockPackage);
            expect(!manager->packageExists(mockPackage), "uninstall must remove the package from the register");

            // Install a package whose archive doesn't exist -> download fails gracefully
            PackageInfo brokenPackage("brokenpkg", "tester", "2024-01-01",
                                      "https://example.invalid/does_not_exist.zip", "broken", "1.0", { });
            manager->install(brokenPackage);

            Timer::callAfterDelay(400, [this] { testDekenFailures(); });
        });
    }

    void testDekenFailures()
    {
        beginTest("Deken failure and malformed data");

        // Forced connection failure
        PackageManager::mockShouldFail = true;
        PackageManager::getInstance()->update();

        waitForDekenIdle(20, [this] {
            // Malformed catalog data -> invalid tree -> empty package list
            PackageManager::mockShouldFail = false;
            PackageManager::mockPackageData = MemoryBlock("not a valid valuetree", 21);
            PackageManager::getInstance()->update();

            waitForDekenIdle(20, [this] {
                finish();
            });
        });
    }

    void finish()
    {
        editor->removeChildComponent(deken.get());
        deken.reset();

        // Reset the seams so nothing leaks into other tests
        PackageManager::mockPackageData = MemoryBlock();
        PackageManager::mockShouldFail = false;

        if (dekenTempDir.exists())
            dekenTempDir.deleteRecursively();

        signalDone(true);
    }

    // Polls a predicate up to `attempts` times (100ms apart), then runs done
    void waitForCondition(int const attempts, std::function<bool()> const& predicate, std::function<void()> const& done)
    {
        if (attempts <= 0 || predicate()) {
            done();
            return;
        }
        Timer::callAfterDelay(100, [this, attempts, predicate, done] {
            waitForCondition(attempts - 1, predicate, done);
        });
    }

    // Waits until the deken package-manager background thread is idle
    void waitForDekenIdle(int const attempts, std::function<void()> const& done)
    {
        waitForCondition(attempts, [] {
            auto* manager = PackageManager::getInstanceWithoutCreating();
            return manager && !manager->isThreadRunning();
        }, done);
    }

    std::unique_ptr<PatchStore> store;
    std::unique_ptr<Deken> deken;
    File dekenTempDir;
};
