#pragma once

#include <QString>
#include <QSet>

namespace fsnext {

// Static utility for file type classification.
// Centralises extension → category / label / MIME mapping used by:
//   • FileListModel (roles for QML)
//   • FsFileIcon / FsFileMediumCard (icon selection)
//   • Theme.qml fileIconBg / fileIconText (colour pairing)
class FileTypeHelper {
public:
    FileTypeHelper() = delete;

    // Extract lowercase extension from a filename (e.g. "report.pdf" → "pdf").
    // Returns an empty string when:
    //   • no dot is found
    //   • the name starts with a dot and has no other dot (dotfiles like
    //     ".bashrc" — Unix convention treats those as having no extension,
    //     not the suffix "bashrc"; this also matches FileNameSanitizer's
    //     handling).
    //   • the name ends with a dot ("foo." — trailing-dot names that
    //     Windows itself rejects).
    static QString extension(const QString &fileName)
    {
        const int dot = fileName.lastIndexOf(QLatin1Char('.'));
        if (dot < 1 || dot == fileName.size() - 1) return {};
        return fileName.mid(dot + 1).toLower();
    }

    // Classify an extension into a semantic category.
    // Returns one of:
    //   "image" "video" "audio" "archive" "document" "spreadsheet"
    //   "presentation" "code" "app" "disc" "font" "generic"
    static QString category(const QString &ext)
    {
        if (ext.isEmpty()) return QStringLiteral("generic");

        // Image
        static const QSet<QString> image = {
            QStringLiteral("jpg"),  QStringLiteral("jpeg"), QStringLiteral("png"),
            QStringLiteral("gif"),  QStringLiteral("webp"), QStringLiteral("bmp"),
            QStringLiteral("tiff"), QStringLiteral("tif"),  QStringLiteral("svg"),
            QStringLiteral("ico"),  QStringLiteral("heic"), QStringLiteral("heif"),
            QStringLiteral("raw"),  QStringLiteral("psd"),  QStringLiteral("ai"),
        };
        if (image.contains(ext)) return QStringLiteral("image");

        // Video
        static const QSet<QString> video = {
            QStringLiteral("mp4"),  QStringLiteral("mkv"),  QStringLiteral("avi"),
            QStringLiteral("mov"),  QStringLiteral("wmv"),  QStringLiteral("flv"),
            QStringLiteral("webm"), QStringLiteral("m4v"),  QStringLiteral("mpg"),
            QStringLiteral("mpeg"), QStringLiteral("3gp"),  QStringLiteral("ts"),
            QStringLiteral("vob"),  QStringLiteral("rmvb"),
        };
        if (video.contains(ext)) return QStringLiteral("video");

        // Audio
        static const QSet<QString> audio = {
            QStringLiteral("mp3"),  QStringLiteral("wav"),  QStringLiteral("flac"),
            QStringLiteral("aac"),  QStringLiteral("ogg"),  QStringLiteral("m4a"),
            QStringLiteral("wma"),  QStringLiteral("opus"), QStringLiteral("alac"),
            QStringLiteral("ape"),  QStringLiteral("mid"),  QStringLiteral("midi"),
        };
        if (audio.contains(ext)) return QStringLiteral("audio");

        // Archive
        static const QSet<QString> archive = {
            QStringLiteral("zip"),  QStringLiteral("rar"),  QStringLiteral("7z"),
            QStringLiteral("tar"),  QStringLiteral("gz"),   QStringLiteral("bz2"),
            QStringLiteral("xz"),   QStringLiteral("zst"),  QStringLiteral("lz4"),
            QStringLiteral("cab"),  QStringLiteral("tgz"),
        };
        if (archive.contains(ext)) return QStringLiteral("archive");

        // Document (text & PDF)
        static const QSet<QString> doc = {
            QStringLiteral("pdf"),  QStringLiteral("doc"),  QStringLiteral("docx"),
            QStringLiteral("odt"),  QStringLiteral("rtf"),  QStringLiteral("txt"),
            QStringLiteral("md"),   QStringLiteral("epub"), QStringLiteral("mobi"),
            QStringLiteral("djvu"), QStringLiteral("tex"),  QStringLiteral("pages"),
        };
        if (doc.contains(ext)) return QStringLiteral("document");

        // Spreadsheet
        static const QSet<QString> sheet = {
            QStringLiteral("xls"),  QStringLiteral("xlsx"), QStringLiteral("csv"),
            QStringLiteral("ods"),  QStringLiteral("numbers"),
        };
        if (sheet.contains(ext)) return QStringLiteral("spreadsheet");

        // Presentation
        static const QSet<QString> pres = {
            QStringLiteral("ppt"),  QStringLiteral("pptx"), QStringLiteral("odp"),
            QStringLiteral("key"),
        };
        if (pres.contains(ext)) return QStringLiteral("presentation");

        // Code & markup
        static const QSet<QString> code = {
            QStringLiteral("js"),   QStringLiteral("ts"),   QStringLiteral("jsx"),
            QStringLiteral("tsx"),  QStringLiteral("py"),   QStringLiteral("cpp"),
            QStringLiteral("c"),    QStringLiteral("h"),    QStringLiteral("hpp"),
            QStringLiteral("java"), QStringLiteral("go"),   QStringLiteral("rs"),
            QStringLiteral("rb"),   QStringLiteral("php"),  QStringLiteral("swift"),
            QStringLiteral("kt"),   QStringLiteral("cs"),   QStringLiteral("dart"),
            QStringLiteral("lua"),  QStringLiteral("sh"),   QStringLiteral("bat"),
            QStringLiteral("ps1"),  QStringLiteral("sql"),  QStringLiteral("json"),
            QStringLiteral("xml"),  QStringLiteral("yaml"), QStringLiteral("yml"),
            QStringLiteral("toml"), QStringLiteral("html"), QStringLiteral("css"),
            QStringLiteral("scss"), QStringLiteral("less"), QStringLiteral("vue"),
            QStringLiteral("qml"),  QStringLiteral("cmake"),
        };
        if (code.contains(ext)) return QStringLiteral("code");

        // Application / executable
        static const QSet<QString> app = {
            QStringLiteral("exe"),  QStringLiteral("msi"),  QStringLiteral("apk"),
            QStringLiteral("dmg"),  QStringLiteral("deb"),  QStringLiteral("rpm"),
            QStringLiteral("appx"), QStringLiteral("msix"), QStringLiteral("ipa"),
            QStringLiteral("app"),
        };
        if (app.contains(ext)) return QStringLiteral("app");

        // Disc image
        static const QSet<QString> disc = {
            QStringLiteral("iso"),  QStringLiteral("img"),  QStringLiteral("bin"),
            QStringLiteral("cue"),  QStringLiteral("nrg"),  QStringLiteral("vhd"),
            QStringLiteral("vmdk"),
        };
        if (disc.contains(ext)) return QStringLiteral("disc");

        // Font
        static const QSet<QString> font = {
            QStringLiteral("ttf"),  QStringLiteral("otf"),  QStringLiteral("woff"),
            QStringLiteral("woff2"), QStringLiteral("eot"),
        };
        if (font.contains(ext)) return QStringLiteral("font");

        return QStringLiteral("generic");
    }

    // Short 3-letter label for icon display (e.g. "PDF", "VID", "IMG").
    static QString label(const QString &ext)
    {
        if (ext.isEmpty()) return QStringLiteral("FILE");

        const QString cat = category(ext);
        if (cat == QStringLiteral("image"))        return QStringLiteral("IMG");
        if (cat == QStringLiteral("video"))        return QStringLiteral("VID");
        if (cat == QStringLiteral("audio"))        return QStringLiteral("AUD");
        if (cat == QStringLiteral("archive"))      return QStringLiteral("ZIP");
        if (cat == QStringLiteral("code"))         return QStringLiteral("CODE");
        if (cat == QStringLiteral("app"))          return QStringLiteral("APP");
        if (cat == QStringLiteral("disc"))         return QStringLiteral("ISO");
        if (cat == QStringLiteral("font"))         return QStringLiteral("FONT");

        // For document/spreadsheet/presentation, use the extension itself (more specific)
        if (ext == QStringLiteral("pdf"))  return QStringLiteral("PDF");
        if (ext == QStringLiteral("doc") || ext == QStringLiteral("docx")) return QStringLiteral("DOC");
        if (ext == QStringLiteral("xls") || ext == QStringLiteral("xlsx")) return QStringLiteral("XLS");
        if (ext == QStringLiteral("ppt") || ext == QStringLiteral("pptx")) return QStringLiteral("PPT");
        if (ext == QStringLiteral("txt"))  return QStringLiteral("TXT");
        if (ext == QStringLiteral("csv"))  return QStringLiteral("CSV");

        // Generic: first 3 chars uppercased
        return ext.left(3).toUpper();
    }
};

} // namespace fsnext
