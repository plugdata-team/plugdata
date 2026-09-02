#include "Utility/Decompress.h"

// Unit test for the xz/tar extraction in Utility/Decompress.h, which is used
// to unpack downloaded packages (Deken, patch store, toolchains).
//
// The embedded archive below is a real .tar.xz (pax format, made with
// bsdtar + xz) containing every entry type extractTar() handles:
//
//   pkg/                                regular directory
//   pkg/file1.txt                       regular file
//   pkg/subdir/file2.txt                file in a subdirectory
//   pkg/link_to_file1                   symbolic link (type '2')
//   pkg/hardlink_to_file1               hard link (type '1')
//   pkg/very_long_directory_name_x5/    125-char path -> pax extended header
//   pkg/very_long_directory_name_x5/deep.txt
//
// Alongside the happy path, corrupted and truncated streams must fail
// cleanly, and garbage tar data after a valid xz layer must not crash.

class DecompressTest : public PlugDataUnitTest
{
public:
    DecompressTest(PluginEditor* editor) : PlugDataUnitTest(editor, "Decompress Test")
    {
    }

private:
    static constexpr char const* archiveBase64 =
        "/Td6WFoAAATm1rRGBMDWB4D0ASEBHAAAAAAAAHGVftLgef8Dzl0AKBhLBIZIGx7nZy1w6LpX1Ahk7WxWxzQ0BADInYtyjXTL2n3U"
        "MwzGNEwoDAD9VGT9RxhZlI5jOoFgMaKN8QIykMJ9MRS7hGllmlhFkYQe9LmfVufIBSs9Lk0mrycbLSJ7OnOnBeoo0s+ZKCwLtPPw"
        "G/3TVPaSnrac0cgTxTWLH1bFRMSiqBFDD5TaPnb3mzj24CatB4IwBflDJrzGv6RX8BzYUZ62cqZURoZWRtkKNPsWIttrPqb+atL5"
        "djyJMIgqj4GwKzyBAvnSwuSf8c3eyzuSIrsXuV2Gpcg2sF5BUZqAaf84JB87l2HpGR7pR3JsfkxhyBEPYQhajOCDS02cpn4b6ppI"
        "n5MaRBI8nNle4DD2+QW30WeBI8fW5eH90E/KCzA1rkjfYywha6/DfwdBl4pSVYHlF4qab02gzbv/6XwEeXcSg0LBGuoJP+cvIddF"
        "T4GdLfzsYwDKdP3x/28i30i3nHTKWc/MPN1t0EOB7Th7efWC5SWr+3OWDb1WRFIgf6v5FAH3bkz2inUJyRwW4QS4dIVWRNpFFXTx"
        "0bAio8UlYX40hh26mOby1D2ZG8GVlbBHMh1D9oMzUe4RGMOfi8LQI0ZH47fPGeAeVgEZM4vPlI8mb/OlaAqLQEybOe5XKvhEqffT"
        "0+8ZerZ+02r/l9C8gmWzU0rMiJjCZdQPgenF5YHkgUyM/0MFsMQyuz3VGVvVtn9gadcCaW0CGnJJbrePvBrb8lb8gg1CbKN4u0hA"
        "4CHrOeMx+mdnTNAyKp0UsIWT4X8bjL5LjZQiYOFs+azY1KRQAfkwzi2k8xZv/IVI4nj7Y4/nAqypgbhfHKlS6Z/ZdigZwpfZRcqy"
        "1AIYePxf6mdQ5yyNHBASFhIzMZm3hofIw8wZxk7R1fgu10FM+G4zx6gjHDEtjOXTd9nRbeqpucIoxoLNwNwqfCZufaoCyMRlhaks"
        "4Ww3uraHtDog1fPlaRWvHRzUk0NA4/85oONm1mz5E5+EXexr6LLlsr1ykt6/XIFfL41+r2wKULd9+IW6RKGgy6oh+yvJ5YlbONhz"
        "rMbbhtIu0/7lF9Yt5Ot37p0XXyXTJMYqaCSpydYChfUxQAbWO9G0MO81PsJVlhCwLAkl1ge3q7c7j2RQBXl3fYyAuwSWtigy41C8"
        "C4YzuG5iUhSrrgByJQ8OKQZXe45jVHOqQLE7cdVc+MxrgDsXrWFh0dDI+CR+mQb39K3yxIbQGTGOhzJdk5RBoho+zkpZLx3n0qiK"
        "VGmKRkT5WiEym9aOJR9P3/42aEPHvk8KnQHGJhIh5UYegC7xQgAAAACWpljJmTx38AAB8geA9AEAqeygb7HEZ/sCAAAAAARZWg==";

    void perform() override
    {
        beginTest("Extract a tar.xz with files, directories, symlinks, hard links and pax headers");

        MemoryOutputStream decoded;
        Base64::convertFromBase64(decoded, archiveBase64);
        auto const* data = static_cast<uint8_t const*>(decoded.getData());
        auto const dataSize = static_cast<int>(decoded.getDataSize());

        auto const destRoot = File::getSpecialLocation(File::tempDirectory).getChildFile("plugdata_decompress_test");
        destRoot.deleteRecursively();
        destRoot.createDirectory();

        bool allPassed = true;
        auto check = [this, &allPassed](bool const condition, String const& description) {
            expect(condition, description);
            allPassed = allPassed && condition;
        };

        check(Decompress::extractTarXz(data, dataSize, destRoot, dataSize * 4), "extractTarXz must succeed on a valid archive");

        auto const pkg = destRoot.getChildFile("pkg");
        check(pkg.isDirectory(), "directory entry must be created");
        check(pkg.getChildFile("file1.txt").loadFileAsString() == "hello plugdata\n", "regular file content must round-trip");
        check(pkg.getChildFile("subdir").getChildFile("file2.txt").loadFileAsString() == "nested content here\n", "nested file content must round-trip");

#if !JUCE_WINDOWS
        check(pkg.getChildFile("link_to_file1").isSymbolicLink(), "symlink entry must be created as a symlink");
        check(pkg.getChildFile("link_to_file1").loadFileAsString() == "hello plugdata\n", "symlink must resolve to the linked file");
        check(pkg.getChildFile("hardlink_to_file1").loadFileAsString() == "hello plugdata\n", "hard link must share the linked file's content");
#endif

        // The 125-character directory name doesn't fit the 100-byte tar name
        // field, so this passes through the pax extended header path
        String longName = "";
        for (int i = 0; i < 5; i++)
            longName += "very_long_directory_name_";
        check(pkg.getChildFile(longName).getChildFile("deep.txt").loadFileAsString() == "deep file\n", "pax long-path file must be extracted");

        beginTest("Corrupted and truncated archives must fail cleanly");

        // Flip bytes in the compressed payload (past the magic, before the footer)
        HeapArray<uint8_t> corrupted(data, data + dataSize);
        for (int i = 100; i < 140; i++)
            corrupted[i] ^= 0xA5;
        check(!Decompress::extractTarXz(corrupted.data(), dataSize, destRoot), "corrupted xz stream must fail");

        check(!Decompress::extractTarXz(data, dataSize / 2, destRoot), "truncated xz stream must fail");

        uint8_t garbage[256];
        for (int i = 0; i < 256; i++)
            garbage[i] = static_cast<uint8_t>(rng.nextInt(256));
        check(!Decompress::extractTarXz(garbage, sizeof(garbage), destRoot), "random data must fail");

        // A tar header declaring a file size far beyond the archive's actual
        // size must be rejected instead of read out of bounds (this exact
        // input used to SIGBUS in extractTar's regular-file branch)
        uint8_t fakeTar[1024] = {};
        memcpy(fakeTar, "some_file_name", 14);
        memcpy(fakeTar + 124, "77777777777", 11); // ~8GB octal size in a 1KB archive
        fakeTar[156] = '0';
        check(!Decompress::extractTar(fakeTar, sizeof(fakeTar), destRoot), "tar entry larger than the archive must fail");

        destRoot.deleteRecursively();
        signalDone(allPassed);
    }
};
