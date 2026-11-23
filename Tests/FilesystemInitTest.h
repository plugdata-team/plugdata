#include "Utility/Decompress.h"
#include "Utility/OSUtils.h"
#include <BinaryData.h>

#include <string>
#include <unordered_map>
#include <unordered_set>

// End-to-end test for the startup filesystem initialisation (see
// PluginProcessor::initialiseFilesystem). DecompressTest covers the tar.xz
// extraction in isolation with a crafted archive; this test exercises the whole
// init sequence against the *real* bundled filesystem (BinaryData::Filesystem):
//
//   1. extract the compressed layout into a temp "Versions" dir,
//   2. move the extracted "plugdata_version" dir into place (versionDataDir),
//   3. create the Abstractions/Documentation/Extra directory junctions (Windows)
//      or symlinks (macOS/Linux) that point the home dir at the version dir.
//
// Historically this has been flaky on Windows (incomplete extraction, dangling
// or mis-targeted junctions), so the sequence is run many times over and each
// run is checked two ways:
//
//   * the extracted tree matches the compressed layout exactly - every archive
//     entry is present with the right size and bytes, and nothing extra is left
//     behind (catches missing / truncated / partial / leftover files),
//   * every linked directory resolves and the files inside it are readable
//     *through* the junction/symlink with identical content (catches dangling
//     or wrongly-targeted links).
//
// The "expected" layout is derived independently of extractTar(): the archive is
// decompressed and its tar walked here to build the reference set, so a bug in
// the extractor shows up as a mismatch rather than being masked.
//
// This mirrors the orchestration in PluginProcessor::initialiseFilesystem; keep
// the two in sync if the init logic changes.

class FilesystemInitTest final : public PlugDataUnitTest {
public:
    FilesystemInitTest(PluginEditor* editor)
        : PlugDataUnitTest(editor, "Filesystem Init Test")
    {
    }

private:
    struct ExpectedFile {
        int64 size = 0;
        uint64 hash = 0;
    };

    // FNV-1a over a byte range, used to compare archive bytes against disk bytes
    static uint64 hashBytes(uint8_t const* data, size_t const len)
    {
        uint64 h = 1469598103934665603ULL;
        for (size_t i = 0; i < len; ++i) {
            h ^= data[i];
            h *= 1099511628211ULL;
        }
        return h;
    }

    static std::string stripLeadingDotSlash(std::string s)
    {
        while (s.rfind("./", 0) == 0)
            s.erase(0, 2);
        return s;
    }

    // Map a tar entry name ("plugdata_version/Abstractions/foo.pd") to its path
    // relative to the extracted version dir ("Abstractions/foo.pd"). Returns false
    // for the root dir itself or anything outside it.
    static bool toRelativePath(std::string const& name, std::string const& rootName, std::string& relPath)
    {
        auto n = stripLeadingDotSlash(name);
        while (!n.empty() && n.back() == '/')
            n.pop_back();

        if (n == rootName)
            return false; // the version dir itself

        auto const prefix = rootName + "/";
        if (n.rfind(prefix, 0) != 0)
            return false; // unexpected entry outside the root

        relPath = n.substr(prefix.size());
        return !relPath.empty();
    }

    // Walk a decompressed tar, collecting the regular files and directories it
    // declares. Deliberately independent of Decompress::extractTar so it can act
    // as the reference the extractor is checked against. Returns false if the
    // archive is malformed (which would be a real problem with the bundled data).
    bool buildExpectedLayout(uint8_t const* data, size_t const size,
        std::string& rootName,
        std::unordered_map<std::string, ExpectedFile>& files,
        std::unordered_set<std::string>& dirs)
    {
        size_t offset = 0;
        std::string longLinkName, paxPath, paxLinkPath;

        auto deriveRoot = [&rootName](std::string const& name) {
            if (!rootName.empty())
                return;
            auto n = stripLeadingDotSlash(name);
            auto const slash = n.find('/');
            rootName = slash == std::string::npos ? n : n.substr(0, slash);
        };

        while (offset + 512 <= size) {
            uint8_t const* header = data + offset;
            if (header[0] == '\0')
                break; // end of archive

            std::string name;
            if (!longLinkName.empty()) {
                name = longLinkName;
                longLinkName.clear();
            } else if (!paxPath.empty()) {
                name = paxPath;
                paxPath.clear();
            } else {
                char nameField[101] = { 0 };
                std::memcpy(nameField, header, 100);
                name = std::string(nameField);

                char prefixField[156] = { 0 };
                std::memcpy(prefixField, header + 345, 155);
                std::string const prefix(prefixField);
                if (!prefix.empty())
                    name = prefix + "/" + name;
            }

            if (name.empty())
                break;

            // Trim trailing whitespace (name already stops at the first NUL)
            auto const lastGood = name.find_last_not_of(" \t\n\r\f\v");
            name.erase(lastGood == std::string::npos ? 0 : lastGood + 1);

            size_t const fileSize = std::strtoull(reinterpret_cast<char const*>(header + 124), nullptr, 8);
            char const typeFlag = header[156];
            size_t const paddedEntry = 512 + ((fileSize + 511) & ~static_cast<size_t>(511));

            if (typeFlag == 'L') {
                size_t const fileOffset = offset + 512;
                if (fileOffset + fileSize <= size) {
                    longLinkName.assign(reinterpret_cast<char const*>(data + fileOffset), fileSize);
                    if (!longLinkName.empty() && longLinkName.back() == '\0')
                        longLinkName.pop_back();
                }
                offset += paddedEntry;
                continue;
            }

            if (typeFlag == 'x' || name.find("PaxHeaders.") == 0) {
                size_t const fileOffset = offset + 512;
                if (fileOffset + fileSize <= size) {
                    auto [path, linkpath] = Decompress::parsePaxPathAndLink(data + fileOffset, fileSize);
                    paxPath = path;
                    paxLinkPath = linkpath;
                }
                offset += paddedEntry;
                continue;
            }

            deriveRoot(name);
            paxLinkPath.clear();

            std::string relPath;
            bool const underRoot = toRelativePath(name, rootName, relPath);

            if (typeFlag == '5') {
                if (underRoot)
                    dirs.insert(relPath);
            } else if (typeFlag == '0' || typeFlag == '\0') {
                size_t const fileOffset = offset + 512;
                if (fileSize > size - fileOffset)
                    return false; // declared size runs past the archive
                if (underRoot)
                    files[relPath] = { static_cast<int64>(fileSize), hashBytes(data + fileOffset, fileSize) };
            }
            // Symlink/hardlink entries ('2'/'1') don't occur in the bundled
            // filesystem and aren't extracted on Windows, so they're ignored.

            offset += paddedEntry;
        }

        return !rootName.empty() && !files.empty();
    }

    // Replicates PluginProcessor::initialiseFilesystem's link creation: wipe any
    // existing link, then create a junction (Windows) or symlink (otherwise),
    // retrying a few times.
    static bool createLink(File const& linkPath, File const& targetPath, int const maxRetries = 3)
    {
        for (int retry = 0; retry < maxRetries; ++retry) {
            if (linkPath.exists())
                linkPath.deleteRecursively();

#if JUCE_WINDOWS
            if (OSUtils::createJunction(linkPath.getFullPathName().replaceCharacters("/", "\\").toStdString(),
                    targetPath.getFullPathName().toStdString()))
                return true;
#else
            if (targetPath.createSymbolicLink(linkPath, true))
                return true;
#endif
            if (retry < maxRetries - 1)
                Thread::sleep(50);
        }
        return false;
    }

    static int envInt(char const* name, int const fallback)
    {
        auto const value = SystemStats::getEnvironmentVariable(name, {});
        return value.isNotEmpty() ? value.getIntValue() : fallback;
    }

    void perform() override
    {
        beginTest("Extract the bundled filesystem and build the reference layout");

        bool allPassed = true;
        auto check = [this, &allPassed](bool const condition, String const& description) {
            expect(condition, description);
            allPassed = allPassed && condition;
        };

        auto const resource = BinaryData::getResource(BinaryData::Filesystem);
        if (resource.empty()) {
            // Without the bundled resource file (plugdata-resources.bin next to the
            // executable) there's nothing to extract. This shouldn't happen in a
            // normal run - the app extracts the same resource at startup - so skip
            // rather than report a misleading failure.
            logMessage("Filesystem resource unavailable, skipping filesystem init test");
            signalDone(true);
            return;
        }

        auto const* archiveData = reinterpret_cast<uint8_t const*>(resource.data());
        auto const archiveSize = static_cast<int>(resource.size());

        // Decompress once to build the reference layout (what the extractor should
        // produce), independently of Decompress::extractTar.
        HeapArray<uint8_t> tarData;
        check(Decompress::extractXz(archiveData, archiveSize, tarData), "bundled filesystem must decompress");

        std::string rootName;
        std::unordered_map<std::string, ExpectedFile> expectedFiles;
        std::unordered_set<std::string> expectedDirs;
        bool const built = !tarData.empty()
            && buildExpectedLayout(tarData.data(), tarData.size(), rootName, expectedFiles, expectedDirs);
        check(built, "reference layout must be built from the bundled archive");

        if (!built) {
            signalDone(allPassed);
            return;
        }

        tarData.clear(); // the per-file hashes are all we need from here on

        logMessage("Reference layout: " + String(expectedFiles.size()) + " files, "
            + String(expectedDirs.size()) + " directories under '" + String(rootName) + "'");

        // One representative file under each linked directory, used to check that
        // files are reachable through the junction/symlink with identical content.
        std::unordered_map<std::string, std::string> representativeFor;
        StringArray const linkedDirs { "Abstractions", "Documentation", "Extra" };
        for (auto const& dirName : linkedDirs) {
            auto const prefix = dirName.toStdString() + "/";
            for (auto const& entry : expectedFiles) {
                if (entry.first.rfind(prefix, 0) == 0) {
                    representativeFor[dirName.toStdString()] = entry.first;
                    break;
                }
            }
            check(representativeFor.count(dirName.toStdString()) > 0,
                "archive must contain at least one file under " + dirName);
        }

        // A unique sandbox so concurrent test instances (e.g. the VST3 suite test,
        // which hosts many plugin instances at once) can't collide. The suffix is
        // kept short on purpose: Windows caps paths at 260 chars (std::filesystem
        // doesn't use the long-path prefix here) and the bundled layout nests up to
        // ~172 chars deep, so the sandbox root must stay well under ~70 chars.
#if JUCE_WINDOWS
        // Same short, username-independent root production extracts under
        // (commonDocumentsDirectory/plugdata), rather than the long temp path.
        auto const sandboxBase = File::getSpecialLocation(File::commonDocumentsDirectory);
#else
        auto const sandboxBase = File::getSpecialLocation(File::tempDirectory);
#endif
        auto const testRoot = sandboxBase.getChildFile("pdfsit_"
            + String::toHexString(static_cast<int>(Time::getHighResolutionTicks()) ^ rng.nextInt()));
        testRoot.deleteRecursively();

        auto const homeDir = testRoot.getChildFile("home");
        auto const versionsDir = testRoot.getChildFile("Versions");
        auto const versionDataDir = versionsDir.getChildFile("current");
        auto const extractedRoot = versionsDir.getChildFile(String(rootName));

        int const fullCycles = jmax(1, envInt("PLUGDATA_FSINIT_ITERATIONS", 5));
        bool const verifyEveryCycle = SystemStats::getEnvironmentVariable("PLUGDATA_FSINIT_FULL_VERIFY", {}) == "1";

        beginTest("Repeatedly extract and link the filesystem (" + String(fullCycles) + " cycles)");

        for (int cycle = 0; cycle < fullCycles && allPassed; ++cycle) {
            // --- Extraction phase (mirrors initialiseFilesystem's extract + move) ---
            versionDataDir.deleteRecursively();
            extractedRoot.deleteRecursively();
            versionsDir.createDirectory();
            // The home dir must exist before a junction can be created under it
            // (createJunction/createSymbolicLink don't create missing parents).
            // In production this dir already exists by the time links are made.
            homeDir.createDirectory();

            bool const extracted = Decompress::extractTarXz(archiveData, archiveSize, versionsDir, archiveSize * 4);
            check(extracted, "cycle " + String(cycle) + ": extractTarXz must succeed");
            check(extractedRoot.isDirectory(), "cycle " + String(cycle) + ": extracted '" + String(rootName) + "' dir must exist");
            check(OSUtils::moveFileTo(extractedRoot, versionDataDir), "cycle " + String(cycle) + ": moving the extracted dir into place must succeed");

            if (!versionDataDir.isDirectory()) {
                check(false, "cycle " + String(cycle) + ": version data dir must exist after the move");
                break;
            }

            // --- Link phase. Run it several times without re-extracting, to stress
            // recreating junctions/symlinks over existing ones (the "we always
            // update the symlinks" path that has historically failed on Windows). ---
            for (int pass = 0; pass < 3; ++pass) {
                for (auto const& dirName : linkedDirs) {
                    bool const linked = createLink(homeDir.getChildFile(dirName), versionDataDir.getChildFile(dirName));
                    check(linked, "cycle " + String(cycle) + " pass " + String(pass) + ": link for " + dirName + " must be created");
                }
            }

            // --- Verify the extracted tree matches the archive ---
            bool const fullContent = verifyEveryCycle || cycle == 0 || cycle == fullCycles - 1;
            verifyExtraction(versionDataDir, expectedFiles, expectedDirs, fullContent, cycle, check);

            // --- Verify files are reachable through the junctions/symlinks ---
            verifyLinks(homeDir, versionDataDir, linkedDirs, representativeFor, cycle, check);
        }

        testRoot.deleteRecursively();
        signalDone(allPassed);
    }

    template<typename CheckFn>
    void verifyExtraction(File const& versionDataDir,
        std::unordered_map<std::string, ExpectedFile> const& expectedFiles,
        std::unordered_set<std::string> const& expectedDirs,
        bool const fullContent, int const cycle, CheckFn&& check)
    {
        int missing = 0, sizeMismatch = 0, hashMismatch = 0, extra = 0;
        String firstProblem;
        auto note = [&firstProblem](String const& p) {
            if (firstProblem.isEmpty())
                firstProblem = p;
        };

        for (auto const& relDir : expectedDirs) {
            if (!versionDataDir.getChildFile(String(relDir)).isDirectory()) {
                ++missing;
                note("missing dir " + String(relDir));
            }
        }

        for (auto const& [rel, info] : expectedFiles) {
            auto const f = versionDataDir.getChildFile(String(rel));
            if (!f.existsAsFile()) {
                ++missing;
                note("missing file " + String(rel));
                continue;
            }
            if (f.getSize() != info.size) {
                ++sizeMismatch;
                note("size mismatch " + String(rel));
                continue;
            }
            if (fullContent) {
                MemoryBlock block;
                if (!f.loadFileAsData(block) || hashBytes(static_cast<uint8_t const*>(block.getData()), block.getSize()) != info.hash) {
                    ++hashMismatch;
                    note("content mismatch " + String(rel));
                }
            }
        }

        // Nothing should exist on disk that the archive didn't declare
        for (auto const& f : versionDataDir.findChildFiles(File::findFiles, true)) {
            auto rel = f.getRelativePathFrom(versionDataDir).replaceCharacters("\\", "/").toStdString();
            if (expectedFiles.find(rel) == expectedFiles.end()) {
                ++extra;
                note("unexpected file " + String(rel));
            }
        }

        String const detail = "(missing=" + String(missing) + ", size=" + String(sizeMismatch)
            + ", content=" + String(hashMismatch) + ", extra=" + String(extra)
            + (firstProblem.isEmpty() ? "" : ", first: " + firstProblem) + ")";
        check(missing == 0 && sizeMismatch == 0 && hashMismatch == 0 && extra == 0,
            "cycle " + String(cycle) + ": extracted tree matches the archive " + detail);
    }

    template<typename CheckFn>
    void verifyLinks(File const& homeDir, File const& versionDataDir,
        StringArray const& linkedDirs,
        std::unordered_map<std::string, std::string> const& representativeFor,
        int const cycle, CheckFn&& check)
    {
        for (auto const& dirName : linkedDirs) {
            auto const linkDir = homeDir.getChildFile(dirName);
            auto const realDir = versionDataDir.getChildFile(dirName);

            check(linkDir.isDirectory(), "cycle " + String(cycle) + ": junction " + dirName + " resolves as a directory");

            // The whole directory listing must be visible through the link
            auto const viaLink = linkDir.getNumberOfChildFiles(File::findFilesAndDirectories);
            auto const direct = realDir.getNumberOfChildFiles(File::findFilesAndDirectories);
            check(viaLink == direct, "cycle " + String(cycle) + ": " + dirName + " lists the same entries through the junction ("
                                         + String(viaLink) + " vs " + String(direct) + ")");

            // A file inside must be readable through the link with identical bytes
            auto const it = representativeFor.find(dirName.toStdString());
            if (it == representativeFor.end())
                continue;

            auto const rel = String(it->second);
            auto const throughLink = homeDir.getChildFile(rel);
            auto const direct2 = versionDataDir.getChildFile(rel);

            MemoryBlock a, b;
            bool const readOk = throughLink.loadFileAsData(a) && direct2.loadFileAsData(b);
            check(readOk && a == b, "cycle " + String(cycle) + ": " + rel + " is readable through the junction with identical content");
        }
    }
};
