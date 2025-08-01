#include "./settings.h"
#include "./knownfieldmodel.h"
#include "./targetlevelmodel.h"

#include "resources/config.h"

#include <tagparser/backuphelper.h>
#include <tagparser/mediafileinfo.h>
#include <tagparser/tag.h>

#include <qtutilities/resources/resources.h>

#include <QApplication>
#include <QFile>
#include <QSettings>

using namespace QtUtilities;
using namespace TagParser;

namespace Settings {

AutoCompletition::AutoCompletition()
    : fields(nullptr, KnownFieldModel::DefaultSelection::None)
{
}

Editor::Editor()
    : fields(nullptr, KnownFieldModel::DefaultSelection::CommonFields)
    , defaultTargets(nullptr, TargetLevelModel::DefaultSelection::MostUsefulTargets)
{
}

DbQuery::DbQuery()
    : fields(QList<ChecklistItem>() << KnownFieldModel::mkItem(KnownField::Title) << KnownFieldModel::mkItem(KnownField::TrackPosition)
                                    << KnownFieldModel::mkItem(KnownField::DiskPosition) << KnownFieldModel::mkItem(KnownField::Album)
                                    << KnownFieldModel::mkItem(KnownField::Artist) << KnownFieldModel::mkItem(KnownField::RecordDate)
                                    << KnownFieldModel::mkItem(KnownField::Genre) << KnownFieldModel::mkItem(KnownField::Cover, Qt::Unchecked)
                                    << KnownFieldModel::mkItem(KnownField::Lyrics, Qt::Unchecked))
{
}

Settings &values()
{
    static Settings settings;
    return settings;
}

void restore(QSettings &settings)
{
    auto &v = values();
    v.error = QtUtilities::errorMessageForSettings(settings);

    settings.beginGroup(QStringLiteral("editor"));
    switch (settings.value(QStringLiteral("adoptfields"), static_cast<int>(v.editor.adoptFields)).toInt()) {
    case 1:
        v.editor.adoptFields = AdoptFields::WithinDirectory;
        break;
    case 2:
        v.editor.adoptFields = AdoptFields::Always;
        break;
    default:
        v.editor.adoptFields = AdoptFields::Never;
        break;
    }
    v.editor.saveAndShowNextOnEnter = settings.value(QStringLiteral("saveandshownextonenter"), v.editor.saveAndShowNextOnEnter).toBool();
    v.editor.askBeforeDeleting = settings.value(QStringLiteral("askbeforedeleting"), v.editor.askBeforeDeleting).toBool();
    switch (settings.value(QStringLiteral("multipletaghandling"), 0).toInt()) {
    case 0:
        v.editor.multipleTagHandling = MultipleTagHandling::SingleEditorPerTarget;
        break;
    case 1:
        v.editor.multipleTagHandling = MultipleTagHandling::SeparateEditors;
        break;
    }
    v.editor.hideTagSelectionComboBox = settings.value(QStringLiteral("hidetagselectioncombobox"), false).toBool();
    settings.beginGroup(QStringLiteral("autocorrection"));
    v.editor.autoCompletition.insertTitleFromFilename = settings.value(QStringLiteral("inserttitlefromfilename")).toBool();
    v.editor.autoCompletition.trimWhitespaces = settings.value(QStringLiteral("trimwhitespaces"), true).toBool();
    v.editor.autoCompletition.formatNames = settings.value(QStringLiteral("formatnames"), false).toBool();
    v.editor.autoCompletition.fixUmlauts = settings.value(QStringLiteral("fixumlauts"), false).toBool();
    settings.beginGroup(QStringLiteral("customsubst"));
    v.editor.autoCompletition.customSubstitution.enabled = settings.value(QStringLiteral("enabled")).toBool();
    v.editor.autoCompletition.customSubstitution.regex = settings.value(QStringLiteral("regex")).toRegularExpression();
    v.editor.autoCompletition.customSubstitution.replacement = settings.value(QStringLiteral("replacement")).toString();
    settings.endGroup();
    settings.endGroup();
    v.editor.backupDirectory = settings.value(QStringLiteral("tempdir")).toString().toStdString();
    v.editor.hideCoverButtons = settings.value(QStringLiteral("hidecoverbtn"), v.editor.hideCoverButtons).toBool();
    settings.endGroup();

    v.editor.fields.restore(settings, QStringLiteral("selectedfields"));
    v.editor.autoCompletition.fields.restore(settings, QStringLiteral("autocorrectionfields"));

    settings.beginGroup(QStringLiteral("info"));
    v.editor.forceFullParse = settings.value(QStringLiteral("forcefullparse"), v.editor.forceFullParse).toBool();
#ifndef TAGEDITOR_NO_WEBVIEW
    v.editor.noWebView = settings.value(QStringLiteral("nowebview"), v.editor.noWebView).toBool();
#endif
    settings.endGroup();

    settings.beginGroup(QStringLiteral("filebrowser"));
    v.fileBrowser.hideBackupFiles = settings.value(QStringLiteral("hidebackupfiles"), v.fileBrowser.hideBackupFiles).toBool();
    v.fileBrowser.readOnly = settings.value(QStringLiteral("readonly"), v.fileBrowser.readOnly).toBool();
    settings.endGroup();

    settings.beginGroup(QStringLiteral("tagprocessing"));
    switch (settings.value(QStringLiteral("preferredencoding"), static_cast<int>(v.tagPocessing.preferredEncoding)).toInt()) {
    case 0:
        v.tagPocessing.preferredEncoding = TagParser::TagTextEncoding::Latin1;
        break;
    case 2:
        v.tagPocessing.preferredEncoding = TagParser::TagTextEncoding::Utf16BigEndian;
        break;
    case 3:
        v.tagPocessing.preferredEncoding = TagParser::TagTextEncoding::Utf16LittleEndian;
        break;
    default:
        v.tagPocessing.preferredEncoding = TagParser::TagTextEncoding::Utf8;
    }
    switch (settings.value(QStringLiteral("unsupportedfieldhandling"), static_cast<int>(v.tagPocessing.unsupportedFieldHandling)).toInt()) {
    case 1:
        v.tagPocessing.unsupportedFieldHandling = UnsupportedFieldHandling::Discard;
        break;
    default:
        v.tagPocessing.unsupportedFieldHandling = UnsupportedFieldHandling::Ignore;
    }
    v.tagPocessing.autoTagManagement = settings.value(QStringLiteral("autotagmanagement"), v.tagPocessing.autoTagManagement).toBool();
    v.tagPocessing.preserveModificationTime
        = settings.value(QStringLiteral("preservemodificationtime"), v.tagPocessing.preserveModificationTime).toBool();
    v.tagPocessing.preserveMuxingApp = settings.value(QStringLiteral("preservemuxingapp"), v.tagPocessing.preserveMuxingApp).toBool();
    v.tagPocessing.preserveWritingApp = settings.value(QStringLiteral("preservewritingapp"), v.tagPocessing.preserveWritingApp).toBool();
    v.tagPocessing.convertTotalFields = settings.value(QStringLiteral("converttotalfields"), v.tagPocessing.convertTotalFields).toBool();
    settings.beginGroup(QStringLiteral("id3v1"));
    switch (settings.value(QStringLiteral("usage"), 0).toInt()) {
    case 1:
        v.tagPocessing.creationSettings.id3v1usage = TagUsage::KeepExisting;
        break;
    case 2:
        v.tagPocessing.creationSettings.id3v1usage = TagUsage::Never;
        break;
    default:
        v.tagPocessing.creationSettings.id3v1usage = TagUsage::Always;
        break;
    }
    settings.endGroup();
    settings.beginGroup(QStringLiteral("id3v2"));
    switch (settings.value(QStringLiteral("usage"), 0).toInt()) {
    case 1:
        v.tagPocessing.creationSettings.id3v2usage = TagUsage::KeepExisting;
        break;
    case 2:
        v.tagPocessing.creationSettings.id3v2usage = TagUsage::Never;
        break;
    default:
        v.tagPocessing.creationSettings.id3v2usage = TagUsage::Always;
    }
    v.tagPocessing.creationSettings.id3v2MajorVersion = static_cast<std::uint8_t>(settings.value(QStringLiteral("versiontobeused")).toUInt());
    if (v.tagPocessing.creationSettings.id3v2MajorVersion < 1 || v.tagPocessing.creationSettings.id3v2MajorVersion > 4) {
        v.tagPocessing.creationSettings.id3v2MajorVersion = 3;
    }
    v.tagPocessing.creationSettings.setFlag(
        TagCreationFlags::KeepExistingId3v2Version, settings.value(QStringLiteral("keepversionofexistingtag"), true).toBool());
    v.tagPocessing.creationSettings.setFlag(
        TagCreationFlags::MergeMultipleSuccessiveId3v2Tags, settings.value(QStringLiteral("mergemultiplesuccessivetags"), true).toBool());
    settings.endGroup();
    v.editor.defaultTargets.restore(settings, QStringLiteral("targets"));
    settings.beginGroup(QStringLiteral("filelayout"));
    v.tagPocessing.fileLayout.forceRewrite = settings.value(QStringLiteral("forcerewrite"), true).toBool();
    switch (settings.value(QStringLiteral("tagpos")).toInt()) {
    case 0:
        v.tagPocessing.fileLayout.preferredTagPosition = ElementPosition::BeforeData;
        break;
    case 1:
        v.tagPocessing.fileLayout.preferredTagPosition = ElementPosition::AfterData;
        break;
    }
    v.tagPocessing.fileLayout.forceTagPosition = settings.value(QStringLiteral("forcetagpos"), true).toBool();
    switch (settings.value(QStringLiteral("indexpos")).toInt()) {
    case 0:
        v.tagPocessing.fileLayout.preferredIndexPosition = ElementPosition::BeforeData;
        break;
    case 1:
        v.tagPocessing.fileLayout.preferredIndexPosition = ElementPosition::AfterData;
        break;
    }
    v.tagPocessing.fileLayout.forceIndexPosition = settings.value(QStringLiteral("forceindexpos"), true).toBool();
    v.tagPocessing.fileLayout.minPadding
        = settings.value(QStringLiteral("minpad"), static_cast<qulonglong>(v.tagPocessing.fileLayout.minPadding)).toULongLong();
    v.tagPocessing.fileLayout.maxPadding
        = settings.value(QStringLiteral("maxpad"), static_cast<qulonglong>(v.tagPocessing.fileLayout.maxPadding)).toULongLong();
    v.tagPocessing.fileLayout.preferredPadding
        = settings.value(QStringLiteral("prefpad"), static_cast<qulonglong>(v.tagPocessing.fileLayout.preferredPadding)).toULongLong();
    settings.endGroup();
    settings.endGroup();

    settings.beginGroup(QStringLiteral("mainwindow"));
    v.mainWindow.geometry = settings.value(QStringLiteral("geometry")).toByteArray();
    v.mainWindow.state = settings.value(QStringLiteral("windowstate")).toByteArray();
    v.mainWindow.currentFileBrowserDirectory = settings.value(QStringLiteral("currentfilebrowserdirectory")).toString();
    v.mainWindow.layoutLocked = settings.value(QStringLiteral("layoutlocked"), v.mainWindow.layoutLocked).toBool();
    settings.endGroup();

    settings.beginGroup(QStringLiteral("dbquery"));
    v.dbQuery.widgetShown = settings.value(QStringLiteral("visible"), v.dbQuery.widgetShown).toBool();
    v.dbQuery.override = settings.value(QStringLiteral("override"), v.dbQuery.override).toBool();
    v.dbQuery.fields.restore(settings, QStringLiteral("fields"));
    v.dbQuery.musicBrainzUrl = settings.value(QStringLiteral("musicbrainzurl")).toString();
    v.dbQuery.lyricsWikiaUrl = settings.value(QStringLiteral("lyricwikiurl")).toString();
    v.dbQuery.makeItPersonalUrl = settings.value(QStringLiteral("makeitpersonalurl")).toString();
    v.dbQuery.tekstowoUrl = settings.value(QStringLiteral("tekstowourl")).toString();
    v.dbQuery.coverArtArchiveUrl = settings.value(QStringLiteral("coverartarchiveurl")).toString();
    settings.endGroup();

    settings.beginGroup(QStringLiteral("renamedlg"));
    v.renamingUtility.scriptSource = settings.value(QStringLiteral("src")).toInt();
    v.renamingUtility.externalScript = settings.value(QStringLiteral("file")).toString();
    v.renamingUtility.editorScript = settings.value(QStringLiteral("script")).toString();
    settings.endGroup();

    v.qt.restore(settings);
}

void save()
{
    auto s = QtUtilities::getSettings(QStringLiteral(PROJECT_NAME));
    auto &settings = *s;
    auto &v = values();

    settings.beginGroup(QStringLiteral("editor"));
    settings.setValue(QStringLiteral("adoptfields"), static_cast<int>(v.editor.adoptFields));
    settings.setValue(QStringLiteral("saveandshownextonenter"), v.editor.saveAndShowNextOnEnter);
    settings.setValue(QStringLiteral("askbeforedeleting"), v.editor.askBeforeDeleting);
    settings.setValue(QStringLiteral("multipletaghandling"), static_cast<int>(v.editor.multipleTagHandling));
    settings.setValue(QStringLiteral("hidetagselectioncombobox"), v.editor.hideTagSelectionComboBox);
    settings.beginGroup(QStringLiteral("autocorrection"));
    settings.setValue(QStringLiteral("inserttitlefromfilename"), v.editor.autoCompletition.insertTitleFromFilename);
    settings.setValue(QStringLiteral("trimwhitespaces"), v.editor.autoCompletition.trimWhitespaces);
    settings.setValue(QStringLiteral("formatnames"), v.editor.autoCompletition.formatNames);
    settings.setValue(QStringLiteral("fixumlauts"), v.editor.autoCompletition.fixUmlauts);
    settings.beginGroup(QStringLiteral("customsubst"));
    settings.setValue(QStringLiteral("enabled"), v.editor.autoCompletition.customSubstitution.enabled);
    settings.setValue(QStringLiteral("regex"), v.editor.autoCompletition.customSubstitution.regex);
    settings.setValue(QStringLiteral("replacement"), v.editor.autoCompletition.customSubstitution.replacement);
    settings.endGroup();
    settings.endGroup();
    settings.setValue(QStringLiteral("tempdir"), QString::fromStdString(v.editor.backupDirectory));
    settings.setValue(QStringLiteral("hidecoverbtn"), v.editor.hideCoverButtons);
    settings.endGroup();

    v.editor.fields.save(settings, QStringLiteral("selectedfields"));
    v.editor.autoCompletition.fields.save(settings, QStringLiteral("autocorrectionfields"));

    settings.beginGroup(QStringLiteral("info"));
    settings.setValue(QStringLiteral("forcefullparse"), v.editor.forceFullParse);
#ifndef TAGEDITOR_NO_WEBVIEW
    settings.setValue(QStringLiteral("nowebview"), v.editor.noWebView);
#endif
    settings.endGroup();

    settings.beginGroup(QStringLiteral("filebrowser"));
    settings.setValue(QStringLiteral("hidebackupfiles"), v.fileBrowser.hideBackupFiles);
    settings.setValue(QStringLiteral("readonly"), v.fileBrowser.readOnly);
    settings.endGroup();

    settings.beginGroup(QStringLiteral("tagprocessing"));
    settings.setValue(QStringLiteral("preferredencoding"), static_cast<int>(v.tagPocessing.preferredEncoding));
    settings.setValue(QStringLiteral("unsupportedfieldhandling"), static_cast<int>(v.tagPocessing.unsupportedFieldHandling));
    settings.setValue(QStringLiteral("autotagmanagement"), v.tagPocessing.autoTagManagement);
    settings.setValue(QStringLiteral("preservemodificationtime"), v.tagPocessing.preserveModificationTime);
    settings.setValue(QStringLiteral("preservemuxingapp"), v.tagPocessing.preserveMuxingApp);
    settings.setValue(QStringLiteral("preservewritingapp"), v.tagPocessing.preserveWritingApp);
    settings.setValue(QStringLiteral("converttotalfields"), v.tagPocessing.convertTotalFields);
    settings.beginGroup(QStringLiteral("id3v1"));
    settings.setValue(QStringLiteral("usage"), static_cast<int>(v.tagPocessing.creationSettings.id3v1usage));
    settings.endGroup();
    settings.beginGroup(QStringLiteral("id3v2"));
    settings.setValue(QStringLiteral("usage"), static_cast<int>(v.tagPocessing.creationSettings.id3v2usage));
    settings.setValue(QStringLiteral("versiontobeused"), v.tagPocessing.creationSettings.id3v2MajorVersion);
    settings.setValue(QStringLiteral("keepversionofexistingtag"), v.tagPocessing.creationSettings.flags & TagCreationFlags::KeepExistingId3v2Version);
    settings.setValue(
        QStringLiteral("mergemultiplesuccessivetags"), v.tagPocessing.creationSettings.flags & TagCreationFlags::MergeMultipleSuccessiveId3v2Tags);
    settings.endGroup();
    v.editor.defaultTargets.save(settings, QStringLiteral("targets"));
    settings.beginGroup(QStringLiteral("filelayout"));
    settings.setValue(QStringLiteral("forcerewrite"), v.tagPocessing.fileLayout.forceRewrite);
    settings.setValue(QStringLiteral("tagpos"), static_cast<int>(v.tagPocessing.fileLayout.preferredTagPosition));
    settings.setValue(QStringLiteral("forcetagpos"), v.tagPocessing.fileLayout.forceTagPosition);
    settings.setValue(QStringLiteral("indexpos"), static_cast<int>(v.tagPocessing.fileLayout.preferredIndexPosition));
    settings.setValue(QStringLiteral("forceindexpos"), v.tagPocessing.fileLayout.forceIndexPosition);
    settings.setValue(QStringLiteral("minpad"), QVariant::fromValue(v.tagPocessing.fileLayout.minPadding));
    settings.setValue(QStringLiteral("maxpad"), QVariant::fromValue(v.tagPocessing.fileLayout.maxPadding));
    settings.setValue(QStringLiteral("prefpad"), QVariant::fromValue(v.tagPocessing.fileLayout.preferredPadding));
    settings.endGroup();
    settings.endGroup();

    settings.beginGroup(QStringLiteral("mainwindow"));
    settings.setValue(QStringLiteral("geometry"), v.mainWindow.geometry);
    settings.setValue(QStringLiteral("windowstate"), v.mainWindow.state);
    settings.setValue(QStringLiteral("currentfilebrowserdirectory"), v.mainWindow.currentFileBrowserDirectory);
    settings.setValue(QStringLiteral("layoutlocked"), v.mainWindow.layoutLocked);
    settings.endGroup();

    settings.beginGroup(QStringLiteral("dbquery"));
    settings.setValue(QStringLiteral("visible"), v.dbQuery.widgetShown);
    settings.setValue(QStringLiteral("override"), v.dbQuery.override);
    v.dbQuery.fields.save(settings, QStringLiteral("fields"));
    settings.setValue(QStringLiteral("musicbrainzurl"), v.dbQuery.musicBrainzUrl);
    settings.setValue(QStringLiteral("lyricwikiurl"), v.dbQuery.lyricsWikiaUrl);
    settings.setValue(QStringLiteral("makeitpersonalurl"), v.dbQuery.makeItPersonalUrl);
    settings.setValue(QStringLiteral("tekstowourl"), v.dbQuery.tekstowoUrl);
    settings.setValue(QStringLiteral("coverartarchiveurl"), v.dbQuery.coverArtArchiveUrl);
    settings.endGroup();

    settings.beginGroup(QStringLiteral("renamedlg"));
    settings.setValue(QStringLiteral("src"), v.renamingUtility.scriptSource);
    settings.setValue(QStringLiteral("file"), v.renamingUtility.externalScript);
    settings.setValue(QStringLiteral("script"), v.renamingUtility.editorScript);
    settings.endGroup();

    v.qt.save(settings);

    settings.sync();
    v.error = QtUtilities::errorMessageForSettings(settings);
}

} // namespace Settings
