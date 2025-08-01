#include "../cli/mainfeatures.h"

#include "resources/config.h"

#include <c++utilities/conversion/stringbuilder.h>
#include <c++utilities/conversion/stringconversion.h>
#include <c++utilities/io/misc.h>
#include <c++utilities/io/path.h>

// order of includes and definition of operator << matters for C++ to resolve the correct overload

#include <tagparser/diagnostics.h>
#include <tagparser/mediafileinfo.h>
#include <tagparser/progressfeedback.h>

#include <cstdlib>
#include <cstring>
#include <filesystem>

namespace CppUtilities {

/*!
 * \brief Prints a DiagMessage to enable using it in CPPUNIT_ASSERT_EQUAL.
 */
inline std::ostream &operator<<(std::ostream &os, const TagParser::DiagMessage &diagMessage)
{
    return os << diagMessage.levelName() << ':' << ' ' << diagMessage.message() << ' ' << '(' << diagMessage.context() << ')';
}

} // namespace CppUtilities

using namespace CppUtilities;

#include <c++utilities/tests/testutils.h>

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include <fstream>
#include <iostream>

#ifdef stdout
#undef stdout
#endif
#ifdef stderr
#undef stderr
#endif

using namespace std;
using namespace CppUtilities::Literals;
using namespace TagParser;
using namespace CPPUNIT_NS;

enum class TagStatus { Original, TestMetaDataPresent, Removed };

/*!
 * \brief The CliTests class tests the command line interface.
 */
class CliTests : public TestFixture {
    CPPUNIT_TEST_SUITE(CliTests);
#if defined(PLATFORM_UNIX) || defined(CPP_UTILITIES_HAS_EXEC_APP)
    CPPUNIT_TEST(testBasicReading);
    CPPUNIT_TEST(testBasicWriting);
    CPPUNIT_TEST(testModifyingCover);
    CPPUNIT_TEST(testSpecifyingNativeFieldIds);
    CPPUNIT_TEST(testHandlingOfTargets);
    CPPUNIT_TEST(testId3SpecificOptions);
    CPPUNIT_TEST(testEncodingOption);
    CPPUNIT_TEST(testMultipleFiles);
    CPPUNIT_TEST(testOutputFile);
    CPPUNIT_TEST(testBackupDir);
    CPPUNIT_TEST(testMultipleValuesPerField);
    CPPUNIT_TEST(testHandlingAttachments);
    CPPUNIT_TEST(testDisplayingInfo);
    CPPUNIT_TEST(testSettingTrackMetaData);
    CPPUNIT_TEST(testExtraction);
    CPPUNIT_TEST(testReadingAndWritingDocumentTitle);
    CPPUNIT_TEST(testFileLayoutOptions);
    CPPUNIT_TEST(testJsonExport);
    CPPUNIT_TEST(testScriptProcessing);
#endif
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp() override;
    void tearDown() override;

#if defined(PLATFORM_UNIX) || defined(CPP_UTILITIES_HAS_EXEC_APP)
    void testBasicReading();
    void testBasicWriting();
    void testModifyingCover();
    void testSpecifyingNativeFieldIds();
    void testHandlingOfTargets();
    void testId3SpecificOptions();
    void testEncodingOption();
    void testMultipleFiles();
    void testOutputFile();
    void testBackupDir();
    void testMultipleValuesPerField();
    void testHandlingAttachments();
    void testDisplayingInfo();
    void testSettingTrackMetaData();
    void testExtraction();
    void testReadingAndWritingDocumentTitle();
    void testFileLayoutOptions();
    void testJsonExport();
    void testScriptProcessing();
#endif

private:
};

CPPUNIT_TEST_SUITE_REGISTRATION(CliTests);

void CliTests::setUp()
{
}

void CliTests::tearDown()
{
}

#if defined(PLATFORM_UNIX) || defined(CPP_UTILITIES_HAS_EXEC_APP)
template <typename StringType, bool negateErrorCond = false>
bool testContainsSubstrings(const StringType &str, std::initializer_list<const typename StringType::value_type *> substrings)
{
#if defined(PLATFORM_WINDOWS)
    auto substringsWindows = std::vector<std::string>();
    substringsWindows.reserve(substrings.size());
    for (const auto *const substring : substrings) {
        findAndReplace(substringsWindows.emplace_back(substring), "\n", "\r\n");
    }
#endif
    auto failedSubstrings = std::vector<const typename StringType::value_type *>();
    auto currentPos = typename StringType::size_type();
#if defined(PLATFORM_WINDOWS)
    auto currentSubstr = substrings.begin();
    for (const auto &substr : substringsWindows) {
        if ((currentPos = str.find(substr, currentPos)) == StringType::npos) {
            failedSubstrings.emplace_back(*currentSubstr);
        }
        currentPos += substr.size();
        ++currentSubstr;
#else
    for (const auto *substr : substrings) {
        if ((currentPos = str.find(substr, currentPos)) == StringType::npos) {
            failedSubstrings.emplace_back(substr);
        }
        currentPos += std::strlen(substr);
#endif
    }

    bool res = failedSubstrings.empty();
    if (negateErrorCond) {
        res = !res;
    }
    if (!res) {
        if (!negateErrorCond) {
            cout << "  - test failed: output does NOT contain required substrings\n";
        } else {
            cout << "  - test failed: output DOES contain substrings it shouldn't\n";
        }
        cout << "Output:\n" << str;
        cout << "Failed substrings:\n";
        for (const auto &substr : failedSubstrings) {
            cout << substr << "\n";
        }
    }
    return res;
}

template <typename StringType>
bool testNotContainsSubstrings(const StringType &str, std::initializer_list<const typename StringType::value_type *> substrings)
{
    return testContainsSubstrings<StringType, true>(str, substrings);
}

/*!
 * \brief Tests basic reading and writing of tags.
 */
void CliTests::testBasicReading()
{
    cout << "\nBasic reading" << endl;
    string stdout, stderr;

    // get specific field
    const auto mkvFile(testFilePath("matroska_wave1/test2.mkv"));
    const auto flacFile(testFilePath("flac/test.flac"));
    const char *const args1[] = { "tageditor", "get", "title", "-f", mkvFile.data(), nullptr };
    TESTUTILS_ASSERT_EXEC(args1);
    CPPUNIT_ASSERT(stderr.empty());
    // context of the following fields is the album (so "Title" means the title of the album)
    CPPUNIT_ASSERT(testContainsSubstrings(stdout, { "album", "Title             Elephant Dream - test 2" }));
    CPPUNIT_ASSERT(stdout.find("Release date      2010") == string::npos);

    // get all fields
    const char *const args2[] = { "tageditor", "get", "-f", mkvFile.data(), nullptr };
    TESTUTILS_ASSERT_EXEC(args2);
    CPPUNIT_ASSERT(stderr.empty());
    // clang-format off
    CPPUNIT_ASSERT(testContainsSubstrings(stdout,
        { "Matroska tag",
          "Title             Elephant Dream - test 2",
          "Comment           Matroska Validation File 2, 100,000 timecode scale, odd aspect ratio, and CRC-32. Codecs are AVC and AAC",
          "Release date      2010",
        }));
    // clang-format on

    // test a file with some more fields
    const char *const args3[] = { "tageditor", "get", "-f", flacFile.data(), nullptr };
    TESTUTILS_ASSERT_EXEC(args3);
    CPPUNIT_ASSERT(stderr.empty());
    // clang-format off
    CPPUNIT_ASSERT(testContainsSubstrings(stdout,
        { "Vorbis comment",
          "Title             Sad Song",
          "Album             Don't Go Away (Apple Lossless)",
          "Artist            Oasis",
          "Genre             Alternative & Punk",
          "Track             3/4",
          "Disk              1/1",
          "Encoder           Lavf",
          "Record date       1998",
          "Composer          Noel Gallagher"
        }));
    // clang-format on
}

void CliTests::testBasicWriting()
{
    cout << "\nBasic writing" << endl;
    string stdout, stderr;

    const auto mkvFile(workingCopyPath("matroska_wave1/test2.mkv"));
    const auto mkvFileBackup(mkvFile + ".bak");
    const char *const args1[] = { "tageditor", "get", "-f", mkvFile.data(), nullptr };

    // set some fields, keep other field
    const char *const args2[] = { "tageditor", "set", "title=A new title", "genre=Testfile", "-f", mkvFile.data(), nullptr };
    TESTUTILS_ASSERT_EXEC(args2);
    CPPUNIT_ASSERT(stdout.find("Changes have been applied") != string::npos);
    TESTUTILS_ASSERT_EXEC(args1);
    CPPUNIT_ASSERT(stderr.empty());
    CPPUNIT_ASSERT(testContainsSubstrings(stdout,
        {
            "Title             A new title",
            "Genre             Testfile",
            "Comment           Matroska Validation File 2, 100,000 timecode scale, odd aspect ratio, and CRC-32. Codecs are AVC and AAC",
            "Release date      2010",
        }));
    // clear backup file
    remove(mkvFileBackup.data());

    // set some fields, discard other
    const char *const args3[] = { "tageditor", "set", "title=Foo", "artist=Bar", "--remove-other-fields", "-f", mkvFile.data(), nullptr };
    TESTUTILS_ASSERT_EXEC(args3);
    TESTUTILS_ASSERT_EXEC(args1);
    CPPUNIT_ASSERT(stderr.empty());
    CPPUNIT_ASSERT(testContainsSubstrings(stdout, { "Title             Foo", "Artist            Bar" }));
    CPPUNIT_ASSERT(stdout.find("Release date") == string::npos);
    CPPUNIT_ASSERT(stdout.find("Comment") == string::npos);
    CPPUNIT_ASSERT(stdout.find("Genre") == string::npos);

    CPPUNIT_ASSERT_EQUAL(0, remove(mkvFile.data()));
    CPPUNIT_ASSERT_EQUAL(0, remove(mkvFileBackup.data()));
}

/*!
 * \brief Tests adding a cover and other fields which are directly read from a file.
 */
void CliTests::testModifyingCover()
{
    cout << "\nModifying cover" << endl;
    string stdout, stderr;
    const auto coverFile = testFilePath("matroska_wave1/logo3_256x256.png");
    const auto lyricsFile = workingCopyPath("lyrics.txt", WorkingCopyMode::NoCopy);
    const auto mp3File1 = workingCopyPath("mtx-test-data/mp3/id3-tag-and-xing-header.mp3");
    const auto mp3File1Backup = mp3File1 + ".bak";
    writeFile(lyricsFile, "I\nam\nno\nsong\nwriter\n");

    // add two front covers and one back cover and lyrics from a file
    const auto otherCover = "cover=" + coverFile;
    const auto frontCover0 = "cover0=" % coverFile + ":front-cover:foo";
    const auto frontCover1 = "cover0=" % coverFile + ":front-cover:bar";
    const auto backCover0 = "cover0=" % coverFile + ":back-cover";
    const auto lyrics = "lyrics>=" + lyricsFile;
    const char *const args1[] = { "tageditor", "get", "-f", mp3File1.data(), nullptr };
    const char *const args2[] = { "tageditor", "set", otherCover.data(), frontCover0.data(), frontCover1.data(), backCover0.data(), lyrics.data(),
        "--pedantic", "-f", mp3File1.data(), nullptr };
    TESTUTILS_ASSERT_EXEC(args2);
    TESTUTILS_ASSERT_EXEC(args1);
    CPPUNIT_ASSERT_MESSAGE("covers added",
        testContainsSubstrings(stdout,
            { " - \033[1mID3v2 tag (version 2.3.0)\033[0m\n", "    Lyrics            I\nam\nno\nsong\nwriter\n",
                "    Cover (other)     can't display image/png as string (use --extract)\n"
                "    Cover (front-cover) can't display image/png as string (use --extract)\n"
                "      description:    foo\n"
                "    Cover (front-cover) can't display image/png as string (use --extract)\n"
                "      description:    bar\n"
                "    Cover (back-cover) can't display image/png as string (use --extract)\n" }));
    CPPUNIT_ASSERT_EQUAL(0, remove(mp3File1Backup.data()));

    // test whether empty trailing ":" does *not* affect all descriptions
    const char *const args3[] = { "tageditor", "set", "cover0=:front-cover:", "-f", mp3File1.data(), nullptr };
    TESTUTILS_ASSERT_EXEC(args3);
    TESTUTILS_ASSERT_EXEC(args1);
    CPPUNIT_ASSERT_MESSAGE("covers not altered",
        testContainsSubstrings(stdout,
            {
                " - \033[1mID3v2 tag (version 2.3.0)\033[0m\n",
                "    Cover (other)     can't display image/png as string (use --extract)\n"
                "    Cover (front-cover) can't display image/png as string (use --extract)\n"
                "      description:    foo\n"
                "    Cover (front-cover) can't display image/png as string (use --extract)\n"
                "      description:    bar\n"
                "    Cover (back-cover) can't display image/png as string (use --extract)\n",
            }));
    CPPUNIT_ASSERT_EQUAL(-1, remove(mp3File1Backup.data()));

    // remove all front covers by omitting trailing ":"
    const char *const args4[] = { "tageditor", "set", "cover0=:front-cover", "-f", mp3File1.data(), nullptr };
    TESTUTILS_ASSERT_EXEC(args4);
    TESTUTILS_ASSERT_EXEC(args1);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("front covers removed", std::string::npos, stdout.find("front-cover"));
    CPPUNIT_ASSERT_MESSAGE("other covers not altered",
        testContainsSubstrings(stdout,
            {
                " - \033[1mID3v2 tag (version 2.3.0)\033[0m\n",
                "    Cover (other)     can't display image/png as string (use --extract)\n"
                "    Cover (back-cover) can't display image/png as string (use --extract)\n",
            }));
    CPPUNIT_ASSERT_EQUAL(0, remove(mp3File1Backup.data()));

    // remove all covers
    const char *const args5[] = { "tageditor", "set", "cover0=", "-f", mp3File1.data(), nullptr };
    TESTUTILS_ASSERT_EXEC(args5);
    TESTUTILS_ASSERT_EXEC(args1);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("All covers removed", std::string::npos, stdout.find("Cover"));
    CPPUNIT_ASSERT_EQUAL(0, remove(mp3File1Backup.data()));

    CPPUNIT_ASSERT_EQUAL(0, remove(mp3File1.data()));
}

/*!
 * \brief Tests specifying native fields IDs when getting and setting fields.
 */
void CliTests::testSpecifyingNativeFieldIds()
{
    cout << "\nSpecifying native field IDs" << endl;
    string stdout, stderr;

    // get specific field
    const string mkvFile(workingCopyPath("matroska_wave1/test2.mkv"));
    const string mkvFileBackup(mkvFile + ".bak");
    const string mp4File(workingCopyPath("mtx-test-data/aac/he-aacv2-ps.m4a"));
    const string mp4FileBackup(mp4File + ".bak");
    const string vorbisFile(workingCopyPath("mtx-test-data/ogg/qt4dance_medium.ogg"));
    const string vorbisFileBackup(vorbisFile + ".bak");
    const string opusFile(workingCopyPath("mtx-test-data/opus/v-opus.ogg"));
    const string opusFileBackup(opusFile + ".bak");
    const char *const args1[] = { "tageditor", "set", "mkv:FOO=bar", "mp4:©foo=bar", "mp4:invalid", "vorbis:BAR=foo", "-f", mkvFile.data(),
        mp4File.data(), vorbisFile.data(), opusFile.data(), nullptr };
    TESTUTILS_ASSERT_EXEC(args1);
    // FIXME: provide a way to specify raw data type
    CPPUNIT_ASSERT(testContainsSubstrings(
        stderr, { "making MP4 tag field ©foo: It was not possible to find an appropriate raw data type id. UTF-8 will be assumed." }));
    CPPUNIT_ASSERT(testContainsSubstrings(stderr, { "Unable to parse denoted field ID \"invalid\": MP4 ID must be exactly 4 chars" }));

    const char *const args2[]
        = { "tageditor", "get", "mkv:FOO", "mp4:©foo", "vorbis:BAR", "generic:year", "generic:releasedate", "-f", mkvFile.data(), nullptr };
    TESTUTILS_ASSERT_EXEC(args2);
    CPPUNIT_ASSERT(stderr.empty());
    CPPUNIT_ASSERT(testContainsSubstrings(stdout, { "Record date       none" }));
    CPPUNIT_ASSERT(testContainsSubstrings(stdout, { "Release date      2010" }));
    CPPUNIT_ASSERT(testContainsSubstrings(stdout, { "FOO               bar" }));

    const char *const args3[]
        = { "tageditor", "get", "mkv:FOO", "mp4:©foo", "vorbis:BAR", "mp4:invalid", "generic:recorddate", "-f", mp4File.data(), nullptr };
    TESTUTILS_ASSERT_EXEC(args3);
    CPPUNIT_ASSERT(stderr.empty());
    CPPUNIT_ASSERT(testContainsSubstrings(stdout, { "test" }));
    CPPUNIT_ASSERT(testContainsSubstrings(stdout, { "Record date       none" }));
    // FIXME: number of whitespaces currently not correct because UTF-8 ©-sign counts as two characters
    CPPUNIT_ASSERT(testContainsSubstrings(stdout, { "©foo             bar" }));
    CPPUNIT_ASSERT(testContainsSubstrings(stdout, { "invalid           unable to parse - MP4 ID must be exactly 4 chars" }));

    const char *const args4[] = { "tageditor", "get", "mkv:FOO", "mp4:©foo", "vorbis:BAR", "generic:recorddate", "-f", vorbisFile.data(), nullptr };
    TESTUTILS_ASSERT_EXEC(args4);
    CPPUNIT_ASSERT(stderr.empty());
    CPPUNIT_ASSERT(testContainsSubstrings(stdout, { "Record date       none" }));
    CPPUNIT_ASSERT(testContainsSubstrings(stdout, { "BAR               foo" }));

    const char *const args5[] = { "tageditor", "get", "mkv:FOO", "mp4:©foo", "vorbis:BAR", "generic:recorddate", "-f", opusFile.data(), nullptr };
    TESTUTILS_ASSERT_EXEC(args5);
    CPPUNIT_ASSERT(stderr.empty());
    CPPUNIT_ASSERT(testContainsSubstrings(stdout, { "Record date       none" }));
    CPPUNIT_ASSERT(testContainsSubstrings(stdout, { "BAR               foo" }));

    CPPUNIT_ASSERT_EQUAL(0, remove(mkvFile.data()));
    CPPUNIT_ASSERT_EQUAL(0, remove(mkvFileBackup.data()));
    CPPUNIT_ASSERT_EQUAL(0, remove(mp4File.data()));
    CPPUNIT_ASSERT_EQUAL(0, remove(mp4FileBackup.data()));
    CPPUNIT_ASSERT_EQUAL(0, remove(vorbisFile.data()));
    CPPUNIT_ASSERT_EQUAL(0, remove(vorbisFileBackup.data()));
    CPPUNIT_ASSERT_EQUAL(0, remove(opusFile.data()));
    CPPUNIT_ASSERT_EQUAL(0, remove(opusFileBackup.data()));
}

/*!
 * \brief Tests adding and removing of targets.
 */
void CliTests::testHandlingOfTargets()
{
    cout << "\nHandling of targets" << endl;
    string stdout, stderr;
    const string mkvFile(workingCopyPath("matroska_wave1/test2.mkv"));
    const string mkvFileBackup(mkvFile + ".bak");
    const char *const args1[] = { "tageditor", "get", "-f", mkvFile.data(), nullptr };

    // add song title (title field for tag with level 30)
    const char *const args2[] = { "tageditor", "set", "target-level=30", "title=The song title", "genre=The song genre", "target-level=50",
        "genre=The album genre", "-f", mkvFile.data(), nullptr };
    TESTUTILS_ASSERT_EXEC(args2);
    TESTUTILS_ASSERT_EXEC(args1);
    CPPUNIT_ASSERT(testContainsSubstrings(stdout, { "song", "Title             The song title", "Genre             The song genre" }));
    CPPUNIT_ASSERT(testContainsSubstrings(stdout, { "album", "Title             Elephant Dream - test 2", "Genre             The album genre" }));
    remove(mkvFileBackup.data());

    // remove tags targeting level 30 and 50 and add new tag targeting level 30 and the audio track
    const char *const args3[]
        = { "tageditor", "set", "target-level=30", "target-tracks=3134325680", "title=The audio track", "encoder=likely some AAC encoder",
              "--remove-target", "target-level=30", "--remove-target", "target-level=50", "-f", mkvFile.data(), nullptr };
    TESTUTILS_ASSERT_EXEC(args3);
    TESTUTILS_ASSERT_EXEC(args1);
    CPPUNIT_ASSERT(testContainsSubstrings(stdout, { "song" }));
    CPPUNIT_ASSERT(testNotContainsSubstrings(stdout, { "song", "song" }));
    CPPUNIT_ASSERT(stdout.find("album") == string::npos);
    CPPUNIT_ASSERT(
        testContainsSubstrings(stdout, { "3134325680", "Title             The audio track", "Encoder           likely some AAC encoder" }));

    CPPUNIT_ASSERT_EQUAL(0, remove(mkvFile.data()));
    CPPUNIT_ASSERT_EQUAL(0, remove(mkvFileBackup.data()));
}

/*!
 * \brief Tests ID3v1/D3v2/MP3 specific options and some more fields.
 */
void CliTests::testId3SpecificOptions()
{
    cout << "\nID3/MP3 specific options" << endl;
    string stdout, stderr;
    const string mp3File1(workingCopyPath("mtx-test-data/mp3/id3-tag-and-xing-header.mp3"));
    const string mp3File1Backup(mp3File1 + ".bak");

    // verify both ID3 tags are detected
    const char *const args1[] = { "tageditor", "get", "-f", mp3File1.data(), nullptr };
    TESTUTILS_ASSERT_EXEC(args1);
    CPPUNIT_ASSERT(testContainsSubstrings(stdout,
        { " - \033[1mID3v1 tag\033[0m\n"
          "    Title             Cohesion\n"
          "    Album             Double Nickels On The Dime\n"
          "    Artist            Minutemen\n"
          "    Genre             Punk Rock\n"
          "    Comment           ExactAudioCopy v0.95b4\n"
          "    Track             4\n"
          "    Record date       1984\n",
            " - \033[1mID3v2 tag (version 2.3.0)\033[0m\n"
            "    Title             Cohesion\n"
            "    Album             Double Nickels On The Dime\n"
            "    Artist            Minutemen\n"
            "    Genre             Punk Rock\n"
            "    Comment           ExactAudioCopy v0.95b4\n"
            "    Track             4/43\n"
            "    Record date       1984\n"
            "    Duration          00:00:00\n"
            "    Encoder settings  LAME 64bits version 3.99 (http://lame.sf.net)" }));

    // remove ID3v1 tag, convert ID3v2 tag to version 4
    const char *const args2[] = { "tageditor", "set", "--id3v1-usage", "never", "--id3v2-version", "4", "-f", mp3File1.data(), nullptr };
    TESTUTILS_ASSERT_EXEC(args2);
    TESTUTILS_ASSERT_EXEC(args1);
    CPPUNIT_ASSERT(testContainsSubstrings(stdout,
        { " - \033[1mID3v2 tag (version 2.4.0)\033[0m\n"
          "    Title             Cohesion\n"
          "    Album             Double Nickels On The Dime\n"
          "    Artist            Minutemen\n"
          "    Genre             Punk Rock\n"
          "    Comment           ExactAudioCopy v0.95b4\n"
          "    Track             4/43\n"
          "    Record date       1984\n"
          "    Duration          00:00:00\n"
          "    Encoder settings  LAME 64bits version 3.99 (http://lame.sf.net)" }));
    CPPUNIT_ASSERT_EQUAL(0, remove(mp3File1Backup.data()));

    // convert remaining ID3v2 tag to version 2, add an ID3v1 tag again and set a field with unicode char by the way
    const char *const args3[] = { "tageditor", "set", "album=Dóuble Nickels On The Dime", "track=5/10", "disk=2/3", "duration=1:45:15",
        "--id3v1-usage", "always", "--id3v2-version", "2", "--id3-init-on-create", "-f", mp3File1.data(), nullptr };
    TESTUTILS_ASSERT_EXEC(args3);
    TESTUTILS_ASSERT_EXEC(args1);
    CPPUNIT_ASSERT(testContainsSubstrings(stdout,
        { " - \033[1mID3v1 tag\033[0m\n"
          "    Title             Cohesion\n"
          "    Album             Dóuble Nickels On The Dime\n"
          "    Artist            Minutemen\n"
          "    Genre             Punk Rock\n"
          "    Comment           ExactAudioCopy v0.95b4\n"
          "    Track             5\n"
          "    Record date       1984\n",
            " - \033[1mID3v2 tag (version 2.2.0)\033[0m\n"
            "    Title             Cohesion\n"
            "    Album             Dóuble Nickels On The Dime\n"
            "    Artist            Minutemen\n"
            "    Genre             Punk Rock\n"
            "    Comment           ExactAudioCopy v0.95b4\n"
            "    Track             5/10\n"
            "    Disk              2/3\n"
            "    Record date       1984\n"
            "    Duration          01:45:15\n"
            "    Encoder settings  LAME 64bits version 3.99 (http://lame.sf.net)" }));
    CPPUNIT_ASSERT_EQUAL(0, remove(mp3File1.data()));
    CPPUNIT_ASSERT_EQUAL(0, remove(mp3File1Backup.data()));
}

bool bufferContains(const char *buffer, size_t bufferSize, const char *needle, size_t needleSize)
{
    for (const char *const bufferEnd = buffer + bufferSize, *const needleEnd = needle + needleSize; buffer != bufferEnd; ++buffer) {
        const char *needleIterator = needle;
        for (const char *bufferIterator = buffer; needleIterator != needleEnd && *needleIterator == *bufferIterator;
            ++needleIterator, ++bufferIterator)
            ;
        if (needleIterator >= needleEnd) {
            return true;
        }
    }
    return false;
}

template <size_t headSize> bool fileHeadContains(const string &fileName, const char *needle, size_t needleSize)
{
    ifstream file;
    file.exceptions(ios_base::failbit | ios_base::badbit);
    file.open(fileName, ios_base::in | ios_base::binary);
    char buffer[headSize];
    file.read(buffer, sizeof(buffer));
    return bufferContains(buffer, sizeof(buffer), needle, needleSize);
}

/*!
 * \brief Tests the option to set the encoding.
 */
void CliTests::testEncodingOption()
{
    cout << "\nEncoding option" << endl;
    string stdout, stderr;
    const string mp3File1(workingCopyPath("mtx-test-data/mp3/id3-tag-and-xing-header.mp3"));
    const string mp3File1Backup(mp3File1 + ".bak");

    // try to use UTF-8 for ID3v2.3: not supported and hence should be ignored, UTF-8 CLI arguments must be converted to UTF-16
    const char *const args1[] = { "tageditor", "set", "album=Täst", "--encoding", "utf8", "--id3v1-usage", "never", "--id3v2-version", "3", "-f",
        mp3File1.data(), nullptr };
    TESTUTILS_ASSERT_EXEC(args1);
    CPPUNIT_ASSERT(
        stderr.find(
            "setting tags: Can't use specified encoding \"utf8\" in ID3v2 tag (version 2.3.0) because the tag format/version doesn't support it.")
        != string::npos);

    // check whether encoding is UTF-16 and BOM is present
    CPPUNIT_ASSERT(fileHeadContains<1024>(mp3File1, "\x01\xff\xfe\x54\x00\xe4\x00\x73\x00\x74\x00\x00\x00", 13));
    remove(mp3File1Backup.data());

    // try to use UTF-8 for ID3v2.4: UTF-8 should be supported
    const char *const args2[] = { "tageditor", "set", "album=Täst", "--encoding", "utf8", "--id3v1-usage", "never", "--id3v2-version", "4", "-f",
        mp3File1.data(), nullptr };
    TESTUTILS_ASSERT_EXEC(args2);
    CPPUNIT_ASSERT(stdout.find("Can't use specified encoding") == string::npos);

    // check whether encoding is UTF-8
    CPPUNIT_ASSERT(fileHeadContains<1024>(mp3File1, "\x03\x54\xc3\xa4\x73\x74\x00", 7));

    CPPUNIT_ASSERT_EQUAL(0, remove(mp3File1.data()));
    CPPUNIT_ASSERT_EQUAL(0, remove(mp3File1Backup.data()));
}

/*!
 * \brief Tests reading and writing multiple files at once.
 */
void CliTests::testMultipleFiles()
{
    cout << "\nReading and writing multiple files at once" << endl;
    string stdout, stderr;
    const string mkvFile1(workingCopyPath("matroska_wave1/test1.mkv"));
    const string mkvFile2(workingCopyPath("matroska_wave1/test2.mkv"));
    const string mkvFile3(workingCopyPath("matroska_wave1/test3.mkv"));

    // get tags of 3 files at once
    const char *const args1[] = { "tageditor", "get", "-f", mkvFile1.data(), mkvFile2.data(), mkvFile3.data(), nullptr };
    TESTUTILS_ASSERT_EXEC(args1);
    CPPUNIT_ASSERT(testContainsSubstrings(stdout,
        { "Title             Big Buck Bunny - test 1", "Title             Elephant Dream - test 2", "Title             Elephant Dream - test 3" }));
    // clear backup files
    remove((mkvFile1 + ".bak").c_str()), remove((mkvFile2 + ".bak").c_str()), remove((mkvFile3 + ".bak").c_str());

    // set title and part number of 3 files at once
    const char *const args2[] = { "tageditor", "set", "target-level=30", "title=test1", "title=test2", "title=test3", "part+=1", "target-level=50",
        "title=MKV testfiles", "totalparts=3", "-f", mkvFile1.data(), mkvFile2.data(), mkvFile3.data(), nullptr };
    TESTUTILS_ASSERT_EXEC(args2);
    TESTUTILS_ASSERT_EXEC(args1);
    CPPUNIT_ASSERT(testContainsSubstrings(stdout,
        { " - \033[1mMatroska tag targeting \"level 50 'album, opera, concert, movie, episode'\"\033[0m\n"
          "    Title             MKV testfiles\n"
          "    Comment           Matroska Validation File1, basic MPEG4.2 and MP3 with only SimpleBlock\n"
          "    Total parts       3\n"
          "    Release date      2010\n"
          " - \033[1mMatroska tag targeting \"level 30 'track, song, chapter'\"\033[0m\n"
          "    Title             test1\n"
          "    Part              1",
            " - \033[1mMatroska tag targeting \"level 50 'album, opera, concert, movie, episode'\"\033[0m\n"
            "    Title             MKV testfiles\n"
            "    Comment           Matroska Validation File 2, 100,000 timecode scale, odd aspect ratio, and CRC-32. Codecs are AVC and AAC\n"
            "    Total parts       3\n"
            "    Release date      2010\n"
            " - \033[1mMatroska tag targeting \"level 30 'track, song, chapter'\"\033[0m\n"
            "    Title             test2\n"
            "    Part              2",
            " - \033[1mMatroska tag targeting \"level 50 'album, opera, concert, movie, episode'\"\033[0m\n"
            "    Title             MKV testfiles\n"
            "    Comment           Matroska Validation File 3, header stripping on the video track and no SimpleBlock\n"
            "    Total parts       3\n"
            "    Release date      2010",
            " - \033[1mMatroska tag targeting \"level 30 'track, song, chapter'\"\033[0m\n"
            "    Title             test3\n"
            "    Part              3" }));

    // clear working copies if all tests have been
    CPPUNIT_ASSERT_EQUAL(0, remove(mkvFile1.data()));
    CPPUNIT_ASSERT_EQUAL(0, remove(mkvFile2.data()));
    CPPUNIT_ASSERT_EQUAL(0, remove(mkvFile3.data()));
    CPPUNIT_ASSERT_EQUAL(0, remove((mkvFile1 + ".bak").data()));
    CPPUNIT_ASSERT_EQUAL(0, remove((mkvFile2 + ".bak").data()));
    CPPUNIT_ASSERT_EQUAL(0, remove((mkvFile3 + ".bak").data()));
}

/*!
 * \brief Tests reading and writing multiple files at once with output files are specified.
 */
void CliTests::testOutputFile()
{
    std::cout << "\nReading and writing multiple files at once with output files specified" << std::endl;
    auto stdout = std::string(), stderr = std::string();
    const auto mkvFile1(workingCopyPath("matroska_wave1/test1.mkv"));
    const auto mkvFile2(workingCopyPath("matroska_wave1/test2.mkv"));
    const auto tempDir = std::filesystem::temp_directory_path();
    const auto tempFile1 = (tempDir / "test1.mkv").string();
    const auto tempFile2 = (tempDir / "test2.mkv").string();

    const char *const args1[] = { "tageditor", "set", "target-level=30", "title=test1", "title=test2", "-f", mkvFile1.data(), mkvFile2.data(), "-o",
        tempFile1.data(), tempFile2.data(), nullptr };
    TESTUTILS_ASSERT_EXEC(args1);

    // original files have not been modified
    const char *const args2[] = { "tageditor", "get", "-f", mkvFile1.data(), mkvFile2.data(), nullptr };
    TESTUTILS_ASSERT_EXEC(args2);
    CPPUNIT_ASSERT(stdout.find("Matroska tag targeting") != string::npos);
    CPPUNIT_ASSERT(stdout.find("Title             test1") == string::npos);
    CPPUNIT_ASSERT(stdout.find("Title             test2") == string::npos);

    // specified output files contain new titles
    const char *const args3[] = { "tageditor", "get", "-f", tempFile1.data(), tempFile2.data(), nullptr };
    TESTUTILS_ASSERT_EXEC(args3);
    CPPUNIT_ASSERT(testContainsSubstrings(stdout,
        { " - \033[1mMatroska tag targeting \"level 30 'track, song, chapter'\"\033[0m\n"
          "    Title             test1\n",
            " - \033[1mMatroska tag targeting \"level 30 'track, song, chapter'\"\033[0m\n"
            "    Title             test2\n" }));

    CPPUNIT_ASSERT_EQUAL(0, remove(mkvFile1.data()));
    CPPUNIT_ASSERT_EQUAL(0, remove(mkvFile2.data()));
    CPPUNIT_ASSERT_EQUAL(0, remove(tempFile1.data()));
    CPPUNIT_ASSERT_EQUAL(0, remove(tempFile2.data()));
}

/*!
 * \brief Tests the "--temp-dir /some/path" option of the set operation.
 */
void CliTests::testBackupDir()
{
    cout << "\nSpecifying a backup/temp dir for set operation" << endl;
    string stdout, stderr;
    const string mkvFileName("matroska_wave1/test1.mkv");
    const auto mkvFile(workingCopyPath(mkvFileName));
    CPPUNIT_ASSERT(mkvFile.size() >= mkvFileName.size());
    const auto backupDir(mkvFile.substr(0, mkvFile.size() - mkvFileName.size()));

    const char *const args1[] = { "tageditor", "set", "target-level=30", "title=test1", "-f", mkvFile.data(), "--temp-dir", "..", nullptr };
    TESTUTILS_ASSERT_EXEC(args1);

    // specified output file contains new title
    const char *const args2[] = { "tageditor", "get", "-f", mkvFile.data(), nullptr };
    TESTUTILS_ASSERT_EXEC(args2);
    CPPUNIT_ASSERT(testContainsSubstrings(stdout,
        {
            " - \033[1mMatroska tag targeting \"level 30 'track, song, chapter'\"\033[0m\n"
            "    Title             test1\n",
        }));

    CPPUNIT_ASSERT_EQUAL(0, remove(mkvFile.data()));
    CPPUNIT_ASSERT_EQUAL(0, remove((backupDir + "test1.mkv").data()));
    CPPUNIT_ASSERT(remove((mkvFile + ".bak").data()));
}

/*!
 * \brief Tests tagging multiple values per field.
 */
void CliTests::testMultipleValuesPerField()
{
    cout << "\nMultiple values per field" << endl;
    string stdout, stderr;
    const auto mkvFile1(workingCopyPath("matroska_wave1/test1.mkv"));
    const auto mkvFile2(workingCopyPath("matroska_wave1/test2.mkv"));
    const auto mp3File(workingCopyPath("misc/multiple_id3v2_4_values.mp3"));

    const char *const args0[] = { "tageditor", "get", "-f", mp3File.data(), nullptr };
    TESTUTILS_ASSERT_EXEC(args0);
    CPPUNIT_ASSERT(testContainsSubstrings(stdout,
        {
            "Artist            B-Front",
            "Artist            Second Artist Example",
            "Genre             Hardstyle",
            "Genre             Test",
            "Genre             Example",
            "Genre             Hard Dance",
        }));

    const char *const args1[] = { "tageditor", "set", "artist=test1", "+artist=test2", "+artist=test3", "artist=test4", "artist=the only one",
        "genre2=foo", "+genre=bar", "-f", mkvFile1.data(), mkvFile2.data(), mp3File.data(), nullptr };
    TESTUTILS_ASSERT_EXEC(args1);

    const char *const args2[] = { "tageditor", "get", "-f", mkvFile1.data(), nullptr };
    TESTUTILS_ASSERT_EXEC(args2);
    CPPUNIT_ASSERT(testContainsSubstrings(stdout, { "Artist            test1", "Artist            test2", "Artist            test3" }));
    CPPUNIT_ASSERT(stdout.find("Artist            test4") == string::npos); // should be in mkvFile2
    CPPUNIT_ASSERT(stdout.find("Artist            the only one") == string::npos); // should be in mp3File
    CPPUNIT_ASSERT(stdout.find("Genre             foo") == string::npos); // should be in mp3File
    CPPUNIT_ASSERT(stdout.find("Genre             bar") == string::npos); // should be in mp3File

    const char *const args3[] = { "tageditor", "get", "-f", mkvFile2.data(), nullptr };
    TESTUTILS_ASSERT_EXEC(args3);
    CPPUNIT_ASSERT(stdout.find("Artist            test1") == string::npos);
    CPPUNIT_ASSERT(stdout.find("Artist            test2") == string::npos);
    CPPUNIT_ASSERT(stdout.find("Artist            test3") == string::npos);
    CPPUNIT_ASSERT(stdout.find("Artist            test4") != string::npos);
    CPPUNIT_ASSERT(stdout.find("Artist            the only one") == string::npos); // should be in mp3File
    CPPUNIT_ASSERT(stdout.find("Genre             foo") == string::npos); // should be in mp3File
    CPPUNIT_ASSERT(stdout.find("Genre             bar") == string::npos); // should be in mp3File

    TESTUTILS_ASSERT_EXEC(args0);
    CPPUNIT_ASSERT(stdout.find("Artist            test1") == string::npos);
    CPPUNIT_ASSERT(stdout.find("Artist            test2") == string::npos);
    CPPUNIT_ASSERT(stdout.find("Artist            test3") == string::npos);
    CPPUNIT_ASSERT(stdout.find("Artist            test4") == string::npos);
    CPPUNIT_ASSERT(testContainsSubstrings(stdout,
        {
            "Artist            the only one",
            "Genre             foo",
            "Genre             bar",
        }));

    CPPUNIT_ASSERT_EQUAL(0, remove(mkvFile1.data()));
    CPPUNIT_ASSERT_EQUAL(0, remove(mkvFile2.data()));
    CPPUNIT_ASSERT_EQUAL(0, remove(mp3File.data()));
    CPPUNIT_ASSERT_EQUAL(0, remove((mkvFile1 + ".bak").data()));
    CPPUNIT_ASSERT_EQUAL(0, remove((mkvFile2 + ".bak").data()));
    CPPUNIT_ASSERT_EQUAL(0, remove((mp3File + ".bak").data()));
}

/*!
 * \brief Tests handling attachments.
 */
void CliTests::testHandlingAttachments()
{
    cout << "\nAttachments" << endl;
    string stdout, stderr;
    const string mkvFile1(workingCopyPath("matroska_wave1/test1.mkv"));
    const string mkvFile1Backup(mkvFile1 + ".bak");
    const string mkvFile2("path=" + testFilePath("matroska_wave1/test2.mkv"));

    // add attachment
    const char *const args2[] = { "tageditor", "set", "--add-attachment", "name=test2.mkv", "mime=video/x-matroska", "desc=Test attachment",
        mkvFile2.data(), "-f", mkvFile1.data(), nullptr };
    TESTUTILS_ASSERT_EXEC(args2);
    const char *const args1[] = { "tageditor", "info", "-f", mkvFile1.data(), nullptr };
    TESTUTILS_ASSERT_EXEC(args1);
    CPPUNIT_ASSERT(testContainsSubstrings(stdout,
        { "Attachments:", "Name                          test2.mkv", "MIME-type                     video/x-matroska",
            "Description                   Test attachment", "Size                          20.16 MiB (21142764 byte)" }));
    // clear backup file
    CPPUNIT_ASSERT_EQUAL(0, remove(mkvFile1Backup.data()));

    // update attachment
    const char *const args3[]
        = { "tageditor", "set", "--update-attachment", "name=test2.mkv", "desc=Updated test attachment", "-f", mkvFile1.data(), nullptr };
    TESTUTILS_ASSERT_EXEC(args3);
    TESTUTILS_ASSERT_EXEC(args1);
    CPPUNIT_ASSERT(testContainsSubstrings(stdout,
        { "Attachments:", "Name                          test2.mkv", "MIME-type                     video/x-matroska",
            "Description                   Updated test attachment", "Size                          20.16 MiB (21142764 byte)" }));
    // clear backup file
    CPPUNIT_ASSERT_EQUAL(0, remove(mkvFile1Backup.data()));

    // extract assigned attachment again
    const auto tmpFile = (std::filesystem::temp_directory_path() / "extracted.mkv").string();
    const char *const args4[] = { "tageditor", "extract", "--attachment", "name=test2.mkv", "-f", mkvFile1.data(), "-o", tmpFile.data(), nullptr };
    TESTUTILS_ASSERT_EXEC(args4);
    fstream origFile, extFile;
    origFile.exceptions(ios_base::failbit | ios_base::badbit), extFile.exceptions(ios_base::failbit | ios_base::badbit);
    origFile.open(mkvFile2.data() + 5, ios_base::in | ios_base::binary), extFile.open(tmpFile.data(), ios_base::in | ios_base::binary);
    origFile.seekg(0, ios_base::end), extFile.seekg(0, ios_base::end);
    std::int64_t origFileSize = origFile.tellg(), extFileSize = extFile.tellg();
    CPPUNIT_ASSERT_EQUAL(origFileSize, extFileSize);
    for (origFile.seekg(0), extFile.seekg(0); origFileSize > 0; --origFileSize) {
        CPPUNIT_ASSERT_EQUAL(origFile.get(), extFile.get());
    }
    remove(tmpFile.data());

    // remove assigned attachment
    const char *const args5[] = { "tageditor", "set", "--remove-attachment", "name=test2.mkv", "-f", mkvFile1.data(), nullptr };
    TESTUTILS_ASSERT_EXEC(args5);
    TESTUTILS_ASSERT_EXEC(args1);
    CPPUNIT_ASSERT(stdout.find("Attachments:") == string::npos);
    CPPUNIT_ASSERT(stdout.find("Name                          test2.mkv") == string::npos);

    CPPUNIT_ASSERT_EQUAL(0, remove(mkvFile1.data()));
    CPPUNIT_ASSERT_EQUAL(0, remove(mkvFile1Backup.data()));
}

/*!
 * \brief Tests displaying general file info.
 */
void CliTests::testDisplayingInfo()
{
    cout << "\nDisplaying general file info" << endl;
    string stdout, stderr;

    // test valid Matroska file
    const auto mkvFile1 = testFilePath("matroska_wave1/test2.mkv");
    const char *const args1[] = { "tageditor", "info", "--pedantic", "-f", mkvFile1.data(), nullptr };
    TESTUTILS_ASSERT_EXEC(args1);
    CPPUNIT_ASSERT(testContainsSubstrings(stdout,
        { " - \033[1mContainer format: Matroska\033[0m\n"
          "    Size                          20.16 MiB\n"
          "    Mime-type                     video/x-matroska\n"
          "    Duration                      47 s 509 ms\n"
          "    Overall avg. bitrate          3.48 Mbit/s\n"
          "    Document type                 matroska\n"
          "    Read version                  1\n"
          "    Version                       1\n"
          "    Document read version         2\n"
          "    Document version              2\n"
          "    Tag position                  before data\n"
          "    Index position                before data\n",
            " - \033[1mTracks: H.264-Main@L3.1-576p / AAC-LC-2ch\033[0m\n"
            "    ID                            1863976627\n"
            "    Type                          Video\n"
            "    Format                        Advanced Video Coding Main Profile\n"
            "    Abbreviation                  H.264 Main\n"
            "    Raw format ID                 V_MPEG4/ISO/AVC\n"
            "    FPS                           24\n",
            "    ID                            3134325680\n"
            "    Type                          Audio\n"
            "    Format                        Advanced Audio Coding Low Complexity Profile\n"
            "    Abbreviation                  MPEG-4 AAC-LC\n"
            "    Raw format ID                 A_AAC\n"
            "    Channel config                2 channels: front-left, front-right\n"
            "    Sampling frequency            48000 Hz" }));

    // test broken Matroska file
    const auto mkvFile2 = testFilePath("matroska_wave1/test4.mkv");
    const char *const args2[] = { "tageditor", "--no-color", "info", "--validate", "--pedantic", "-f", mkvFile2.data(), nullptr };
    TESTUTILS_ASSERT_EXEC_EXIT_STATUS(args2, EXIT_PARSING_FAILURE);
    CPPUNIT_ASSERT(testContainsSubstrings(stdout,
        {
            " - Container format: Matroska\n"
            "    Size                          20.33 MiB\n"
            "    Mime-type                     video/x-matroska\n"
            "    Document type                 matroska\n"
            "    Read version                  1\n"
            "    Version                       1\n"
            "    Document read version         1\n"
            "    Document version              1\n",
            " - Tracks: Theora-720p / Vorbis-2ch\n"
            "    ID                            1368622492\n"
            "    Type                          Video\n"
            "    Format                        Theora\n"
            "    Raw format ID                 V_THEORA\n"
            "    FPS                           24\n"
            "    Pixel size                    width: 1280, height: 720\n"
            "    Display size                  width: 1280, height: 720\n"
            "    Labeled as                    default",
            "    ID                            3171450505\n"
            "    Type                          Audio\n"
            "    Format                        Vorbis\n"
            "    Raw format ID                 A_VORBIS\n"
            "    Channel count                 2\n"
            "    Sampling frequency            48000 Hz\n"
            "    Labeled as                    default\n",
        }));
    CPPUNIT_ASSERT(testContainsSubstrings(stderr,
        {
            " - Diagnostic messages:\n",
            "parsing EBML element header: EBML ID length at 35 is not supported, trying to skip.",
            "parsing header of EBML element 0x1549A966 \"segment info\" at 169: 134 bytes have been skipped",
        }));

    // test MP4 file with AAC track using SBR and PS extensions
    const auto mp4File1 = testFilePath("mtx-test-data/aac/he-aacv2-ps.m4a");
    const char *const args3[] = { "tageditor", "info", "-f", mp4File1.data(), nullptr };
    TESTUTILS_ASSERT_EXEC(args3);
    CPPUNIT_ASSERT(testContainsSubstrings(stdout,
        { " - \033[1mContainer format: MPEG-4 Part 14\033[0m\n"
          "    Size                          898.34 KiB\n"
          "    Mime-type                     audio/mp4\n"
          "    Duration                      3 min\n"
          "    Overall avg. bitrate          39.9 kbit/s\n"
          "    Document type                 mp42\n"
          "    Creation time                 2014-12-10 16:22:41\n"
          "    Modification time             2014-12-10 16:22:41\n",
            " - \033[1mTracks: HE-AAC-2ch\033[0m\n"
            "    ID                            1\n"
            "    Name                          soun\n"
            "    Type                          Audio\n"
            "    Format                        Advanced Audio Coding Low Complexity Profile\n"
            "    Abbreviation                  MPEG-4 AAC-LC\n"
            "    Extensions                    Spectral Band Replication and Parametric Stereo / HE-AAC v2\n"
            "    Raw format ID                 mp4a\n"
            "    Size                          879.65 KiB (900759 byte)\n"
            "    Duration                      3 min 138 ms 666 µs 600 ns\n"
            "    Channel config                1 channel: front-center\n"
            "    Extension channel config      2 channels: front-left, front-right\n"
            "    Bitrate                       40 kbit/s\n"
            "    Bits per sample               16\n"
            "    Sampling frequency            24000 Hz\n"
            "    Extension sampling frequency  48000 Hz\n"
            "    Sample count                  4222\n"
            "    Creation time                 2014-12-10 16:22:41\n"
            "    Modification time             2014-12-10 16:22:41" }));
}

/*!
 * \brief Tests setting track meta-data.
 */
void CliTests::testSettingTrackMetaData()
{
    cout << "\nSetting track meta-data" << endl;
    string stdout, stderr;

    // test Matroska file
    const string mkvFile(workingCopyPath("matroska_wave1/test2.mkv"));
    const string mp4File(workingCopyPath("mtx-test-data/aac/he-aacv2-ps.m4a"));
    const char *const args1[] = { "tageditor", "set", "title=title of tag", "track-id=1863976627", "name=video track", "track-id=3134325680",
        "name=audio track", "language=ger", "default=yes", "forced=yes", "tag=any", "artist=setting tag value again", "track-id=any",
        "name1=sbr and ps", "language1=eng", "-f", mkvFile.data(), mp4File.data(), nullptr };
    const char *const args2[] = { "tageditor", "info", "-f", mkvFile.data(), nullptr };
    const char *const args3[] = { "tageditor", "get", "-f", mkvFile.data(), nullptr };
    TESTUTILS_ASSERT_EXEC(args1);
    TESTUTILS_ASSERT_EXEC(args2);
    CPPUNIT_ASSERT(testContainsSubstrings(stdout,
        { " - \033[1mContainer format: Matroska\033[0m\n"
          "    Size                          20.16 MiB\n"
          "    Mime-type                     video/x-matroska\n"
          "    Duration                      47 s 509 ms\n"
          "    Overall avg. bitrate          3.48 Mbit/s\n"
          "    Document type                 matroska\n"
          "    Read version                  1\n"
          "    Version                       1\n"
          "    Document read version         2\n"
          "    Document version              2\n"
          "    Tag position                  before data\n"
          "    Index position                before data\n",
            " - \033[1mTracks: H.264-Main@L3.1-576p / AAC-LC-2ch-ger\033[0m\n"
            "    ID                            1863976627\n"
            "    Name                          video track\n"
            "    Type                          Video\n"
            "    Format                        Advanced Video Coding Main Profile\n"
            "    Abbreviation                  H.264 Main\n"
            "    Raw format ID                 V_MPEG4/ISO/AVC\n"
            "    FPS                           24\n",
            "    ID                            3134325680\n"
            "    Name                          audio track\n"
            "    Type                          Audio\n"
            "    Language                      German\n"
            "    Format                        Advanced Audio Coding Low Complexity Profile\n"
            "    Abbreviation                  MPEG-4 AAC-LC\n"
            "    Raw format ID                 A_AAC\n"
            "    Channel config                2 channels: front-left, front-right\n"
            "    Sampling frequency            48000 Hz\n"
            "    Labeled as                    default, forced" }));
    TESTUTILS_ASSERT_EXEC(args3);
    CPPUNIT_ASSERT(testContainsSubstrings(stdout,
        { " - \033[1mMatroska tag targeting \"level 50 'album, opera, concert, movie, episode'\"\033[0m\n"
          "    Title             title of tag\n"
          "    Artist            setting tag value again\n"
          "    Comment           Matroska Validation File 2, 100,000 timecode scale, odd aspect ratio, and CRC-32. Codecs are AVC and AAC\n"
          "    Release date      2010" }));

    const char *const args4[] = { "tageditor", "info", "-f", mp4File.data(), nullptr };
    TESTUTILS_ASSERT_EXEC(args4);
    CPPUNIT_ASSERT(testContainsSubstrings(stdout,
        { " - \033[1mContainer format: MPEG-4 Part 14\033[0m\n"
          "    Size                          898.49 KiB\n"
          "    Mime-type                     audio/mp4\n"
          "    Duration                      3 min\n"
          "    Overall avg. bitrate          39.9 kbit/s\n"
          "    Document type                 mp42\n"
          "    Creation time                 2014-12-10 16:22:41\n"
          "    Modification time             2014-12-10 16:22:41\n",
            " - \033[1mTracks: HE-AAC-2ch-eng\033[0m\n"
            "    ID                            1\n"
            "    Name                          sbr and ps\n"
            "    Type                          Audio\n"
            "    Language                      English\n"
            "    Format                        Advanced Audio Coding Low Complexity Profile\n"
            "    Abbreviation                  MPEG-4 AAC-LC\n"
            "    Extensions                    Spectral Band Replication and Parametric Stereo / HE-AAC v2\n"
            "    Raw format ID                 mp4a\n"
            "    Size                          879.65 KiB (900759 byte)\n"
            "    Duration                      3 min 138 ms 666 µs 600 ns\n"
            "    Channel config                1 channel: front-center\n"
            "    Extension channel config      2 channels: front-left, front-right\n"
            "    Bitrate                       40 kbit/s\n"
            "    Bits per sample               16\n"
            "    Sampling frequency            24000 Hz\n"
            "    Extension sampling frequency  48000 Hz\n"
            "    Sample count                  4222\n"
            "    Creation time                 2014-12-10 16:22:41\n"
            "    Modification time             2014-12-10 16:22:41" }));
    CPPUNIT_ASSERT_EQUAL(0, remove(mkvFile.data()));
    CPPUNIT_ASSERT_EQUAL(0, remove(mp4File.data()));
    CPPUNIT_ASSERT_EQUAL(0, remove((mkvFile + ".bak").data()));
    CPPUNIT_ASSERT_EQUAL(0, remove((mp4File + ".bak").data()));
}

/*!
 * \brief Tests extraction of field values (used to extract cover or other binary fields).
 * \remarks Extraction of attachments is already tested in testHandlingAttachments().
 */
void CliTests::testExtraction()
{
    std::cout << "\nExtraction" << std::endl;
    auto stdout = std::string(), stderr = std::string();
    const auto mp4File1 = testFilePath("mtx-test-data/alac/othertest-itunes.m4a");
    const auto tempFile = (std::filesystem::temp_directory_path() / "extracted.jpeg").string();

    // test extraction of cover
    const char *const args1[] = { "tageditor", "extract", "cover", "-f", mp4File1.data(), "-o", tempFile.data(), nullptr };
    TESTUTILS_ASSERT_EXEC(args1);
    auto diag = Diagnostics();
    auto progress = AbortableProgressFeedback();
    auto extractedInfo = MediaFileInfo(tempFile);
    extractedInfo.open(true);
    extractedInfo.parseContainerFormat(diag, progress);
    CPPUNIT_ASSERT_EQUAL(static_cast<std::uint64_t>(22771), extractedInfo.size());
    CPPUNIT_ASSERT(ContainerFormat::Jpeg == extractedInfo.containerFormat());
    extractedInfo.invalidate();

    // test assignment of cover by the way
    const auto mp4File2 = workingCopyPath("mtx-test-data/aac/he-aacv2-ps.m4a");
    const auto coverArg = argsToString("cover=", tempFile);
    const char *const args2[] = { "tageditor", "set", coverArg.data(), "-f", mp4File2.data(), nullptr };
    TESTUTILS_ASSERT_EXEC(args2);
    const char *const args3[] = { "tageditor", "extract", "cover", "-f", mp4File2.data(), "-o", tempFile.data(), nullptr };
    CPPUNIT_ASSERT_EQUAL(0, remove(tempFile.data()));
    TESTUTILS_ASSERT_EXEC(args3);
    extractedInfo.open(true);
    extractedInfo.parseContainerFormat(diag, progress);
    CPPUNIT_ASSERT_EQUAL(static_cast<std::uint64_t>(22771), extractedInfo.size());
    CPPUNIT_ASSERT(ContainerFormat::Jpeg == extractedInfo.containerFormat());
    extractedInfo.close();
    CPPUNIT_ASSERT_EQUAL(0, remove(tempFile.data()));
    CPPUNIT_ASSERT_EQUAL(0, remove(mp4File2.data()));
    CPPUNIT_ASSERT_EQUAL(0, remove((mp4File2 + ".bak").data()));
    CPPUNIT_ASSERT_EQUAL(Diagnostics(), diag);
}

/*!
 * \brief Tests reading and writing the document title.
 */
void CliTests::testReadingAndWritingDocumentTitle()
{
    cout << "\nDocument title" << endl;
    string stdout, stderr;

    const string mkvFile(workingCopyPath("matroska_wave1/test2.mkv"));

    const char *const args1[] = { "tageditor", "set", "--doc-title", "Foo", "-f", mkvFile.data(), nullptr };
    TESTUTILS_ASSERT_EXEC(args1);

    const char *const args2[] = { "tageditor", "info", "-f", mkvFile.data(), nullptr };
    TESTUTILS_ASSERT_EXEC(args2);
    CPPUNIT_ASSERT(stdout.find("Title                         Foo") != string::npos);

    CPPUNIT_ASSERT_EQUAL(0, remove(mkvFile.data()));
    CPPUNIT_ASSERT_EQUAL(0, remove((mkvFile + ".bak").data()));
}

/*!
 * \brief Tests options for controlling file layout.
 */
void CliTests::testFileLayoutOptions()
{
    cout << "\nFile layout options" << endl;
    string stdout, stderr;

    const string mp4File1(workingCopyPath("mtx-test-data/alac/othertest-itunes.m4a"));
    const char *const args1[] = { "tageditor", "set", "--tag-pos", "back", "--force", "--layout-only", "-f", mp4File1.data(), nullptr };
    TESTUTILS_ASSERT_EXEC(args1);

    const char *const args2[] = { "tageditor", "info", "-f", mp4File1.data(), nullptr };
    TESTUTILS_ASSERT_EXEC(args2);
    CPPUNIT_ASSERT(stdout.find("Tag position                  after data") != string::npos);

    CPPUNIT_ASSERT_EQUAL(0, remove(mp4File1.data()));
    CPPUNIT_ASSERT_EQUAL(0, remove((mp4File1 + ".bak").data()));

    const string mp4File2(workingCopyPath("mp4/test1.m4a"));
    const char *const args3[] = { "tageditor", "set", "genre=Rock", "--index-pos", "back", "--force", "-f", mp4File2.data(), nullptr };
    TESTUTILS_ASSERT_EXEC(args3);

    const char *const args4[] = { "tageditor", "info", "-f", mp4File2.data(), nullptr };
    TESTUTILS_ASSERT_EXEC(args4);
    CPPUNIT_ASSERT(stdout.find("Tag position                  after data") != string::npos);

    const char *const args5[] = { "tageditor", "get", "-f", mp4File2.data(), nullptr };
    TESTUTILS_ASSERT_EXEC(args5);
    CPPUNIT_ASSERT(testContainsSubstrings(stdout,
        { " - \033[1mMP4/iTunes tag\033[0m\n"
          "    Title             You Shook Me All Night Long\n"
          "    Album             Who Made Who\n"
          "    Artist            ACDC\n"
          "    Genre             Rock\n"
          "    Track             2/9\n"
          "    Encoder           Nero AAC codec / 1.5.3.0, remuxed with Lavf57.56.100\n"
          "    Record date       1986\n"
          "    Encoder settings  ndaudio 1.5.3.0 / -q 0.34" }));
    remove((mp4File2 + ".bak").data());

    const char *const args6[] = { "tageditor", "set", "--index-pos", "front", "--force", "--layout-only", "-f", mp4File2.data(), nullptr };
    TESTUTILS_ASSERT_EXEC(args6);

    TESTUTILS_ASSERT_EXEC(args4);
    CPPUNIT_ASSERT(stdout.find("Tag position                  before data") != string::npos);

    CPPUNIT_ASSERT_EQUAL(0, remove(mp4File2.data()));
    CPPUNIT_ASSERT_EQUAL(0, remove((mp4File2 + ".bak").data()));
}

/*!
 * \brief Tests the JSON export.
 */
void CliTests::testJsonExport()
{
#ifndef TAGEDITOR_JSON_EXPORT
    cout << "\nSkipping JSON export (feature not enabled)" << endl;
#else
    cout << "\nJSON export" << endl;
    string stdout, stderr;

    const auto file = testFilePath("matroska_wave1/test3.mkv");
    const auto expectedJson = readFile(testFilePath("matroska_wave1-test3.json"));
    const char *const args[] = { "tageditor", "export", "--pretty", "-f", file.data(), nullptr };
    TESTUTILS_ASSERT_EXEC(args);
    const char *const jqArgs[]
        = { "jq", "--argjson", "expected", expectedJson.data(), "--argjson", "actual", stdout.data(), "-n", "$actual == $expected", nullptr };
    const auto *const logJsonExport = std::getenv(PROJECT_VARNAME_UPPER "_LOG_JQ_INVOCATION");
    execHelperAppInSearchPath("jq", jqArgs, stdout, stderr, !logJsonExport || !std::strlen(logJsonExport));
    CPPUNIT_ASSERT_EQUAL(""s, stderr);
    CPPUNIT_ASSERT_EQUAL("true\n"s, stdout);
#endif // TAGEDITOR_JSON_EXPORT
}

/*!
 * \brief Tests the --script parameter of the set operation.
 */
void CliTests::testScriptProcessing()
{
#ifndef TAGEDITOR_USE_JSENGINE
    std::cout << "\nSkipping script processing (feature not enabled)" << std::endl;
#else
    std::cout << "\nScript processing" << endl;
    auto stdout = std::string(), stderr = std::string();

    const auto file = workingCopyPath("mtx-test-data/alac/othertest-itunes.m4a");
    const auto script = testFilePath("script-processing-test.js");
    const char *args[] = { "tageditor", "set", "--pedantic", "debug", "--script", script.data(), "--script-settings", "set:title=foo",
        "set:artist=bar", "dryRun=false", "-f", file.data(), nullptr };
    TESTUTILS_ASSERT_EXEC_EXIT_STATUS(args, EXIT_PARSING_FAILURE);
    CPPUNIT_ASSERT(testContainsSubstrings(stderr,
        { "executing JavaScript for othertest-itunes.m4a: entering main() function", "settings: set:title, set:artist, dryRun", "tag: MP4/iTunes tag",
            "supported fields: album, albumArtist, arranger, ", "soundEngineer, storeDescription, synopsis, title, track",
            "MP4/iTunes tag: applying changes", " - change title[0] from 'Sad Song' to 'foo'", " - change artist[0] from 'Oasis' to 'bar'",
            "executing JavaScript for othertest-itunes.m4a: done with return value: true", "Changes are about to be applied" }));
    CPPUNIT_ASSERT(testContainsSubstrings(
        stdout, { "Loading JavaScript file", script.data(), "Setting tag information for", file.data(), "Changes have been applied." }));

    args[9] = "dryRun=true";
    TESTUTILS_ASSERT_EXEC_EXIT_STATUS(args, EXIT_PARSING_FAILURE);
    CPPUNIT_ASSERT(testContainsSubstrings(stderr,
        { "executing JavaScript for othertest-itunes.m4a: entering main() function", "MP4/iTunes tag: applying changes",
            " - set title[0] to 'foo' (no change)", " - set artist[0] to 'bar' (no change)",
            "executing JavaScript for othertest-itunes.m4a: done with return value: false" }));
    CPPUNIT_ASSERT_EQUAL(std::string::npos, stderr.find("Changes are about to be applied"));
    CPPUNIT_ASSERT(testContainsSubstrings(stdout,
        { "Loading JavaScript file", script.data(), "Setting tag information for", file.data(),
            " - Skipping file because JavaScript returned a falsy value other than undefined." }));
#endif
}

#endif // defined(PLATFORM_UNIX) || defined(CPP_UTILITIES_HAS_EXEC_APP)
