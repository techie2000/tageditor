#ifndef MAINFEATURES_H
#define MAINFEATURES_H

#include <c++utilities/application/argumentparser.h>

#define EXIT_IO_FAILURE (EXIT_FAILURE + 1)
#define EXIT_PARSING_FAILURE (EXIT_FAILURE + 2)

namespace CppUtilities {
class Argument;
}

namespace Cli {

struct SetTagInfoArgs {
    SetTagInfoArgs(CppUtilities::Argument &filesArg, CppUtilities::Argument &verboseArg, CppUtilities::Argument &pedanticArg);
    CppUtilities::Argument &filesArg;
    CppUtilities::Argument &verboseArg;
    CppUtilities::Argument &pedanticArg;
    CppUtilities::ConfigValueArgument quietArg;
    CppUtilities::ConfigValueArgument docTitleArg;
    CppUtilities::ConfigValueArgument removeOtherFieldsArg;
    CppUtilities::ConfigValueArgument treatUnknownFilesAsMp3FilesArg;
    CppUtilities::ConfigValueArgument id3v1UsageArg;
    CppUtilities::ConfigValueArgument id3v2UsageArg;
    CppUtilities::ConfigValueArgument mergeMultipleSuccessiveTagsArg;
    CppUtilities::ConfigValueArgument id3v2VersionArg;
    CppUtilities::ConfigValueArgument id3InitOnCreateArg;
    CppUtilities::ConfigValueArgument id3TransferOnRemovalArg;
    CppUtilities::ConfigValueArgument encodingArg;
    CppUtilities::ConfigValueArgument removeTargetArg;
    CppUtilities::ConfigValueArgument addAttachmentArg;
    CppUtilities::ConfigValueArgument updateAttachmentArg;
    CppUtilities::ConfigValueArgument removeAttachmentArg;
    CppUtilities::ConfigValueArgument removeExistingAttachmentsArg;
    CppUtilities::ConfigValueArgument minPaddingArg;
    CppUtilities::ConfigValueArgument maxPaddingArg;
    CppUtilities::ConfigValueArgument prefPaddingArg;
    CppUtilities::ConfigValueArgument tagPosValueArg;
    CppUtilities::ConfigValueArgument forceTagPosArg;
    CppUtilities::ConfigValueArgument tagPosArg;
    CppUtilities::ConfigValueArgument indexPosValueArg;
    CppUtilities::ConfigValueArgument forceIndexPosArg;
    CppUtilities::ConfigValueArgument indexPosArg;
    CppUtilities::ConfigValueArgument forceRewriteArg;
    CppUtilities::ConfigValueArgument valuesArg;
    CppUtilities::ConfigValueArgument outputFilesArg;
    CppUtilities::ConfigValueArgument backupDirArg;
    CppUtilities::ConfigValueArgument layoutOnlyArg;
    CppUtilities::ConfigValueArgument preserveModificationTimeArg;
    CppUtilities::ConfigValueArgument preserveMuxingAppArg;
    CppUtilities::ConfigValueArgument preserveWritingAppArg;
    CppUtilities::ConfigValueArgument preserveTotalFieldsArg;
    CppUtilities::ConfigValueArgument jsArg;
    CppUtilities::ConfigValueArgument jsSettingsArg;
    CppUtilities::ConfigValueArgument coverTypeDelimiterArg;
    CppUtilities::OperationArgument setTagInfoArg;
};

extern const char *const fieldNames;
extern const char *const fieldNamesForSet;
extern int exitCode;
void applyGeneralConfig(const CppUtilities::Argument &timeSapnFormatArg);
void printFieldNames(const CppUtilities::ArgumentOccurrence &occurrence);
void displayFileInfo(const CppUtilities::ArgumentOccurrence &, const CppUtilities::Argument &filesArg, const CppUtilities::Argument &verboseArg,
    const CppUtilities::Argument &pedanticArg, const CppUtilities::Argument &validateArg);
void generateFileInfo(const CppUtilities::ArgumentOccurrence &, const CppUtilities::Argument &inputFileArg,
    const CppUtilities::Argument &outputFileArg, const CppUtilities::Argument &validateArg);
void displayTagInfo(const CppUtilities::Argument &fieldsArg, const CppUtilities::Argument &showUnsupportedArg, const CppUtilities::Argument &filesArg,
    const CppUtilities::Argument &verboseArg, const CppUtilities::Argument &pedanticArg);
void setTagInfo(const Cli::SetTagInfoArgs &args);
void extractField(const CppUtilities::Argument &fieldArg, const CppUtilities::Argument &attachmentArg, const CppUtilities::Argument &inputFilesArg,
    const CppUtilities::Argument &outputFileArg, const CppUtilities::Argument &indexArg, const CppUtilities::Argument &verboseArg);
void exportToJson(const CppUtilities::ArgumentOccurrence &, const CppUtilities::Argument &filesArg, const CppUtilities::Argument &prettyArg);

} // namespace Cli

#endif // MAINFEATURES_H
